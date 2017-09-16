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
        rbig = cBigNumber(t.value.c_str(), t.base);

    // Initialize the fixed-width integers if we have a Big integer and it fits.
    size_t last;
    if (rbig && rbig.value().length() == 1) {
        r64 = stoull(t.value, &last, t.base);
        DCHECK_EQ(last, t.value.size());
        DCHECK_EQ(rbig.value().toCBNL(), r64.value());

        try {
            r32 = stoul(t.value, &last, t.base);
        }
        catch (std::exception& e) {
            base::OutputDebugLine("Invalid 32-bit input: " + t.value +
                                  ". Error: " + e.what());
        }
        DCHECK_EQ(last, t.value.size());
    }

    // Initialize the floating-point quantity from every decimal.
    if (t.CheckTypeFlags(Token::ValidFloat)) {
        DCHECK_EQ(t.base, 10);
        double fp64 = stold(t.value, &last);
        DCHECK(last == t.value.size());
        rreal = fp64;
    }
}

std::string Result::ToString() const {
    if (!Valid()) return "Invalid!";

    // Pick the shorter value as this is used only for debugging AST.
    if (r32) return std::to_string(*r32);
    if (r64) return std::to_string(*r64);
    if (rbig) {
        cBigString buf;
        return rbig->toa(buf);
    }

    return std::to_string(*rreal);
}

void Result::ApplyFunction(const std::string& fname) {
    if (fname == "abs") {
        if ((r32 && *r32 < 0) || (rreal && *rreal < 0.0))
            *this *= Result(
                Token("-1", 10, Token::ValidFloat | Token::ValidInt));
        return;
    }

    if (fname == "sin") {
        if (rreal)
            *rreal = sin(*rreal);
        r32 = std::nullopt;
        r64 = std::nullopt;
        rbig = std::nullopt;
        return;
    }

    if (fname == "cos") {
        if (rreal)
            *rreal = cos(*rreal);
        r32 = std::nullopt;
        r64 = std::nullopt;
        rbig = std::nullopt;
        return;
    }

    if (fname == "tan") {
        if (rreal)
            *rreal = tan(*rreal);
        r32 = std::nullopt;
        r64 = std::nullopt;
        rbig = std::nullopt;
        return;
    }

    if (fname == "rad") {
        if (rreal)
            *rreal = *rreal / 180.0 * M_PI;
        r32 = std::nullopt;
        r64 = std::nullopt;
        rbig = std::nullopt;
        return;
    }

    if (fname == "deg") {
        if (rreal)
            *rreal = *rreal / M_PI * 180.0;
        r32 = std::nullopt;
        r64 = std::nullopt;
        rbig = std::nullopt;
        return;
    }

    if (fname == "sqrt") {
        if (rreal)
            *rreal = sqrt(*rreal);
        r32 = std::nullopt;
        r64 = std::nullopt;
        if (rbig)
            rbig = rbig.value().sqrt();
        return;
    }

    if (fname == "log2") {
        if (rreal)
            *rreal = std::log2(*rreal);
        if (r32)
            *r32 = static_cast<uint32_t>(std::log2(*r32));
        if (r64)
            *r64 = static_cast<uint64_t>(std::log2(*r64));
        rbig = std::nullopt;
        return;
    }

    throw Exception("Unsupported unary function: " + fname);
}

void Result::ApplyFunction(const std::string& fname, const Result& arg2) {
    if (fname == "pow") {
        if (rbig && arg2.rbig)
            rbig->pow(*arg2.rbig);
        if (rreal && arg2.rreal)
            *rreal = std::pow(*rreal, *arg2.rreal);
        if (r32 && arg2.r32)
            *r32 = static_cast<uint32_t>(std::pow(*r32, *arg2.r32));
        if (r64 && arg2.r64)
            *r64 = static_cast<int64_t>(std::pow(*r64, *arg2.r64));
        return;
    }

    throw Exception("Unsupported binary function: " + fname);
}

