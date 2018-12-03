#include "handler_nws_image.h"

#include "filename.h"
#include "image.h"
#include "string.h"

namespace {

void parseIrregularTime(const std::string& text, struct timespec& time) {
  // Unlike the NWS text files, the NWS image files on GOES-R
  // don't use a consistent pattern for time in their name.
  //
  // Example file names:
  // - 201801010001834-pacsfc48_latestBW.gif
  // - 201803640503770-pacsfc72_latestBW.gif
  // - 201803640503019-USA_latest.gif
  // - 2018041050104193-pac24_latestBW.gif
  //
  // As you can see, the month is not followed by the day of the
  // month, but the day of the year. The number of seconds and
  // sub-seconds may miss a leading 0. These ambiguities make that
  // we stick to extracting the year, month, day, hour, and minute.
  //
  const char* buf = text.c_str();
  const char* format = "%Y%m";
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  auto ptr = strptime(buf, format, &tm);

  // Only use time if strptime was successful
  if (ptr != (buf + 6)) {
    return;
  }

  // Number of characters used for day of year
  const auto month = tm.tm_mon + 1;
  auto mlen = 2;
  buf = ptr;

  // April contains both 2 and 3 digits day of year.
  // If it starts with a '1' it must be 3 digits.
  if ((month == 4 && buf[0] == '1') || month > 4) {
    mlen = 3;
  }

  // Interpret variable length day of year.
  char tmp[4];
  int yday;
  memcpy(tmp, buf, mlen);
  tmp[mlen] = 0;
  auto rv = sscanf(tmp, "%d", &yday);
  if (rv != 1 || yday >= 367) {
    return;
  }

  buf += mlen;
  format = "%H%M";
  ptr = strptime(buf, format, &tm);

  // Only use time if strptime was successful
  if (ptr != (buf + 4)) {
    return;
  }

  // Set day to January 1 before mktime, so we can use simple
  // arithmetic to get to the real day of the year.
  tm.tm_mon = 0;
  tm.tm_mday = 1;
  time.tv_sec = mktime(&tm) + (60 * 60 * 24 * (yday - 1));
  time.tv_nsec = 0;
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

  // In the GOES-15 LRIT stream these text files have a time stamp
  // header; in the GOES-R HRIT stream they don't.
  struct timespec time = {0, 0};
  if (f->hasHeader<lrit::TimeStampHeader>()) {
    time = f->getHeader<lrit::TimeStampHeader>().getUnix();
  } else {
    auto text = f->getHeader<lrit::AnnotationHeader>().text;
    parseIrregularTime(text, time);
  }

  FilenameBuilder fb;
  fb.dir = config_.dir;
  fb.filename = getBasename(*f);
  fb.time = time;

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
