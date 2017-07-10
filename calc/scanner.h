#pragma once

namespace parser {

struct Token {
    enum class Type {
        Int,

        LParen = '(',
        RParen = ')',

        // Arithmetic ops, mostly binary.
        Minus = '-',
        Plus = '+',
        Mult = '*',
        Div = '/',

        // Bitwise ops, mostly binary.
        Not = '~',
        Or = '|',
        And = '&',
        Xor = '^',
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

// All-or-nothing scanner. Pass the C-style expression in and get a sequence of tokens.
std::deque<Token> Scan(const std::string& inp);
}