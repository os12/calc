## TL;DR
This is a small [bc-style](https://www.gnu.org/software/bc/) calculator written in portable, modern C++. It takes a C-style expression with the following operators: plus, minus, multiple, divide, bit shifts, and/or/xor as well as a few built-in functions: sin/cos/abs/log/pow.

```(1 << 31) + 1024 | 0x0001```

```log2(1000+20+4)```

```cos(rad(90))```

## Details
The code parses C-style expressions using a hand-written [recursive descent parser](https://en.wikipedia.org/wiki/Recursive_descent_parser) and builds and the [AST](https://en.wikipedia.org/wiki/Abstract_syntax_tree). The AST is then walked to compute the result which is presented in UI in a human-readable form. Specifically, several result forms are computed at once, in order to model the abstract C machine: 32-bin integer (presented as signed and unsigned), 64-bit integer, big number and a floating-point quantity (double).

## Dependencies
The following libraries are configured as Git sub-modules and built during compilation:
* [nana](https://github.com/cnjinhao/nana)
  * Implements minimal UI functionality in modern C++
  * Builds on Windows and Linux
* [glog](https://github.com/google/glog)
  * Implements logging and assertions

Also, the code includes a [big number](http://www.imach.uran.ru/cbignum) library which implements arbitrary precision integers. So, things like these compute: ```10**100``` and the result is presented in the "big" box in the UI.

## The build process
First get the code:
* ```git clone https://github.com/os12/calc.git```
* ```cd calc```
* ```git submodule update```

The application currently builds on Windows. Get Visual Studio 2017 “Community” edition from [here](https://www.visualstudio.com/), open `calc.sln` and build it. 

Nana support both Visual Studio solutions as well as CMake, yet I need to see whether that later works on Windows. If so, creating ```CMakeLists.txt``` for the calculator code should be trivial.

## Screen shot
![Screen shot](https://github.com/os12/calc/raw/master/docs/calc.png)
