//===--- LLVMDependencies.h -----------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

#ifndef TREE_FUSER_LLVM_DEPENDENCIES
#define TREE_FUSER_LLVM_DEPENDENCIES

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include <sstream>
#include <unistd.h>
#define DEBUG_TYPE ""

using namespace llvm;

// why this is here lol
struct StrictAccessInfo {
  bool IsReadOnly;
  int Id;
  bool IsLocal;
  bool IsGlobal;
};

extern bool hasFuseAnnotation(clang::FunctionDecl *FunDecl);
extern bool hasTreeAnnotation(const clang::CXXRecordDecl *RecordDecl);
extern bool hasChildAnnotation(clang::FieldDecl *FieldDecl);
extern bool hasStrictAccessAnnotation(clang::Decl *Decl);
extern std::vector<StrictAccessInfo> getStrictAccessInfo(clang::Decl *Decl);

#endif
