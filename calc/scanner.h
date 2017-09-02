#pragma once

#include <glog/logging.h>

namespace parser {

struct Token {
    // This enum is scoped to the Token struct and some of the members are used
    // directly for scanning single-chat tokes.
    enum Type {
        // This one is variable-sized.
        Int,

        LParen = '(',
        RParen = ')',

        // Arithmetic ops, mostly binary.
        Minus = '-',
        Plus = '+',
        Mult = '*',
        Div = '/',

        Coma = ',',

        // Bitwise ops, mostly binary.
        Not = '~',
        Or = '|',
        And = '&',
        Xor = '^',
        LShift,
        RShift,

        // Built-in algebraic and trigonomic functions.
        Function,

        // Exponent. It also exists as a binary function "pow".
        Pow, // **

        // Built-in constants.
        Pi,

        EoF
    };

    explicit Token(Type type, std::string value = "", int base = 10)
        : type(type), value(value), base(base) {}

    bool IsEoF() const { return type == EoF; }

    Type type;
    std::string value;
    int base = -1;
};

std::string ToString(Token::Type tt);

inline std::ostream& operator<<(std::ostream& s, Token::Type tt) {
    s << ToString(tt);
    return s;
}

namespace detail {

// A minimal buffering scanner. Extracts one token at a time.
class Buffer {
public:
    bool Scan(char c, bool eof, Token* t);
    bool FetchQueued(bool eof, Token* t);

    bool Empty() const { return buf_.empty(); }

    std::string AsString() const {
        return std::string(buf_.begin(), buf_.end());
    }

private:
    bool VariableSizedToken(bool eof, Token* t);

    enum class State { None, TwoChar, VarSized } state_ = State::None;

    // Contains the input needed for processing multi-char tokens.
    std::deque<char> buf_;
};

}  // namespace detail

// On-demand scanner backed by a STL-style range. The point here is to read tokens on
// demand.
//
// Requirements:
//  I - a forward iterator
//
template <typename I>
class Scanner {
public:
    Scanner(I begin, I end) : begin_(begin), end_(end) {
        static_assert(sizeof(I::value_type) == 1, "Expecting an iterator over char!");
    }

    bool ReachedEof() { return Next().type == Token::EoF; }

    // Returns the next token (by scanning or from the queue).
    const auto& Next() {
        return Get(0);
    }

    // Returns the Nth token (by scanning or from the queue).
    const auto& Get(size_t idx) {
        while (queue_.size() <= idx) {
            DCHECK(queue_.empty() || !queue_.back().IsEoF());
            queue_.push_back(Fetch());
            if (queue_.back().IsEoF())
                break;
        }

        if (idx >= queue_.size()) {
            DCHECK(!queue_.empty() && queue_.back().IsEoF());
            throw Exception("Reached end of input while trying to fetch token idx=" +
                            std::to_string(idx) + " at " + ToString(queue_.front().type));
        }

        return queue_[idx];
    }

    // Drops the 'next' token (as it has been consumed by the caller).
    void Pop() {
        DCHECK(!queue_.empty());
        CHECK(queue_.front().type != Token::EoF);
        queue_.pop_front();
    }

private:
    Token Fetch() {
        Token t(Token::EoF);
        if (buf_.FetchQueued(begin_ == end_ /* eof */, &t))
            return t;

        while (begin_ != end_) {
            auto last = begin_++;
            if (buf_.Scan(*last, begin_ == end_, &t))
                return t;
        }

        DCHECK(t.type == Token::EoF);

        // Deal with unfinished var-sized tokens.
        if (!buf_.Empty())
            throw Exception("Unrecognized ASCII string: '" + buf_.AsString() + "'");
        return t;
    }

    // The remaining range. Note, 'begin_' keeps moving forward towards the immutable
    // 'end_'.
    I begin_, end_;

    // Cached input bytes. This is needed for processing multi-byte token.
    detail::Buffer buf_;

    // 0 or 1 tokens. This backs the Next() implementation.
    std::deque<Token> queue_;
};

template <typename I>
auto MakeScanner(I begin, I end) {
    return Scanner<I>(begin, end);
}

}  // namespace parser
