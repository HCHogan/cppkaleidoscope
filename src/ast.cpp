#include "ast.hpp"

NumberExprAST::NumberExprAST(double Val) : Val(Val) {}

VariableExprAST::VariableExprAST(const std::string &Name) : Name(Name) {}

BinaryExprAST::BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                             std::unique_ptr<ExprAST> RHS)
    : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

CallExprAST::CallExprAST(const std::string &Callee,
                         std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(Callee), Args(std::move(Args)) {}

PrototypeAST::PrototypeAST(const std::string &Name, std::vector<std::string> Args)
    : Name(Name), Args(std::move(Args)) {}

const std::string &PrototypeAST::getName() const { return Name; }

FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                         std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}
