//===-----------------------------------------------====//
// May 29 2021
// Original:
// https://github.com/GaZAR-UG/llvm-opt-pass/blob/main/transformation.cpp
// Philipp Schubert
//
//    Copyright (c) 2021
//    GaZAR UG (haftungsbeschränkt)
//    Bielefeld, Germany
//    philipp@gazar.eu
//
//===-----------------------------------------------====//

#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassInstrumentation.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

namespace {

//===------------------------------------------===//
/// LLVM Module Analysis Pass
/// CallSiteFinderAnalysis retrieves all call sites at which direct calls
/// to the "void foo()" function are found.
///

class CallSiteFinderAnalysis
    : public llvm::AnalysisInfoMixin<CallSiteFinderAnalysis> {
public:
  explicit CallSiteFinderAnalysis() = default;
  ~CallSiteFinderAnalysis() = default;

  // Unique key to AnalysisInfoMixin
  static inline llvm::AnalysisKey Key;
  friend llvm::AnalysisInfoMixin<CallSiteFinderAnalysis>;

  // Result type of this analysis pass
  using Result = llvm::SetVector<llvm::CallBase *>;

  Result run(llvm::Module &M,
             [[maybe_unused]] llvm::ModuleAnalysisManager &MAM) {
    // The demangled(!) function name that we wish to find
    const static llvm::StringRef TargeFunName = "foo()";
    Result TargetCallSites;

    llvm::outs() << "running code analysis...\n";
    for (auto &F : M) {
      for (auto &BB : F) {
        for (auto &I : BB) {
          if (auto *CB = llvm::dyn_cast<llvm::CallBase>(&I)) {
            // Only find direct function calls
            if (!CB->isIndirectCall() && CB->getCalledFunction() &&
                llvm::demangle(CB->getCalledFunction()->getName().str()) ==
                    TargeFunName) {
              llvm::outs() << "found a direct call to '" << TargeFunName
                           << "'!\n";
              TargetCallSites.insert(CB);
            }
          }
        }
      }
    }
    return TargetCallSites;
  }
};

//==-----------------------------------------------===/
/// LLVM module transformation pass.
/// CallSiteReplacer queries the analysis pass in the above and replaces
/// all direct calls to the "void foo()" that have been found by the
/// CallSiteFinderAnalysis with calls to "void bar(int)".As a parameter
/// to the "void bar(int)" function it provides a counter variable that
/// counts the number of replacements that took.
///

class CallSiteReplacer : public llvm::PassInfoMixin<CallSiteReplacer> {
public:
  explicit CallSiteReplacer() = default;
  ~CallSiteReplacer() = default;

  // Transform the bitcode/IR in the given LLVM module
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM) {
    // Request the results of CallSiteFinderAnalysis pass.
    // If the results are not yet available, because no
    // other pass requested them until now,
    // they will be computed on-the-fly.

    auto TargetCallSites = MAM.getResult<CallSiteFinderAnalysis>(M);
    // The name of the function that we wish to call instead

    const static llvm::StringRef ReplacementFunName = "_Z3bari";
    auto *ReplacementFun = M.getFunction(ReplacementFunName);
    static unsigned ReplacementCounter = 1;

    llvm::outs() << "applying code transformation...\n";
    for (auto *TargetCallSite : TargetCallSites) {
      // Create an LLVM constant int from our replacement counter.

      auto *ConstInt = llvm::ConstantInt::get(
          llvm::IntegerType::get(M.getContext(), 32 /*bits*/),
          ReplacementCounter);

      // Construct the new call site.
      auto *NewCallSite = llvm::CallInst::Create(
          llvm::FunctionCallee(ReplacementFun), {ConstInt});
      // Replace the target call site with new call sites.
      llvm::ReplaceInstWithInst(TargetCallSite, NewCallSite);
      ++ReplacementCounter;
    }
    // We should invalidate only the part we modified but instead we
    // invalidate the result of all other analysis passes.
    return llvm::PreservedAnalyses::none();
  }
};
} // end anonymous namespace

// The above code can be compiled into a .so library which
// can then be used as a plugin for LLVM's optimizer 'opt'.
// But instead we will do everything by ourselves.

int main(int argc, char **argv) {
  if (argc != 2) {
    llvm::outs() << "usage: <prog> <IR file>\n";
    return 1;
  }

  // Parse an LLVM IR file.
  llvm::SMDiagnostic Diag;
  llvm::LLVMContext CTX;
  std::unique_ptr<llvm::Module> M = llvm::parseIRFile(argv[1], Diag, CTX);

  // Check for validity of module.
  bool BrokenDbgInfo = false;
  if (llvm::verifyModule(*M, &llvm::errs(), &BrokenDbgInfo)) {
    llvm::errs() << "error: invalid module\n";
    return 1;
  }
  if (BrokenDbgInfo) {
    llvm::errs() << "caution: debug info is broken\n";
  }

  llvm::PassBuilder PB;
  llvm::ModuleAnalysisManager MAM;
  llvm::ModulePassManager MPM;
  CallSiteFinderAnalysis CSF;

  // Register our analysis pass.
  MAM.registerPass([&]() { return std::move(CSF); });
  PB.registerModuleAnalyses(MAM);
  // Add our transformation pass.
  MPM.addPass(CallSiteReplacer());
  // Make sure that the passes did not modified the original module.
  MPM.addPass(llvm::VerifierPass());

  // Run our transformation pass.
  MPM.run(*M, MAM);
  llvm::outs() << "the tranformed program:\n"
             << "-------------\n";
  llvm::outs() << *M;
  return 0;
}