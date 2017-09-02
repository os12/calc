#include "stdafx.h"
#include "parser.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <glog/logging.h>

#define STRINGIFY0(x) #x
#define STRINGIFY(x) STRINGIFY0(x)

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
                | FUNCTION LPAREN args RPAREN
                | constatnt
    args        := expression [ COMA args ]
    constant    := PI

--------------------------------------------------------------------------------
*/

template <typename I>
struct Context {
    Context(Scanner<I> scanner) : scanner_(std::move(scanner)) {
        operators_.push(Operator::Sentinel);
    }
    Context(const Context&) = delete;
    Context(Context&&) = default;

    static const int OpMultiplier = 1024;

    // Enum for binary/unary operators sorted by their precedence. The interesting
    // thing here is that each enum value must me unique in C++, yet pairs like
    // BMinus/Plus and Mult/Div must have identical values in order to process
    // the expressions correctly. That is, a Mult/Div pair must be processed from
    // left to right (a.k.a. left-associative ops). So, let's invent a multiplier for
    // the values and then strip it in comparisons.
    enum class Operator {
        Sentinel = 0,

        Or = 1 * OpMultiplier,            // the lowest
        Xor = 2 * OpMultiplier,
        And = 3 * OpMultiplier,
        LShift = 4 * OpMultiplier,
        RShift = 5 * OpMultiplier,

        BMinus = 6 * OpMultiplier,
        Plus = 6 * OpMultiplier + 1,

        Mult = 7 * OpMultiplier,
        Div = 7 * OpMultiplier + 1,

        Pow = 8 * OpMultiplier,         // the highest binary op

        UMinus = 9 * OpMultiplier,
        Not = 10 * OpMultiplier         // the highest unary op
    };

    // The key "less than" operator. Strips the multiplier along with the
    // least-significant units in order to make some operators equal.
    friend bool operator<(Operator op1, Operator op2) {
        return static_cast<int>(op1) / OpMultiplier <
               static_cast<int>(op2) / OpMultiplier;
    }

    // Derive these from the key operator.
    friend bool operator>=(Operator op1, Operator op2) { return !(op1 < op2); }
    friend bool operator>(Operator op1, Operator op2) { return op2 < op1; }
    friend bool operator<=(Operator op1, Operator op2) { return !(op1 > op2); }

    bool NextIsBinOp() {
        static const std::set<Token::Type> bin_ops = {Token::Minus,
                                                      Token::Plus,
                                                      Token::Mult,
                                                      Token::Div,
                                                      Token::LShift,
                                                      Token::RShift,
                                                      Token::Or,
                                                      Token::Xor,
                                                      Token::And,
                                                      Token::Pow};

        return bin_ops.find(scanner_.Next().type) != bin_ops.end();
    }

    Result ConsumeInt() {
        DCHECK_EQ(scanner_.Next().type, Token::Int);
        Result r(scanner_.Next().value, scanner_.Next().base);
        scanner_.Pop();
        return r;
    }

    Result ConsumeConstant() {
        DCHECK_EQ(scanner_.Next().type, Token::Pi);
        Result r(STRINGIFY(M_PI));
        DCHECK(r.rreal);
        scanner_.Pop();
        return r;
    }

    Operator ConsumeBinaryOp() {
        if (scanner_.Eof())
            throw Exception("Abrupt end of input while parsing a 'binary op'.");

        static const std::map<Token::Type, Operator> bin_ops = {
            {Token::Minus, Operator::BMinus},
            {Token::Plus, Operator::Plus},
            {Token::Mult, Operator::Mult},
            {Token::Div, Operator::Div},
            {Token::LShift, Operator::LShift},
            {Token::RShift, Operator::RShift},
            {Token::And, Operator::And},
            {Token::Or, Operator::Or},
            {Token::Xor, Operator::Xor},
            {Token::Pow, Operator::Pow}};

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
            case Token::Minus:
                scanner_.Pop();
                return Operator::UMinus;
            case Token::Not:
                scanner_.Pop();
                return Operator::Not;

            default:
                throw Exception("Failed to parse a unary op");
        }
    }

    void ApplyFunction(Token token) {
        DCHECK_EQ(token.type, Token::Function);
        DCHECK(!operands_.empty());

        switch (operands_.size()) {
        case 1:
            operands_.top().ApplyFunction(token.value);
            break;

        case 2: {
            auto arg2 = operands_.top();
            operands_.pop();
            operands_.top().ApplyFunction(token.value, arg2);
            break;
        }

        default:
            throw Exception("No known functions take " +
                            std::to_string(operands_.size()) + " arguments");
        }

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
            case Operator::Pow:
                // Convert the binary op into a function.
                operands_.top().ApplyFunction("pow", other);
                break;

            default:
                throw Exception("Unexpected op while computing an expression: " +
                                std::to_string(static_cast<int>(operators_.top())));
        }

        operators_.pop();
    }

    void PushOperator(Operator op) {
        while (operators_.top() >= op)
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
void Args(Context<I>& ctx) {
    if (ctx.scanner_.Eof())
        throw Exception("Abrupt end of input while parsing 'args'.");

    Expression(ctx);

    while (!ctx.scanner_.Eof() && ctx.scanner_.Next().type == Token::Coma) {
        ctx.scanner_.Pop();
        Args(ctx);
    }
}

template <typename I>
void Term(Context<I>& ctx) {
    if (ctx.scanner_.Eof())
        throw Exception("Abrupt end of input while parsing a 'term'.");

    switch (ctx.scanner_.Next().type) {
        // The terminals
        case Token::Int:
            ctx.operands_.push({ctx.ConsumeInt()});
            return;

        // Unary ops
        case Token::Minus:
        case Token::Not:
            ctx.PushOperator(ctx.ConsumeUnaryOp());
            Expression(ctx);
            return;

        // A sub-expression with parens: ( .... )
        case Token::LParen:
            ctx.scanner_.Pop();
            ctx.PushSentinel();
            Expression(ctx);
            if (ctx.scanner_.Eof())
                throw Exception("Missing RParen");
            if (ctx.scanner_.Next().type != Token::RParen)
                throw Exception("Unxpected token while expecting RParen: " +
                                ToString(ctx.scanner_.Next().type));
            ctx.scanner_.Pop();
            ctx.PopSentinel();
            break;

        // A function call with parens: xxxx( .... )
        case Token::Function: {
            Token func = ctx.scanner_.Next();
            ctx.scanner_.Pop();
            if (ctx.scanner_.Eof())
                throw Exception("Missing LParen");
            if (ctx.scanner_.Next().type != Token::LParen)
                throw Exception("Unxpected token while expecting LParen: " +
                                ToString(ctx.scanner_.Next().type));
            ctx.scanner_.Pop();
            Args(ctx);
            if (ctx.scanner_.Eof())
                throw Exception("Missing RParen");
            if (ctx.scanner_.Next().type != Token::RParen)
                throw Exception("Unxpected token while expecting RParen: " +
                                ToString(ctx.scanner_.Next().type));
            ctx.scanner_.Pop();
            ctx.ApplyFunction(func);
            break;
        }

        // Built-in constants.
        case Token::Pi:
            ctx.operands_.push({ctx.ConsumeConstant()});
            return;

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
