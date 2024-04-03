#include <memory>
#include <llvm/IR/Module.h>
#include "KaleidoscopeJIT.hpp"
#include "ast.hpp"

void InitializeModuleAndManagers();

extern std::unique_ptr<llvm::Module> TheModule;

extern std::unique_ptr<KaleidoscopeJIT> TheJIT;

extern std::unique_ptr<LLVMContext> TheContext;

Function *getFunction(std::string Name);

extern std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
