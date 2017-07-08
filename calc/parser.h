#pragma once

#include <cassert>
#include <deque>
#include "big/Cbignum.h"

namespace parser {

struct Result {
  explicit Result(uint64_t r64 = 0, uint32_t r32 = 0, double rreal = 0.0)
      : r64(r64), r32(r32), rreal(rreal) {}

  uint64_t r64;
  uint32_t r32;
  double rreal;
  cBigNumber rbig;

  Result &operator+=(Result b);
  Result &operator-=(Result b);
  Result &operator*=(Result b);
  Result &operator/=(Result b);
  Result &operator>>=(Result b);
  Result &operator<<=(Result b);
  Result &operator|=(Result b);
  Result &operator&=(Result b);
  Result &operator^=(Result b);

  Result &operator~();
};

struct Token {
  enum class Type {
    Int,
    
    LParen  = '(',
    RParen  = ')',
    
    // Arithmetic
    Minus   = '-',
    Plus    = '+',
    Mult    = '*',
    Div     = '/',
    
    // Bitwise
    Not     = '~',
    Or      = '|',
    And     = '&',
    Xor     = '^',
    LShift,
    RShift
  };

  Token(Type t, std::string value = "", int base = 10)
      : type_(t), value_(value), base_(base) {}

  Result ExtractInt() const {
    Result r;
    r.rbig = cBigNumber(value_.data(), base_);
    size_t last;
    r.r64 = stoull(value_, &last, base_);
    assert(last == value_.size());
    r.r32 = stol(value_, &last, base_);
    assert(last == value_.size());

    // Deal with hex-->double conversion
    if (base_ == 10)
      r.rreal = stold(value_, &last);
    else
      r.rreal = r.r64;
    return r;
  }

  Type type_;
  std::string value_;
  int base_;
};

std::string ToString(Token::Type tt);

inline std::ostream& operator<<(std::ostream& s, Token::Type tt) {
    s << ToString(tt);
}

std::deque<Token> Scan(const std::string &inp);
Result Compute(const std::string &input);
}
