## TL;DR
This is a small [bc-style](https://www.gnu.org/software/bc/) calculator written in portable, modern C++. It takes a C-style expression with the following operators: plus, minus, multiple, divide, bit shifts, and/or/xor and pow.

```(1<<31) + 1024```

## Details
The code parses a C-style expression using a hand-written [recursive descent parser](https://en.wikipedia.org/wiki/Recursive_descent_parser) and builds and the [AST](https://en.wikipedia.org/wiki/Abstract_syntax_tree). The AST is then walked to compute the expression's result which is presented in a GUI in a human-readable form. Specifically, several result forms are computed at once, in order to model the abstract C machine: 32-bin integer (signed and unsigned), 64-bit integer, big number and a floating-point quantity (double).

## Dependencies
The following libraries are configured as Git sub-modules and built during compilation:
* [nana](https://github.com/cnjinhao/nana)
  * Implements minimal UI functionality in modern C++
  * Builds on Windows and Linux
* [glog](https://github.com/google/glog)
  * Implements logging and assertions

## The build process
First get the code:
* ```git clone https://github.com/os12/calc.git```
* ```cd calc```
* ```git submodule update```

The application currently builds on Windows. Get Visual Studio 2017 “Community” edition from [here](https://www.visualstudio.com/), open `calc.sln` and build it. 

Nana support both Visual Studio solutions as well as CMake, yet I need to see whether that later works on Windows. If so, creating ```CMakeLists.txt``` for the calculator code should be trivial.

## Screen shot
![Screen shot](https://github.com/os12/calc/raw/master/docs/calc.png)
