#include "stdafx.h"
#include "parser.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <glog/logging.h>

#define STRINGIFY0(x) #x
#define STRINGIFY(x) STRINGIFY0(x)

namespace parser {
namespace {

// This is the central structure that holds the parsing context (well, the Scanner
// instance) and provides the ConsumeXxxx() methods needed for building the AST.
template <typename I>
struct Context {
    Context(Scanner<I> scanner) : scanner_(std::move(scanner)) {}
    Context(const Context&) = delete;
    Context(Context&&) = default;

    Result ConsumeInt() {
        DCHECK_EQ(scanner_.Next().type, Token::Number);
        Result r(scanner_.Next());
        scanner_.Pop();
        return r;
    }

    Result ConsumeConstant() {
        DCHECK_EQ(scanner_.Next().type, Token::Pi);
        Result r(Token(STRINGIFY(M_PI), 10, Token::ValidFloat));
        DCHECK(r.real);
        scanner_.Pop();
        return r;
    }

    detail::Operator ConsumeBinaryOp() {
        if (scanner_.ReachedEof())
            throw Exception("Abrupt end of input while parsing a 'binary op'.");

        DCHECK(scanner_.Next().IsBinOp());
        auto op = scanner_.Next().GetBinOp();

        scanner_.Pop();
        return op;
    }

