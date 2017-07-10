#include "stdafx.h"
#include "parser.h"

#include <glog/logging.h>

namespace parser {

Result::Result(const std::string& number, int base) {
    // Initialize the Big integer if we have no decimals.
    if (number.find('.') == std::string::npos)
        rbig = cBigNumber(number.data(), base);

    // Initialize the fixed-width integers if we have a Big integer and it fits.
    size_t last;
    if (rbig && rbig.value().length() <= 1) {
        r64 = stoull(number, &last, base);
        DCHECK_EQ(last, number.size());
        DCHECK_EQ(rbig.value().toCBNL(), r64.value());

        if (*r64 <= std::numeric_limits<uint32_t>::max()) {
            r32 = static_cast<uint32_t>(*r64);
            DCHECK_EQ(*r64, *r32);
        }
    }

    // Initialize the floating-point quantity from every decimal.
    if (base == 10) {
        double fp64 = stold(number, &last);
        if (last == number.size())
            rreal = fp64;
    }
}

Result& Result::operator+=(Result other) {
    if (r32 && other.r32)
        *r32 += *other.r32;
    if (r64 && other.r64)
        *r64 += *other.r64;
    if (rreal && other.rreal)
        *rreal += *other.rreal;
    if (rbig && other.rbig)
        *rbig += *other.rbig;
    return *this;
}

Result& Result::operator-=(Result other) {
    if (r32 && other.r32)
        *r32 -= *other.r32;
    if (r64 && other.r64)
        *r64 -= *other.r64;
    if (rreal && other.rreal)
        *rreal -= *other.rreal;
    if (rbig && other.rbig)
        *rbig -= *other.rbig;
    return *this;
}

Result& Result::operator*=(Result other) {
    if (r32 && other.r32)
        *r32 *= *other.r32;
    if (r64 && other.r64)
        *r64 *= *other.r64;
    if (rreal && other.rreal)
        *rreal *= *other.rreal;
    if (rbig && other.rbig)
        *rbig *= *other.rbig;
    return *this;
}

Result& Result::operator/=(Result other) {
    if (r32 && other.r32)
        *r32 /= *other.r32;
    if (r64 && other.r64)
        *r64 /= *other.r64;
    if (rreal && other.rreal)
        *rreal /= *other.rreal;
    if (rbig && other.rbig)
        *rbig /= *other.rbig;
    return *this;
}

Result& Result::operator<<=(Result other) {
    if (r32 && other.r32) {
        if (*other.r32 < 32)
            *r32 <<= *other.r32;
        else
            r32 = std::nullopt;
    }
    if (r64 && other.r64) {
        if (*other.r64 < 64)
            *r64 <<= *other.r64;
        else
            r64 = std::nullopt;
    }
    rreal = std::nullopt;
    if (rbig && other.rbig)
        *rbig <<= *other.rbig;
    return *this;
}

Result& Result::operator>>=(Result other) {
    if (r32 && other.r32) {
        if (*other.r32 < 32)
            *r32 >>= *other.r32;
        else
            r32 = std::nullopt;
    }
    if (r64 && other.r64) {
        if (*other.r64 < 64)
            *r64 >>= *other.r64;
        else
            r64 = std::nullopt;
    }
    rreal = std::nullopt;
    if (rbig && other.rbig)
        *rbig >>= *other.rbig;
    return *this;
}

Result& Result::operator&=(Result other) {
    if (r32 && other.r32)
        *r32 &= *other.r32;
    if (r64 && other.r64)
        *r64 &= *other.r64;
    rreal = std::nullopt;
    if (rbig && other.rbig)
        *rbig &= *other.rbig;
    return *this;
}

Result& Result::operator|=(Result other) {
    if (r32 && other.r32)
        *r32 |= *other.r32;
    if (r64 && other.r64)
        *r64 |= *other.r64;
    rreal = std::nullopt;
    if (rbig && other.rbig)
        *rbig |= *other.rbig;
    return *this;
}

Result& Result::operator^=(Result other) {
    if (r32 && other.r32)
        *r32 ^= *other.r32;
    if (r64 && other.r64)
        *r64 ^= *other.r64;
    rreal = std::nullopt;
    if (rbig && other.rbig)
        *rbig ^= *other.rbig;
    return *this;
}

Result& Result::operator~() {
    if (r32)
        *r32 = ~r32.value();
    if (r64)
        *r64 = ~*r64;
    rreal = std::nullopt;
    if (rbig)
        *rbig = ~*rbig;
    return *this;
}

namespace {

#if 0
    expression := term [ binop term ]
    binop := MINUS | PLUS | MULT | DIV
    term := INT | MINUS expression | NOT expression | LPAREN expression RPAREN
#endif

template <typename I>
struct Context {
    Context(I begin, I end) : begin_(begin), end_(end) {
        operators_.push(Operator::Sentinel);
    }
    Context(const Context&) = delete;

    enum class Operator {
        Sentinel,

        Or,
        Xor,
        And,  // the lowest
        LShift,
        RShift,
        BMinus,
        Plus,
        Mult,
        Div,  // higher
        UMinus,
        Not  // the highest
    };

    bool Eof() const { return begin_ == end_; }

    void Consume() {
        assert(!Eof());
        ++begin_;
    }

    auto Next() const {
        assert(!Eof());
        return begin_->type;
    }

