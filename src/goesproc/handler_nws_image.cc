#include "handler_nws_image.h"

#include "filename.h"
#include "image.h"

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
    // Can't parse the time from file name.
    // Unlike the NWS text files, the NWS image files on GOES-R
    // don't use a consistent pattern for time in their name.
  }

  FilenameBuilder fb;
  fb.time = time;
  fb.filename = getBasename(*f);
  auto filename = fb.build(config_.filename);
  auto path = config_.dir + "/" + filename;

  // If this is a GIF we can write it directly
  if (nlh.noaaSpecificCompression == 5) {
    fileWriter_->write(path + ".gif", f->read());
    return;
  }

  auto image = Image::createFromFile(f);
  fileWriter_->write(path + "." + config_.format, image->getRawImage());
  return;
}

std::string NWSImageHandler::getBasename(const lrit::File& f) const {
  size_t pos;

  auto text = f.getHeader<lrit::AnnotationHeader>().text;

  // Use annotation without the "dat327221257926.lrit" suffix
  pos = text.find("dat");
  if (pos != std::string::npos) {
    text = text.substr(0, pos);
  }

  // Remove .lrit suffix
  pos = text.find(".lrit");
  if (pos != std::string::npos) {
    text = text.substr(0, pos);
  }

  // Add time if available
  if (f.hasHeader<lrit::TimeStampHeader>()) {
    auto tsh = f.getHeader<lrit::TimeStampHeader>();
    text += "_" + tsh.getTimeShort();
  }

  return text;
}
