#pragma once

#include "session_pdu.h"

class SHAHandler {
public:
  void handle(std::unique_ptr<SessionPDU> spdu);
};
