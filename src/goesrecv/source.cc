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
    airspy->setGain(18);
    airspy->setSamplePublisher(std::move(config.source.samplePublisher));
    return std::unique_ptr<Source>(airspy.release());
  }
#endif
#ifdef BUILD_RTLSDR
  if (type == "rtlsdr") {
    auto rtlsdr = RTLSDR::open();
    rtlsdr->setSampleRate(2400000);
    rtlsdr->setTunerGain(30);
    rtlsdr->setSamplePublisher(std::move(config.source.samplePublisher));
    return std::unique_ptr<Source>(rtlsdr.release());
  }
#endif
  if (type == "nanomsg") {
    auto nanomsg = Nanomsg::open(config);
    nanomsg->setSampleRate(config.nanomsg.sampleRate);
    nanomsg->setSamplePublisher(std::move(config.source.samplePublisher));
    return std::unique_ptr<Source>(nanomsg.release());
  }

  throw std::runtime_error("Invalid source configured");
}

Source::~Source() {
}
