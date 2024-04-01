#include <memory>
#include <llvm/IR/Module.h>

void InitializeModule();

extern std::unique_ptr<llvm::Module> TheModule;
