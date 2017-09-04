#pragma once

#include "session_pdu.h"

class FileHandler {
public:
  FileHandler(const std::string& dir);

  void handle(std::unique_ptr<SessionPDU> spdu);

protected:
  void save(const SessionPDU& spdu, const std::string& path);
  void createDirectories(const std::string& path);

  std::string dir_;
};
