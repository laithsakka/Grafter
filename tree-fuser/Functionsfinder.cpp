//===--- FuncrionFinder.cpp -----------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This class is responsible for traversing the AST, tracking methods with fuse
// annotation  and apply tree fuser semantic checks.
//===----------------------------------------------------------------------===//

#include "FunctionsFinder.h"
#include "AccessPath.h"
#include "Logger.h"

bool FunctionsFinder::VisitFunctionDecl(clang::FunctionDecl *FuncDeclaration) {
  if (FuncDeclaration->hasBody() == false)
    return true;

  // must have fuste AnnotationInfoibute to be considered
  if (!hasFuseAnnotation(FuncDeclaration))
    return true;

  FunctionAnalyzer *FuncInfo = new FunctionAnalyzer(FuncDeclaration);
  if (!FuncInfo->isValidFuse()) {
    Logger::getStaticLogger().logInfo("function " +
                                      FuncDeclaration->getNameAsString() +
                                      " is not valid fuse methods");

  } else {
    Logger::getStaticLogger().logInfo("function " +
                                      FuncDeclaration->getNameAsString() +
                                      " is valid fuse methods");
  }

  FunctionsInformation[FuncDeclaration] = FuncInfo;
  return true;
}
