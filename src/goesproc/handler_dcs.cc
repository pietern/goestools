#include "handler_dcs.h"

#include <regex>

#include "filename.h"
#include "string.h"

namespace {

// Expect the file to be named like this:
//
//   pH-19143220215-A.lrit
//
bool goesrParseDCSTime(const std::string& name, struct timespec& time) {
  auto pos = name.find('-');
  if (pos == std::string::npos) {
    return false;
  }

  if (pos + 1 >= name.size()) {
    return false;
  }

  const char* buf = name.c_str() + pos + 1;
  const char* format = "%y%j%H%M%S";
  struct tm tm;
  auto ptr = strptime(buf, format, &tm);

  // strptime was successful if it returned a pointer to the next char
  if (ptr == nullptr || ptr[0] != '-') {
    return false;
  }

  time.tv_sec = mktime(&tm);
  time.tv_nsec = 0;
  return true;
}

} // namespace

DCSHandler::DCSHandler(
  const Config::Handler& config,
  const std::shared_ptr<FileWriter>& fileWriter)
  : config_(config),
    fileWriter_(fileWriter) {
}

void DCSHandler::handle(std::shared_ptr<const lrit::File> f) {
  auto ph = f->getHeader<lrit::PrimaryHeader>();
  if (ph.fileType != 130) {
    return;
  }

  auto text = f->getHeader<lrit::AnnotationHeader>().text;
  struct timespec time = {0, 0};

  if (!goesrParseDCSTime(text, time)) {
      // Unable to extract timestamp from file name
    }


  // Skip if the time could not be determined
  if (time.tv_sec == 0) {
    return;
  }

  FilenameBuilder fb;
  fb.dir = config_.dir;
  fb.filename = removeSuffix(f->getHeader<lrit::AnnotationHeader>().text);
  fb.time = time;
  auto path = fb.build(config_.filename, "dcs");
  fileWriter_->write(path, f->read());
  if (config_.json) {
    fileWriter_->writeHeader(*f, path);
  }
}
