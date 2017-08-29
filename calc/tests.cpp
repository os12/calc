#include "stdafx.h"
#include "parser.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <glog/logging.h>

namespace {

void Check(const std::string& expr, uint32_t expexted_result) {
    auto result = parser::Compute(expr);
    CHECK(result.Valid());
    CHECK(result.r32);
    CHECK_EQ(*result.r32, expexted_result);
}

void CheckBig(const std::string& expr, const std::string& expexted_result) {
    auto result = parser::Compute(expr);
    CHECK(result.rbig);
    cBigString buf;
    CHECK(result.rbig.value().toa(buf) == expexted_result);
}

template<typename T>
bool FPEqual(T a, T b) {
    if (fabs(a - b) < std::numeric_limits<T>::epsilon())
        return true;

    // Note, this breaks large value comparisons such as:
    //  10000000000000000.0 10000000000000200.0
    T relative_err;
    if (fabs(b) > fabs(a))
        relative_err = fabs((a - b) / b);
    else
        relative_err = fabs((a - b) / a);
    return relative_err <= 0.00000000001;
}

void CheckReal(const std::string& expr, double expexted_result) {
    auto result = parser::Compute(expr);
    CHECK(result.rreal);
    CHECK(FPEqual(*result.rreal, expexted_result));
}

void CheckInvalid(const std::string& expr) {
    bool ex_fired = false;
    try {
        parser::Compute(expr);
    } catch (std::exception&) {
        ex_fired = true;
    }
    CHECK(ex_fired);
}

}

namespace tests {

void Run() {
    Check("1", 1);
    Check("1234", 1234);
    Check("0x1234", 0x1234);
    Check("(1+2)*3", 9);
    Check("1+2*3", 7);

    // spaces
    Check(" 1 + 2 ", 3);
    Check(" 12 ", 12);
    Check("12    ", 12);
    Check("12\t", 12);

    // malformed expressions
    CheckInvalid("12(");
    CheckInvalid("12+");
    CheckInvalid("+");
    CheckInvalid("+12");
    CheckInvalid("(12");
    CheckInvalid(")12");

    // C-style math
    Check("1<<2", 4);
    Check("1|2", 3);

    // Big numbers
    CheckBig("100000000*10000000", "1000000000000000");

    // functions
    Check("abs(-1)", 1);
    Check("abs(1)", 1);
    CheckReal("cos(0)", 1);
    CheckReal("cos(0.0)", 1.0);
    CheckReal("rad(90)", 1.570796326794);
    CheckReal("cos(rad(90))", 0.0);
    CheckReal("cos(rad(90))", 0);
    CheckReal("10000000000000000.0 + 200.0", 10000000000000000.0);
}

}