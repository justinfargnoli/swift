//===--- SILCloner.h - Defines the SILCloner class --------------*- C++ -*-===//
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
//
// This file defines the SILCloner class, used for cloning SIL instructions.
//
//===----------------------------------------------------------------------===//

#include "swift/SIL/SILCloner.h"
#include "swift/SIL/SILFunctionBuilder.h"

namespace swift {

std::unique_ptr<SILModule> cloneModule(SILModule *originalModule) {
  // This implementation is based on llvm::CloneModule which can be found here:
  // https://llvm.org/doxygen/namespacellvm.html#ab371d6b308eb9772bdec63cf7a041407

  // Create a new module to copy the contents of \p originalModule into. 
  llvm::PointerUnion<FileUnit *, ModuleDecl *> context{};
  context = (ModuleDecl *)originalModule->getSwiftModule();
  auto newModule = SILModule::createEmptyModule(
      context, originalModule->Types,
      originalModule->getOptions());

  // initialize the new \c SILGlobalVariable s
  for (auto &originalGlobalVar : originalModule->getSILGlobals()) {
    SILGlobalVariable::create(
        *newModule, originalGlobalVar.getLinkage(),
        originalGlobalVar.isSerialized(), originalGlobalVar.getName(),
        originalGlobalVar.getLoweredType(), originalGlobalVar.getLocation(),
        originalGlobalVar.getDecl());
  }

  {
    // initialize the new \c SILFunction s
    SILFunctionBuilder functionBuilder{*newModule};
    for (SILFunction &originalFunction : *originalModule) {
      // copy \p originalFunction into \p newModule
      functionBuilder.createFunction(
        originalFunction.getLinkage(), originalFunction.getName(),
        originalFunction.getLoweredFunctionType(),
        originalFunction.getGenericEnvironment(),
        originalFunction.getLocation(), originalFunction.isBare(),
        originalFunction.isTransparent(), originalFunction.isSerialized(), 
        originalFunction.isDynamicallyReplaceable(), 
        originalFunction.getEntryCount(), originalFunction.isThunk(),
        originalFunction.getClassSubclassScope(),
        originalFunction.getInlineStrategy(), originalFunction.getEffectsKind(),
        /*insertBefore=*/nullptr, originalFunction.getDebugScope());
    }
  }

  // copy the \c SILGlobalVariable initializers
  // TODO

  // copy the bodies of the \c SILFunction s 
  {
    auto originalFunction = originalModule->begin();
    auto newFunction = newModule->begin();
    for (; originalFunction != originalModule->end() &&
          newFunction != newModule->end(); ++originalFunction, ++newFunction) {
      SILFunctionCloner funcCloner(&*newFunction);
      funcCloner.cloneFunction(&*originalFunction);
    }
    assert(originalFunction == originalModule->end() && 
          newFunction == newModule->end() && 
          "`originalModule` and `newModule` are not of the same legnth.");
  }

  return newModule;
}

}