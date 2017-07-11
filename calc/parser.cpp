#include "stdafx.h"
#include "parser.h"

#include <glog/logging.h>

namespace parser {

namespace {

/* 
--------------------------------------------------------------------------------
  Grammar:
 
    input       := expression EOF
    expression  := term [ binop term ]
    binop       := MINUS | PLUS | MULT | DIV
    term        := INT
                | MINUS expression
                | NOT expression
                | LPAREN expression RPAREN
                | FUNCTION LPAREN expression RPAREN

--------------------------------------------------------------------------------
*/

template <typename I>
struct Context {
    Context(Scanner<I> scanner) : scanner_(std::move(scanner)) {
        operators_.push(Operator::Sentinel);
    }
    Context(const Context&) = delete;
    Context(Context&&) = default;

    enum class Operator {
        Sentinel,

        Or,         // the lowest
        Xor,
        And,
        LShift,
        RShift,
        BMinus,
        Plus,
        Mult,
        Div,        // the highest binary op

        UMinus,
        Not         // the highest unary op
    };

    bool NextIsBinOp() {
        static const std::set<Token::Type> bin_ops = {Token::Type::Minus,
                                                      Token::Type::Plus,
                                                      Token::Type::Mult,
                                                      Token::Type::Div,
                                                      Token::Type::LShift,
                                                      Token::Type::RShift,
                                                      Token::Type::Or,
                                                      Token::Type::Xor,
                                                      Token::Type::And};

        return bin_ops.find(scanner_.Next().type) != bin_ops.end();
    }

    Result ConsumeInt() {
        DCHECK_EQ(scanner_.Next().type, Token::Type::Int);
        Result r(scanner_.Next().value, scanner_.Next().base);
        scanner_.Pop();
        return r;
    }

    Operator ConsumeBinaryOp() {
        if (scanner_.Eof())
            throw Exception("Abrupt end of input while parsing a 'binary op'.");

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

        auto i = bin_ops.find(scanner_.Next().type);
        if (i == bin_ops.end())
            throw Exception("Failed to parse a binary op");

        scanner_.Pop();
        return i->second;
    }

    Operator ConsumeUnaryOp() {
        if (scanner_.Eof())
            throw Exception("Abrupt end of input while parsing a 'unary op'.");

        switch (scanner_.Next().type) {
            case Token::Type::Minus:
                scanner_.Pop();
                return Operator::UMinus;
            case Token::Type::Not:
                scanner_.Pop();
                return Operator::Not;

            default:
                throw Exception("Failed to parse a unary op");
        }
    }

    void ApplyUnaryFunction(Token token) {
        DCHECK_EQ(operands_.size(), 1);
        DCHECK_EQ(token.type, Token::Type::Function);

        operands_.top().ApplyUnaryFunction(token.value);
    }

    void PopOperator() {
        DCHECK(!operators_.empty());
        DCHECK(!operands_.empty());

        // Deal with unary operators.
        switch (operators_.top()) {
            case Operator::UMinus:
                operators_.pop();
                operands_.top() *= Result("-1");
                return;

            case Operator::Not:
                operators_.pop();
                operands_.top().operator~();
                return;

            default:
                break;
        }

        // Deal with binary ops.
        auto other = operands_.top();
        operands_.pop();
        DCHECK(!operands_.empty());
        switch (operators_.top()) {
            case Operator::BMinus:
                operands_.top() -= other;
                break;
            case Operator::Plus:
                operands_.top() += other;
                break;
            case Operator::Mult:
                operands_.top() *= other;
                break;
            case Operator::Div:
                operands_.top() /= other;
                break;
            case Operator::LShift:
                operands_.top() <<= other;
                break;
            case Operator::RShift:
                operands_.top() >>= other;
                break;
            case Operator::And:
                operands_.top() &= other;
                break;
            case Operator::Or:
                operands_.top() |= other;
                break;
            case Operator::Xor:
                operands_.top() ^= other;
                break;

            default:
                throw Exception("Unexpected op while computing an expression: " +
                                std::to_string(static_cast<int>(operators_.top())));
        }

        operators_.pop();
    }

