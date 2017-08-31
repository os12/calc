#include "stdafx.h"

#include <glog/logging.h>
#include <stack>


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

const std::set<std::string> _functions = {
    "abs", "sin", "cos", "tan", "rad", "deg", "sqrt", "log2"};

}  // namespace

#define CASE(v) case Token::v: return #v

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
    CASE(Pow);
    CASE(Function);
    CASE(Pi);
    CASE(EoF);
    };

    LOG(FATAL) << "Unhandled token type: " << static_cast<int>(tt);
}

#undef CASE

bool detail::Buffer::Scan(char c, bool eof, Token* t) {
    switch (state_) {
    case State::None:
        DCHECK(buf_.empty());
        buf_.push_back(c);
        return FetchQueued(eof, t);

    case State::TwoChar:
        // Complete a two-char token: "<<", ">>", "**" and deal with "*".
        DCHECK_EQ(buf_.size(), 1);
        if (buf_.front() == '<' && c == '<') {
            buf_.clear();
            state_ = State::None;
            *t = Token{Token::LShift, "<<"};
            return true;
        }
        if (buf_.front() == '>' && c == '>') {
            buf_.clear();
            state_ = State::None;
            *t = Token{Token::RShift, "<<"};
            return true;
        }
        if (buf_.front() == '*') {
            state_ = State::None;
            if (c == '*') {
                buf_.clear();
                *t = Token{Token::Pow, "**"};
            } else {
                buf_.front() = c;
                *t = Token{Token::Mult, "*"};
            }
            return true;
        }
        throw Exception("Invalid input: unexpected char: " + std::string(1, c));

    case State::VarSized:
        buf_.push_back(c);
        return VariableSizedToken(eof, t);
    }

    LOG(FATAL) << "Unexpected state: " << static_cast<int>(state_);
}

bool detail::Buffer::FetchQueued(bool eof, Token* t) {
    if (buf_.empty())
        return false;

    DCHECK(buf_.size() == 1 && state_ == State::None)
        << "Expecting a single terminating char!";

    switch (buf_.front()) {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
        // Eat whitespace
        buf_.erase(buf_.begin(), buf_.begin() + 1);
        return false;

    case '-':
    case '+':
    case '/':
    case '(':
    case ')':
    case '~':
    case '|':
    case '&':
    case '^':
        // Take a single-character token.
        *t = Token{Token::Type(buf_.front()), std::string(1, buf_.front())};
        buf_.erase(buf_.begin(), buf_.begin() + 1);
        return true;

    // Deal with Mult/Exp as they start with '*'.
    case '*':
      state_ = State::TwoChar;
      return false;

    // Start a two-character token: <<, >>
    case '<':
    case '>':
        state_ = State::TwoChar;
        return false;

    default:
        state_ = State::VarSized;
        return VariableSizedToken(eof, t);
    }
}

bool detail::Buffer::VariableSizedToken(bool eof, Token* t) {
    DCHECK(state_ == State::VarSized);
    DCHECK(!buf_.empty());

    // First deal the known functions and constants. The thing here is that we must deal
    // with ASCII strings.
    {
        // Built-in function names.
        auto it = _functions.end();
        if (buf_.size() >= 3)
            it = _functions.find(std::string(&buf_[0], 3));
        if (it == _functions.end() && buf_.size() >= 4)
            it = _functions.find(std::string(&buf_[0], 4));
        if (it != _functions.end()) {
            *t = Token{Token::Function, *it};
            buf_.erase(buf_.begin(), buf_.begin() + it->size());
            state_ = State::None;
            return true;
        }

        // Built-in constants.
        if (buf_.size() >= 2 && std::string(&buf_[0], 2) == "pi") {
            *t = Token{Token::Pi};
            buf_.erase(buf_.begin(), buf_.begin() + 2);
            state_ = State::None;
            return true;
        }

        // Now this bit is weird. We are in the "var sized token" mode and must stay
        // there until we know for sure that this ASCII string is garbage.
        if (isalpha(buf_.front())) {
            if (buf_.size() >= 4)
                throw Exception("Unrecognized ASCII string starting with '" +
                                AsString().substr(0, 4) + "'");
            return false;
        }
    }

    // The following variable-sized input must comprise an Integer.
    auto it = buf_.begin();
    std::string number;
    int base = 10;

    if (buf_.size() >= 2 && std::string(&buf_[0], 2) == "0x") {
        base = 16;
        it += 2;
    }

    while (it != buf_.end()) {
        if (IsHexOrFloatDigit(*it))
            number += *(it++);
        else
            break;
    }

    if (number.empty()) {
        if (it != buf_.end())
            throw Exception("Malformed base/" + std::to_string(base) +
                            " integer starting with '" + std::string(1, buf_.front()) +
                            "'");
        return false;
    }

    if (base == 10 && NumberContainsHexChars(number))
        throw Exception("Invalid input: hex chars in a base/10 number: " + number);

    // We have a well-formed Integer if we reached EoF or a non-number char.
    if (eof || it != buf_.end()) {
        *t = Token{Token::Int, number, base};
        buf_.erase(buf_.begin(), it);
        state_ = State::None;
        return true;
    }

    return false;
}

}
