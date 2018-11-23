#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <iostream>
#include <sstream>

#include "file.h"
#include "json.h"

struct Options {
  bool extract = false;
  bool json = false;
  bool verbose = false;
};

int dump(const Options& opts, const std::string& name) {
  auto file = lrit::File(name);
  std::cout << name << ":" << std::endl;
  for (const auto& it : file.getHeaderMap()) {
    if (it.first == lrit::PrimaryHeader::CODE) {
      auto h = file.getHeader<lrit::PrimaryHeader>();
      std::cout << "Primary (" << decltype(h)::CODE << "):" << std::endl;

      std::string type;
      switch (h.fileType) {
      case 0:
        type = "Image Data";
        break;
      case 1:
        type = "Service Message";
        break;
      case 2:
        type = "Alphanumeric Text";
        break;
      case 3:
        type = "Meteorological Data";
        break;
      case 4:
        type = "GTS Messages";
        break;
      default:
        type = "unknown";
        break;
      }

      std::cout << "  File type: "
                << int(h.fileType)
                << " (" << type << ")"
                << std::endl;

      // LRIT value is in bytes
      auto headerBits = h.totalHeaderLength * 8;
      auto headerBytes = h.totalHeaderLength;
      std::cout << "  Header header length (bits/bytes): "
                << headerBits
                << "/"
                << headerBytes
                << std::endl;

      // LRIT value is in bits
      auto dataBits = h.dataLength;
      auto dataBytes = h.dataLength / 8;
      std::cout << "  Data length (bits/bytes): "
                << dataBits
                << "/"
                << dataBytes
                << std::endl;

      std::cout << "  Total length (bits/bytes): "
                << (headerBits + dataBits)
                << "/"
                << (headerBytes + dataBytes)
                << std::endl;
    } else if (it.first == lrit::ImageStructureHeader::CODE) {
      auto h = file.getHeader<lrit::ImageStructureHeader>();
      std::cout << "Image structure (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Bits per pixel: "
                << int(h.bitsPerPixel) << std::endl;
      std::cout << "  Columns: "
                << h.columns << std::endl;
      std::cout << "  Lines: "
                << h.lines << std::endl;
      std::cout << "  Compression: "
                << int(h.compression) << std::endl;
    } else if (it.first == lrit::ImageNavigationHeader::CODE) {
      auto h = file.getHeader<lrit::ImageNavigationHeader>();
      std::cout << "Image navigation (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Projection name: "
                << h.projectionName << std::endl;
      std::cout << "  Column scaling: "
                << h.columnScaling << std::endl;
      std::cout << "  Line scaling: "
                << h.lineScaling << std::endl;
      std::cout << "  Column offset: "
                << h.columnOffset << std::endl;
      std::cout << "  Line offset: "
                << h.lineOffset << std::endl;
    } else if (it.first == lrit::ImageDataFunctionHeader::CODE) {
      auto h = file.getHeader<lrit::ImageDataFunctionHeader>();
      std::cout << "Image data function (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Data: ";
      if (opts.verbose) {
        const auto str = std::string((const char*) h.data.data(), h.data.size());
        std::istringstream iss(str);
        std::string line;
        std::cout << std::endl;
        while (std::getline(iss, line, '\n')) {
          std::cout << "    " << line << std::endl;
        }
      } else {
        std::cout << "(omitted, length: " << h.data.size() << ")" << std::endl;
      }
    } else if (it.first == lrit::AnnotationHeader::CODE) {
      auto h = file.getHeader<lrit::AnnotationHeader>();
      std::cout << "Annotation (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Text: "
                << h.text << std::endl;
    } else if (it.first == lrit::TimeStampHeader::CODE) {
      std::array<char, 128> tsbuf;
      auto h = file.getHeader<lrit::TimeStampHeader>();
      auto ts = h.getUnix();
      auto len = strftime(
        tsbuf.data(),
        tsbuf.size(),
        "%Y-%m-%d %H:%M:%S",
        gmtime(&ts.tv_sec));
      std::cout << "Time stamp (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Time stamp: "
                << std::string(tsbuf.data(), len) << std::endl;
    } else if (it.first == lrit::AncillaryTextHeader::CODE) {
      auto h = file.getHeader<lrit::AncillaryTextHeader>();
      std::cout << "Ancillary text (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Text: "
                << h.text << std::endl;
    } else if (it.first == lrit::SegmentIdentificationHeader::CODE) {
      auto h = file.getHeader<lrit::SegmentIdentificationHeader>();
      std::cout << "Segment identification (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Image identifier: "
                << h.imageIdentifier << std::endl;
      std::cout << "  Segment number: "
                << h.segmentNumber << std::endl;
      std::cout << "  Start column of segment: "
                << h.segmentStartColumn << std::endl;
      std::cout << "  Start line of segment: "
                << h.segmentStartLine << std::endl;
      std::cout << "  Number of segments: "
                << h.maxSegment << std::endl;
      std::cout << "  Width of final image: "
                << h.maxColumn << std::endl;
      std::cout << "  Height of final image: "
                << h.maxLine << std::endl;
    } else if (it.first == lrit::NOAALRITHeader::CODE) {
      auto h = file.getHeader<lrit::NOAALRITHeader>();
      std::cout << "NOAA LRIT (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Agency signature: "
                << std::string(h.agencySignature) << std::endl;
      std::cout << "  Product ID: "
                << h.productID << std::endl;
      std::cout << "  Product SubID: "
                << h.productSubID << std::endl;
      std::cout << "  Parameter: "
                << h.parameter << std::endl;
      std::cout << "  NOAA-specific compression: "
                << int(h.noaaSpecificCompression) << std::endl;
    } else if (it.first == lrit::HeaderStructureRecordHeader::CODE) {
      // Ignore...
    } else if (it.first == lrit::RiceCompressionHeader::CODE) {
      auto h = file.getHeader<lrit::RiceCompressionHeader>();
      std::cout << "Rice compression (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  Flags: "
                << h.flags << std::endl;
      std::cout << "  Pixels per block: "
                << int(h.pixelsPerBlock) << std::endl;
      std::cout << "  Scan lines per packet: "
                << int(h.scanLinesPerPacket) << std::endl;
    } else if (it.first == lrit::DCSFileNameHeader::CODE) {
      auto h = file.getHeader<lrit::DCSFileNameHeader>();
      std::cout << "DCS file name (" << decltype(h)::CODE << "):" << std::endl;
      std::cout << "  File name: " << h.fileName << std::endl;

    } else {
      std::cerr << "Header " << it.first << " not handled..." << std::endl;
      return 1;
    }
  }
  return 0;
}

