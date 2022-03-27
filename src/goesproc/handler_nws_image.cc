#include "handler_nws_image.h"

#include "filename.h"
#include "image.h"
#include "string.h"

namespace {

std::string parseTime(const std::string& text, struct timespec& time) {
  // This field used an irregular pattern before November 2020.
  // See https://github.com/pietern/goestools/issues/100 for historical context.
  const char* buf = text.c_str();
  const char* format = "%Y%m%d%H%M%S";
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  const auto ptr = strptime(buf, format, &tm);

  // Only use time if strptime was successful.
  // Format with zero padding is always 14 characters.
  // The character after the time must be '-'.
  if (ptr != (buf + 14) || ptr[0] != '-') {
    return text;
  }

  time.tv_sec = mktime(&tm);
  time.tv_nsec = 0;

  // Return everything after the separator.
  return std::string(&ptr[1]);
}

} // namespace

NWSImageHandler::NWSImageHandler(
  const Config::Handler& config,
  const std::shared_ptr<FileWriter>& fileWriter)
  : config_(config),
    fileWriter_(fileWriter) {
}

void NWSImageHandler::handle(std::shared_ptr<const lrit::File> f) {
  auto ph = f->getHeader<lrit::PrimaryHeader>();
  if (ph.fileType != 0) {
    return;
  }

  // Filter NWS
  auto nlh = f->getHeader<lrit::NOAALRITHeader>();
  if (nlh.productID != 6) {
    return;
  }

  FilenameBuilder fb;
  fb.dir = config_.dir;
  fb.filename = getBasename(*f);

  // In the GOES-15 LRIT stream these text files have a time stamp
  // header; in the GOES-R HRIT stream they don't.
  if (f->hasHeader<lrit::TimeStampHeader>()) {
    fb.time = f->getHeader<lrit::TimeStampHeader>().getUnix();
  } else {
    // If time can successfully be extracted from the filename
    // then remove it from the filename passed to the builder.
    fb.filename = parseTime(fb.filename, fb.time);
  }

  // If this is a GIF we can write it directly
  if (nlh.noaaSpecificCompression == 5) {
    auto path = fb.build(config_.filename, "gif");
    fileWriter_->write(path, f->read());
    if (config_.json) {
      fileWriter_->writeHeader(*f, path);
    }
    return;
  }

  auto image = Image::createFromFile(f);
  auto path = fb.build(config_.filename, config_.format);
  fileWriter_->write(path, image->getRawImage());
  if (config_.json) {
    fileWriter_->writeHeader(*f, path);
  }
  return;
}

std::string NWSImageHandler::getBasename(const lrit::File& f) const {
  auto str = removeSuffix(f.getHeader<lrit::AnnotationHeader>().text);

  // Use annotation without the "dat327221257926" suffix
  auto pos = str.find("dat");
  if (pos != std::string::npos) {
    str = str.substr(0, pos);
  }

  return str;
}
