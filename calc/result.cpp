#include "stdafx.h"
#include "parser.h"
#include "utils.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <glog/logging.h>

namespace parser {

Result::Result(const Token& t) {
    // Initialize the Big integer if we have no decimals.
    if (t.CheckTypeFlags(Token::ValidInt))
        big = cBigNumber(t.value.c_str(), t.base);

    // Initialize the fixed-width integers if we have a Big integer and it fits.
    size_t last;
    if (big && big.value().length() == 1) {
        u64 = stoull(t.value, &last, t.base);
        DCHECK_EQ(last, t.value.size());
        DCHECK_EQ(big.value().toCBNL(), u64.value());

        try {
            u32 = stoul(t.value, &last, t.base);
        }
        catch (std::exception& e) {
            utils::OutputDebugLine("Invalid unsigned 32-bit input: " + t.value +
                                   ". Error: " + e.what());
        }
        DCHECK_EQ(last, t.value.size());

        try {
            i32 = stol(t.value, &last, t.base);
        }
        catch (std::exception& e) {
            utils::OutputDebugLine("Invalid signed 32-bit input: " + t.value +
                                   ". Error: " + e.what());
        }
    }

    // Initialize the floating-point quantity from every decimal.
    if (t.CheckTypeFlags(Token::ValidFloat)) {
        DCHECK_EQ(t.base, 10);
        double fp64 = stold(t.value, &last);

        // The scanner is quite careful not not let these through.
        DCHECK(last == t.value.size());
        if (last != t.value.size())
            throw Exception("Malformed floating-point number: " + t.value);
        real = fp64;
    }
}

std::string Result::ToString() const {
    if (!Valid()) return "Invalid!";

    // Pick the shorter value as this is used only for debugging AST.
    if (u32) return std::to_string(*u32);
    if (u64) return std::to_string(*u64);
    if (big) {
        cBigString buf;
        return big->toa(buf);
    }

    return std::to_string(*real);
}

void Result::ApplyFunction(const std::string& fname) {
    if (fname == "abs") {
        if ((i32 && *i32 < 0) || (real && *real < 0))
            *this *= Result(
                Token("-1", 10, Token::ValidFloat | Token::ValidInt));
        return;
    }

    if (fname == "sin") {
        if (real)
            *real = sin(*real);
        u32 = std::nullopt;
        i32 = std::nullopt;
        u64 = std::nullopt;
        big = std::nullopt;
        return;
    }

    if (fname == "cos") {
        if (real)
            *real = cos(*real);
        u32 = std::nullopt;
        i32 = std::nullopt;
        u64 = std::nullopt;
        big = std::nullopt;
        return;
    }

    if (fname == "tan") {
        if (real)
            *real = tan(*real);
        u32 = std::nullopt;
        u64 = std::nullopt;
        i32 = std::nullopt;
        big = std::nullopt;
        return;
    }

    if (fname == "rad") {
        if (real)
            *real = *real / 180.0 * M_PI;
        u32 = std::nullopt;
        i32 = std::nullopt;
        u64 = std::nullopt;
        big = std::nullopt;
        return;
    }

    if (fname == "deg") {
        if (real)
            *real = *real / M_PI * 180.0;
        u32 = std::nullopt;
        i32 = std::nullopt;
        u64 = std::nullopt;
        big = std::nullopt;
        return;
    }

    if (fname == "sqrt") {
        if (real)
            *real = sqrt(*real);
        u32 = std::nullopt;
        i32 = std::nullopt;
        u64 = std::nullopt;
        if (big)
            big = big.value().sqrt();
        return;
    }

    if (fname == "log2") {
        if (real)
            *real = std::log2(*real);
        if (u32)
            *u32 = static_cast<uint32_t>(std::log2(*u32));
        if (i32)
            *i32 = static_cast<uint32_t>(std::log2(*i32));
        if (u64)
            *u64 = static_cast<uint64_t>(std::log2(*u64));
        big = std::nullopt;
        return;
    }

    throw Exception("Unsupported unary function: " + fname);
}

void Result::ApplyFunction(const std::string& fname, const Result& arg2) {
    if (IsZero() && arg2.IsNegative())
        throw Exception("Thou shalt not divide by zero!");

    if (fname == "pow") {
        if (big && arg2.big)
            big->pow(*arg2.big);
        if (real && arg2.real)
            *real = std::pow(*real, *arg2.real);
        if (u32 && arg2.u32)
            *u32 = static_cast<uint32_t>(std::pow(*u32, *arg2.u32));
        if (i32 && arg2.i32)
            *i32 = static_cast<uint32_t>(std::pow(*i32, *arg2.i32));
        if (u64 && arg2.u64)
            *u64 = static_cast<int64_t>(std::pow(*u64, *arg2.u64));
        return;
    }

    throw Exception("Unsupported binary function: " + fname);
}

#define APPLY_UNARY_OP(field, op) \
    if (field && other.field)     \
        *field op* other.field;   \
    else                          \
        field = std::nullopt

#define TRIVIAL_UNARY_OP_OPERATOR_BATCH(op) \
    do {                                    \
        APPLY_UNARY_OP(u32, op);            \
        APPLY_UNARY_OP(i32, op);            \
        APPLY_UNARY_OP(u64, op);            \
        APPLY_UNARY_OP(real, op);           \
        APPLY_UNARY_OP(big, op);            \
    } while (0)

Result& Result::operator+=(Result other) {
    TRIVIAL_UNARY_OP_OPERATOR_BATCH(+=);
    return *this;
}

Result& Result::operator-=(Result other) {
    TRIVIAL_UNARY_OP_OPERATOR_BATCH(-=);
    return *this;
}

Result& Result::operator*=(Result other) {
    TRIVIAL_UNARY_OP_OPERATOR_BATCH(*=);
    return *this;
}

Result& Result::operator%=(Result other) {
    if (other.IsZero())
        throw Exception("Thou shalt not divide by zero!");

    APPLY_UNARY_OP(u32, %=);
    APPLY_UNARY_OP(i32, %=);
    APPLY_UNARY_OP(u64, %=);
    APPLY_UNARY_OP(big, %=);
    if (real && other.real)
        real = fmod(*real, *other.real);
    else
        real = std::nullopt;
    return *this;
}

Result& Result::operator/=(Result other) {
    if (other.IsZero())
        throw Exception("Thou shalt not divide by zero!");

    TRIVIAL_UNARY_OP_OPERATOR_BATCH(/=);
    return *this;
}

Result& Result::operator<<=(Result other) {
    if (u32 && other.u32 && *other.u32 < 32)
        *u32 <<= *other.u32;
    else
        u32 = std::nullopt;
    i32 = std::nullopt;
    if (u64 && other.u64 && *other.u64 < 64)
        *u64 <<= *other.u64;
    else
        u64 = std::nullopt;
    real = std::nullopt;
    APPLY_UNARY_OP(big, <<=);
    return *this;
}

Result& Result::operator>>=(Result other) {
    if (u32 && other.u32 && *other.u32 < 32)
        *u32 >>= *other.u32;
    else
        u32 = std::nullopt;
    i32 = std::nullopt;
    if (u64 && other.u64  && *other.u64 < 64)
        *u64 >>= *other.u64;
    else
        u64 = std::nullopt;
    real = std::nullopt;
    APPLY_UNARY_OP(big, >>=);
    return *this;
}

Result& Result::operator&=(Result other) {
    APPLY_UNARY_OP(u32, &=);
    i32 = std::nullopt;
    APPLY_UNARY_OP(u64, &=);
    real = std::nullopt;
    APPLY_UNARY_OP(big, &=);
    return *this;
}

Result& Result::operator|=(Result other) {
    APPLY_UNARY_OP(u32, |=);
    i32 = std::nullopt;
    APPLY_UNARY_OP(u64, |=);
    real = std::nullopt;
    APPLY_UNARY_OP(big, |=);
    return *this;
}

Result& Result::operator^=(Result other) {
    APPLY_UNARY_OP(u32, ^=);
    i32 = std::nullopt;
    APPLY_UNARY_OP(u64, ^=);
    real = std::nullopt;
    APPLY_UNARY_OP(big, ^=);
    return *this;
}

#undef APPLY_UNARY_OP

Result& Result::operator~() {
    if (u32)
        *u32 = ~u32.value();
    i32 = std::nullopt;
    if (u64)
        *u64 = ~*u64;
    real = std::nullopt;
    if (big)
        *big = ~*big;
    return *this;
}

#define CHECK_FIELD(name)                 \
    do {                                  \
        if (a.name != a.name)             \
            return false;                 \
        if (a.name && *a.name != *b.name) \
            return false;                 \
    } while (0)

bool operator==(const Result& a, const Result& b) {
    // The empty objects are considered equal.
    if (!a.Valid() && !b.Valid())
        return true;

    CHECK_FIELD(u32);
    CHECK_FIELD(i32);
    CHECK_FIELD(u64);
    CHECK_FIELD(big);

    if (a.real != b.real)
        return false;
    if (a.real && !utils::FPEqual(*a.real, *b.real))
        return false;

    return true;
}

#undef CHECK_FIELD

bool Result::IsZero() const {
    DCHECK(Valid());
    return (u32 && *u32 == 0) || (u64 && *u64 == 0) || (i32 && *i32 == 0) ||
           (big && *big == 0) || (real && utils::FPEqual(*real, 0.0));
}

bool Result::IsNegative() const {
    DCHECK(Valid());
    return (i32 && *i32 < 0) ||
        (big && *big < 0) || (real && *real < 0.0);
}

}  // namespace parser