    detail::Operator ConsumeUnaryOp() {
        if (scanner_.ReachedEof())
            throw Exception("Abrupt end of input while parsing a 'unary op'.");

        switch (scanner_.Next().type) {
            case Token::Minus:
                scanner_.Pop();
                return detail::Operator::UMinus;
            case Token::Not:
                scanner_.Pop();
                return detail::Operator::Not;

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
//  <input>         ::= <expression> EOF
template <typename I>
std::unique_ptr<ast::Node> Input(Context<I>& ctx) {
    if (ctx.scanner_.ReachedEof())
        throw Exception("Abrupt end of input while parsing the 'input' rule.");

    auto ast = Expression(ctx);

    if (!ctx.scanner_.ReachedEof())
        throw Exception("Unexpected token while parsing the 'input' rule: '" +
                        ctx.scanner_.Next().ToString() + "'");

    return ast;
}

// Grammar:
//  <expression>    ::= <term> [ <binop> <term> ]
//  <binop>         ::= MINUS | PLUS | MULT | DIV | REM | LSHIFT | RSHIFT |
//                      POW | AND | OR | XOR
template <typename I>
std::unique_ptr<ast::Node> Expression(Context<I>& ctx) {
    if (ctx.scanner_.ReachedEof())
        throw Exception("Abrupt end of input while parsing an 'expression' rule.");

    return ExpressionHelper(ctx, Term(ctx), 0);
}

// Implements an operator-precedence parser. See pseudo-code at
// https://en.wikipedia.org/wiki/Operator-precedence_parser
template <typename I>
std::unique_ptr<ast::Node> ExpressionHelper(Context<I>& ctx,
                                            std::unique_ptr<ast::Node> left,
                                            int min_precedence) {
    auto lookahead = ctx.scanner_.Next();
    while (lookahead.IsBinOp() &&
           OperatorPrecedence(lookahead.GetBinOp()) >= min_precedence) {
        auto op = ctx.ConsumeBinaryOp();
        auto right = Term(ctx);
        lookahead = ctx.scanner_.Next();

        while (lookahead.IsBinOp() &&
               OperatorPrecedence(lookahead.GetBinOp()) > OperatorPrecedence(op)
               /* OR lookahad is a right-associative operator
                 whose precedence is equal to op's */) {
            right = ExpressionHelper(
                ctx, std::move(right), OperatorPrecedence(lookahead.GetBinOp()));
            lookahead = ctx.scanner_.Next();
        }

        left = std::make_unique<ast::BinaryOp>(op, std::move(left), std::move(right));
    }

    return left;
}

// Grammar:
//  <args>          ::= <expression> [ COMA <args> ]
template <typename I>
std::list<std::unique_ptr<ast::Node>> Args(Context<I>& ctx) {
    if (ctx.scanner_.ReachedEof())
        throw Exception("Abrupt end of input while parsing the 'args' rule.");

    std::list<std::unique_ptr<ast::Node>> rv;
    rv.emplace_back(Expression(ctx));

    while (!ctx.scanner_.ReachedEof() && ctx.scanner_.Next().type == Token::Coma) {
        ctx.scanner_.Pop();
        rv.splice(rv.end(), Args(ctx));

    }

    return rv;
}

// Grammar:
//  <term>          ::= INT
//                  | MINUS <term>
//                  | NOT <term>
//                  | LPAREN <expression> RPAREN
//                  | FUNCTION LPAREN <args> RPAREN
//                  | <constatnt>
//  <constant>      ::= PI
template <typename I>
std::unique_ptr<ast::Node> Term(Context<I>& ctx) {
    if (ctx.scanner_.ReachedEof())
        throw Exception("Abrupt end of input while parsing the 'term' rule.");

    std::unique_ptr<ast::Node> ast;

    switch (ctx.scanner_.Next().type) {
        // The terminals
        case Token::Number:
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
                throw Exception("Unxpected token while expecting RParen: '" +
                                ctx.scanner_.Next().ToString() + "'");
            ctx.scanner_.Pop();
            break;

        // A function call with parens: xxxx( .... )
        case Token::Function: {
            Token func = ctx.scanner_.Next();
            ctx.scanner_.Pop();
            if (ctx.scanner_.ReachedEof())
                throw Exception("Missing LParen");
            if (ctx.scanner_.Next().type != Token::LParen)
                throw Exception("Unxpected token while expecting LParen: '" +
                                ctx.scanner_.Next().ToString() + "'");
            ctx.scanner_.Pop();
            ast = std::make_unique<ast::Function>(func, Args(ctx));
            if (ctx.scanner_.ReachedEof())
                throw Exception("Missing RParen");
            if (ctx.scanner_.Next().type != Token::RParen)
                throw Exception("Unxpected token while expecting RParen: '" +
                                ctx.scanner_.Next().ToString() + "'");
            ctx.scanner_.Pop();
            break;
        }

        default:
            throw Exception("Failed to parse the 'term' rule. Unexpected token: '" +
                            ctx.scanner_.Next().ToString() + "'");
    }

    DCHECK(ast);
    return ast;
}

}  // namespace

namespace ast {

Result Node::Compute(std::vector<int>& indent_stack, int indent) const {
    std::string line;

    auto istack = indent_stack.begin();
    for (int i = 0; i < indent; ++i) {
        if (i == indent - 1) {
            line += " +--";
            break;
        }
        if (istack != indent_stack.end() && *istack == i) {
            line += " |  ";
            ++istack;
        }
        else
            line += "    ";
    }

    utils::OutputDebugLine(line + Print());
    return DoCompute(indent_stack, indent);
}

Result Terminal::DoCompute(std::vector<int>& indent_stack, int indent) const {
    DCHECK(value.Valid());
    return value;
}

Result BinaryOp::DoCompute(std::vector<int>& indent_stack, int indent) const {
    indent_stack.push_back(indent);
    auto lresult = left_ast->Compute(indent_stack, indent + 1);

    DCHECK(!indent_stack.empty());
    DCHECK_EQ(indent_stack.back(), indent);
    indent_stack.pop_back();
    auto rresult = right_ast->Compute(indent_stack, indent + 1);

    // Deal with binary ops.
    switch (op) {
    case detail::Operator::BMinus:
        lresult -= rresult;
        break;
    case detail::Operator::Plus:
        lresult += rresult;
        break;
    case detail::Operator::Mult:
        lresult *= rresult;
        break;
    case detail::Operator::Div:
        lresult /= rresult;
        break;
    case detail::Operator::Rem:
        lresult %= rresult;
        break;
    case detail::Operator::LShift:
        lresult <<= rresult;
        break;
    case detail::Operator::RShift:
        lresult >>= rresult;
        break;
    case detail::Operator::And:
        lresult &= rresult;
        break;
    case detail::Operator::Or:
        lresult |= rresult;
        break;
    case detail::Operator::Xor:
        lresult ^= rresult;
        break;
    case detail::Operator::Pow:
        // Convert the binary op into a function.
        lresult.ApplyFunction("pow", rresult);
        break;
    default:
        throw Exception("Unexpected binary op: " + std::to_string(static_cast<int>(op)));
    }

    if (!lresult.Valid())
        throw Exception("Binary operator " + ToString(op) + " yields no result");

    return lresult;
}

Result UnaryOp::DoCompute(std::vector<int>& indent_stack, int indent) const {
    indent_stack.push_back(indent);

    auto r = arg_ast->Compute(indent_stack, indent + 1);

    // Deal with unary operators.
    switch (op) {
    case detail::Operator::UMinus:
        r *= Result(Token("-1", 10, Token::ValidFloat | Token::ValidInt));
        break;

    case detail::Operator::Not:
        r.operator~();
        break;

    default:
        throw Exception("Unexpected unary op: " + ToString(op));
    }

    DCHECK(!indent_stack.empty());
    DCHECK_EQ(indent_stack.back(), indent);
    indent_stack.pop_back();

    return r;
}

Result Function::DoCompute(std::vector<int>& indent_stack, int indent) const {
    indent_stack.push_back(indent);

    std::deque<Result> results;
    for (const auto& arg : args)
        results.push_back(arg->Compute(indent_stack, indent + 1));

    switch (results.size()) {
    case 1:
        results.front().ApplyFunction(token.value);
        break;

    case 2:
        results[0].ApplyFunction(token.value, results[1]);
        break;

    default:
        throw Exception("No known function takes " + std::to_string(results.size()) +
                        " arguments");
    }

    DCHECK(!indent_stack.empty());
    DCHECK_EQ(indent_stack.back(), indent);
    indent_stack.pop_back();

    return results.front();
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

}  // namespace parser
