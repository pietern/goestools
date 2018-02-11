#pragma once

#include <memory>
#include <vector>

#include "handler.h"

// Takes a list of paths to LRIT files and/or directories.
//
// This class sorts the files in chronological order and then feeds
// them to the handlers.
//
class LRITProcessor {
public:
  explicit LRITProcessor(std::vector<std::unique_ptr<Handler> > handlers);

  void run(int argc, char** argv);

protected:
  std::vector<std::unique_ptr<Handler> > handlers_;
};
