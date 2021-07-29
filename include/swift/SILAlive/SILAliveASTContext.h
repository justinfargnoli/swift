//===--- SILAliveASTContext.h -------------------------------------------------===//
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

#ifndef SWIFT_SILALIVE_SILALIVEASTCONTEXT_H
#define SWIFT_SILALIVE_SILALIVEASTCONTEXT_H

#include "ir/function.h"

namespace swift {

class AliveModule {
  std::vector<std::unique_ptr<IR::Function>> functions;

public:
  void pushFunction(std::unique_ptr<IR::Function> function);

  std::vector<std::unique_ptr<IR::Function>>::iterator begin();
  std::vector<std::unique_ptr<IR::Function>>::iterator end();
};

class SILAliveContext {
  llvm::Optional<std::unique_ptr<AliveModule>> aliveMod;

public:
  llvm::Optional<std::unique_ptr<AliveModule>> &aliveModule();
};

}

#endif // SWIFT_SILALIVE_SILALIVEASTCONTEXT_H
