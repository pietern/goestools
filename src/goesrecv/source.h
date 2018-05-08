#pragma once

#include <memory>

#include "config.h"
#include "sample_publisher.h"
#include "types.h"

// Pure virtual base class for every source of samples.
class Source {
public:
  static std::unique_ptr<Source> build(
      const std::string& type,
      Config& config);

  virtual ~Source();

  // Sample rate is set in the configuration
  virtual uint32_t getSampleRate() const = 0;

  // Start producing samples
  virtual void start(const std::shared_ptr<Queue<Samples> >& queue) = 0;

  // Stop producing samples
  virtual void stop() = 0;
};
