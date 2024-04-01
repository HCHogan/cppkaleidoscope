#include <print>
#include "parser.hpp"
#include "codegen.hpp"

int main() {
  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40; // highest.

  // Prime the first token.
  fprintf(stderr, "ready> ");
  getNextToken();

  // initialize codegen
  InitializeModule();

  // Run the main "interpreter loop" now.
  MainLoop();

  TheModule->print(llvm::errs(), nullptr);

  return 0;
}
