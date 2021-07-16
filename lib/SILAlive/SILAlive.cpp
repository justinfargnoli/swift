//===--- SILAlive.cpp -------------------------------------------------===//
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

#include "llvm_util/llvm2alive.h"

#include "llvm/Analysis/TargetLibraryInfo.h"

#include "swift/AST/IRGenOptions.h"
#include "swift/AST/IRGenRequests.h"
#include "swift/Basic/PrimarySpecificPaths.h"
#include "swift/SIL/SILCloner.h"
#include "swift/SIL/SILModule.h"
#include "swift/SILAlive/SILAlive.h"
#include "swift/SILOptimizer/PassManager/Passes.h"
#include "swift/Subsystems.h"
#include "swift/TBDGen/TBDGen.h"

using namespace swift;

// Lower the SILModule to the Lowered stage so that it's ready to be 
// lowered to LLVM IR.
void lowerSIL(SILModule &SILMod) {
  switch (SILMod.getStage()) {
  case SILStage::Raw:
    assert(!runSILDiagnosticPasses(SILMod));
    SWIFT_FALLTHROUGH;
  case SILStage::Canonical:
    runSILLoweringPasses(SILMod);
    break;
  case SILStage::Lowered:
    break;
 }
 assert(SILMod.getStage() == SILStage::Lowered && "SILStage must be Lowered");
}

GeneratedModule genIR(std::unique_ptr<SILModule> SILMod) {
  // Lower \p SILMod to SILStage::Lowered
  lowerSIL(*SILMod);

  // Generate LLVM IR for the \p SILMod
  llvm::GlobalVariable *HashGlobal;
  return performIRGeneration(
      SILMod->getSwiftModule(), IRGenOptions{}, TBDGenOptions{}, 
      std::move(SILMod), /*ModuleName=*/ "", PrimarySpecificPaths{},
      /*parallelOutputFilenames=*/ {}, &HashGlobal);
}

IR::Function *aliveIRGen(llvm::Function &F, llvm::Triple triple) {
  return llvm_util::llvm2alive(F, 
      llvm::TargetLibraryInfo{llvm::TargetLibraryInfoImpl{triple}}, 
      std::vector<std::string_view>()).getPointer();
}

bool SILAliveLLVM(SILModule *M) { 
  std::unique_ptr<SILModule> SILMod(M);

  // FIXME: don't allow clone to be run on SILStage::Lowered
  // Clone the SIL Module
  auto SILModClone = cloneModule(SILMod.get());
  // TODO: Remove
  llvm::errs() << "\n- BEFORE SILMod dump\n";
  SILMod->dump(); 
  llvm::errs() << "\n- AFTER SILMod dump\n";
  llvm::errs() << "\n- BEFORE SILModClone dump\n";
  SILModClone->dump(); 
  llvm::errs() << "\n- AFTER SILModClone dump\n";
  // Generate LLVM IR for the SILModule
  GeneratedModule generatedModule = genIR(std::move(SILModClone));
  llvm::Module *IRMod = generatedModule.getModule();
  assert(IRMod && "IR module generation failed.");

  // TODO: Remove
  llvm::errs() << "\n- BEFORE IRMod dump\n";
  IRMod->dump(); 
  llvm::errs() << "\n- AFTER IRMod dump\n";

  // The version of this tool that translated SIL to Alive IR directly 
  // will be a \c SILFunctionTransform instead of a \c SILModuleTransform.
  // So, for simplicity, only consider the first function.
  auto begin = IRMod->begin();
  assert(begin != IRMod->end() && 
      "The must be at least one function in the IR module.");
  llvm::Function *functionLLVM = &*begin;

  // Lower LLVM IR to Alive IR
  auto functionAlive = aliveIRGen(*functionLLVM, 
      generatedModule.getTargetMachine()->getTargetTriple());
  assert(!functionAlive && "AliveIRGen failed");

  // Store Alive IR in ASTContext
  // TODO

  return false; 
}
