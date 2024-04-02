#include <print>
#include "parser.hpp"
#include "KaleidoscopeJIT.hpp"
#include "codegen.hpp"

int main() {
  LLVMInitializeNativeTarget();
  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40; // highest.

  // Prime the first token.
  fprintf(stderr, "ready> ");
  getNextToken();

  TheJIT = std::make_unique<KaleidoscopeJIT>();

  // initialize codegen
  InitializeModuleAndManagers();

  // Run the main "interpreter loop" now.
  MainLoop();

  TheModule->print(llvm::errs(), nullptr);

  return 0;
}
