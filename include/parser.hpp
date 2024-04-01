#include <map>
void MainLoop();

int getNextToken();

extern std::map<char, int> BinopPrecedence;
