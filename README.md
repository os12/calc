# TL;DR
This is a small [bc](https://www.gnu.org/software/bc/)-style calculator written in portable, modern C++. It takes a C-style expression with the following operators: plus, minus, multiple, divide, bit shifts, and/or/xor as well as a few built-in functions: sin/cos/abs/log/pow.

```(1 << 31) + 1024 | 0x0001```

```log2(1000+20+4)```

```cos(rad(90))```
## Screen shot
![Screen shot](https://github.com/os12/calc/raw/master/docs/calc.png)

# Details
The code parses C-style expressions using a hand-written [recursive descent parser](https://en.wikipedia.org/wiki/Recursive_descent_parser) and builds the [AST](https://en.wikipedia.org/wiki/Abstract_syntax_tree). The AST is then walked to compute the result which is presented in a human-readable form. Specifically, several result forms are computed at once, in order to model the abstract C machine: 32-bin integer (presented as signed and unsigned), 64-bit integer, big number and a floating-point quantity (double).

## Implementation overview
#### The grammar

Here is the language grammar in something very close to the [BNF](https://en.wikipedia.org/wiki/Backus–Naur_form):

```
<input>       ::= expression EOF
<expression>  ::= term [ binop term ]
<binop>       ::= MINUS | PLUS | MULT | DIV | LSHIFT | RSHIFT | POW | AND | OR | XOR
<term>        ::= INT
              | MINUS <term>
              | NOT <term>
              | LPAREN <expression> RPAREN
              | FUNCTION LPAREN <args> RPAREN
              | <constatnt>
<constant>    ::= PI
<args>        := <expression> [ COMA <args> ]
```

#### Operators
The code parses C-style expressions using the aforementioned grammar. The `<expression>` rule allows for an arbitrary long string of terms connected with binary operators, that must be put in order. That is done by building what is known as an [operator precedence parser](https://en.wikipedia.org/wiki/Operator-precedence_parser) which puts the operators in the following order:
* (lowest) Or    
* Xor   
* And   
*    LShift
*    RShift
*    BMinus
*    Plus  
*    Mult  
*   Div   
*    Pow   
*   UMinus
* (highest) Not   

#### The AST
The abstract syntax tree is built out of the following four node types:
* `Terminal` - represents a single terminal such as a number or a symbolic constant.
* `BinaryOp` - represents a binary operator such as "*" or "<<".
* `UnaryOp` - represents a unary operator such as "-".
* `Function` - represents a unary/binary function such as "sin", "abs", etc.

Here are a few examples taken directly from the calculator's debug output. Note that binary operators must be executed in a left-associative fashion.

<table>
<tr>
<th>Debug output</th><th>AST in graphical form</th>
</tr>
<tr><td colspan="2">Walking AST for exression: <b>1 + 2**3</b></td></tr>
<tr>
<td>
<pre>
BinaryOp: Plus
	Terminal: 1
	BinaryOp: Pow
		Terminal: 2
		Terminal: 3
</pre>
</td>
<td><img src="https://github.com/os12/calc/raw/master/docs/expr1.png" alt="expr1 graph"></td>
</tr>
<tr><td colspan="2">Walking AST for exression: <b>10-2-3</b></td></tr>
<tr>
<td>
<pre>
BinaryOp: BMinus
	BinaryOp: BMinus
		Terminal: 10
		Terminal: 2
	Terminal: 3
</pre>
</td><td><img src="https://github.com/os12/calc/raw/master/docs/expr2.png" alt="expr2 graph"></td>
</tr>
<tr><td colspan="2">Walking AST for exression: <b>1--1</b></td></tr>
<tr>
<td>
<pre>
    BinaryOp: BMinus
    	Terminal: 1
    	UnaryOp: UMinus
    		Terminal: 1
</pre></td>
<td><img src="https://github.com/os12/calc/raw/master/docs/expr3.png" alt="expr3 graph"></td></tr>
<tr><td colspan="2">Walking AST for exression: <b>1*(2+3*4)</b></td></tr>
<tr>
<td>
<pre>
BinaryOp: Mult
	Terminal: 1
	BinaryOp: Plus
		Terminal: 2
		BinaryOp: Mult
			Terminal: 3
			Terminal: 4
</pre>
</td>
<td><img src="https://github.com/os12/calc/raw/master/docs/expr4.png" alt="expr4 graph"></td></tr>
</tr>
</table>

#### The scanner
The scanner is a templated class that operates on a STL-style range defined by two iterators. It produces the following token types on demand:
* `Int` a variable-sized integer accepted in various C-style formats
* Parentheses: `LParen`, `RParen`
* Arithmetic ops, mostly binary: `Minus`, `Plus`, `Mult`, `Div`
* `Coma`
* Bitwise ops, mostly binary: `Not`, `Or`, `And`, `Xor`, `LShift`, `RShift`
* Built-in algebraic and trigonomic functions: `Function`
* Exponent op (It also exists as a binary function "pow"): `Pow`: `**`
* Built-in constants: `Pi`
* End of file: `EoF`

## Dependencies
The following libraries are configured as Git sub-modules and built during compilation:
* [nana](https://github.com/cnjinhao/nana)
  * Implements minimal UI functionality in modern C++
  * Builds on Windows and Linux
* [glog](https://github.com/google/glog)
  * Implements logging and assertions

Also, the code includes a [big number](http://www.imach.uran.ru/cbignum) library which implements arbitrary-precision integers and operations on them. So, things like these compute: ```10**100``` and the result is presented in the "big" box in the UI.

## The build process
First get the code:
* ```git clone https://github.com/os12/calc.git```
* ```cd calc```
* ```git submodule update```

The application currently builds on Windows. Get Visual Studio 2017 “Community” edition from [here](https://www.visualstudio.com/), open `calc.sln` and build it. 

Nana supports both Visual Studio solutions as well as CMake, yet I need to see whether that later works on Windows. If so, creating ```CMakeLists.txt``` for the calculator code should be trivial.

