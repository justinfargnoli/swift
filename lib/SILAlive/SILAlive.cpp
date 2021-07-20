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

#include <iostream>

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
    llvm_unreachable("Not reachable, all cases handled");
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

IR::Function *aliveIRGen(llvm::Function &F, const llvm::DataLayout &dataLayout, 
      llvm::Triple triple) {
  // FIXME: There may be a problem with calling `llvm_util::initializer` more 
  // than once during program execution
  //
  // Initialize llvm_util
  llvm_util::initializer llvm_util_init(std::cerr, dataLayout);

  return llvm_util::llvm2alive(F, 
      llvm::TargetLibraryInfo{llvm::TargetLibraryInfoImpl{triple}}, 
      std::vector<std::string_view>()).getPointer();
}

bool SILAliveLLVM(SILModule *M) {
  std::unique_ptr<SILModule> SILMod(M);

  // Clone the SIL Module
  assert(SILMod->getStage() != SILStage::Lowered &&
         "SILAliveLLVM doesn't support SILStage::Lowered");
  auto SILModClone = cloneModule(SILMod.get());

  // Generate LLVM IR for the SILModule
  GeneratedModule generatedModule = genIR(std::move(SILModClone));
  llvm::Module *IRMod = generatedModule.getModule();
  assert(IRMod && "IR module generation failed.");

  // The version of this tool that translated SIL to Alive IR directly 
  // will be a \c SILFunctionTransform instead of a \c SILModuleTransform.
  // So, for simplicity, only consider the first function.
  auto begin = IRMod->begin();
  assert(begin != IRMod->end() && 
      "The must be at least one function in the IR module.");
  llvm::Function *functionLLVM = &*begin;

  // Lower LLVM IR to Alive IR
  auto functionAlive = aliveIRGen(*functionLLVM, IRMod->getDataLayout(),
      generatedModule.getTargetMachine()->getTargetTriple());
  assert(functionAlive && "AliveIRGen failed");

  // Store Alive IR in ASTContext
  // TODO

  return true; 
}
