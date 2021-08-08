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
#include "tools/transform.h"
#include <iostream>

namespace swift {

void AliveModule::pushFunction(std::unique_ptr<IR::Function> function) {
  assert(function && "Cannot add a `nullptr` `IR::Function`");
  functions.push_back(std::move(function));
}

std::vector<std::unique_ptr<IR::Function>>::iterator AliveModule::begin() { 
  return functions.begin(); 
}
  
std::vector<std::unique_ptr<IR::Function>>::iterator AliveModule::end() { 
  return functions.end(); 
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

// void globalVariablesNamesUsed(std::vector<std::string> &globalVarNames, 
//       llvm::Module &llvmMod, llvm::Function &func) {
//   assert(globalVarNames.empty() && "`globalVarNames` has left over elements");

//   // // Iterate through all global variables in this `llvm::Module`
//   // for (const auto &globalVariable : llvmMod.globals()) {
//   //   // Iterate through all of the uses of this `globalVariable `
//   //   for (const auto *user : globalVariable.users()) {
//   //     // If the `user` is an `llvm::Function`
//   //     if (const auto *instruction = llvm::dyn_cast<llvm::Instruction>(user)) {
//   //       // ... and if this function has the same name as `func`
//   //       if (instruction->getFunction()->getName().equals(func.getName())) {
//   //         // ... then add the name of `globalVariable` to `globalVarNames`
//   //         globalVarNames.emplace_back(globalVariable.getName());
//   //       }
//   //     }
//   //   }
//   // }
//   for (const auto &basicBlock : func) {
//     for (const auto &instruction : basicBlock) {
//       for (const llvm::Value *operand : instruction.operands()) {
//         llvm::errs() << "\n --- 1 \n";
//         llvm::errs() << "\n is a Value : " << llvm::isa<llvm::Value>(operand) << " \n";
//         operand->dump();
//         llvm::errs() << "\n is a Constant : "
//                      << llvm::isa<llvm::Constant>(operand) << " \n";
//         llvm::errs() << "\n is a GlobalValue : " << llvm::isa<llvm::GlobalValue>(operand) << " \n";
//         if (const llvm::GlobalValue *globalVar =
//                 llvm::dyn_cast<llvm::GlobalValue>(operand)) {
//           llvm::errs() << "\n --- 2 \n";
//           globalVarNames.emplace_back(globalVar->getName());
//         }
//       }
//     }
//   }
// }

std::unique_ptr<IR::Function> aliveIRFunctionGen(llvm::Function &F, 
      llvm::Module &llvmMod,
      const llvm::DataLayout &dataLayout, 
      llvm::Triple triple) {
  // Initialize llvm_util
  llvm_util::initializer llvm_util_init{std::cout, dataLayout};

  // Compute the names of the global variables used in `F`
  // std::vector<std::string> globalVarNames{};
  // globalVariablesNamesUsed(globalVarNames, llvmMod, F);
  // {
  //   llvm::errs() << "\n[ \n";
  //   for (const auto &gv : globalVarNames) {
  //     llvm::errs() << gv << ", \n";
  //   }
  //   llvm::errs() << "] \n";
  // }
  // std::vector<std::string_view> globalVarNamesView{};
  // for (const auto &globalVarName : globalVarNames) {
  //   globalVarNamesView.emplace_back(globalVarName);
  // }

  auto functionAlive = llvm_util::llvm2alive(F, 
      llvm::TargetLibraryInfo{llvm::TargetLibraryInfoImpl{triple}}/*, 
      globalVarNamesView*/);
  assert(functionAlive.hasValue() && "`functionAlive` is empty");

  return std::make_unique<IR::Function>(std::move(*functionAlive));
}

std::unique_ptr<AliveModule> aliveIRGen(GeneratedModule generatedModule) {
  auto aliveModule = std::make_unique<AliveModule>();

  auto LLVMIRMod = generatedModule.getModule();
  assert(LLVMIRMod && "No LLVM IR Module found.");

  for (auto &functionLLVM : *LLVMIRMod) {
    // Lower LLVM function IR to Alive function IR
    auto functionAlive = aliveIRFunctionGen(functionLLVM, *LLVMIRMod, 
        LLVMIRMod->getDataLayout(), 
        generatedModule.getTargetMachine()->getTargetTriple());
    assert(functionAlive && "aliveIRFunctionGen failed.");

    // Add `functionAlive` to the `aliveModule`
    aliveModule->pushFunction(std::move(functionAlive));
  }
  
  return aliveModule;
}

std::unique_ptr<AliveModule> aliveIRGen(SILModule &SILMod) {
  // Clone the SIL Module
  auto SILModClone = cloneModule(SILMod);

  // Generate LLVM IR for the SILModule
  auto generatedModule = genLLVMIR(std::move(SILModClone));
  assert(generatedModule.getModule() && "IR module generation failed.");

  return aliveIRGen(std::move(generatedModule));
}

void translationValidationFunction(IR::Function func1, IR::Function func2) {
  tools::Transform transform{"transform", std::move(func1), std::move(func2)};
  tools::TransformVerify transformVerify{transform, false};
  std::cout << transformVerify.verify();
}

void reportUnmatchedFunction(IR::Function &func) {
  // FIXME: Remove use of `std::cout`.
  // The fundamental problem is figuring out how to print the `IR::Function`

  std::cout << "\n--- (" << func.getName()
      << ") : no matching function in target. \n";
  std::cout << func << "\n";
}

void translationValidation(std::unique_ptr<AliveModule> mod1, 
      std::unique_ptr<AliveModule> mod2) {
  // Move all `IR::Function`s of `mod2` into `mod2Map`
  llvm::StringMap<std::unique_ptr<IR::Function>> mod2Map{};
  for (auto func = mod2->begin(); func != mod2->end(); ++func) {
    auto result = mod2Map.try_emplace((*func)->getName(), std::move(*func));
    assert(result.second && "Multiple functions in `mod2` have the same name!");
  }

  // Iterate through the `IR::Function`s in mod1
  for (auto func1 = mod1->begin(); func1 != mod1->end(); ++func1) {
    assert(*func1 && "`func1` doesn't exist!");

    // Find the matching `IR::Function` from `mod2`
    // If a match isn't found default construct an `IR::Function`
    auto &func2 = mod2Map[(*func1)->getName()];

    // If func2 is a match and isn't the deafult constructed `IR::Function`
    if (not func2->getName().empty()) {
      // Run translation validation
      translationValidationFunction(std::move(**func1), std::move(*func2));
    } else {
      // Otherwise tell the user that there wasn't a matching `IR::Function` for
      // `func1`
      reportUnmatchedFunction(**func1);
    }

    // Remove `func2` from the `mod2Map` 
    auto result = mod2Map.erase(func2->getName());
    assert(result && "Expected `func2` in `mod2Map`.");
  }

  // Report all of the `IR::Function`s in `mod2` which haven't been matched
  for (auto func = mod2Map.begin(); func != mod2Map.end(); ++func) {
    reportUnmatchedFunction(*func->getValue());
  }
}

bool translationValidationOptimizationPass(SILModule &SILMod) {
  auto aliveModule = aliveIRGen(SILMod);
  assert(aliveModule && "Cannot use `nullptr` `aliveModule`");

  auto &contextAliveModule =
      SILMod.getASTContext().getSILAliveContext().aliveModule();
      
  if (contextAliveModule.hasValue()) {
    // With the source `contextAliveModule.getValue()` and target `aliveModule`
    // run translation validation
    assert(contextAliveModule.getValue() && 
        "Cannot use `nullptr` `contextAliveModule`");
    translationValidation(std::move(contextAliveModule.getValue()), 
        std::move(aliveModule));

    contextAliveModule = None;
  } else {
    // Put \p aliveModule into \c AliveModule put to either
    // - replace `None` with an \c AliveModule 
    // - replace the existing \c AliveModule
    // so that the next time this function is invoked it can be used as the source
    // of translation validation. 
    contextAliveModule = std::move(aliveModule);
  }

  return true;
}

} 
