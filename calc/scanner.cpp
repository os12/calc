#include "stdafx.h"
#include <string>
#include <deque>
#include <stack>
#include <cassert>

#include "parser.h"

namespace {

bool IsHexOrFloatDigit(char c) {
    return isdigit(c) || c == '.' || (toupper(c) >= 'A' && toupper(c) <= 'F');
}

bool NumberContainsHexChars(const std::string &s) {
    for (auto c : s) {
        assert(IsHexOrFloatDigit(c) && c != '.');
        if (!isdigit(c)) return true;
    }
    return false;
}

}  // namespace

namespace parser {

#define CASE(v) case Token::Type::v: return #v

std::string ToString(Token::Type tt) {
    switch (tt) {
    CASE(Int);
    CASE(LParen);
    CASE(RParen);
    CASE(Minus);
    CASE(Plus);
    CASE(Mult);
    CASE(Div);
    CASE(Not);
    CASE(Or);
    CASE(And);
    CASE(Xor);
    CASE(LShift);
    CASE(RShift);
    };

    throw std::runtime_error(std::string("Unrecognized token type: ") +
                             std::to_string(static_cast<int>(tt)));
}

#undef CASE

std::deque<Token> Scan(const std::string& inp) {
    std::deque<Token> out;

    for (int pos = 0; pos < inp.size();) {
        switch (inp[pos]) {
            // Eat whitespace
            case ' ':
            case '\t':
                ++pos;
                break;

            // Take single-character tokens
            case '-':
            case '+':
            case '*':
            case '/':
            case '(':
            case ')':
            case '~':
            case '|':
            case '&':
            case '^':
                out.push_back(Token::Type(inp[pos]));
                ++pos;
                break;

            // Take two-character LShift token: <<
            case '<':
                ++pos;
                if (pos >= inp.size() || inp[pos] != '<')
                    throw std::runtime_error(
                        std::string("Invalid input: unexpected char: ") + inp[pos]);
                ++pos;
                out.push_back({Token::Type::LShift});
                break;

            // Take two-character RShift token: >>
            case '>':
                ++pos;
                if (pos >= inp.size() || inp[pos] != '>')
                    throw std::runtime_error(
                        std::string("Invalid input: unexpected char: ") + inp[pos]);
                ++pos;
                out.push_back({Token::Type::RShift});
                break;

            // The remaining one is the Integer (variable size, prefixes, etc)
            default: {
                std::string number;
                int base = 10;
                bool real = false;

                if (inp.size() - pos > 2 && inp.substr(pos, 2) == "0x") {
                    base = 16;
                    pos += 2;
                }

                while (pos < inp.size() && IsHexOrFloatDigit(inp[pos])) {
                    number += inp[pos];
                    ++pos;
                }

                // Deal with unexpected characters.
                if (number.empty())
                    throw std::runtime_error(
                        std::string("Invalid input: unexpected char: ") + inp[pos]);

                if (real && base != 10)
                    throw std::runtime_error(
                        "Invalid input: no support for non-decimal real numbers");

                if (base == 10 && NumberContainsHexChars(number))
                    throw std::runtime_error(
                        "Invalid input: hex chars in a base/10 number: " + number);

                // Done. We have a well-formed Int.
                out.push_back(Token{Token::Type::Int, number, base});
            }
        }
    }

    return out;
}

}
