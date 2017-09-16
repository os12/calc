#include "stdafx.h"

namespace utils {

void OutputDebugLine(const std::string& line) {
#if defined(_DEBUG)
    OutputDebugStringA(line.c_str());
    OutputDebugStringA("\n");
#endif
}

}