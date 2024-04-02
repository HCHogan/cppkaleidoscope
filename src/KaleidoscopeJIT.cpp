#include "KaleidoscopeJIT.hpp"

using namespace llvm;
using namespace llvm::orc;

KaleidoscopeJIT::KaleidoscopeJIT(std::unique_ptr<ExecutionSession> ES,
                                 JITTargetMachineBuilder JTMB, DataLayout DL)
    : ES(std::move(ES)), DL(std::move(DL)), Mangle(*this->ES, this->DL),
      ObjectLayer(*this->ES,
                  []() { return std::make_unique<SectionMemoryManager>(); }),
      CompileLayer(*this->ES, ObjectLayer,
                   std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))),
      MainJD(this->ES->createBareJITDylib("<main>")) {
  MainJD.addGenerator(
      cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
          DL.getGlobalPrefix())));
  if (JTMB.getTargetTriple().isOSBinFormatCOFF()) {
    ObjectLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
    ObjectLayer.setAutoClaimResponsibilityForObjectSymbols(true);
  }
}

KaleidoscopeJIT::~KaleidoscopeJIT() {
  if (auto Err = ES->endSession())
    ES->reportError(std::move(Err));
}

Expected<std::unique_ptr<KaleidoscopeJIT>> KaleidoscopeJIT::Create() {
  auto EPC = SelfExecutorProcessControl::Create();
  if (!EPC)
    return EPC.takeError();

  auto ES = std::make_unique<ExecutionSession>(std::move(*EPC));

  JITTargetMachineBuilder JTMB(
      ES->getExecutorProcessControl().getTargetTriple());

  auto DL = JTMB.getDefaultDataLayoutForTarget();
  if (!DL)
    return DL.takeError();

  return std::make_unique<KaleidoscopeJIT>(std::move(ES), std::move(JTMB),
                                           std::move(*DL));
}

const DataLayout &KaleidoscopeJIT::getDataLayout() const { return DL; }

JITDylib &KaleidoscopeJIT::getMainJITDylib() { return MainJD; }

Error KaleidoscopeJIT::addModule(ThreadSafeModule TSM, ResourceTrackerSP RT) {
  if (!RT)
    RT = MainJD.getDefaultResourceTracker();
  return CompileLayer.add(RT, std::move(TSM));
}

Expected<ExecutorSymbolDef> KaleidoscopeJIT::lookup(StringRef Name) {
  return ES->lookup({&MainJD}, Mangle(Name.str()));
}
