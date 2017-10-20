#pragma once

#include <optional>
#include "big/Cbignum.h"
#include "scanner.h"
#include "utils.h"

namespace parser {

// The compulation result. Generally, a subset of fields has meaningful values as various
// operators and functions restrict the full function's (well, it's an expression really)
// range.
struct Result {
    explicit Result() = default;

    explicit Result(uint32_t u32) : u32(u32), i32(u32), big(u32) {
        real = u32;
        u64 = u32;
    }

    explicit Result(const Token& t);

    bool Valid() const { return u64 || u32 || i32 || real || big; }

    // A partial string conversion for debugging.
    std::string ToString() const;

    // Overload normal arithmetic operators.
    Result& operator+=(Result b);
    Result& operator-=(Result b);
    Result& operator*=(Result b);
    Result& operator/=(Result b);
    Result& operator>>=(Result b);
    Result& operator<<=(Result b);
    Result& operator|=(Result b);
    Result& operator&=(Result b);
    Result& operator^=(Result b);
    Result& operator%=(Result b);

    Result& operator~();

    friend bool operator==(const Result& a, const Result& b);

    bool IsZero() const;
    bool IsNegative() const;
    bool IsPositive() const { return !(IsZero() || IsNegative()); }

    //
    // Apply built-in math functions: abs, sin, exp, etc...
    //

    // Unary function - all that's needed is the name.
    void ApplyFunction(const std::string& fname);

    // Binary function - takes the second arg.
    void ApplyFunction(const std::string& fname, const Result& arg2);

    std::optional<uint32_t>     u32;
    std::optional<int32_t>      i32;
    std::optional<uint64_t>     u64;
    std::optional<double>       real;
    std::optional<cBigNumber>   big;
};

inline std::ostream& operator<<(std::ostream& s, const Result& r) {
    s << r.ToString();
    return s;
}

namespace ast {

//
// These structures comprise the Abstract Syntax Tree that is built during parsing.
//

// The base node. Provides the public function Compute().
struct Node {
    virtual ~Node() = default;

    // Returns the result of computation performed on the entire AST.
    Result Compute(std::vector<int>& indent_stack, int indent) const;

protected:
    virtual Result DoCompute(std::vector<int>& indent_stack, int indent) const = 0;
    virtual std::string Print() const = 0;
};

// Terminal: represents a single terminal such as a number or a symbolic constant.
struct Terminal : Node {
    Terminal(Result value) : value(std::move(value)) {}

    const Result value;

protected:
    Result DoCompute(std::vector<int>& indent_stack, int indent) const override;
    std::string Print() const override { return "Terminal: " + value.ToString(); };
};

// BinaryOp: represents a binary operator such as "*" or "<<".
struct BinaryOp : Node {
    BinaryOp(detail::Operator op,
             std::unique_ptr<Node> left_ast,
             std::unique_ptr<Node> right_ast)
        : op(op), left_ast(std::move(left_ast)), right_ast(std::move(right_ast)) {}

    const detail::Operator op;
    const std::unique_ptr<Node> left_ast, right_ast;

protected:
    Result DoCompute(std::vector<int>& indent_stack, int indent) const override;
    std::string Print() const override { return "BinaryOp: " + ToString(op); };
};

// UnaryUp: represents a unary operator such as "-".
struct UnaryOp : Node {
    UnaryOp(detail::Operator op, std::unique_ptr<Node> arg_ast)
        : op(op), arg_ast(std::move(arg_ast)) {}

    const detail::Operator op;
    const std::unique_ptr<Node> arg_ast;

protected:
    Result DoCompute(std::vector<int>& indent_stack, int indent) const override;
    std::string Print() const override { return "UnaryOp: " + ToString(op); };
};

// Function: represents a unary/binary function such as "sin", "abs", etc.
struct Function : Node {
    Function(Token token, std::list<std::unique_ptr<Node>> args)
        : token(token), args(move(args)) {}

    const Token token;
    const std::list<std::unique_ptr<Node>> args;

protected:
    Result DoCompute(std::vector<int>& indent_stack, int indent) const override;
    std::string Print() const override { return "Function: " + token.value; };
};

}  // namespace ast

// The Parser (and the Scanner) throw this object when the input is invalid.
class Exception : public std::runtime_error {
public:
    Exception(std::string msg) : runtime_error(msg) {}
};

// The main parser interface:
//  - takes a C-style expression
//  - returns the AST
// Errors are reported via C++ exceptions.
std::unique_ptr<ast::Node> Parse(const std::string &input);

// Helper function to call Compute() on the AST and deal with empty input.
inline Result Compute(const std::string& input) {
    auto ast = Parse(input);
    if (ast == nullptr)
        return Result();

    utils::OutputDebugLine("Walking AST for exression: " + input);

    std::vector<int> indent_stack;
    return ast->Compute(indent_stack, 0);
}

}  // namespace parser
