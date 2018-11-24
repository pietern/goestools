#pragma once

#include <atomic>
#include <fstream>
#include <memory>
#include <thread>

#include "source.h"

class File : public Source {
public:
  static std::unique_ptr<File> open(const Config& config);

  explicit File(std::ifstream f);
  ~File();

  void setSamplePublisher(std::unique_ptr<SamplePublisher> samplePublisher) {
    samplePublisher_ = std::move(samplePublisher);
  }

  void setSampleRate(uint32_t rate);

  virtual uint32_t getSampleRate() const override;

  virtual void start(const std::shared_ptr<Queue<Samples> >& queue) override;

  virtual void stop() override;

protected:
  void loop();

  std::ifstream f_;
  std::atomic<bool> stop_;
  std::thread thread_;

  // Not used by this source but part of base class interface
  uint32_t sampleRate_;

  // Set on start; cleared on stop
  std::shared_ptr<Queue<Samples> > queue_;

  // Optional publisher for samples
  std::unique_ptr<SamplePublisher> samplePublisher_;
};
