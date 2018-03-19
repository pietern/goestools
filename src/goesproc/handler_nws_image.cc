#include "handler_nws_image.h"

#include <cassert>

#include "lib/util.h"

#include "image.h"

NWSImageHandler::NWSImageHandler(
  const Config::Handler& config,
  const std::shared_ptr<FileWriter>& fileWriter)
  : config_(config),
    fileWriter_(fileWriter) {
  // Ensure output directory exists
  mkdirp(config_.dir);
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

  auto filename = getBasename(*f);

  // If this is a GIF we can write it directly
  if (nlh.noaaSpecificCompression == 5) {
    auto path = config_.dir + "/" + filename + ".gif";
    fileWriter_->write(path, f->read());
    return;
  }

  auto image = Image::createFromFile(f);
  auto path = config_.dir + "/" + filename + "." + config_.format;
  fileWriter_->write(path, image->getRawImage());
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
