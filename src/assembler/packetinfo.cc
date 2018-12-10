#include <array>
#include <fstream>
#include <iostream>

#include <util/error.h>

#include "vcdu.h"

int main(int argc, char** argv) {
  std::ifstream f(argv[1]);
  ASSERT(f.good());

  std::array<uint8_t, 892> buf;
  for (;;) {
    f.read((char*) buf.data(), buf.size());
    if (f.eof()) {
      break;
    }

    VCDU vcdu(buf);
    std::cout
      << "SCID: " << vcdu.getSCID()
      << ", VCID: " << vcdu.getVCID()
      << ", counter: " << vcdu.getCounter()
      << std::endl;
  }

  return 0;
}
