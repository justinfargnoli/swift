//===--- SILAlive.cpp --------------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2021 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
///
/// \file
///
/// This utility is a command line tool that given  two sil or sib files invokes
/// Alive's translation validation engine on them via SILAlive. 
///
//===----------------------------------------------------------------------===//

#include "swift/Demangling/Demangle.h"
#include "swift/Basic/LLVM.h"
#include "swift/Basic/LLVMInitialize.h"
#include "swift/Basic/Range.h"
#include "swift/Frontend/DiagnosticVerifier.h"
#include "swift/Frontend/Frontend.h"
#include "swift/Frontend/PrintingDiagnosticConsumer.h"
#include "swift/SIL/SILBuilder.h"
#include "swift/SIL/SILUndef.h"
#include "swift/SILAlive/SILAlive.h"
#include "swift/SILOptimizer/Analysis/Analysis.h"
#include "swift/SILOptimizer/PassManager/PassManager.h"
#include "swift/SILOptimizer/PassManager/Passes.h"
#include "swift/Serialization/SerializedModuleLoader.h"
#include "swift/Serialization/SerializedSILLoader.h"
#include "swift/Subsystems.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Signals.h"
#include <cstdio>
#include <functional>

using namespace swift;

static llvm::cl::opt<std::string> SourceFilename(llvm::cl::desc("source file"),
                                                llvm::cl::init("-"),
                                                llvm::cl::Positional);

static llvm::cl::opt<std::string> TargetFilename(llvm::cl::desc("target file"),
                                                llvm::cl::init("-"),
                                                llvm::cl::Positional);

static llvm::cl::list<std::string>
    ImportPaths("I",
                llvm::cl::desc("add a directory to the import search path"));

static llvm::cl::opt<std::string>
    ModuleName("module-name",
               llvm::cl::desc("The name of the module if processing"
                              " a module. Necessary for processing "
                              "stdin."));

static llvm::cl::opt<bool>
    DemangleNames("demangle",
                  llvm::cl::desc("Demangle names of entities outputted"));

static llvm::cl::opt<std::string>
    ModuleCachePath("module-cache-path",
                    llvm::cl::desc("Clang module cache path"));

static llvm::cl::opt<std::string> ResourceDir(
    "resource-dir",
    llvm::cl::desc("The directory that holds the compiler resource files"));

static llvm::cl::opt<std::string>
    SDKPath("sdk", llvm::cl::desc("The path to the SDK for use with the clang "
                                  "importer."),
            llvm::cl::init(""));

static llvm::cl::opt<std::string> Triple("target",
                                         llvm::cl::desc("target triple"));

// This function isn't referenced outside its translation unit, but it
// can't use the "static" keyword because its address is used for
// getMainExecutable (since some platforms don't support taking the
// address of main, and some platforms can't implement getMainExecutable
// without being given the address of a function in the main executable).
void anchorForGetMainExecutable() {}

std::unique_ptr<SILModule> makeSILModule(std::string fileName, char **argv) {
  CompilerInvocation Invocation;

  Invocation.setMainExecutablePath(llvm::sys::fs::getMainExecutable(
      argv[0], reinterpret_cast<void *>(&anchorForGetMainExecutable)));

  // Give the context the list of search paths to use for modules.
  Invocation.setImportSearchPaths(ImportPaths);
  // Set the SDK path and target if given.
  if (SDKPath.getNumOccurrences() == 0) {
    const char *SDKROOT = getenv("SDKROOT");
    if (SDKROOT)
      SDKPath = SDKROOT;
  }
  if (!SDKPath.empty())
    Invocation.setSDKPath(SDKPath);
  if (!Triple.empty())
    Invocation.setTargetTriple(Triple);
  if (!ResourceDir.empty())
    Invocation.setRuntimeResourcePath(ResourceDir);
  Invocation.getClangImporterOptions().ModuleCachePath = ModuleCachePath;
  Invocation.setParseStdlib();
  Invocation.getLangOptions().DisableAvailabilityChecking = true;
  Invocation.getLangOptions().EnableAccessControl = false;
  Invocation.getLangOptions().EnableObjCAttrRequiresFoundation = false;

  serialization::ExtendedValidationInfo extendedInfo;
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> FileBufOrErr =
      Invocation.setUpInputForSILTool(SourceFilename, ModuleName,
                                      /*alwaysSetModuleToMain*/ true,
                                      /*bePrimary*/ false, extendedInfo);
  if (!FileBufOrErr) {
    fprintf(stderr, "Error! Failed to open file: %s\n", SourceFilename.c_str());
    exit(-1);
  }

  CompilerInstance CI;
  PrintingDiagnosticConsumer PrintDiags;
  CI.addDiagnosticConsumer(&PrintDiags);

  if (CI.setup(Invocation))
    return nullptr;
  CI.performSema();

  // If parsing produced an error, don't run any passes.
  if (CI.getASTContext().hadError())
    return nullptr;

  return performASTLowering(CI.getMainModule(), CI.getSILTypes(), 
        CI.getSILOptions());
}

int main(int argc, char **argv) {
  PROGRAM_START(argc, argv);
  INITIALIZE_LLVM();

  llvm::cl::ParseCommandLineOptions(argc, argv, "SIL OPT\n");

  auto sourceSILMod = makeSILModule(SourceFilename, argv);
  assert(sourceSILMod && "Source file wasn't able to be parsed into a SILModule");
  auto targetSILMod = makeSILModule(TargetFilename, argv);
  assert(targetSILMod && "Target file wasn't able to be parsed into a SILModule");

  translationValidation(std::move(sourceSILMod), 
      std::move(targetSILMod));
}
