#include "config.h"

#include <toml/toml.h>

namespace {

bool loadHandlers(const toml::Value& v, Config& out) {
  auto ths = v.find("handler");
  if (ths->size() == 0) {
    out.ok = false;
    out.error = "No handlers found";
    return false;
  }

  for (size_t i = 0; i < ths->size(); i++) {
    auto th = ths->find(i);

    Config::Handler h;
    h.type = th->get<std::string>("type");
    h.product = th->get<std::string>("product");

    auto region = th->find("region");
    if (region) {
      h.region = region->as<std::string>();
    }

    auto channel = th->find("channel");
    if (channel) {
      h.channel = channel->as<std::string>();
    }

    auto dir = th->find("dir");
    if (dir) {
      h.dir = dir->as<std::string>();
    } else {
      h.dir = ".";
    }

    out.handlers.push_back(h);
  }

  return true;
}

} // namespace

Config Config::load(const std::string& file) {
  Config out;

  auto pr = toml::parseFile(file);
  if (!pr.valid()) {
    out.ok = false;
    out.error = pr.errorReason;
    return out;
  }

  const auto& v = pr.value;
  if (!loadHandlers(v, out)) {
    return out;
  }

  return out;
}

Config::Config() : ok(true) {
}
