#include "handler_emwin.h"

#include "lib/util.h"
#include "lib/zip.h"

#include "filename.h"
#include "string.h"

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

  // Decompress if this is a ZIP file
  if (nlh.noaaSpecificCompression == 10) {
    auto zip = Zip(f->getData());
    fb.filename = zip.fileName();

    // Use filename and extension straight from ZIP file
    fileWriter_->write(fb.build("{filename}"), zip.read());
    return;
  }

  // Write with TXT extension if this is not a compressed file
  if (nlh.noaaSpecificCompression == 0) {
    auto filename = removeSuffix(f->getHeader<lrit::AnnotationHeader>().text);
    fb.filename = filename;

    // Compressed TXT files also use the uppercase TXT extension
    fileWriter_->write(fb.build("{filename}", "TXT"), f->read());
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
