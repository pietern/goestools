#pragma once

#include "config.h"
#include "handler.h"

class NWSImageHandler : public Handler {
public:
  explicit NWSImageHandler(const Config::Handler& config);

  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  std::string getBasename(const lrit::File& f) const;

  Config::Handler config_;
};
