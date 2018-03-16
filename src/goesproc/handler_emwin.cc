#include "handler_emwin.h"

#include "lib/util.h"

#include "zip.h"

EMWINHandler::EMWINHandler(
  const Config::Handler& config,
  const std::shared_ptr<FileWriter>& fileWriter)
  : config_(config),
    fileWriter_(fileWriter) {
  // Ensure output directory exists
  mkdirp(config_.dir);
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

  // Decompress if this is a ZIP file
  if (nlh.noaaSpecificCompression == 10) {
    auto zip = Zip(f->getData());
    auto path = config_.dir + "/" + zip.fileName();
    fileWriter_->write(path, zip.read());
    return;
  }

  // Write with TXT extension if this is not a compressed file
  if (nlh.noaaSpecificCompression == 0) {
    auto text = f->getHeader<lrit::AnnotationHeader>().text;

    // Remove .lrit suffix
    auto pos = text.rfind(".lrit");
    if (pos != std::string::npos) {
      text = text.substr(0, pos);
    }

    // Compressed TXT files also use the uppercase TXT extension
    auto path = config_.dir + "/" + text + ".TXT";
    fileWriter_->write(path, f->read());
    return;
  }
}