    bool NextIsBinOp() const {
        static const std::set<Token::Type> bin_ops = {Token::Type::Minus,
                                                      Token::Type::Plus,
                                                      Token::Type::Mult,
                                                      Token::Type::Div,
                                                      Token::Type::LShift,
                                                      Token::Type::RShift,
                                                      Token::Type::Or,
                                                      Token::Type::Xor,
                                                      Token::Type::And};

        return bin_ops.find(Next()) != bin_ops.end();
    }

    Result ConsumeInt() {
        assert(!Eof() && Next() == Token::Type::Int);
        Result r(begin_->value, begin_->base);
        Consume();
        return r;
    }

    Operator ConsumeBinOp() {
        if (Eof())
            throw std::runtime_error("Abrupt end of input while parsing a 'binary op'.");

        static const std::map<Token::Type, Operator> bin_ops = {
            {Token::Type::Minus, Operator::BMinus},
            {Token::Type::Plus, Operator::Plus},
            {Token::Type::Mult, Operator::Mult},
            {Token::Type::Div, Operator::Div},
            {Token::Type::LShift, Operator::LShift},
            {Token::Type::RShift, Operator::RShift},
            {Token::Type::And, Operator::And},
            {Token::Type::Or, Operator::Or},
            {Token::Type::Xor, Operator::Xor}};

        auto i = bin_ops.find(Next());
        if (i == bin_ops.end()) throw std::runtime_error("Failed to parse a binary op");

        Consume();
        return i->second;
    }

    Operator ConsumeUnOp() {
        if (Eof())
            throw std::runtime_error("Abrupt end of input while parsing a 'unary op'.");

        switch (Next()) {
            case Token::Type::Minus:
                Consume();
                return Operator::UMinus;
            case Token::Type::Not:
                Consume();
                return Operator::Not;

            default:
                throw std::runtime_error("Failed to parse a unary op");
        }
    }

    void PopOperator() {
        if (operators_.top() == Operator::UMinus) {
            auto result =operands_.top();
            operands_.pop();
            operators_.pop();
            result *= Result("-1");
            operands_.push(result);
        } else if (operators_.top() == Operator::Not) {
            auto r = ~operands_.top();
            operands_.pop();
            operators_.pop();
            operands_.push(r);
        } else {
            auto other = operands_.top();
            operands_.pop();
            auto a = operands_.top();
            operands_.pop();

            switch (operators_.top()) {
                case Operator::BMinus:
                    a -= other;
                    break;
                case Operator::Plus:
                    a += other;
                    break;
                case Operator::Mult:
                    a *= other;
                    break;
                case Operator::Div:
                    a /= other;
                    break;
                case Operator::LShift:
                    a <<= other;
                    break;
                case Operator::RShift:
                    a >>= other;
                    break;
                case Operator::And:
                    a &= other;
                    break;
                case Operator::Or:
                    a |= other;
                    break;
                case Operator::Xor:
                    a ^= other;
                    break;

                default:
                    throw std::runtime_error(
                        "Unexpected op while computing an expression...");
            }

            operators_.pop();
            operands_.push(a);
        }
    }

    void PushOperator(Operator op) {
        while (operators_.top() > op)
            PopOperator();
        operators_.push(op);
    }

    I begin_, end_;
    std::stack<Operator> operators_;
    std::stack<Result> operands_;
};

template <typename I>
void Expression(Context<I>& ctx) {
    if (ctx.Eof())
        throw std::runtime_error("Abrupt end of input while parsing an 'expression'.");

    Term(ctx);

    while (!ctx.Eof() && ctx.NextIsBinOp()) {
        ctx.PushOperator(ctx.ConsumeBinOp());
        Term(ctx);
    }

    while (ctx.operators_.top() != Context<I>::Operator::Sentinel)
        ctx.PopOperator();
}

template <typename I>
void Term(Context<I>& ctx) {
    if (ctx.Eof())
        throw std::runtime_error("Abrupt end of input while parsing a 'term'.");

    switch (ctx.Next()) {
        // The terminals
        case Token::Type::Int:
            ctx.operands_.push({ctx.ConsumeInt()});
            return;

        // Unary ops
        case Token::Type::Minus:
        case Token::Type::Not:
            ctx.PushOperator(ctx.ConsumeUnOp());
            Expression(ctx);
            return;

        // A sub-expression with parens: ( .... )
        case Token::Type::LParen:
            ctx.Consume();
            ctx.operators_.push(Context<I>::Operator::Sentinel);
            Expression(ctx);
            if (ctx.Eof())
                throw std::runtime_error("Missing RParen");
            if (ctx.Next() != Token::Type::RParen)
                throw std::runtime_error(
                    std::string("Unxpected token while expecting RParen: ") +
                    ToString(ctx.Next()));
            ctx.Consume();
            assert(ctx.operators_.top() == Context<I>::Operator::Sentinel);
            ctx.operators_.pop();
            break;

        default:
            throw std::runtime_error(
                std::string("Failed to parse a 'term': unexpected token: ") +
                ToString(ctx.Next()));
    }
}

template <typename I>
Result Parse(I begin, I end) {
    Context<I> ctx{begin, end};
    Expression(ctx);
    return ctx.operands_.top();
}

}  // namespace

Result Compute(const std::string& inp) {
    auto token_stream = Scan(inp);
    if (token_stream.empty())
        return Result();

    return Parse(token_stream.begin(), token_stream.end());
}

}  // namespace parser
