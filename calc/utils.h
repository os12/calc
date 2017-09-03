#pragma once

#include <string>

namespace base {

// Takes a line (without the EoL marker) and dumps it to the Windows' debug output stream.
void OutputDebugLine(const std::string& line);

}  // namespace base

namespace utils {

// Floating point equality function adapted from:
//  http://floating-point-gui.de/errors/comparison/
template <typename T>
bool FPEqual(T a, T b) {
    const T diff = fabs(a - b);

    if (a == b)
        return true;  // shortcut, handles infinities

    const auto epsilon = std::numeric_limits<T>::epsilon();

    if (a == 0 || b == 0 || diff < std::numeric_limits<T>::min()) {
        // Our a or b is zero or both are extremely close to it relative error is less
        // meaningful here.

        // This does not make sense.
        // return diff < epsilon * std::numeric_limits<T>::min();
        return diff < epsilon;
    } else {
        // Use relative error
        return diff / std::min((fabs(a) + fabs(b)), std::numeric_limits<T>::max()) <
               epsilon;
    }
}

}