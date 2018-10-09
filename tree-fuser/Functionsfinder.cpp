//===--- FunctionsFinder.cpp ----------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This class is responsible for traversing the AST, find methods with fuse
// annotation, apply tree fuser semantic checks on them and peform some analysis.
//===----------------------------------------------------------------------===//

#include "FunctionsFinder.h"
#include "AccessPath.h"
#include "Logger.h"

#define DEBUG_TYPE "function-finder"

std::unordered_map<clang::FunctionDecl *, FunctionAnalyzer *>
    FunctionsFinder::FunctionsInformation =
        unordered_map<clang::FunctionDecl *, FunctionAnalyzer *>();

bool FunctionsFinder::VisitFunctionDecl(clang::FunctionDecl *FuncDeclaration) {
  if (!FuncDeclaration->isThisDeclarationADefinition())
    return true;

  // must have fuste AnnotationInfoibute to be considered
  if (!hasFuseAnnotation(FuncDeclaration))
    return true;

  FunctionAnalyzer *FuncInfo = new FunctionAnalyzer(FuncDeclaration);

  FunctionsInformation[FuncDeclaration] = FuncInfo;
  LLVM_DEBUG(if (FuncInfo->isValidFuse()) { FuncInfo->dump(); });
  return true;
}

bool FunctionsFinder::isValidFuse(clang::FunctionDecl *funcDecl) {
  if (!FunctionsInformation.count(funcDecl))
    return false;

  return FunctionsInformation[funcDecl]->isValidFuse();
}

FunctionAnalyzer *
FunctionsFinder::getFunctionInfo(clang::FunctionDecl *FuncDecl) {
  assert(FunctionsInformation.count(FuncDecl));
  return FunctionsInformation[FuncDecl];
}

void FunctionsFinder::findFunctions(const ASTContext &Context) {
  TraverseDecl(Context.getTranslationUnitDecl());
  bool KeepLooping = true;

  // Make sure that all traversing calls are to valid fuse function otherwise
  // invalidate the function
  while (KeepLooping) {
    KeepLooping = false;
    for (auto &Entry : FunctionsInformation) {
      FunctionAnalyzer *ContainingFunctionInfo = Entry.second;
      if (ContainingFunctionInfo->isValidFuse())
        for (auto &TraversingCall :
             ContainingFunctionInfo->getTraversingCalls()) {
          clang::FunctionDecl *CalledFunction = TraversingCall.first;

          if (!isValidFuse(CalledFunction)) {
            ContainingFunctionInfo->setValidFuse(false);
            ContainingFunctionInfo->getFunctionDecl()->dump();
            CalledFunction->dump();
            KeepLooping = true;
            break;
          }
        }
    }
  }

  for (auto &Entry : FunctionsInformation)
    if (!Entry.second->isValidFuse()) {
      Logger::getStaticLogger().logInfo("function " +
                                        Entry.first->getNameAsString() +
                                        " is not valid fuse methods");

    } else {
      Logger::getStaticLogger().logInfo("function " +
                                        Entry.first->getNameAsString() +
                                        " is valid fuse methods");
    }
}