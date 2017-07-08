#include "stdafx.h"

namespace base {

void OutputDebugLine(const std::string& line) {
    OutputDebugStringA(line.c_str());
    OutputDebugStringA("\n");
}

}