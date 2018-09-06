//===--- FunctionsFinder.h-------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This class is responsible for traversing the ast, tracking methods with fuse
// annotation  and apply tree fuser semantic checks.
//===----------------------------------------------------------------------===//

#ifndef TREE_FUSER_FUNCTION_FINDER
#define TREE_FUSER_FUNCTION_FINDER

#include "FunctionAnalyzer.h"
#include "LLVMDependencies.h"

#include <stdio.h>
#include <vector>

class FunctionsFinder : public RecursiveASTVisitor<FunctionsFinder> {
public:

  /// Sotre for information about the analyzed functions
  unordered_map<clang::FunctionDecl *, FunctionAnalyzer *> FunctionsInformation;

  /// Initiate a traversal to analyze functions
  void findFunctions(const ASTContext &Context) {
    TraverseDecl(Context.getTranslationUnitDecl());
  }

  /// Returns wether a function is treefuser traversal
  bool isValidFuse(clang::FunctionDecl *funcDecl) {
    if (!FunctionsInformation.count(funcDecl))
      return false;

    return FunctionsInformation[funcDecl]->isValidFuse();
  }

  bool VisitFunctionDecl(FunctionDecl *funDeclaration);
};

#endif /* function_finder_hpp */
