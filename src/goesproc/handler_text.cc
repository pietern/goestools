#include "handler_text.h"

#include <regex>

#include "filename.h"
#include "string.h"

namespace {

// Expect the file to be named like this:
//
//   16-TEXTdat_17348_201455.lrit
//
bool goesrParseTextTime(const std::string& name, struct timespec& time) {
  auto pos = name.find('_');
  if (pos == std::string::npos) {
    return false;
  }

  if (pos + 1 >= name.size()) {
    return false;
  }

  const char* buf = name.c_str() + pos + 1;
  const char* format = "%y%j_%H%M%S";
  struct tm tm;
  auto ptr = strptime(buf, format, &tm);

  // strptime was successful if it returned a pointer to the next char
  if (ptr == nullptr || ptr[0] != '.') {
    return false;
  }

  time.tv_sec = mktime(&tm);
  time.tv_nsec = 0;
  return true;
}

} // namespace

TextHandler::TextHandler(
  const Config::Handler& config,
  const std::shared_ptr<FileWriter>& fileWriter)
  : config_(config),
    fileWriter_(fileWriter) {
}

void TextHandler::handle(std::shared_ptr<const lrit::File> f) {
  auto ph = f->getHeader<lrit::PrimaryHeader>();
  if (ph.fileType != 2) {
    return;
  }

  // Filter out NWS
  auto nlh = f->getHeader<lrit::NOAALRITHeader>();
  if (nlh.productID == 1) {
    // LRIT / GOES-N series.
    // All text files on the LRIT stream use the same product ID and
    // product sub ID in the NOAA LRIT header. Look at the annotation
    // header to see if this is an NWS report or not.
    auto text = f->getHeader<lrit::AnnotationHeader>().text;
    const auto prefix = std::string("NWS");
    if (text.compare(0, prefix.size(), prefix) == 0) {
      return;
    }
  } else {
    // Something else; ignore
    return;
  }

  struct timespec time = {0, 0};

  // In the GOES-15 LRIT stream these text files have a time stamp
  // header; in the GOES-R HRIT stream they don't.
  if (f->hasHeader<lrit::TimeStampHeader>()) {
    time = f->getHeader<lrit::TimeStampHeader>().getUnix();
  } else {
    auto text = f->getHeader<lrit::AnnotationHeader>().text;

    // Parse time from file name.
    if (!goesrParseTextTime(text, time)) {
      // Unable to extract timestamp from file name
    }
  }

  // Skip if the time could not be determined
  if (time.tv_sec == 0) {
    return;
  }

  FilenameBuilder fb;
  fb.dir = config_.dir;
  fb.filename = removeSuffix(f->getHeader<lrit::AnnotationHeader>().text);
  fb.time = time;
  auto path = fb.build(config_.filename, "txt");
  fileWriter_->write(path, f->read());
  if (config_.json) {
    fileWriter_->writeHeader(*f, path);
  }
}
