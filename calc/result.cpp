#include "stdafx.h"
#include "parser.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <glog/logging.h>

namespace parser {

Result::Result(const std::string& number, int base) {
    // Initialize the Big integer if we have no decimals.
    if (number.find('.') == std::string::npos)
        rbig = cBigNumber(number.data(), base);

    // Initialize the fixed-width integers if we have a Big integer and it fits.
    size_t last;
    if (rbig && rbig.value().length() <= 1) {
        r64 = stoull(number, &last, base);
        DCHECK_EQ(last, number.size());
        DCHECK_EQ(rbig.value().toCBNL(), r64.value());

        if (*r64 <= std::numeric_limits<uint32_t>::max()) {
            r32 = static_cast<uint32_t>(*r64);
            DCHECK_EQ(*r64, *r32);
        }
    }

    // Initialize the floating-point quantity from every decimal.
    if (base == 10) {
        double fp64 = stold(number, &last);
        if (last == number.size())
            rreal = fp64;
    }
}

void Result::ApplyUnaryFunction(const std::string& fname) {
    if (fname == "abs") {
        if ((r32 && *r32 < 0) || (rreal && *rreal < 0.0))
            *this *= Result("-1");
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
            *rreal = log2(*rreal);
        r32 = std::nullopt;
        r64 = std::nullopt;
        rbig = std::nullopt;
        return;
    }

    throw Exception("Unsupported unary function: " + fname);
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

}  // namespace parser
