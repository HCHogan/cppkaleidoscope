// The Kaleidoscope compiler
#include "KaleidoscopeJIT.hpp"
#include "codegen.hpp"
#include "parser.hpp"
#include "llvm-c/Target.h"
#include <print>

int main() {
  auto TargetTriple = LLVMGetDefaultTargetTriple();
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmParser();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeDisassembler();
}
