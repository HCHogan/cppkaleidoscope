#include <map>
#include <llvm/Support/Error.h>
void MainLoop();

int getNextToken();

extern std::map<char, int> BinopPrecedence;

extern llvm::ExitOnError ExitOnErr;
