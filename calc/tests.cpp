#include "stdafx.h"
#include "parser.h"

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
}

}