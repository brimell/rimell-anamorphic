import re

with open('src/Types.h', 'r') as f:
    content = f.read()

in_struct = False
fields = []

for line in content.splitlines():
    if 'struct RenderParams {' in line:
        in_struct = True
        continue
    if in_struct:
        if '};' in line:
            break
        # Match type and name
        match = re.match(r'^\s*([a-zA-Z0-9_:]+)\s+([a-zA-Z0-9_]+)\s*(?:=|;)(.*)$', line)
        if match:
            t = match.group(1)
            n = match.group(2)
            fields.append((t, n))

print(f"Found {len(fields)} fields")

cpp_content = """#include "SettingsExport.h"
#include "Diagnostics.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace rimell {

OfxStatus exportSettingsToFile(const RenderParams &params, const std::string &filePath) {
  if (filePath.empty()) {
    logPrintf(LogLevel::Warn, "export", "Export file path is empty");
    return kOfxStatErrValue;
  }

  try {
    std::ofstream file(filePath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
      logPrintf(LogLevel::Error, "export", "Failed to open file for writing: %s", filePath.c_str());
      return kOfxStatErrFileNotFound;
    }

    file << "# Rimell Anamorphic Settings Export \\n";
    file << "[Parameters]\\n";
"""

for t, n in fields:
    if t == 'Vec3':
        cpp_content += f'    file << "{n}_r=" << params.{n}.r << "\\n";\n'
        cpp_content += f'    file << "{n}_g=" << params.{n}.g << "\\n";\n'
        cpp_content += f'    file << "{n}_b=" << params.{n}.b << "\\n";\n'
    elif t == 'Vec2':
        cpp_content += f'    file << "{n}_x=" << params.{n}.x << "\\n";\n'
        cpp_content += f'    file << "{n}_y=" << params.{n}.y << "\\n";\n'
    else:
        cpp_content += f'    file << "{n}=" << params.{n} << "\\n";\n'

cpp_content += """
    file.close();
    logPrintf(LogLevel::Info, "export", "Settings exported to: %s", filePath.c_str());
    return kOfxStatOK;
  } catch (const std::exception &ex) {
    logPrintf(LogLevel::Error, "export", "Exception during export: %s", ex.what());
    return kOfxStatErrUnknown;
  }
}

} // namespace rimell
"""

with open('src/SettingsExport.cpp', 'w') as f:
    f.write(cpp_content)

h_content = """#pragma once

#include "Types.h"
#include <string>

namespace rimell {

OfxStatus exportSettingsToFile(const RenderParams &params, const std::string &filePath);

} // namespace rimell
"""

with open('src/SettingsExport.h', 'w') as f:
    f.write(h_content)

