#include "handler_nws.h"

#include <cassert>

#include "lib/util.h"

#include "image.h"

NWSImageHandler::NWSImageHandler(const Config::Handler& config)
    : config_(config) {
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
    auto buf = f->read();
    std::ofstream of(config_.dir + "/" + filename + ".gif");
    of.write(buf.data(), buf.size());
    return;
  }

  auto image = Image::createFromFile(f);
  image->save(config_.dir + "/" + filename + ".png");
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
