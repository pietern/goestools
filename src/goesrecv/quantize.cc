#include "quantize.h"

Quantize::Quantize() {
}

void Quantize::work(
    const std::shared_ptr<Queue<std::vector<std::complex<float> > > >& qin,
    const std::shared_ptr<Queue<std::vector<int8_t> > >& qout) {
  auto input = qin->popForRead();
  if (!input) {
    qout->close();
    return;
  }

  // Clear output so we can use push_back.
  // It will retain the associated memory allocation.
  auto output = qout->popForWrite();
  output->clear();

  auto& rinput = *input;
  auto& routput = *output;
  auto nsamples = input->size();
  for (size_t i = 0; i < nsamples; i++) {
    int8_t v = rinput[i].real() * 127.0f;
    routput.push_back(v);
  }

  // Return input buffer
  qin->pushRead(std::move(input));

  // Publish output if applicable
  if (softBitPublisher_) {
    softBitPublisher_->publish(*output);
  }

  // Return output buffer
  qout->pushWrite(std::move(output));
}
