#include "source.h"

#ifdef BUILD_AIRSPY
#include "airspy_source.h"
#endif

#ifdef BUILD_RTLSDR
#include "rtlsdr_source.h"
#endif

#include "nanomsg_source.h"

std::unique_ptr<Source> Source::build(
    const std::string& type,
    Config& config) {
#ifdef BUILD_AIRSPY
  if (type == "airspy") {
    auto airspy = Airspy::open();
    airspy->setSampleRate(3000000);
    airspy->setFrequency(config.airspy.frequency);
    airspy->setGain(config.airspy.gain);
    airspy->setSamplePublisher(std::move(config.airspy.samplePublisher));
    return std::unique_ptr<Source>(airspy.release());
  }
#endif
#ifdef BUILD_RTLSDR
  if (type == "rtlsdr") {
    auto rtlsdr = RTLSDR::open();
    rtlsdr->setSampleRate(2400000);
    rtlsdr->setFrequency(config.rtlsdr.frequency);
    rtlsdr->setTunerGain(config.rtlsdr.gain);
    rtlsdr->setSamplePublisher(std::move(config.rtlsdr.samplePublisher));
    return std::unique_ptr<Source>(rtlsdr.release());
  }
#endif
  if (type == "nanomsg") {
    auto nanomsg = Nanomsg::open(config);
    nanomsg->setSampleRate(config.nanomsg.sampleRate);
    nanomsg->setSamplePublisher(std::move(config.nanomsg.samplePublisher));
    return std::unique_ptr<Source>(nanomsg.release());
  }

  throw std::runtime_error("Invalid source configured");
}

Source::~Source() {
}
