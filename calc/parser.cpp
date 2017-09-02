#include "stdafx.h"
#include "parser.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <glog/logging.h>

#define STRINGIFY0(x) #x
#define STRINGIFY(x) STRINGIFY0(x)

namespace parser {

namespace {

int GetBinOpPrecedence(const Token& t) {
    DCHECK(t.IsBinOp());

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

    auto i = bin_ops.find(t.type);
    DCHECK(i != bin_ops.end());

    return OperatorPrecedence(i->second);
}

// This is the central structure that holds the parsing context:
//  - the scanner
//  - oprator and operand stacks
template <typename I>
struct Context {
    Context(Scanner<I> scanner) : scanner_(std::move(scanner)) {}
    Context(const Context&) = delete;
    Context(Context&&) = default;

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
        if (scanner_.ReachedEof())
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
        if (scanner_.ReachedEof())
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

    Scanner<I> scanner_;
};

template<typename I>
auto MakeContext(Scanner<I> scanner) {
    return Context<I>(std::move(scanner));
}

// Grammar:
//  input       := expression EOF
template <typename I>
std::unique_ptr<ast::Node> Input(Context<I>& ctx) {
    if (ctx.scanner_.ReachedEof())
        throw Exception("Abrupt end of input while parsing the 'input'.");

    auto ast = Expression(ctx);

    if (!ctx.scanner_.ReachedEof())
        throw Exception("Unexpected token after parsing an 'expression': " +
                        ToString(ctx.scanner_.Next().type));

    return ast;
}

// Grammar:
//  expression  := term [ binop term ]
//  binop       := MINUS | PLUS | MULT | DIV | LSHIFT | RSHIFT | POW | AND | OR | XOR
template <typename I>
std::unique_ptr<ast::Node> Expression(Context<I>& ctx) {
    if (ctx.scanner_.ReachedEof())
        throw Exception("Abrupt end of input while parsing an 'expression'.");

    return ExpressionHelper(ctx, Term(ctx), 0);
}

// Implements an operator-precedence parser. See pseudo-code at
// https://en.wikipedia.org/wiki/Operator-precedence_parser
template <typename I>
std::unique_ptr<ast::Node> ExpressionHelper(Context<I>& ctx,
                                            std::unique_ptr<ast::Node> left,
                                            int min_precedence) {
    auto lookahead = ctx.scanner_.Next();
    while (lookahead.IsBinOp() && GetBinOpPrecedence(lookahead) >= min_precedence) {
        auto op = ctx.ConsumeBinaryOp();
        auto right = Term(ctx);
        lookahead = ctx.scanner_.Next();

        while (lookahead.IsBinOp() &&
               GetBinOpPrecedence(lookahead) > OperatorPrecedence(op)
               /* OR lookahad is a right-associative operator
                 whose precedence is equal to op's */) {
            right =
                ExpressionHelper(ctx, std::move(right), GetBinOpPrecedence(lookahead));
            lookahead = ctx.scanner_.Next();
        }

        left = std::make_unique<ast::BinaryOp>(op, std::move(left), std::move(right));
    }

    return left;
}

// Grammar:
//  args        := expression [ COMA args ]
template <typename I>
std::list<std::unique_ptr<ast::Node>> Args(Context<I>& ctx) {
    if (ctx.scanner_.ReachedEof())
        throw Exception("Abrupt end of input while parsing 'args'.");

    std::list<std::unique_ptr<ast::Node>> rv;
    rv.emplace_back(Expression(ctx));

    while (!ctx.scanner_.ReachedEof() && ctx.scanner_.Next().type == Token::Coma) {
        ctx.scanner_.Pop();
        rv.splice(rv.end(), Args(ctx));

    }

    return rv;
}

// Grammar:
//  term        := INT
//              | MINUS term
//              | NOT term
//              | LPAREN expression RPAREN
//              | FUNCTION LPAREN args RPAREN
//              | constatnt
//
//  constant    := PI
template <typename I>
std::unique_ptr<ast::Node> Term(Context<I>& ctx) {
    if (ctx.scanner_.ReachedEof())
        throw Exception("Abrupt end of input while parsing a 'term'.");

    std::unique_ptr<ast::Node> ast;

    switch (ctx.scanner_.Next().type) {
        // The terminals
        case Token::Int:
            ast = std::make_unique<ast::Terminal>(ctx.ConsumeInt());
            break;
        case Token::Pi:
            ast = std::make_unique<ast::Terminal>(ctx.ConsumeConstant());
            break;

        // Unary ops
        case Token::Minus:
        case Token::Not: {
            const auto op = ctx.ConsumeUnaryOp();
            ast = std::make_unique<ast::UnaryOp>(op, Term(ctx));
            break;
        }

        // A sub-expression with parens: ( .... )
        case Token::LParen:
            ctx.scanner_.Pop();

            ast = Expression(ctx);
            if (ctx.scanner_.ReachedEof())
                throw Exception("Missing RParen");
            if (ctx.scanner_.Next().type != Token::RParen)
                throw Exception("Unxpected token while expecting RParen: " +
                                ToString(ctx.scanner_.Next().type));
            ctx.scanner_.Pop();
            break;

        // A function call with parens: xxxx( .... )
        case Token::Function: {
            Token func = ctx.scanner_.Next();
            ctx.scanner_.Pop();
            if (ctx.scanner_.ReachedEof())
                throw Exception("Missing LParen");
            if (ctx.scanner_.Next().type != Token::LParen)
                throw Exception("Unxpected token while expecting LParen: " +
                                ToString(ctx.scanner_.Next().type));
            ctx.scanner_.Pop();
            ast = std::make_unique<ast::Function>(func, Args(ctx));
            if (ctx.scanner_.ReachedEof())
                throw Exception("Missing RParen");
            if (ctx.scanner_.Next().type != Token::RParen)
                throw Exception("Unxpected token while expecting RParen: " +
                                ToString(ctx.scanner_.Next().type));
            ctx.scanner_.Pop();
            break;
        }

        default:
            throw Exception("Failed to parse a 'term': unexpected token: " +
                            ToString(ctx.scanner_.Next().type));
    }

    DCHECK(ast);
    return ast;
}

}  // namespace

namespace ast {

Result Terminal::Compute(int indent) {
    base::OutputDebugLine(Indent(indent) + "Terminal: " + result.ToString());
    DCHECK(result.Valid());
    return result;
}

Result BinaryOp::Compute(int indent) {
    base::OutputDebugLine(Indent(indent) + "bin op: " + ToString(op));
    auto lresult = left->Compute(indent + 1);
    auto rresult = right->Compute(indent + 1);

    // Deal with binary ops.
    switch (op) {
    case Operator::BMinus:
        lresult -= rresult;
        break;
    case Operator::Plus:
        lresult += rresult;
        break;
    case Operator::Mult:
        lresult *= rresult;
        break;
    case Operator::Div:
        lresult /= rresult;
        break;
    case Operator::LShift:
        lresult <<= rresult;
        break;
    case Operator::RShift:
        lresult >>= rresult;
        break;
    case Operator::And:
        lresult &= rresult;
        break;
    case Operator::Or:
        lresult |= rresult;
        break;
    case Operator::Xor:
        lresult ^= rresult;
        break;
    case Operator::Pow:
        // Convert the binary op into a function.
        lresult.ApplyFunction("pow", rresult);
        break;
    default:
        throw Exception("Unexpected binary op: " + ToString(op));
    }
    return lresult;
}

Result UnaryOp::Compute(int indent) {
    base::OutputDebugLine(Indent(indent) + "unary op: " + ToString(op));
    auto r = value->Compute(indent + 1);

    // Deal with unary operators.
    switch (op) {
    case Operator::UMinus:
        r *= Result("-1");
        return r;

    case Operator::Not:
        r.operator~();
        return r;
    }
    throw Exception("Unexpected unary op: " + ToString(op));
}

Result Function::Compute(int indent) {
    base::OutputDebugLine(Indent(indent) + "function call: " + token.value);

    std::deque<Result> results;
    for (const auto& arg : args)
        results.push_back(arg->Compute(indent + 1));

    switch (results.size()) {
        case 1:
            results.front().ApplyFunction(token.value);
            return results.front();

        case 2:
            results[0].ApplyFunction(token.value, results[1]);
            return results[0];
    }
    throw Exception("No known function takes " + std::to_string(results.size()) +
                    " arguments");
}

}  // namespace ast

std::unique_ptr<ast::Node> Parse(const std::string& inp) {
    auto scanner = MakeScanner(inp.begin(), inp.end());
    if (scanner.ReachedEof())
        return nullptr;

    auto ctx = MakeContext(std::move(scanner));
    auto ast = Input(ctx);
    DCHECK(ctx.scanner_.ReachedEof());
    return ast;
}

#define CASE(v) case Operator::v: return #v

std::string ToString(Operator op) {
    switch (op) {
    CASE(UMinus);
    CASE(BMinus);
    CASE(Plus);
    CASE(Mult);
    CASE(Div);
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
}  // namespace parser
