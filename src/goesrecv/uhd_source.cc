#include <thread>

#include "uhd_source.h"

#include <pthread.h>

#include <cstring>
#include <iostream>

#include <util/error.h>
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/thread_priority.hpp>


std::unique_ptr<UHD> UHD::open( std::string type )
{
    std::string args = "";

    // Filter to a device type if one was specified
    if ( not type.empty( ) )
    {
        args = "type=" + type;
    }

    // Set the scheudling priority on the current thread
    uhd::set_thread_priority_safe( );

    // Create a USRP device
    uhd::usrp::multi_usrp::sptr dev = nullptr;

    UHD_SAFE_CALL
    (
        dev = uhd::usrp::multi_usrp::make( args );
    )

    // Abort if we didn't open a device
    if ( dev == nullptr )
    {
        std::cout << "Unable to open UHD device. " << std::endl;

        exit( 1 );
    }

    std::cout << std::endl << "Using Device " << dev->get_pp_string( ) << std::endl;

    // Lock mboard clocks
    dev->set_clock_source( "internal" );

    return std::make_unique<UHD>( dev );
}

UHD::UHD( uhd::usrp::multi_usrp::sptr dev ) : dev_( dev )
{
    // Load list of supported sample rates
    sampleRates_ = loadSampleRates( );
}

UHD::~UHD( )
{

}

std::vector<uint32_t> UHD::loadSampleRates( )
{
    return { 200000, 250000, 2000000, 8000000, 10000000, 12500000, 16000000, 20000000, 56000000 };
}

void UHD::setFrequency( uint32_t freq )
{
    ASSERT( dev_ != nullptr );

    uhd::tune_request_t tune_request( freq );

    dev_->set_rx_freq( tune_request );
}

void UHD::setSampleRate( uint32_t rate )
{
    ASSERT( dev_ != nullptr );

    dev_->set_rx_rate( rate );

    // Set the bandwidth to match the sample rate
    dev_->set_rx_bandwidth( rate );

    sampleRate_ = rate;
}

uint32_t UHD::getSampleRate( ) const
{
    return sampleRate_;
}

void UHD::setGain( int gain )
{
    ASSERT( dev_ != nullptr );

    dev_->set_rx_gain( gain );
}

void UHD::start( const std::shared_ptr<Queue<Samples>> &queue )
{
    ASSERT( dev_ != nullptr );

    queue_ = queue;

    thread_ = std::thread( [&] {

        running = true;

        size_t num_acc_samps = 0; // number of accumulated samples

        // set the antenna
        dev_->set_rx_antenna( "TX/RX" );

        std::this_thread::sleep_for( std::chrono::seconds( 1 ) ); // allow for some setup time

        // Check Ref and LO Lock detect
        std::vector<std::string> sensor_names;

        sensor_names = dev_->get_rx_sensor_names( 0 );

        if ( std::find( sensor_names.begin( ), sensor_names.end( ), "lo_locked" ) != sensor_names.end( ) )
        {
            uhd::sensor_value_t lo_locked = dev_->get_rx_sensor( "lo_locked", 0 );

            std::cout << std::endl << "Checking RX: " << lo_locked.to_pp_string( ) << std::endl << std::endl;

            UHD_ASSERT_THROW( lo_locked.to_bool( ) );
        }

        sensor_names = dev_->get_mboard_sensor_names( 0 );

        // create a receive streamer of complex floats
        uhd::stream_args_t stream_args( "fc32" );

        uhd::rx_streamer::sptr rx_stream = dev_->get_rx_stream( stream_args );

        // setup streaming
        uhd::stream_cmd_t stream_cmd_start( uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS );

        stream_cmd_start.stream_now = true;

        rx_stream->issue_stream_cmd( stream_cmd_start );

        uhd::rx_metadata_t md;

        std::vector<std::complex<float>> buff( rx_stream->get_max_num_samps( ) * 2 );

        while ( running )
        {
            size_t num_rx_samps = rx_stream->recv( &buff.front( ), buff.size( ), md );

            // handle the error codes
            switch ( md.error_code )
            {
                case uhd::rx_metadata_t::ERROR_CODE_NONE:
                    break;

                case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
                    if ( num_acc_samps == 0 )
                        continue;

                    std::cerr << "Timeout before all samples received, possible packet loss" << std::endl;

                    goto done_loop;


                case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
                    if ( md.out_of_sequence == true )
                        std::cerr << "Samples out of sequence" << std::endl;
                    else
                        std::cerr << "Sample buffer overflow.  Try reducing the sample rate." << std::endl;

                    goto done_loop;

                default:
                    std::cerr << "Received error code " << md.error_code << std::endl;
                    std::cerr << "flags " << md.out_of_sequence << std::endl;
                    goto done_loop;
            }

            transfer_buffer( buff, num_rx_samps );

#ifdef HANDLEIT
            // HANDLE

            // Expect multiple of 2
            ASSERT( ( num_rx_samps & 0x2 ) == 0 );

            if ( num_rx_samps % 4 != 0 )
            {
                std::cout << "not div by four " << num_rx_samps << std::endl;
                continue;
            }
            // Grab buffer from queue
            auto out = queue_->popForWrite( );

            out->resize( num_rx_samps );

            std::complex<float> *p = out->data( );

            for ( int i = 0; i < int( num_rx_samps ); i++ )
            {
                p[i] = buff[i];

                // std::cout << "count " << num_rx_samps << std::endl;
            }

            // Publish output if applicable
            if ( samplePublisher_ )
            {
                samplePublisher_->publish( *out );
            }

            // Return buffer to queue
            queue_->pushWrite( std::move( out ) );

             // HANDLE
#endif
            // Increment the accumulated sample count
            num_acc_samps += num_rx_samps;
        }
    done_loop:

        // setup streaming
        uhd::stream_cmd_t stream_cmd_stop( uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS );

        // stream_cmd.stream_now = true;

        rx_stream->issue_stream_cmd( stream_cmd_stop );

        // finished
        std::cout << std::endl << "Streaming ended" << std::endl << std::endl;

        return EXIT_SUCCESS;
    } );

#ifdef __APPLE__
    pthread_setname_np( "uhd" );
#else
    pthread_setname_np( thread_.native_handle( ), "uhd" );
#endif
}

void UHD::stop( )
{
    ASSERT( dev_ != nullptr );

    // cause the recieve loop to exit
    running = false;

    // Wait for thread to terminate
    thread_.join( );

    // Close queue to signal downstream
    queue_->close( );

    // Clear reference to queue
    queue_.reset( );
}

void UHD::transfer_buffer( std::vector<std::complex<float>> transfer, size_t num_rx_samps )
{
    // Expect multiple of 2
    ASSERT( ( num_rx_samps & 0x2 ) == 0 );

    // Grab buffer from queue
    auto out = queue_->popForWrite( );

    out->resize( num_rx_samps );

    std::complex<float> *p = out->data( );

    for ( int i = 0; i < int( num_rx_samps ); i++ )
    {
        p[i] = transfer[i];
    }

    // Publish output if applicable
    if ( samplePublisher_ )
    {
        samplePublisher_->publish( *out );
    }

    // Return buffer to queue
    queue_->pushWrite( std::move( out ) );
}
