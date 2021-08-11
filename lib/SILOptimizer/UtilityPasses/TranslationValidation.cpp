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
    auto SILMod = getModule();
    assert(SILMod && "TranslationValidation cannot use `nullptr` SILModule.");
    
    bool result = translationValidationOptimizationPass(*SILMod);
    assert(result && "Translation validation optimization pass failed.");
  }
};

} // end anonymous namespace

//===----------------------------------------------------------------------===//
//                           Top Level Entry Point
//===----------------------------------------------------------------------===//

SILTransform *swift::createTranslationValidation() {
  return new TranslationValidation();
}
