#include "codegen.hpp"
#include "KaleidoscopeJIT.hpp"
#include "ast.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

using namespace llvm;

std::unique_ptr<LLVMContext> TheContext;
// TheContext is an opaque object that owns a lot of core LLVM data structures,
// such as the type and constant value tables. We don’t need to understand it in
// detail, we just need a single instance to pass into APIs that require it.

static std::unique_ptr<IRBuilder<>> Builder;
// The Builder object is a helper object that makes it easy to generate LLVM
// instructions. Instances of the IRBuilder class template keep track of the
// current place to insert instructions and has methods to create new
// instructions.

std::unique_ptr<Module> TheModule;
// TheModule is an LLVM construct that contains functions and global variables.
// In many ways, it is the top-level structure that the LLVM IR uses to contain
// code. It will own the memory for all of the IR that we generate, which is why
// the codegen() method returns a raw Value*, rather than a unique_ptr<Value>.

static std::map<std::string, Value *> NamedValues;
// The NamedValues map keeps track of which values are defined in the current
// scope and what their LLVM representation is. (In other words, it is a symbol
// table for the code). In this form of Kaleidoscope, the only things that can
// be referenced are function parameters. As such, function parameters will be
// in this map when generating code for their function body.

std::unique_ptr<KaleidoscopeJIT> TheJIT;
std::unique_ptr<FunctionPassManager> TheFPM;
std::unique_ptr<LoopAnalysisManager> TheLAM;
std::unique_ptr<FunctionAnalysisManager> TheFAM;
std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
std::unique_ptr<ModuleAnalysisManager> TheMAM;
std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
std::unique_ptr<StandardInstrumentations> TheSI;

// The JIT will happily resolve function calls across module boundaries, as long
// as each of the functions called has a prototype, and is added to the JIT
// before it is called. So we only need to store the prototypes for functions
std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

void InitializeModuleAndManagers() {
  // Open a new context and module.
  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("hank's cool jit", *TheContext);
  TheModule->setDataLayout(TheJIT->getDataLayout());

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);

  // Create new pass and analysis managers.
  TheFPM = std::make_unique<FunctionPassManager>();
  TheLAM = std::make_unique<LoopAnalysisManager>();
  TheFAM = std::make_unique<FunctionAnalysisManager>();
  TheCGAM = std::make_unique<CGSCCAnalysisManager>();
  TheMAM = std::make_unique<ModuleAnalysisManager>();
  ThePIC = std::make_unique<PassInstrumentationCallbacks>();
  TheSI = std::make_unique<StandardInstrumentations>(*TheContext,
                                                     /*DebugLogging*/ true);
  TheSI->registerCallbacks(*ThePIC, TheMAM.get());

  // Add transform passes.
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->addPass(InstCombinePass());
  // Reassociate expressions.
  TheFPM->addPass(ReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->addPass(GVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->addPass(SimplifyCFGPass());

  PassBuilder PB;
  PB.registerModuleAnalyses(*TheMAM);
  PB.registerFunctionAnalyses(*TheFAM);
  PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM);
}

Value *LogErrorV(const char *Str) {
  LogError(Str);
  return nullptr;
}

Value *NumberExprAST::codegen() {
  return ConstantFP::get(*TheContext, APFloat(Val));
}

Value *VariableExprAST::codegen() {
  Value *V = NamedValues[Name];
  if (!V)
    return LogErrorV("unknown variable name");
  return V;
}

