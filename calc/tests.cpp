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
    CHECK(result.u32);
    CHECK_EQ(*result.u32, expected_result);
    CHECK(!result.i32 || *result.i32 == *result.big);
}

void CheckSigned32(const std::string& expr, int32_t expected_result) {
    auto result = parser::Compute(expr);
    CHECK(result.Valid());
    CHECK(result.i32);
    CHECK_EQ(*result.i32, expected_result);
    CHECK_EQ(*result.i32, *result.big);
}

void Check64(const std::string& expr, uint64_t expected_result) {
    auto result = parser::Compute(expr);
    CHECK(result.Valid());
    CHECK(result.u64);
    CHECK_EQ(*result.u64, expected_result);
    CHECK_EQ(*result.u64, *result.big);
}

void CheckBig(const std::string& expr, const std::string& expected_result) {
    auto result = parser::Compute(expr);
    CHECK(result.big);
    cBigString buf;
    CHECK(result.big.value().toa(buf) == expected_result);
}

void CheckReal(const std::string& expr, double expected_result) {
    auto result = parser::Compute(expr);
    CHECK(result.real);
    CHECK(utils::FPEqual(*result.real, expected_result));
}

void CheckOnlyReal(const std::string& expr, double expected_result) {
    auto result = parser::Compute(expr);
    CHECK(result.real);
    CHECK(utils::FPEqual(*result.real, expected_result));

    CHECK(!result.i32);
    CHECK(!result.u32);
    CHECK(!result.u64);
    CHECK(!result.big);
}

void CheckNEReal(const std::string& expr, double expected_result) {
    auto result = parser::Compute(expr);
    CHECK(result.real);
    CHECK(!utils::FPEqual(*result.real, expected_result));
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
    Check32("10%4", 2);
    CheckReal("10.0%4.0", 2.0);

    // Floating-point numbers
    CheckOnlyReal("1.0--1.0", 2.0);
    CheckReal("1", 1.0);
    CheckOnlyReal("1.0", 1.0);
    CheckOnlyReal("1e1", 10.0);
    CheckOnlyReal(".1e1", 1.0);
    CheckOnlyReal("0.1e1", 1.0);
    CheckOnlyReal(".1e-1", .01);
    CheckOnlyReal("0.1e-1", .01);
    CheckOnlyReal(".4", .4);
    CheckOnlyReal("100/20.", 5.0);

    CheckInvalid("1e");
    CheckInvalid("e1");
    CheckInvalid(".1e");
    CheckInvalid(".e");
    CheckInvalid(".e1");
    CheckInvalid(".e.");

    // Signed math
    CheckReal("-1/2", -0.5);
    CheckSigned32("-1/2", 0);
    CheckBig("-1/2", "0");

    // Hex numbers
    Check32("0x1e1", 0x1e1);

    // spaces
    Check32(" 1 + 2 ", 3);
    Check32(" 12 ", 12);
    Check32("12    ", 12);
    Check32("12\t", 12);

    // Ill-formed expressions
    CheckInvalid("12(");
    CheckInvalid("12+");
    CheckInvalid("+");
    CheckInvalid("+12");
    CheckInvalid("(12");
    CheckInvalid(")12");

    // Invalid intermediate results
    CheckInvalid("3%(.3^2.)");

    // Ill-formed numbers
    CheckInvalid(".");
    CheckInvalid("0x");
    CheckInvalid("0x.");
    CheckInvalid("0x10.");
    CheckInvalid(".0x");
    CheckInvalid(".a");

    // Division by zero
    CheckInvalid("1/0");
    CheckInvalid("1/0.");
    CheckInvalid("1/.0");
    CheckInvalid("1/0e0");
    CheckInvalid("1/.0e0");
    CheckInvalid("1/0.0e1");
    CheckInvalid("0**-1");

    // C-style math with bitwise ops
    Check32("1<<2", 4);
    Check32("1|2", 3);
    Check64("1<<2", 4);
    Check64("1|2", 3);
    CheckBig("1<<2", "4");
    CheckBig("1|2", "3");

    // Big numbers
    CheckBig("100000000*10000000", "1000000000000000");

    // Functions
    Check32("abs(-1)", 1);
    Check32("abs(1)", 1);
    CheckOnlyReal("cos(0)", 1);
    CheckOnlyReal("cos(0.0)", 1.0);
    CheckOnlyReal("rad(90)", double(M_PI) / 2.0);
    CheckOnlyReal("cos(rad(90))", 0.0);
    CheckOnlyReal("cos(rad(90))", 0);
    CheckNEReal("10000000000000000.0 + 200.0", 10000000000000000.0);

    // Pow. The funky op as well as a normal function.
    Check32("2**3", 8);
    Check64("2**32", 0x100000000LL);
    Check32("2**3 + 1", 9);
    Check32("1 + 2**3", 9);
    Check32("pow(2, 3)", 8);
    Check32("pow(2,3)-2**3", 0);

    // Constants
    CheckOnlyReal("pi", M_PI);
    CheckOnlyReal("pi/2", M_PI_2);
    CheckOnlyReal("deg(pi/2)", 90.0);

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

#if 0
    {
        auto fuzz = [](uint64_t max_length, std::string& input) {
            if (input.size() > max_length)
                return;

            static const std::vector<char> alphabet = {
                '0', '1', '2', '3', '.', 'e', 'f', 'g', '!', '(', ')',
                '%', '^', '*', '-', '+', '&', '<', '>', 'a', 'x', -20};
            while (true) {
                input.clear();
                while (input.size() < max_length) {
                    input.append(1, alphabet.at(rand() % alphabet.size()));
                    try {
                        parser::Compute(input);
                        LOG(INFO) << "Valid expression: " << input;
                    } catch (std::exception&) {
                    }
                }
            }
        };

        srand(static_cast<unsigned int>(time(nullptr)));
        std::string input;
        fuzz(10, input);
    }
#endif
    return true;
}

}