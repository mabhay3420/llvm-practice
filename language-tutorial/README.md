# My First Language Frontend with LLVM Tutorial
Original : [Here](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html)

Build And Run:
```bash
  $ clang++ -g -O3 BuildAST.cpp `llvm-config --cxxflags` -o BuildAST.out
  $ ./BuildAST.out
  ready> def foo(x y) x+foo(y, 4.0);
  Parsed a function definition.
  ready> def foo(x y) x+y y;
  Parsed a function definition.
  Parsed a top-level expr
  ready> def foo(x y) x+y );
  Parsed a function definition.
  Error: unknown token when expecting an expression
  ready> extern sin(a);
  ready> Parsed an extern
  ready> ^D
  $
```