#pragma once

#include <optional>
#include "big/Cbignum.h"
#include "scanner.h"

namespace parser {

// The compulation result. Generally, a subset of fields has meaningful values as various
// operators and functions restrict the full function's (well, it's an expression really)
// range.
struct Result {
    explicit Result() = default;

    explicit Result(uint32_t r32) : r32(r32), rbig(r32) {
        rreal = r32;
        r64 = r32;
    }

    explicit Result(const std::string& number, int base = 10);

    std::optional<uint64_t> r64;
    std::optional<uint32_t> r32;
    std::optional<double> rreal;
    std::optional<cBigNumber> rbig;

    bool Valid() const { return r64 || r32 || rreal || rbig; }

    Result& operator+=(Result b);
    Result& operator-=(Result b);
    Result& operator*=(Result b);
    Result& operator/=(Result b);
    Result& operator>>=(Result b);
    Result& operator<<=(Result b);
    Result& operator|=(Result b);
    Result& operator&=(Result b);
    Result& operator^=(Result b);

    Result& operator~();

    void ApplyUnaryFunction(const std::string& fname);
};

// The Parser (and the Scanner) throw this object when the input is invalid.
class Exception : public std::runtime_error {
   public:
    Exception(std::string msg) : runtime_error(std::move(msg)) {}
};

// The main entry point - pass a C-style expression and get the result.
Result Compute(const std::string &input);

}
