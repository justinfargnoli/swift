//===--- SILAlive.h -------------------------------------------------===//
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

#ifndef SWIFT_SILALIVE_SILALIVE_H
#define SWIFT_SILALIVE_SILALIVE_H

#include "swift/SIL/SILModule.h"
#include "swift/SILAlive/SILAliveASTContext.h"

namespace swift {

bool translationValidationOptimizationPass(SILModule *SILMod);

} 

#endif // SWIFT_SILALIVE_SILALIVE_H