Result& Result::operator+=(Result other) {
    if (r32 && other.r32)
        *r32 += *other.r32;
    if (r64 && other.r64)
        *r64 += *other.r64;
    if (rreal && other.rreal)
        *rreal += *other.rreal;
    if (rbig && other.rbig)
        *rbig += *other.rbig;
    return *this;
}

Result& Result::operator-=(Result other) {
    if (r32 && other.r32)
        *r32 -= *other.r32;
    if (r64 && other.r64)
        *r64 -= *other.r64;
    if (rreal && other.rreal)
        *rreal -= *other.rreal;
    if (rbig && other.rbig)
        *rbig -= *other.rbig;
    return *this;
}

Result& Result::operator*=(Result other) {
    if (r32 && other.r32)
        *r32 *= *other.r32;
    if (r64 && other.r64)
        *r64 *= *other.r64;
    if (rreal && other.rreal)
        *rreal *= *other.rreal;
    if (rbig && other.rbig)
        *rbig *= *other.rbig;
    return *this;
}

Result& Result::operator/=(Result other) {
    if (r32 && other.r32)
        *r32 /= *other.r32;
    if (r64 && other.r64)
        *r64 /= *other.r64;
    if (rreal && other.rreal)
        *rreal /= *other.rreal;
    if (rbig && other.rbig)
        *rbig /= *other.rbig;
    return *this;
}

Result& Result::operator<<=(Result other) {
    if (r32 && other.r32) {
        if (*other.r32 < 32)
            *r32 <<= *other.r32;
        else
            r32 = std::nullopt;
    }
    if (r64 && other.r64) {
        if (*other.r64 < 64)
            *r64 <<= *other.r64;
        else
            r64 = std::nullopt;
    }
    rreal = std::nullopt;
    if (rbig && other.rbig)
        *rbig <<= *other.rbig;
    return *this;
}

Result& Result::operator>>=(Result other) {
    if (r32 && other.r32) {
        if (*other.r32 < 32)
            *r32 >>= *other.r32;
        else
            r32 = std::nullopt;
    }
    if (r64 && other.r64) {
        if (*other.r64 < 64)
            *r64 >>= *other.r64;
        else
            r64 = std::nullopt;
    }
    rreal = std::nullopt;
    if (rbig && other.rbig)
        *rbig >>= *other.rbig;
    return *this;
}

Result& Result::operator&=(Result other) {
    if (r32 && other.r32)
        *r32 &= *other.r32;
    if (r64 && other.r64)
        *r64 &= *other.r64;
    rreal = std::nullopt;
    if (rbig && other.rbig)
        *rbig &= *other.rbig;
    return *this;
}

Result& Result::operator|=(Result other) {
    if (r32 && other.r32)
        *r32 |= *other.r32;
    if (r64 && other.r64)
        *r64 |= *other.r64;
    rreal = std::nullopt;
    if (rbig && other.rbig)
        *rbig |= *other.rbig;
    return *this;
}

Result& Result::operator^=(Result other) {
    if (r32 && other.r32)
        *r32 ^= *other.r32;
    if (r64 && other.r64)
        *r64 ^= *other.r64;
    rreal = std::nullopt;
    if (rbig && other.rbig)
        *rbig ^= *other.rbig;
    return *this;
}

Result& Result::operator~() {
    if (r32)
        *r32 = ~r32.value();
    if (r64)
        *r64 = ~*r64;
    rreal = std::nullopt;
    if (rbig)
        *rbig = ~*rbig;
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

    CHECK_FIELD(r32);
    CHECK_FIELD(r64);
    CHECK_FIELD(rbig);

    if (a.rreal != b.rreal)
        return false;
    if (a.rreal && !utils::FPEqual(*a.rreal, *b.rreal))
        return false;

    return true;
}

#undef CHECK_FIELD
}  // namespace parser
