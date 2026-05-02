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
#include <sstream>
#include <iomanip>

namespace rimell {

std::string generateSettingsJson(const RenderParams &params) {
  std::stringstream ss;
  ss << "{\\n";
"""

for i, (t, n) in enumerate(fields):
    is_last = i == len(fields) - 1
    comma = "" if is_last else ","
    if t == 'Vec3':
        cpp_content += f'  ss << "  \\"{n}\\": [" << params.{n}.r << ", " << params.{n}.g << ", " << params.{n}.b << "]{comma}\\n";\n'
    elif t == 'Vec2':
        cpp_content += f'  ss << "  \\"{n}\\": [" << params.{n}.x << ", " << params.{n}.y << "]{comma}\\n";\n'
    else:
        cpp_content += f'  ss << "  \\"{n}\\": " << params.{n} << "{comma}\\n";\n'

cpp_content += """
  ss << "}\\n";
  return ss.str();
}

} // namespace rimell
"""

with open('src/SettingsExport.cpp', 'w') as f:
    f.write(cpp_content)

h_content = """#pragma once

#include "Types.h"
#include <string>

namespace rimell {

std::string generateSettingsJson(const RenderParams &params);

} // namespace rimell
"""

with open('src/SettingsExport.h', 'w') as f:
    f.write(h_content)