    void PushOperator(Operator op) {
        while (operators_.top() > op)
            PopOperator();
        operators_.push(op);
    }

    void PushSentinel() {
        // Push it directly for the "term := ( expression )" case, without any poping (as
        // that would try calculating a partially-formed expression and fail).
        operators_.push(Operator::Sentinel);
    }

    void PopSentinel() {
        CHECK(!operators_.empty());
        CHECK(operators_.top() == Operator::Sentinel);
        operators_.pop();
    }

    Scanner<I> scanner_;
    std::stack<Operator> operators_;
    std::stack<Result> operands_;
};

template<typename I>
auto MakeContext(Scanner<I> scanner) {
    return Context<I>(std::move(scanner));
}

template <typename I>
void Input(Context<I>& ctx) {
    if (ctx.scanner_.Eof())
        throw Exception("Abrupt end of input while parsing the 'input'.");

    Expression(ctx);

    if (!ctx.scanner_.Eof())
        throw Exception("Unexpected token after parsing an 'expression': " +
                        ToString(ctx.scanner_.Next().type));
}

template <typename I>
void Expression(Context<I>& ctx) {
    if (ctx.scanner_.Eof())
        throw Exception("Abrupt end of input while parsing an 'expression'.");

    Term(ctx);

    while (!ctx.scanner_.Eof() && ctx.NextIsBinOp()) {
        ctx.PushOperator(ctx.ConsumeBinaryOp());
        Term(ctx);
    }

    while (ctx.operators_.top() != Context<I>::Operator::Sentinel)
        ctx.PopOperator();
}

template <typename I>
void Term(Context<I>& ctx) {
    if (ctx.scanner_.Eof())
        throw Exception("Abrupt end of input while parsing a 'term'.");

    switch (ctx.scanner_.Next().type) {
        // The terminals
        case Token::Type::Int:
            ctx.operands_.push({ctx.ConsumeInt()});
            return;

        // Unary ops
        case Token::Type::Minus:
        case Token::Type::Not:
            ctx.PushOperator(ctx.ConsumeUnaryOp());
            Expression(ctx);
            return;

        // A sub-expression with parens: ( .... )
        case Token::Type::LParen:
            ctx.scanner_.Pop();
            ctx.PushSentinel();
            Expression(ctx);
            if (ctx.scanner_.Eof())
                throw Exception("Missing RParen");
            if (ctx.scanner_.Next().type != Token::Type::RParen)
                throw Exception("Unxpected token while expecting RParen: " +
                                ToString(ctx.scanner_.Next().type));
            ctx.scanner_.Pop();
            ctx.PopSentinel();
            break;

        // A function call with parens: xxxx( .... )
        case Token::Type::Function: {
            Token func = ctx.scanner_.Next();
            ctx.scanner_.Pop();
            if (ctx.scanner_.Eof())
                throw Exception("Missing LParen");
            if (ctx.scanner_.Next().type != Token::Type::LParen)
                throw Exception("Unxpected token while expecting LParen: " +
                                ToString(ctx.scanner_.Next().type));
            ctx.scanner_.Pop();
            Expression(ctx);
            if (ctx.scanner_.Eof())
                throw Exception("Missing RParen");
            if (ctx.scanner_.Next().type != Token::Type::RParen)
                throw Exception("Unxpected token while expecting RParen: " +
                                ToString(ctx.scanner_.Next().type));
            ctx.scanner_.Pop();
            ctx.ApplyUnaryFunction(func);
            break;
        }

        default:
            throw Exception("Failed to parse a 'term': unexpected token: " +
                            ToString(ctx.scanner_.Next().type));
    }
}

}  // namespace

Result Compute(const std::string& inp) {
    auto scanner = MakeScanner(inp.begin(), inp.end());
    if (scanner.Eof())
        return Result();

    auto ctx = MakeContext(std::move(scanner));
    Input(ctx);
    DCHECK(ctx.scanner_.Eof());
    DCHECK_EQ(ctx.operands_.size(), 1);
    return ctx.operands_.top();
}

}  // namespace parser
