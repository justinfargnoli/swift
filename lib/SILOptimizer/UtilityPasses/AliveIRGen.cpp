//===--- AliveIRGen.cpp --------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "swift/AST/IRGenOptions.h"
#include "swift/AST/IRGenRequests.h"
#include "swift/Basic/PrimarySpecificPaths.h"
#include "swift/SIL/SILModule.h"
#include "swift/SILOptimizer/PassManager/Passes.h"
#include "swift/SILOptimizer/PassManager/Transforms.h"
#include "swift/Subsystems.h"
#include "swift/TBDGen/TBDGen.h"

using namespace swift;

namespace {

class AliveIRGen : public SILModuleTransform {
  llvm::Module *genIR(std::unique_ptr<SILModule> SILMod) {
    TBDGenOptions TBDGenOpts{};
    IRGenOptions IRGenOpts{};
    PrimarySpecificPaths primarySpecificPaths{};
    llvm::GlobalVariable *HashGlobal;

    GeneratedModule generatedModule = performIRGeneration(
        SILMod->getSwiftModule(), IRGenOpts, TBDGenOpts, std::move(SILMod),
        /*ModuleName=*/ "", primarySpecificPaths,
        /*parallelOutputFilenames=*/ {}, &HashGlobal);

    // TODO: Remove
    llvm::errs() << "\n- BEFORE performIRGeneration dump\n";
    generatedModule.getModule()->dump();
    llvm::errs() << "\n- AFTER performIRGeneration dump\n";

    return generatedModule.getModule();
  }

  void run() override {
    std::unique_ptr<SILModule> SILMod(getModule());

    switch (SILMod->getStage()) {
    case SILStage::Raw:
      assert(!runSILDiagnosticPasses(*SILMod.get()));
      SWIFT_FALLTHROUGH;
    case SILStage::Canonical:
      runSILLoweringPasses(*SILMod.get());
      break;
    case SILStage::Lowered:
      break;
    }

    // Generate LLLVM IR for the SILModule
    llvm::Module *IRMod = genIR(std::move(SILMod));
    assert(IRMod && "IR module generation failed.");

    // TODO: Remove
    llvm::errs() << "\n- BEFORE dump\n";
    IRMod->dump(); // TODO remove
    llvm::errs() << "\n- AFTER dump\n";

    // The version of this tool that translated SIL to Alive IR directly 
    // will be a \c SILFunctionTransform instead of a \c SILModuleTransform.
    // So, for simplicity, only consider the first function.
    llvm::Function *function = IRMod->getFunction("main");

    // Lower LLVM IR to Alive IR
    // TODO

    // Store Alive IR in ASTContext
    // TODO
  }
};

} // end anonymous namespace

//===----------------------------------------------------------------------===//
//                           Top Level Entry Point
//===----------------------------------------------------------------------===//

SILTransform *swift::createAliveIRGen() {
  return new AliveIRGen();
}