Value *BinaryExprAST::codegen() {
  Value *L = LHS->codegen();
  Value *R = RHS->codegen();
  if (!L || !R)
    return nullptr;

  switch (Op) {
  case '+':
    return Builder->CreateFAdd(L, R, "addtmp");
  case '-':
    return Builder->CreateFSub(L, R, "subtmp");
  case '*':
    return Builder->CreateFMul(L, R, "multmp");
  case '<':
    L = Builder->CreateFCmpULT(L, R, "cmptmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}

/// LLVM uses the native C calling conventions by default, allowing these calls
/// to also call into standard library functions like “sin” and “cos”, with no
/// additional effort.
Value *CallExprAST::codegen() {
  Function *CalleeF = getFunction(Callee);
  if (!CalleeF)
    return LogErrorV("Unknown function referenced");
  if (CalleeF->arg_size() != Args.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    ArgsV.push_back(Args[i]->codegen());
    if (!ArgsV.back())
      return nullptr;
  }

  return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

// All basic blocks should be terminated with a control flow instruction.
Value *IfExprAST::codegen() {
  Value *CondV = Cond->codegen();
  if (!CondV)
    return nullptr;

  CondV = Builder->CreateFCmpONE(
      CondV, ConstantFP::get(*TheContext, APFloat(0.0)), "ifcond");
  // get the parent of current working block(the current function)
  Function *TheFunction = Builder->GetInsertBlock()->getParent();

  // Create block for then and else
  // Note that it passes “TheFunction” into the constructor for the “then”
  // block. This causes the constructor to automatically insert the new block
  // into the end of the specified function. The other two blocks are created,
  // but aren’t yet inserted into the function.
  BasicBlock *ThenBB = BasicBlock::Create(*TheContext, "then", TheFunction);
  BasicBlock *ElseBB = BasicBlock::Create(*TheContext, "else");
  BasicBlock *MergeBB = BasicBlock::Create(*TheContext, "ifcont");

  Builder->CreateCondBr(CondV, ThenBB, ElseBB);

  // Emit then value.
  Builder->SetInsertPoint(ThenBB);

  Value *ThenV = Then->codegen();
  if (!ThenV)
    return nullptr;

  // To finish off the “then” block, we create an unconditional branch to the
  // merge block.
  Builder->CreateBr(MergeBB);
  // Codegen of 'Then' can change the notion of 'current block', update ThenBB
  // for the PHI.
  ThenBB = Builder->GetInsertBlock();
  TheFunction->insert(TheFunction->end(), ElseBB);
  Builder->SetInsertPoint(ElseBB);
  Value *ElseV = Else->codegen();
  if (!ElseV)
    return nullptr;
  Builder->CreateBr(MergeBB);
  // Codegen of 'Else' can change the current block, same.
  ElseBB = Builder->GetInsertBlock();

  // Emit the Merge block
  TheFunction->insert(TheFunction->end(), MergeBB);
  Builder->SetInsertPoint(MergeBB);
  PHINode *PN = Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2, "iftmp");

  PN->addIncoming(ThenV, ThenBB);
  PN->addIncoming(ElseV, ElseBB);
  // Finally, the CodeGen function returns the phi node as the value computed by
  // the if/then/else expression.
  return PN;
}

/// This code packs a lot of power into a few lines. Note first that this
/// function returns a “Function*” instead of a “Value*”. Because a “prototype”
/// really talks about the external interface for a function (not the value
/// computed by an expression), it makes sense for it to return the LLVM
/// Function it corresponds to when codegen’d.
Function *PrototypeAST::codegen() {
  std::vector<Type *> Doubles(
      Args.size(),
      Type::getDoubleTy(*TheContext)); // types of function arguments
  FunctionType *FT = FunctionType::get(
      Type::getDoubleTy(*TheContext), Doubles,
      false); // return type + argument type + not vararg(false) = function type
  Function *F =
      Function::Create(FT, Function::ExternalLinkage, Name,
                       TheModule.get()); // funtiontype + name + which module +
                                         // link property = function
  unsigned Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(Args[Idx++]); // set name of arguments
  return F;
}

Function *FunctionAST::codegen() {
  auto &P = *Proto;
  FunctionProtos[Proto->getName()] = std::move(Proto);
  Function *TheFunction = getFunction(P.getName());

  if (!TheFunction)
    return nullptr; // if there was an error creating the function, return
                    // nullptr (why?)
  if (!TheFunction->empty())
    return (Function *)LogErrorV("Function cannot be redefined.");

  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
  // new instructions should be inserted into the end of the new basic block.
  Builder->SetInsertPoint(BB);

  // Record the function arguments in the NamedValues map.
  NamedValues.clear();
  for (auto &Arg : TheFunction->args())
    NamedValues[std::string(Arg.getName())] = &Arg;
  if (Value *RetVal = Body->codegen()) {
    // Finish off the function.
    Builder->CreateRet(RetVal);

    // Validate the generated code, checking for consistency.
    verifyFunction(*TheFunction);

    // opt
    TheFPM->run(*TheFunction, *TheFAM);
    return TheFunction;
  }

  // Error reading body, remove function.
  TheFunction->eraseFromParent();
  return nullptr;
}

Function *getFunction(std::string Name) {
  // First, see if the function has already been added to the current module.
  if (auto *F = TheModule->getFunction(Name))
    return F;

  // If not, check whether we can codegen the declaration from some existing
  // prototype.
  auto FI = FunctionProtos.find(Name);
  if (FI != FunctionProtos.end())
    return FI->second->codegen();

  // If no existing prototype exists, return null.
  return nullptr;
}
