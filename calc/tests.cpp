#include "stdafx.h"
#include "parser.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <glog/logging.h>

namespace {

void Check32(const std::string& expr, uint32_t expected_result) {
    auto result = parser::Compute(expr);
    CHECK(result.Valid());
    CHECK(result.r32);
    CHECK_EQ(*result.r32, expected_result);
}

void Check64(const std::string& expr, uint64_t expected_result) {
    auto result = parser::Compute(expr);
    CHECK(result.Valid());
    CHECK(result.r64);
    CHECK_EQ(*result.r64, expected_result);
}

void CheckBig(const std::string& expr, const std::string& expected_result) {
    auto result = parser::Compute(expr);
    CHECK(result.rbig);
    cBigString buf;
    CHECK(result.rbig.value().toa(buf) == expected_result);
}

void CheckReal(const std::string& expr, double expected_result) {
    auto result = parser::Compute(expr);
    CHECK(result.rreal);
    CHECK(utils::FPEqual(*result.rreal, expected_result));
}

void CheckNEReal(const std::string& expr, double expected_result) {
    auto result = parser::Compute(expr);
    CHECK(result.rreal);
    CHECK(!utils::FPEqual(*result.rreal, expected_result));
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
    Check32("1", 1);
    Check32("1234", 1234);
    Check32("0x1234", 0x1234);
    Check32("(1+2)*3", 9);
    Check32("1+2*3", 7);
    Check32("2*3+1", 7);
    Check32("1-(2+3)", -4);
    Check32("-1+1", 0);
    Check32("1+2+3+4", 10);
    Check32("10-2-3", 5);

    Check32("1--1", 2);
    CheckReal("1.0--1.0", 2.0);

    // spaces
    Check32(" 1 + 2 ", 3);
    Check32(" 12 ", 12);
    Check32("12    ", 12);
    Check32("12\t", 12);

    // malformed expressions
    CheckInvalid("12(");
    CheckInvalid("12+");
    CheckInvalid("+");
    CheckInvalid("+12");
    CheckInvalid("(12");
    CheckInvalid(")12");

    // C-style math
    Check32("1<<2", 4);
    Check32("1|2", 3);

    // Big numbers
    CheckBig("100000000*10000000", "1000000000000000");

    // Functions
    Check32("abs(-1)", 1);
    Check32("abs(1)", 1);
    CheckReal("cos(0)", 1);
    CheckReal("cos(0.0)", 1.0);
    CheckReal("rad(90)", double(M_PI) / 2.0);
    CheckReal("cos(rad(90))", 0.0);
    CheckReal("cos(rad(90))", 0);
    CheckNEReal("10000000000000000.0 + 200.0", 10000000000000000.0);

    // Pow. The funky op as well as a normal function.
    Check32("2**3", 8);
    Check64("2**32", 0x100000000LL);
    Check32("2**3 + 1", 9);
    Check32("1 + 2**3", 9);
    Check32("pow(2, 3)", 8);
    Check32("pow(2,3)-2**3", 0);

    // Constants
    CheckReal("pi", M_PI);
    CheckReal("pi/2", M_PI_2);
    CheckReal("deg(pi/2)", 90.0);
}

}