#include <signal.h>

#include <iostream>

#include "decoder.h"
#include "demodulator.h"
#include "options.h"
#include "publisher.h"

static bool sigint = false;

static void signalHandler(int signum) {
  fprintf(stderr, "Signal caught, exiting!\n");
  sigint = true;
}

int main(int argc, char** argv) {
  auto opts = parseOptions(argc, argv);

  // Convert string option to enum
  Demodulator::Type downlinkType;
  if (opts.downlinkType == "lrit") {
    downlinkType = Demodulator::LRIT;
  } else if (opts.downlinkType == "hrit") {
    downlinkType = Demodulator::HRIT;
  } else {
    std::cerr << "Invalid downlink type: " << opts.downlinkType << std::endl;
    exit(1);
  }

  Demodulator demod(downlinkType);
  if (!demod.configureSource(opts.device)) {
    std::cerr << "Invalid device: " << opts.device << std::endl;
    exit(1);
  }

  Decoder decode(demod.getSoftBitsQueue());

  // Install signal handler
  struct sigaction sa;
  sa.sa_handler = signalHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  demod.start();
  decode.start();

  while (!sigint) {
    pause();
  }

  demod.stop();
  decode.stop();

  return 0;
}
