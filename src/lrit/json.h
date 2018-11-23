#pragma once

#include <nlohmann/json.hpp>

#include "file.h"
#include "lrit.h"

namespace lrit {

nlohmann::json toJSON(const Buffer& b, const HeaderMap& m);
nlohmann::json toJSON(const File& f);

} // namespace lrit
