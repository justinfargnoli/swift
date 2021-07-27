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

#include "swift/AST/IRGenOptions.h"
#include "swift/AST/IRGenRequests.h"
#include "swift/Basic/PrimarySpecificPaths.h"
#include "swift/SIL/SILCloner.h"
#include "swift/SIL/SILModule.h"
#include "swift/SILAlive/SILAlive.h"
#include "swift/SILOptimizer/PassManager/Passes.h"
#include "swift/Subsystems.h"
#include "swift/TBDGen/TBDGen.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm_util/llvm2alive.h"
#include <iostream>

namespace swift {

void AliveModule::pushFunction(std::unique_ptr<IR::Function> function) {
  assert(function && "Cannot add a `nullptr` `IR::Function`");
  functions.push_back(std::move(function));
}

llvm::Optional<std::unique_ptr<AliveModule>> &SILAliveContext::aliveModule() {
  return aliveMod;
}

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

GeneratedModule genLLVMIR(std::unique_ptr<SILModule> SILMod) {
  // Lower \p SILMod to SILStage::Lowered
  assert(SILMod && "Cannot generate LLVM IR from a nullptr.");
  lowerSIL(*SILMod);

  // Generate LLVM IR for the \p SILMod
  llvm::GlobalVariable *HashGlobal;
  return performIRGeneration(
      SILMod->getSwiftModule(), IRGenOptions{}, TBDGenOptions{}, 
      std::move(SILMod), /*ModuleName=*/ "", PrimarySpecificPaths{},
      /*parallelOutputFilenames=*/ {}, &HashGlobal);
}

std::unique_ptr<IR::Function> aliveIRFunctionGen(llvm::Function &F, 
      const llvm::DataLayout &dataLayout, 
      llvm::Triple triple) {
  // NOTE: There may be a problem with calling `llvm_util::initializer` more 
  // than once during program execution
  //
  // Initialize llvm_util
  llvm_util::initializer llvm_util_init(std::cerr, dataLayout);

  auto functionAlivePtr = llvm_util::llvm2alive(F, 
      llvm::TargetLibraryInfo{llvm::TargetLibraryInfoImpl{triple}}, 
      std::vector<std::string_view>()).getPointer();

  return std::unique_ptr<IR::Function>(functionAlivePtr);
}

std::unique_ptr<AliveModule> aliveIRGen(GeneratedModule generatedModule) {
  auto aliveModule = std::make_unique<AliveModule>();

  auto LLVMIRMod = generatedModule.getModule();
  assert(LLVMIRMod && "No LLVM IR Module found.");
  
  for (auto &functionLLVM : *LLVMIRMod) {
    // Lower LLVM function IR to Alive function IR
    auto functionAlive = aliveIRFunctionGen(functionLLVM, 
        LLVMIRMod->getDataLayout(),
        generatedModule.getTargetMachine()->getTargetTriple());
    assert(functionAlive && "aliveIRFunctionGen failed.");
    
    // Add `functionAlive` to the `aliveModule`
    aliveModule->pushFunction(std::move(functionAlive));
  }
  
  return aliveModule;
}

std::unique_ptr<AliveModule> aliveIRGen(SILModule *SILMod) {
  // Clone the SIL Module
  auto SILModClone = cloneModule(SILMod);

  // Generate LLVM IR for the SILModule
  auto generatedModule = genLLVMIR(std::move(SILModClone));
  assert(generatedModule.getModule() && "IR module generation failed.");

  return aliveIRGen(std::move(generatedModule));
}

} 
