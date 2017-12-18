#include <getopt.h>

#include <iostream>
#include <memory>

#include "dcs.h"
#include "file.h"

int main(int argc, char** argv) {
  int rv;
  int nread;

  for (int i = optind; i < argc; i++) {
    auto file = LRIT::File(argv[i]);
    auto ph = file.getHeader<LRIT::PrimaryHeader>();
    if (ph.fileType != 130) {
      std::cerr
        << "File " << argv[i]
        << " has file type " << int(ph.fileType)
        << " (expected: 130)"
        << std::endl;
      exit(1);
    }

    // Read DCS data
    int nbytes = (ph.dataLength + 7) / 8;
    auto ifs = file.getData();
    auto buf = std::make_unique<char[]>(nbytes);
    ifs.read(buf.get(), nbytes);
    assert(ifs);
    nread = 0;

    // Read DCS file header (container for multiple DCS payloads)
    DCS::FileHeader fh;
    rv = fh.readFrom(buf.get(), nbytes);
    assert(rv > 0);
    nread += rv;

    while (nread < nbytes) {
      // Read DCS header
      DCS::Header h;
      rv = h.readFrom(buf.get() + nread, nbytes - nread);
      assert(rv > 0);
      nread += rv;

      // Skip over actual data
      nread += h.dataLength;

      // Skip 14 characters for time with milliseconds (in DCS format).
      // Skip 1 whitepace.
      // Skip 14 characters for time with milliseconds (in DCS format).
      // Skip 4 bytes (same prelude as prelude to first header)
      nread += 14 + 1 + 14 + 4;
    }

    assert(nread == nbytes);
  }

  return 0;
}
