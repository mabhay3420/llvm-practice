#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// if invalid token lexer returns [0-255]
// otherwise one of following
enum Token {
  tok_eof = -1,

  // commands
  tok_def = -2,
  tok_extern = -3,

  // primary

  tok_identifier = -4,
  tok_number = -5,
};

static std::string IdentifierStr; // For tok_identifier
static double NumVal;             // For tok_number

/// gettok - Return the next token from standard input.

static int gettok() {
  static int LastChar = ' ';

  // Skip any whitespace
  while (isspace(LastChar))
    LastChar = getchar();

  // identifier: [a-zA-Z][a-zA-z0-9]*
  if (isalpha(LastChar)) {
    IdentifierStr = LastChar;
    while (isalnum(LastChar = getchar())) {
      IdentifierStr += LastChar;
    }

    if (IdentifierStr == "def")
      return tok_def;
    if (IdentifierStr == "extern")
      return tok_extern;
    return tok_identifier;
  }
  // numerical: [0-9.]+
  if (isdigit(LastChar) || LastChar == '.') {
    std::string NumStr;

    // No error handling : 1.23.45 -> 1.23
    do {
      NumStr += LastChar;
      LastChar = getchar();
    } while (isdigit(LastChar) || LastChar == '.');
    NumVal = strtod(NumStr.c_str(), 0);
    return tok_number;
  }
  if (LastChar == '#') {
    // Comment until end of line
    do
      LastChar = getchar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
    if (LastChar != EOF)
      return gettok();
  }

  if (LastChar == EOF)
    return tok_eof;

  // otherwise return ascii value of char
  int ThisChar = LastChar;
  // for next gettok call
  LastChar = getchar();
  return ThisChar;
}
/** The Abstract Syntax Tree
 * One object for each construct(Node) in the language
 */

/// ExprAST - Base Class for all expression Nodes

class ExprAST {
public:
  virtual ~ExprAST() {}
};

/// NumberExprAST - Expression class for numerical literals like "1.0"
class NumberExprAST : public ExprAST {
  double Val;

public:
  NumberExprAST(double Val) : Val(Val) {}
};

/// VariableExprAST- Expression class for referencing a varible like 'a'
class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
};

/// BinaryExprAST- Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
      : Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

/// CallExprAST - Expression class for function calls.

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
      : Callee(Callee), Args(std::move(Args)) {}
};

/// PrototypeAST - Represent a "prototype" of a function,
/// i.e. Capture its name,argument names(so number of arguments)

class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;

public:
  PrototypeAST(const std::string &name, std::vector<std::string> Args)
      : Name(name), Args(std::move(Args)) {}

  const std::string &getName() const { return Name; }
};

/// FunctionAST - This class represents a function definition

class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body)
      : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

/** Parsing Basics
 * Convert list of tokens to AST
 */

/// CurTok - Token parser is looking at
/// getNextToken reads another token

static int CurTok;
static int getNextToken() { return CurTok = gettok(); }

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *Str) {
  fprintf(stderr, "LogError: %s\n", Str);
  return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
  LogError(Str);
  return nullptr;
}

/// Basic Expression Parsing
/// numberexpr ::= number

static std::unique_ptr<ExprAST> ParseExpression();
static std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto Result = std::make_unique<NumberExprAST>(NumVal);
  getNextToken(); // consume the number
  return std::move(Result);
}

/// parenexpr ::= '( expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
  getNextToken(); // eat (.
  auto V = ParseExpression();
  if (!V)
    return nullptr;
  if (CurTok != ')')
    return LogError("expected ')' ");
  getNextToken(); // eat ).
  return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'

static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;

  getNextToken(); // eat identifier

  if (CurTok != '(') // Simple variable ref.
    return std::make_unique<VariableExprAST>(IdName);

  // Call
  getNextToken(); // eat (
  std::vector<std::unique_ptr<ExprAST>> Args;
  if (CurTok != ')') {
    while (1) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;
      // CurTok always contains the expressionn to look at.
      if (CurTok == ')')
        break;

      if (CurTok != ',')
        return LogError("Expected ')' or ',' in argument list");

      getNextToken();
    }
  }

  // Eat the ')'.
  getNextToken();

  return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr

