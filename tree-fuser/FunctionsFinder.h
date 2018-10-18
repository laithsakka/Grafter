//===--- FunctionsFinder.h-------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This class is responsible for traversing the ast, tracking methods with fuse
// annotation  and apply tree-fuser semantic checks.
//===----------------------------------------------------------------------===//

#ifndef TREE_FUSER_FUNCTION_FINDER
#define TREE_FUSER_FUNCTION_FINDER

#include "FunctionAnalyzer.h"
#include "LLVMDependencies.h"

#include <stdio.h>
#include <vector>

class FunctionsFinder : public clang::RecursiveASTVisitor<FunctionsFinder> {
public:

  /// Store for information about the analyzed functions
  static unordered_map<clang::FunctionDecl *, FunctionAnalyzer *>
      FunctionsInformation;

  /// Initiate a traversal to analyze functions
  void findFunctions(const clang::ASTContext &Context);
  
  /// Returns wether a function is tree-fuser traversal
  bool isValidFuse(clang::FunctionDecl *funcDecl);

  /// Returns the FunctionAnalyzer object related to a given function
  /// delcaration
  static FunctionAnalyzer *getFunctionInfo(clang::FunctionDecl *FuncDecl);

  bool VisitFunctionDecl(clang::FunctionDecl *funDeclaration);
};

#endif /* function_finder_hpp */
