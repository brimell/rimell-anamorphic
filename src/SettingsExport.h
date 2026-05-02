#pragma once

#include "Types.h"
#include <string>

namespace rimell {

OfxStatus exportSettingsToFile(const RenderParams &params, const std::string &filePath);

} // namespace rimell
