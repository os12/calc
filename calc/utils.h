#pragma once

#include <string>

namespace base {

// Takes a line (without the EoL marker) and dumps it to the Windows' debug output stream.
void OutputDebugLine(const std::string& line);

}