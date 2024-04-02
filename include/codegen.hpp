#include <memory>
#include <llvm/IR/Module.h>
#include "KaleidoscopeJIT.hpp"

void InitializeModuleAndManagers();

extern std::unique_ptr<llvm::Module> TheModule;

extern std::unique_ptr<KaleidoscopeJIT> TheJIT;

extern std::unique_ptr<LLVMContext> TheContext;
