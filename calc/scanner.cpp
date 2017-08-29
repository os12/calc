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

const std::set<std::string> _functions3 = {"abs", "sin", "cos", "tan", "rad", "deg"};
const std::set<std::string> _functions4 = {"sqrt"};

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
    CASE(EoF);
    };

    throw Exception("Unrecognized token type: " + std::to_string(static_cast<int>(tt)));
}

#undef CASE

bool detail::Buffer::Scan(char c, bool eof, Token* t) {
    switch (state_) {
    case State::None:
        DCHECK(buf_.empty());
        buf_.push_back(c);
        return FetchQueued(eof, t);

    case State::TwoChar:
        // Complete a two-char token: <<, >>
        DCHECK_EQ(buf_.size(), 1);
        if (buf_.front() == '<' && c == '<') {
            buf_.clear();
            state_ = State::None;
            *t = Token{Token::Type::LShift, "<<"};
            return true;
        }
        if (buf_.front() == '>' && c == '>') {
            buf_.clear();
            state_ = State::None;
            *t = Token{Token::Type::RShift, "<<"};
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
    case '*':
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

    // First deal the known functions. The thing here is that we must deal with ASCII
    // strings.
    if (buf_.size() >= 3 && _functions3.count(std::string(&buf_[0], 3)) > 0) {
        *t = Token{Token::Type::Function, std::string(&buf_[0], 3)};
        buf_.erase(buf_.begin(), buf_.begin() + 3);
        state_ = State::None;
        return true;
    }
    if (buf_.size() >= 4 && _functions4.count(std::string(&buf_[0], 4)) > 0) {
        *t = Token{Token::Type::Function, std::string(&buf_[0], 4)};
        buf_.erase(buf_.begin(), buf_.begin() + 4);
        state_ = State::None;
        return true;
    }
    if (isalpha(buf_.front())) {
        if (buf_.size() >= 4)
            throw Exception("Unrecognized ASCII string starting with '" +
                            BufAsString().substr(0, 4) + "'");
        return false;
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
        *t = Token{Token::Type::Int, number, base};
        buf_.erase(buf_.begin(), it);
        state_ = State::None;
        return true;
    }

    return false;
}

}
