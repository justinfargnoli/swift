//===--- TranslationValidation.cpp --------------------------===//
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

#include "swift/SILAlive/SILAlive.h"
#include "swift/SILOptimizer/PassManager/Transforms.h"

using namespace swift;

namespace {

class TranslationValidation : public SILModuleTransform {
  void run() override { 
    auto aliveModule = aliveIRGen(getModule());
    assert(aliveModule && "Cannot use `nullptr` `aliveModule`");

    auto &contextAliveModule = getModule()->getASTContext().getSILAliveContext()
        .aliveModule();
    if (contextAliveModule.hasValue()) {
      // With the source `contextAliveModule.getValue()` and target `aliveModule`
      // run translation validation
      // TODO: ^
    } 

    // Put \p aliveModule into \c AliveModule put to either
    // - replace `None` with an \c AliveModule 
    // - replace the existing \c AliveModule
    // so that the next time this function is invoked it can be used as the source
    // of translation validation. 
    contextAliveModule = std::move(aliveModule);
  }
};

} // end anonymous namespace

//===----------------------------------------------------------------------===//
//                           Top Level Entry Point
//===----------------------------------------------------------------------===//

SILTransform *swift::createTranslationValidation() {
  return new TranslationValidation();
}
