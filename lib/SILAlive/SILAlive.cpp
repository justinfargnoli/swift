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
}

GeneratedModule genIR(std::unique_ptr<SILModule> SILMod) {
  // Lower \p SILMod to SILStage::Lowered
  lowerSIL(*SILMod.get());

  // Generate LLVM IR for the \p SILMod
  llvm::GlobalVariable *HashGlobal;
  return performIRGeneration(
      SILMod->getSwiftModule(), IRGenOptions{}, TBDGenOptions{}, 
      std::move(SILMod), /*ModuleName=*/ "", PrimarySpecificPaths{},
      /*parallelOutputFilenames=*/ {}, &HashGlobal);
}

std::optional<IR::Function> aliveIRGen(llvm::Function &F, llvm::Triple triple) {
  return llvm_util::llvm2alive(F, 
      llvm::TargetLibraryInfo{llvm::TargetLibraryInfoImpl{triple}}, 
      std::vector<std::string_view>());
}

bool SILAliveLLVM(SILModule *M) { 
  std::unique_ptr<SILModule> SILMod(M);

  // Generate LLLVM IR for the SILModule
  GeneratedModule generatedModule = genIR(std::move(SILMod));
  llvm::Module *IRMod = generatedModule.getModule();
  assert(IRMod && "IR module generation failed.");

  // TODO: Remove
  llvm::errs() << "\n- BEFORE dump\n";
  IRMod->dump(); 
  llvm::errs() << "\n- AFTER dump\n";

  // The version of this tool that translated SIL to Alive IR directly 
  // will be a \c SILFunctionTransform instead of a \c SILModuleTransform.
  // So, for simplicity, only consider the first function.
  auto begin = IRMod->begin();
  assert(begin != IRMod->end() && 
      "The must be at least one function in the IR module.");
  llvm::Function *function = &*begin;

  // Lower LLVM IR to Alive IR
  auto result = aliveIRGen(*function, 
      generatedModule.getTargetMachine()->getTargetTriple());

  // Store Alive IR in ASTContext
  // TODO

  return true; 
}
