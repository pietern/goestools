#pragma once

#include "source.h"
#include <memory>
#include <stdint.h>
#include <thread>
#include <uhd.h>
#include <uhd/exception.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <vector>

class UHD : public Source {
  public:
    static std::unique_ptr<UHD> open(std::string type);

    explicit UHD(uhd::usrp::multi_usrp::sptr dev);

    ~UHD();

    void setFrequency(uint32_t freq);

    // Also sets the bandwidth to match
    void setSampleRate(uint32_t rate);

    // Reads the current sample rate setting from the device
    virtual uint32_t getSampleRate() const override;

    void setGain(int gain);

    void setSamplePublisher(std::unique_ptr<SamplePublisher> samplePublisher) {
        samplePublisher_ = std::move(samplePublisher);
    }

    // Starts the sample streaming loop
    virtual void start(const std::shared_ptr<Queue<Samples>> &queue) override;

    // Stops the sample streaming loop
    virtual void stop() override;

  protected:
    // When set to false the sample streaming loop exits
    bool running;

    // A reference to the device
    uhd::usrp::multi_usrp::sptr dev_;

    // Background RX thread
    std::thread thread_;

    // Copies samples from our internal buffer into the application's queue
    virtual void transfer_buffer(std::vector<std::complex<float>> transfer,
                                 size_t num_rx_samps);

    // Set on start; cleared on stop
    std::shared_ptr<Queue<Samples>> queue_;

    // Optional publisher for samples
    std::unique_ptr<SamplePublisher> samplePublisher_;
};