static std::unique_ptr<ExprAST> ParsePrimary() {
  switch (CurTok) {
  default:
    return LogError("unknown token when expecting an expression");
  case tok_identifier:
    return ParseIdentifierExpr();
  case tok_number:
    return ParseNumberExpr();
  case '(':
    return ParseParenExpr();
  }
}

/// BinoPrecedence - This holds the precedence for
/// each binary operator that is defined.

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS);
static std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending
/// binary operator token.
static int GetTokPrecedence() {

  if (!isascii(CurTok))
    return -1;

  // Make sure it's a declared binop
  int TokPrec = BinopPrecedence[CurTok];
  // returns 0 if key does not exist
  if (TokPrec <= 0)
    return -1;
  return TokPrec;
}

/// expression
///   ::= primary binoprhs
static std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParsePrimary();

  if (!LHS)
    return nullptr;

  // 0 is the minimal operator precedence that the function can eat
  return ParseBinOpRHS(0, std::move(LHS));
}

/// binoprhs
///   ::= ( '+' primary)*

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {
  // Assuming the next token is binop,get it's precedence
  while (1) {
    int TokPrec = GetTokPrecedence();

    // Choose only equal or stronger binop
    if (TokPrec < ExprPrec)
      // we are done
      return LHS;

    // Valid Binop
    int BinOp = CurTok;
    getNextToken(); // eat binop

    auto RHS = ParsePrimary();
    if (!RHS)
      return nullptr;

    int NextPrec = GetTokPrecedence();
    // a + b*c = a + (b*c)
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
    }
    // Merge LHS/RHS : Finally we will have no operation
    // left, The Next prec will be 0 and Our Supplied
    // prec will be 1 thus simple merging
    LHS =
        std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
  } // keep merging nodes
}

/// prototype
///   ::= id '(' id* ')'

static std::unique_ptr<PrototypeAST> ParserPrototype() {
  if (CurTok != tok_identifier)
    return LogErrorP("Expected function name in prototype");

  std::string FnName = IdentifierStr;
  getNextToken();
  if (CurTok != '(')
    return LogErrorP("Expected '(' in prototype");

  // Read the list of argument names.
  std::vector<std::string> ArgNames;
  while (getNextToken() == tok_identifier)
    ArgNames.push_back(IdentifierStr);
  if (CurTok != ')')
    return LogErrorP("Expected ')' in prototype");

  getNextToken(); // eat ')' ; reading complete

  return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

/// definition ::= 'def' prototype expression

static std::unique_ptr<FunctionAST> ParseDefinition() {
  getNextToken(); // eat def.
  auto Proto = ParserPrototype();
  if (!Proto)
    return nullptr;

  if (auto E = ParseExpression())
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  return nullptr;
}

/// external ::= 'extern' prototype
/// no body
static std::unique_ptr<PrototypeAST> ParseExtern() {
  getNextToken(); // eat extern;
  return ParserPrototype();
}

/// toplevelexpr ::= expressions
/// a + b
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Anonymous proto : no arguments,no name
    auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  }
  return nullptr;
}

/** Top Level Parsing
 * Handle + MainLoop
 */
static void HandleDefinition() {
  if (ParseDefinition()) {
    fprintf(stderr, "Parsed a function definition.\n");
  } else {
    // invalid token : Skip to the next one.
    getNextToken();
  }
}

static void HandleExtern() {
  if (ParseExtern()) {
    fprintf(stderr, "Parsed an extern.\n");
  } else {
    // Skip token
    getNextToken();
  }
}

static void HandleTopLevelExpression() {
  // Top level expression : Anonymous function
  if (ParseTopLevelExpr()) {
    fprintf(stderr, "Parsed a top-level expr.\n");
  } else {
    // Skip token for error recovery
    getNextToken();
  }
}

/// top ::== definition | external | expression | ';'

static void MainLoop() {
  while (true) {
    fprintf(stderr, "ready> ");
    switch (CurTok) {
    case tok_eof:
      return;
    case ';': // ignore top-level semicolons
      getNextToken();
      break;
    case tok_def:
      HandleDefinition();
      break;
    case tok_extern:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}

int main() {
  // 1 is the lowest precedence allowed.
  // Can be extended
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40; // highest.

  // Prime the first token
  fprintf(stderr, "ready>");
  getNextToken();

  // Run the main "interpreter loop" now.
  MainLoop();
  return 0;
}