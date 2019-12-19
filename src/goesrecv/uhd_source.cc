//
// Support for Ettus Research software defined radio peripherals (USRP)
// Added by Jim Herbert (https://github.com/codient/goestools)
//

#include "uhd_source.h"
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <thread>
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <util/error.h>

std::unique_ptr<UHD> UHD::open(std::string type) {
    std::string args = "";

    // Filter to a device type if one was specified
    if (not type.empty()) {
        args = "type=" + type;
    }

    // Set the scheudling priority on the current thread
    uhd::set_thread_priority_safe();

    // Create a USRP device
    uhd::usrp::multi_usrp::sptr dev = nullptr;

    UHD_SAFE_CALL(dev = uhd::usrp::multi_usrp::make(args);)

    // Abort if we didn't open a device
    if (dev == nullptr) {
        std::cout << "Unable to open UHD device. " << std::endl;

        exit(1);
    }

    std::cout << std::endl
              << "Using Device " << dev->get_pp_string() << std::endl;

    // Select the internal clock source
    dev->set_clock_source("internal");

    return std::make_unique<UHD>(dev);
}

UHD::UHD(uhd::usrp::multi_usrp::sptr dev) : dev_(dev) {
    // Perform necessary constructor operations here
}

UHD::~UHD() {
    // Perform necessary destructor operations here
}

void UHD::setFrequency(uint32_t freq) {
    ASSERT(dev_ != nullptr);

    uhd::tune_request_t tune_request(freq);

    dev_->set_rx_freq(tune_request);
}

void UHD::setSampleRate(uint32_t rate) {
    ASSERT(dev_ != nullptr);

    dev_->set_rx_rate(rate);

    // Set the bandwidth to match the sample rate
    dev_->set_rx_bandwidth(rate);
}

uint32_t UHD::getSampleRate() const {
    ASSERT(dev_ != nullptr);

    return dev_->get_rx_rate();
}

void UHD::setGain(int gain) {
    ASSERT(dev_ != nullptr);

    dev_->set_rx_gain(gain);
}

void UHD::start(const std::shared_ptr<Queue<Samples>> &queue) {
    ASSERT(dev_ != nullptr);

    // This is the application's sample queue
    queue_ = queue;

    thread_ = std::thread([&] {
        // The streamming loop continues to run as long as this is true
        running = true;

        // Keep track of the total accumulated samples
        size_t num_acc_samps = 0;

        // Set the antenana for receiving our signal
        dev_->set_rx_antenna("TX/RX");

        // Check sensors for a local oscillator
        std::vector<std::string> sensor_names = dev_->get_rx_sensor_names(0);

        // If a local oscillator exists, make sure it's locked
        if (std::find(sensor_names.begin(), sensor_names.end(), "lo_locked") !=
            sensor_names.end()) {
            std::cout << std::endl
                      << "-- Found a local oscillator" << std::endl;

            int lock_attempts = 0;

            do {
                if (lock_attempts++ > 3) // Abort after three attempts
                {
                    std::cout << "Unable to lock local oscillator" << std::endl;

                    exit(1);
                }

                // Allow the front end time to settle
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } while (not dev_->get_rx_sensor("lo_locked").to_bool());

            std::cout << "-- Local oscillator locked" << std::endl << std::endl;
        }

        // Specify that we want to receive samples as a pair of 16 bit complex
        // foats
        uhd::stream_args_t stream_args("fc32");

        // Create a receive stream
        uhd::rx_streamer::sptr rx_stream = dev_->get_rx_stream(stream_args);

        // Create a stream command that will start immediately and run
        // continuously
        uhd::stream_cmd_t stream_cmd_start(
            uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

        stream_cmd_start.stream_now = true;

        // Start streaming
        rx_stream->issue_stream_cmd(stream_cmd_start);

        // Holds stream metadata, such as error codes
        uhd::rx_metadata_t md;

        // Create an internal buffer to hold received samples
        std::vector<std::complex<float>> buff(rx_stream->get_max_num_samps() *
                                              2);

        while (running) {
            // Read samples from the device and into our buffer
            size_t num_rx_samps =
                rx_stream->recv(&buff.front(), buff.size(), md);

            // Handle any error codes we receive
            switch (md.error_code) {
            case uhd::rx_metadata_t::ERROR_CODE_NONE:
                break;

            case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
                if (num_acc_samps == 0)
                    continue;

                std::cerr << "Timeout before all samples received, possible "
                             "packet loss"
                          << std::endl;

                goto done_loop;

            case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
                if (md.out_of_sequence == true)
                    std::cerr << "Samples out of sequence" << std::endl;
                else
                    std::cerr << "Sample buffer overflow.  If this persists, "
                                 "try reducing the sample rate."
                              << std::endl;
                num_rx_samps = 0;

                continue;

            default:
                std::cerr << "Received error code " << md.error_code
                          << std::endl;
                std::cerr << "flags " << md.out_of_sequence << std::endl;
                goto done_loop;
            }

            // The receiving queue is expecting a number of samples evenly
            // divisible by four
            if (num_rx_samps % 4 != 0)
                continue;

            // Copies samples from our internal buffer into the application's
            // queue
            transfer_buffer(buff, num_rx_samps);

            // Increment the accumulated sample count
            num_acc_samps += num_rx_samps;
        }
    done_loop:

        // setup streaming
        uhd::stream_cmd_t stream_cmd_stop(
            uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);

        // stream_cmd.stream_now = true;

        rx_stream->issue_stream_cmd(stream_cmd_stop);

        // finished
        std::cout << std::endl << "Streaming ended" << std::endl << std::endl;

        return EXIT_SUCCESS;
    });

#ifdef __APPLE__
    pthread_setname_np("uhd");
#else
    pthread_setname_np(thread_.native_handle(), "uhd");
#endif
}

void UHD::stop() {
    ASSERT(dev_ != nullptr);

    // Causes the sample streaming loop to exit
    running = false;

    // Wait for thread to terminate
    thread_.join();

    // Close queue to signal downstream
    queue_->close();

    // Clear reference to queue
    queue_.reset();
}

// Copies samples from our internal buffer into the application's queue
void UHD::transfer_buffer(std::vector<std::complex<float>> transfer,
                          size_t num_rx_samps) {
    // Expect multiple of 2
    ASSERT((num_rx_samps & 0x2) == 0);

    // Grab buffer from queue
    auto out = queue_->popForWrite();

    out->resize(num_rx_samps);

    std::complex<float> *p = out->data();

    for (int i = 0; i < int(num_rx_samps); i++) {
        p[i] = transfer[i];
    }

    // Publish output if applicable
    if (samplePublisher_) {
        samplePublisher_->publish(*out);
    }

    // Return buffer to queue
    queue_->pushWrite(std::move(out));
}
