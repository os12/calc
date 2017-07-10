#include "stdafx.h"
#include <string>
#include <deque>
#include <stack>
#include <cassert>

#include "parser.h"

namespace parser {

namespace {

bool IsHexOrFloatDigit(char c) {
    return isdigit(c) || c == '.' || (toupper(c) >= 'A' && toupper(c) <= 'F');
}

bool NumberContainsHexChars(const std::string &s) {
    for (auto c : s) {
        assert(IsHexOrFloatDigit(c));
        if (!isdigit(c) && c != '.')
            return true;
    }
    return false;
}

const std::set<std::string> _functions3 = {"abs", "sin", "cos", "tan", "rad", "deg"};
const std::set<std::string> _functions4 = {"sqrt"};

Token ExtractVariableSizedToken(const std::string& input, size_t& pos) {
    auto remaining = [&input](size_t pos) { return input.size() - pos; };

    // First check for known functions.
    if (remaining(pos) >= 3 && _functions3.count(input.substr(pos, 3)) > 0) {
        Token rv{Token::Type::Function, input.substr(pos, 3)};
        pos += 3;
        return rv;
    }
    if (remaining(pos) >= 4 && _functions4.count(input.substr(pos, 4)) > 0) {
        Token rv{Token::Type::Function, input.substr(pos, 4)};
        pos += 4;
        return rv;
    }

    // The following variable-sized input must comprise an Integer.
    std::string number;
    int base = 10;

    if (input.size() - pos > 2 && input.substr(pos, 2) == "0x") {
        base = 16;
        pos += 2;
    }

    while (pos < input.size() && IsHexOrFloatDigit(input[pos]))
        number += input[pos++];

    // Reject non-number chars right here.
    if (number.empty())
        throw std::runtime_error(std::string("Invalid input: unexpected char: ") +
                                 input[pos]);

    if (base == 10 && NumberContainsHexChars(number))
        throw std::runtime_error("Invalid input: hex chars in a base/10 number: " +
                                 number);

    // Done. We have a well-formed Integer.
    return Token{Token::Type::Int, number, base};
}

}  // namespace

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
    CASE(Function);
    };

    throw std::runtime_error(std::string("Unrecognized token type: ") +
                             std::to_string(static_cast<int>(tt)));
}

#undef CASE

std::deque<Token> Scan(const std::string& inp) {
    std::deque<Token> out;

    for (size_t pos = 0; pos < inp.size();) {
        switch (inp[pos]) {
            // Eat whitespace
            case ' ':
            case '\t':
            case '\r':
            case '\n':
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
                out.push_back(Token{Token::Type(inp[pos])});
                ++pos;
                break;

            // Take two-character LShift token: <<
            case '<':
                ++pos;
                if (pos >= inp.size() || inp[pos] != '<')
                    throw std::runtime_error(
                        std::string("Invalid input: unexpected char: ") + inp[pos]);
                ++pos;
                out.push_back(Token{Token::Type::LShift});
                break;

            // Take two-character RShift token: >>
            case '>':
                ++pos;
                if (pos >= inp.size() || inp[pos] != '>')
                    throw std::runtime_error(
                        std::string("Invalid input: unexpected char: ") + inp[pos]);
                ++pos;
                out.push_back(Token{Token::Type::RShift});
                break;

            // The remaining tokens are:
            //  - Integer (variable size, prefixes, etc)
            //  - Function (sin, cos, etc)
            default:
                out.push_back(ExtractVariableSizedToken(inp, pos));
        }
    }

    return out;
}

}