int extract(const Options&, const std::string& name) {
  auto file = lrit::File(name);

  // Streaming copy from file to standard out
  auto data = file.getData();
  auto& ifs = *data;
  auto& ofs = std::cout;
  std::array<char, 8192> buf;
  while (!ifs.eof()) {
    ifs.read(buf.data(), buf.size());
    ofs.write(buf.data(), ifs.gcount());
  }

  return 0;
}

int json(const Options& opts, const std::string& name) {
  auto file = lrit::File(name);
  std::cout << toJSON(file) << std::endl;
  return 0;
}

void usage(int argc, char** argv) {
  fprintf(stderr, "Usage: %s [OPTIONS] FILE\n", argv[0]);
  fprintf(stderr, "Inspect LRIT files.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -x, --extract  Print data section of file\n");
  fprintf(stderr, "  -j, --json     Print header in JSON format\n");
  fprintf(stderr, "      --help     Show this help\n");
  fprintf(stderr, "\n");
  exit(0);
}

int main(int argc, char** argv) {
  Options opts;

  while (1) {
    static struct option longOpts[] = {
      {"extract", no_argument, nullptr, 'x'},
      {"json",    no_argument, nullptr, 'j'},
      {"verbose", no_argument, nullptr, 'v'},
      {"help",    no_argument, nullptr, 0x1337},
      {nullptr,   0,           nullptr, 0},
    };

    auto c = getopt_long(argc, argv, "xjv", longOpts, nullptr);
    if (c == -1) {
      break;
    }

    switch (c) {
    case 0:
      break;
    case 'x':
      opts.extract = true;
      break;
    case 'j':
      opts.json = true;
      break;
    case 'v':
      opts.verbose = true;
      break;
    case 0x1337:
      usage(argc, argv);
      break;
    default:
      exit(1);
    }
  }

  for (auto i = optind; i < argc; i++) {
    struct stat st;
    auto rv = stat(argv[i], &st);
    if (rv < 0) {
      fprintf(stderr, "%s: %s\n", argv[i], strerror(errno));
      exit(1);
    }
    if (opts.extract) {
      rv = extract(opts, argv[i]);
      if (rv != 0) {
        return rv;
      }
    } else {
      if (opts.json) {
        rv = json(opts, argv[i]);
        if (rv != 0) {
          return rv;
        }
        continue;
      }
      rv = dump(opts, argv[i]);
      if (rv != 0) {
        return rv;
      }
    }
  }

  return 0;
}
