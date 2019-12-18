#pragma once

#include <stdint.h>

#include <memory>
#include <thread>
#include <vector>

#include <uhd.h>
#include <uhd/exception.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>


#include "source.h"

class UHD : public Source
{
    public:
        static std::unique_ptr<UHD> open( std::string type );

        explicit UHD( uhd::usrp::multi_usrp::sptr dev );

        ~UHD( );

        std::vector<uint32_t> getSampleRates( ) const
        {
            return sampleRates_;
        }

        void setFrequency( uint32_t freq );

        void setSampleRate( uint32_t rate );

        virtual uint32_t getSampleRate( ) const override;

        void setGain( int gain );

        // void setBiasTee( bool on );

        void setSamplePublisher( std::unique_ptr<SamplePublisher> samplePublisher )
        {
            samplePublisher_ = std::move( samplePublisher );
        }

        virtual void start( const std::shared_ptr<Queue<Samples>> &queue ) override;

        virtual void stop( ) override;

    protected:

        bool running;

        uhd::usrp::multi_usrp::sptr dev_;

        std::vector<uint32_t> loadSampleRates( );

        std::vector<uint32_t> sampleRates_;

        std::uint32_t sampleRate_;

        // Background RX thread
        std::thread thread_;

        virtual void transfer_buffer( std::vector<std::complex<float>> transfer, size_t num_rx_samps );

        // Set on start; cleared on stop
        std::shared_ptr<Queue<Samples> > queue_;

        // Optional publisher for samples
        std::unique_ptr<SamplePublisher> samplePublisher_;
};
