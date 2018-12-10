#include "handler_emwin.h"

#include <regex>

#include <util/string.h>

#include "lib/zip.h"

#include "filename.h"
#include "string.h"

using namespace util;

EMWINHandler::EMWINHandler(
  const Config::Handler& config,
  const std::shared_ptr<FileWriter>& fileWriter)
  : config_(config),
    fileWriter_(fileWriter) {
}

void EMWINHandler::handle(std::shared_ptr<const lrit::File> f) {
  auto ph = f->getHeader<lrit::PrimaryHeader>();
  if (ph.fileType != 2 ) {
    return;
  }

  // Filter EMWIN
  auto nlh = f->getHeader<lrit::NOAALRITHeader>();
  if (nlh.productID != 9) {
    return;
  }

  // Extract time stamp from filename
  struct timespec time = {0, 0};
  if (!extractTimeStamp(*f, time)) {
    return;
  }

  FilenameBuilder fb;
  fb.dir = config_.dir;
  fb.time = time;

  // Assume we can extract WMO abbreviated heading from filename
  if (!extractAWIPS(*f, fb.awips)) {
    return;
  }

  // Decompress if this is a ZIP file
  if (nlh.noaaSpecificCompression == 10) {
    auto zip = Zip(f->getData());
    fb.filename = zip.fileName();

    // Use filename and extension straight from ZIP file
    const auto path = fb.build("{filename}");
    fileWriter_->write(path, zip.read());
    if (config_.json) {
      fileWriter_->writeHeader(*f, path);
    }
    return;
  }

  // Write with TXT extension if this is not a compressed file
  if (nlh.noaaSpecificCompression == 0) {
    auto filename = removeSuffix(f->getHeader<lrit::AnnotationHeader>().text);
    fb.filename = filename;

    // Compressed TXT files also use the uppercase TXT extension
    const auto path = fb.build("{filename}", "TXT");
    fileWriter_->write(path, f->read());
    if (config_.json) {
      fileWriter_->writeHeader(*f, path);
    }
    return;
  }
}

// Extract time stamp from filename.
// See http://www.nws.noaa.gov/emwin/EMWIN_GOES-R_filename_convention.pdf
bool EMWINHandler::extractTimeStamp(
    const lrit::File& f,
    struct timespec& ts) const {
  auto text = f.getHeader<lrit::AnnotationHeader>().text;
  auto parts = split(text, '_');
  if (parts.size() < 5) {
    return false;
  }

  const char* buf = parts[4].c_str();
  const char* format = "%Y%m%d%H%M%S";
  struct tm tm;
  auto ptr = strptime(buf, format, &tm);

  // Only use time if strptime was successful
  if (ptr != (buf + 14)) {
    return false;
  }

  ts.tv_sec = mktime(&tm);
  return true;
}

// Extract AWIPS information from filename.
// See https://www.weather.gov/tg/awips for full description.
bool EMWINHandler::extractAWIPS(const lrit::File& f, struct AWIPS& out) const {
  auto text = f.getHeader<lrit::AnnotationHeader>().text;
  auto parts = split(text, '_');
  if (parts.size() < 6) {
    return false;
  }

  // WMO Abbreviated Heading (https://www.weather.gov/tg/head)
  auto wmoah = std::regex(
    // T1T2A1A2ii
    "(\\w{2})(\\w{2})(\\w{2})"
    // CCCC
    "(\\w{4})"
    // YYGGgg
    "(\\d{2})(\\d{4})"
    // BBB (optional)
    "(\\w{3})?");

  // AWIPS Identifier (https://www.weather.gov/tg/awips)
  auto awipsid = std::regex(
    "^"
    // nnnnnn (Message sequence number to guarantee uniqueness)
    "(\\d{6})-"
    // p (EMWIN assigned product priority integer)
    "(\\d{1})-"
    // NNN
    "(\\w{3})"
    // xxx
    "(\\w{3})"
    // qq
    "(\\w{2})");

  std::smatch smw;
  std::smatch sma;

  // Find WMO Abbreviated Heading in the 2nd chunk
  if (!std::regex_match(parts[1], smw, wmoah)) {
    return false;
  }

  // Find AWIPS identifier in the 6th chunk
  if (!std::regex_search(parts[5], sma, awipsid)) {
    return false;
  }

  out.t1t2 = smw[1];
  out.a1a2 = smw[2];
  out.ii = smw[3];
  out.cccc = smw[4];
  out.yy = smw[5];
  out.gggg = smw[6];
  out.bbb = smw[8];
  out.nnn = sma[3];
  out.xxx = sma[4];
  out.qq = sma[5];
  return true;
}
