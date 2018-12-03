#include "handler_nws_text.h"

#include <regex>

#include "filename.h"
#include "string.h"

NWSTextHandler::NWSTextHandler(
  const Config::Handler& config,
  const std::shared_ptr<FileWriter>& fileWriter)
  : config_(config),
    fileWriter_(fileWriter) {
}

void NWSTextHandler::handle(std::shared_ptr<const lrit::File> f) {
  auto ph = f->getHeader<lrit::PrimaryHeader>();
  if (ph.fileType != 2) {
    return;
  }

  // Filter NWS
  auto nlh = f->getHeader<lrit::NOAALRITHeader>();
  if (nlh.productID == 1) {
    // LRIT / GOES-N series.
    // All text files on the LRIT stream use the same product ID and
    // product sub ID in the NOAA LRIT header. Look at the annotation
    // header to see if this is an NWS report or not.
    auto text = f->getHeader<lrit::AnnotationHeader>().text;
    const auto prefix = std::string("NWS");
    if (text.compare(0, prefix.size(), prefix) != 0) {
      return;
    }
  } else if (nlh.productID == 6) {
    // HRIT / GOES-R series
  } else {
    // Something else; ignore
    return;
  }

  struct timespec time = {0, 0};
  struct AWIPS awips;

  // In the GOES-15 LRIT stream these text files have a time stamp
  // header; in the GOES-R HRIT stream they don't.
  if (f->hasHeader<lrit::TimeStampHeader>()) {
    time = f->getHeader<lrit::TimeStampHeader>().getUnix();
  } else {
    auto text = f->getHeader<lrit::AnnotationHeader>().text;

    // Parse time from file name.
    //
    // Expect the file to be named like this:
    //
    //   20180302011202-discussion.lrit
    //
    const char* buf = text.c_str();
    const char* format = "%Y%m%d%H%M%S";
    struct tm tm;
    auto ptr = strptime(buf, format, &tm);

    // Only use time if strptime was successful
    if (ptr == (buf + 14)) {
      time.tv_sec = mktime(&tm);
      time.tv_nsec = 0;
    } else {
      // Unable to extract timestamp from file name
    }
  }

  // Skip if the time could not be determined
  if (time.tv_sec == 0) {
    return;
  }

  // Skip if the AWPIS Product Identifier cannot be extracted
  auto is = f->getData();
  if (!extractAWIPS(*is, awips)) {
    return;
  }

  FilenameBuilder fb;
  fb.dir = config_.dir;
  fb.filename = removeSuffix(f->getHeader<lrit::AnnotationHeader>().text);
  fb.time = time;
  fb.awips = awips;
  auto path = fb.build(config_.filename, "txt");
  fileWriter_->write(path, f->read());
  if (config_.json) {
    fileWriter_->writeHeader(*f, path);
  }
}

// Extract metadata from contents.
// See https://www.weather.gov/tg/awips for full description.
bool NWSTextHandler::extractAWIPS(std::istream& is, struct AWIPS& out) {
  // WMO Abbreviated Heading (https://www.weather.gov/tg/head)
  auto wmoah = std::regex(
    // T1T2A1A2ii
    "(\\w{2})(\\w{2})(\\w{2}) "
    // CCCC
    "(\\w{4}) "
    // YYGGgg
    "(\\d{2})(\\d{4})"
    // BBB
    "( (\\w{3}))?");

  // AWIPS Identifier (https://www.weather.gov/tg/awips)
  auto awipsid = std::regex(
    // NNN
    "(\\w{3})"
    // xxx (may have trailing spaces)
    "(\\w{0,3})\\s*");

  // Find WMO Abbreviated Heading the first couple of lines
  std::string lw;
  std::string la;
  std::smatch smw;
  std::smatch sma;
  for (auto i = 0; i < 5; i++) {
    std::getline(is, lw);

    // After the WMO Abbreviated Heading matches...
    if (std::regex_match(lw, smw, wmoah)) {
      // ... the AWIPS identifier MUST match.
      std::getline(is, la);
      if (!std::regex_match(la, sma, awipsid)) {
        return false;
      }
      break;
    }
  }

  if (smw.empty()) {
    return false;
  }

  out.t1t2 = smw[1];
  out.a1a2 = smw[2];
  out.ii = smw[3];
  out.cccc = smw[4];
  out.yy = smw[5];
  out.gggg = smw[6];
  out.bbb = smw[8];
  out.nnn = sma[1];
  out.xxx = sma[2];
  return true;
}
