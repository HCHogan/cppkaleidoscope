// The Kaleidoscope compiler
#include "KaleidoscopeJIT.hpp"
#include "codegen.hpp"
#include "parser.hpp"
#include <print>

int main() {
  auto TargetTriple = LLVMGetDefaultTargetTriple();
  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargets();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllAsmParsers();
  LLVMInitializeAllAsmPrinters();
}
