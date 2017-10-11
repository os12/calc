#include "stdafx.h"

#include <glog/logging.h>
#include <stack>


#include "parser.h"

namespace parser {
namespace detail {
namespace {

const std::map<Token::Type, Operator> _known_bin_ops = {{Token::Minus, Operator::BMinus},
                                                        {Token::Plus, Operator::Plus},
                                                        {Token::Mult, Operator::Mult},
                                                        {Token::Div, Operator::Div},
                                                        {Token::Rem, Operator::Rem},
                                                        {Token::LShift, Operator::LShift},
                                                        {Token::RShift, Operator::RShift},
                                                        {Token::And, Operator::And},
                                                        {Token::Or, Operator::Or},
                                                        {Token::Xor, Operator::Xor},
                                                        {Token::Pow, Operator::Pow}};

bool IsHex(char c) { return isdigit(c) || (toupper(c) >= 'A' && toupper(c) <= 'F'); }

bool IsE(char c) { return c == 'e' || c == 'E'; }

bool IsHexOrFloatDigit(char c) {
    return IsHex(c) || c == '.';  // Note, 'e' for "0.1e2" is covered by IsHex()
}

bool ContainsHexChars(const std::string& s) {
    for (auto c : s)
        if (toupper(c) >= 'A' && toupper(c) <= 'F')
            return true;
    return false;
}

bool ContainsFloatChars(const std::string& s) {
    for (auto c : s) {
        switch (c) {
        case '.':
        case 'e':
        case 'E':
            return true;
        }
    }
    return false;
}

// Identifies floating-poing numbers such as ".1", "1e10" and "0.1e-10".
bool IsValidFloat(const std::string &s) {
    if (s.size() >= 2)
        DCHECK(s.substr(0, 2) != "0x");

    size_t dot = 0, e = 0, minus = 0;
    for (auto c : s) {
        if (isdigit(c))
            continue;
        switch (c) {
        case '.':
            ++dot;
            break;
        case 'e':
        case 'E':
            ++e;
            break;
        case '-':
            ++minus;
            break;
        default:
            return false;
        }
    }

    // Check for the "float" markers. The number cannot be a float literal without one
    // of these.
    if (dot == 0 && e == 0)
        return false;
    if (dot > 1 || e > 1)
        return false;

    // ".e", "e."
    if (dot > 0 && e > 0) {
        if (s.find('.') + 1 == s.find('e'))
            return false;
        if (s.find('e') + 1 == s.find('.'))
            return false;
        if (s.back() == '.')
            return false;
    }

    return true;
}

const std::set<std::string> _functions = {
    "abs", "sin", "cos", "tan", "rad", "deg", "sqrt", "log2", "pow"};

}  // namespace
}  // namespace detail

bool Token::IsBinOp() const {
    return detail::_known_bin_ops.find(type) != detail::_known_bin_ops.end();
}

detail::Operator Token::GetBinOp() const {
    DCHECK(IsBinOp());
    return detail::_known_bin_ops.find(type)->second;
}

std::string Token::ToString() const {
	switch (type)
	{
	case Token::Number:
		DCHECK(!value.empty());
		return "Number: " + value;
	case Token::Function:
		DCHECK(!value.empty());
		return "Function: " + value;
	case Token::LParen:
	case Token::RParen:
	case Token::Minus:
	case Token::Plus:
	case Token::Mult:
	case Token::Div:
	case Token::Rem:
	case Token::Coma:
	case Token::Not:
	case Token::Or:
	case Token::And:
	case Token::Xor:
	case Token::LShift:
	case Token::RShift:
	case Token::Pow:
	case Token::Pi:
	case Token::EoF:
		return parser::ToString(type);
	default:
		LOG(FATAL) << "Invalid token type: " << static_cast<int>(type);
	}
}

#define CASE(v) case Token::v: return #v

std::string ToString(Token::Type tt) {
    switch (tt) {
    CASE(Number);
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
    CASE(Coma);
    CASE(Rem);
    CASE(EoF);
    };

    LOG(FATAL) << "Unhandled token type: " << static_cast<int>(tt);
}

#undef CASE

namespace detail {

#define CASE(v) case Operator::v: return #v

std::string ToString(Operator op) {
    switch (op) {
    CASE(UMinus);
    CASE(BMinus);
    CASE(Plus);
    CASE(Mult);
    CASE(Div);
    CASE(Rem);
    CASE(Or);
    CASE(And);
    CASE(Xor);
    CASE(LShift);
    CASE(RShift);
    CASE(Pow);
    };

    LOG(FATAL) << "Unhandled operator: " << static_cast<int>(op);
}

#undef CASE

bool Buffer::Scan(char c, bool eof, Token* t) {
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

bool Buffer::FetchQueued(bool eof, Token* t) {
    if (buf_.empty())
        return false;

    DCHECK_EQ(buf_.size(), 1);
    DCHECK_EQ(state_, State::None);

    switch (buf_.front()) {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
        // Eat whitespace
        buf_.pop_front();
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
    case ',':
    case '%':
        // Take a single-character token.
        *t = Token{Token::Type(buf_.front()), std::string(1, buf_.front())};
        buf_.erase(buf_.begin(), buf_.begin() + 1);
        return true;


    // Start a two-character token: <<, >>
    case '<':
    case '>':
    case '*': // Deal with Mult/Exp as they start with '*'.
        state_ = State::TwoChar;
        return false;

    default:
        state_ = State::VarSized;
        return VariableSizedToken(eof, t);
    }
}

bool Buffer::VariableSizedToken(bool eof, Token* t) {
    DCHECK(state_ == State::VarSized);
    DCHECK(!buf_.empty());

    // The "char" is signed and there is a funky promotion that happens when calling
    // isalpha() as it takes an "int". So, discard nagative values right here.
    for (char c : buf_)
        if (c < 0)
            throw Exception("Invalid character: " + std::to_string(buf_.front()));

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

        // Now this bit is a little weird: we are in the "var sized token" mode and must
        // stay there until we know for sure that this ASCII string is not a built-in
        // function or constant.
        if (isalpha(buf_.front())) {
            if (buf_.size() >= 4)
                throw Exception("Unrecognized character sequence starting with: '" +
                                AsString().substr(0, 4) + "'");
            return false;
        }
    }

    // The following variable-sized input must comprise an integer or a floating-point
    // quantity.
    auto it = buf_.begin();
    std::string number;
    int base = 10;

    // Consume the hex "0x" prefix and note the change of base.
    if (buf_.size() >= 2 && std::string(&buf_[0], 2) == "0x") {
        base = 16;
        it += 2;
    }

    // Consume numeric/hex chars.
    while (it != buf_.end()) {
        switch (base) {
        case 10:
            if (IsHexOrFloatDigit(*it)) {
                number += *(it++);
                continue;
            }

            // Deal with '-' in floating-point cases such as "0.1e-1".
            if (number.size() >= 2 && IsE(number.back()) && *it == '-') {
                number += *(it++);
                continue;
            }
            break;
        case 16:
            if (IsHex(*it)) {
                number += *(it++);
                continue;
            }
            break;
        }
        break;
    }

	// Deal with cases where we started scanning a number but found no valid chars.
    if (number.empty() || number == ".") {
        if (it != buf_.end())
            throw Exception("Malformed base/" + std::to_string(base) +
                            " integer starting with '" + std::string(1, buf_.front()) +
                            "'");
        return false;
    }

	// Deal with hex chars as we can produce a better message.
    if (base == 10) {
        if (ContainsHexChars(number) && !IsValidFloat(number))
            throw Exception("Malformed base/10 integer: " + number);
        if (ContainsFloatChars(number) && !IsValidFloat(number))
            throw Exception("Malformed floating-poing number: " + number);
    }

    // Deal with incomplete floating-point numbers.
    if (IsE(number.back()) || number.back() == '-')
        return false;

    // We have a well-formed number if we reached EoF or a non-number char.
    if (eof || it != buf_.end()) {
        uint32_t type_flags = 0;
        switch (base) {
        case 10:
            // Every base/10 integer is a valid float, yet floating-point numbers
            // are not integers.
            type_flags = Token::ValidFloat;
            if (!IsValidFloat(number))
                type_flags |= Token::ValidInt;
            break;
        case 16:
            // Right now only base/16 integers are supported and every one of them
            // is also a valid float. Well, subject to conversion rules.
            type_flags = Token::ValidInt | Token::ValidFloat;
            break;
        }
        *t = Token{number, base, type_flags};
        DCHECK(Result(*t).Valid());
        buf_.erase(buf_.begin(), it);
        state_ = State::None;
        return true;
    }

    return false;
}

}  // namespace detail
}
