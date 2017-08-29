#pragma once

#include <glog/logging.h>
#include <queue>

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

        // Built-in algebraic and trigonomic functions.
        Function,

        // Built-in constants.
        Pi,

        EoF
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
    std::deque<char> buf_;
};

}

// On-demand scanner backed by a STL container.
template <typename I>
class Scanner {
public:
    Scanner(I begin, I end) : begin_(begin), end_(end) {
        static_assert(sizeof(I::value_type) == 1, "Expecting an iterator over char!");
    }

    bool Eof() { return Next().type == Token::Type::EoF; }

    // Returns the next token (by scanning or from the queue).
    const auto& Next() {
        if (queue_.empty())
            queue_.push(Fetch());
        return queue_.front();
    }

    // Drops the 'next' token (as it has been consumed by the caller).
    void Pop() {
        DCHECK(!queue_.empty());
        CHECK(queue_.front().type != Token::Type::EoF);
        queue_.pop();
    }

private:
    Token Fetch() {
        DCHECK(queue_.empty());

        Token t(Token::Type::EoF);
        if (buf_.FetchQueued(false /* eof */, &t))
            return t;

        while (begin_ != end_) {
            auto last = begin_++;
            if (buf_.Scan(*last, begin_ == end_, &t))
                return t;
        }

        DCHECK(t.type == Token::Type::EoF);

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
    std::queue<Token> queue_;
};

template<typename I>
auto MakeScanner(I begin, I end) {
    return Scanner<I>(begin, end);
}
}