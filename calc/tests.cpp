#include "stdafx.h"
#include "parser.h"
#include "utils.h"

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

bool Run() {
    // Basic numbers and expressions
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

    // Floating-point numbers
    CheckReal("1.0--1.0", 2.0);
    CheckReal("1", 1.0);
    CheckReal("1.0", 1.0);
    CheckReal("1e1", 10.0);
    CheckReal(".1e1", 1.0);
    CheckReal("0.1e1", 1.0);
    CheckReal(".1e-1", .01);
    CheckReal("0.1e-1", .01);
    CheckReal(".4", .4);

    CheckInvalid("1e");
    CheckInvalid("e1");
    CheckInvalid(".1e");
    CheckInvalid(".e");
    CheckInvalid(".e1");
    CheckInvalid(".e.");

    // Hex numbers
    Check32("0x1e1", 0x1e1);

    // spaces
    Check32(" 1 + 2 ", 3);
    Check32(" 12 ", 12);
    Check32("12    ", 12);
    Check32("12\t", 12);

    // ill-formed expressions
    CheckInvalid("12(");
    CheckInvalid("12+");
    CheckInvalid("+");
    CheckInvalid("+12");
    CheckInvalid("(12");
    CheckInvalid(")12");

    // ill-formed numbers
    CheckInvalid(".");
    CheckInvalid("0x");
    CheckInvalid("0x.");
    CheckInvalid("0x10.");
    CheckInvalid(".0x");
    CheckInvalid(".a");

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

    Check32("0xFFFFFFFF", 0xFFFFFFFF);
    Check64("0x0FFFFFFFFFFFFFFF", 0x0FFFFFFFFFFFFFFFull);

    // A long test to brute-force FP parsing
#if 0
    auto compute = [](const std::string& input) {
        try {
            utils::OutputDebugLine("Checking: " + input);
            auto result = parser::Compute(input);
            DCHECK(result.Valid());
        } catch (parser::Exception&) {
        }
    };

    const std::vector<char> alphabet = {'1', '2', 'e', '.', '-', 'f'};

    std::function<void(size_t, std::string*)> select = [&](size_t len, std::string* s) {
        DCHECK(len > 0);
        const auto input_len = s->size();
        for (char c : alphabet) {
            s->resize(input_len);
            *s += c;

            compute(*s);
            compute("0x" + *s);

            if (len > 1)
                select(len - 1, s);
        }
        s->resize(input_len);
    };

    for (size_t len = 1; len < 5; ++len) {
        std::string s;
        s.reserve(10);
        select(len, &s);
    }
#endif

    return true;
}

}