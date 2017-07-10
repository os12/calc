#pragma once

#include <optional>
#include "big/Cbignum.h"

namespace parser {

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

struct Token {
  enum class Type {
    Int,
    
    LParen  = '(',
    RParen  = ')',
    
    // Arithmetic ops, mostly binary.
    Minus   = '-',
    Plus    = '+',
    Mult    = '*',
    Div     = '/',
    
    // Bitwise ops, mostly binary.
    Not     = '~',
    Or      = '|',
    And     = '&',
    Xor     = '^',
    LShift,
    RShift,

    // Algebraic and trigonomic functions.
    Function
  };

  explicit Token(Type type, std::string value = "", int base = 10)
      : type(type), value(value), base(base) {}

  Type type;
  std::string value;
  int base = -1;
};

std::string ToString(Token::Type tt);

inline std::ostream& operator<<(std::ostream& s, Token::Type tt) {
    s << ToString(tt);
    return s;
}

std::deque<Token> Scan(const std::string &inp);
Result Compute(const std::string &input);

}
