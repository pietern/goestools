#pragma once

#include <memory>

#include "lrit/file.h"

// Handler is a base class for anything that can handle LRIT files.
// There are multiple image handlers, text handlers, etc.
class Handler {
public:
  virtual void handle(std::shared_ptr<const lrit::File> f) = 0;
};
