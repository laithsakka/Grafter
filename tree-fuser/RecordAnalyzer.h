//===--- RecordsAnalyzer.h ------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#ifndef TREE_FUSER_RECORD_ANALYZER
#define TREE_FUSER_RECORD_ANALYZER

#include "LLVMDependencies.h"
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace clang;

class RecordInfo;

typedef std::unordered_map<
    clang::ASTContext *,
    std::unordered_map<const clang::RecordDecl *, RecordInfo *>>
    RecordsStore;

class RecordInfo {
  friend class RecordsAnalyzer;

private:
  /// Specify whether a record is a tree structure
  bool IsTreeStructure = false;

  /// Set of recursive declarations
  std::set<clang::FieldDecl *> RecursiveDeclarations;

  /// Specify weather all the member and the nested members are scalers
  int IsCompleteScaler = -1; // not known yet

public:
  /// Return true if the structure is recognized by treefuser as tree
  bool isTreeStructure() const { return IsTreeStructure; }

  /// Return the recursive fields of the tree
  const std::set<clang::FieldDecl *> &getChildAccessDecls() const {
    return RecursiveDeclarations;
  }

  /// Check wether a given declaration is a recursive field
  bool isChildAccessDecl(clang::ValueDecl *Decl) const {
    assert(IsTreeStructure);
    clang::FieldDecl *FieldDecl = dyn_cast<clang::FieldDecl>(Decl);
    if (!clang::ValueDecl::classofKind(clang::Decl::Kind::Field)) {
      assert(FieldDecl == nullptr);
      return false;
    }
    return RecursiveDeclarations.count(FieldDecl);
  }
};

class RecordsAnalyzer : public clang::RecursiveASTVisitor<RecordsAnalyzer> {
public:
  /// Initiate an ast traversal to anaylyze the records source code
  void analyzeRecordsDeclarations(const clang::ASTContext &Context);

  /// Return recursive field declarations
  static const std::set<clang::FieldDecl *> &
  getChildAccessDecls(const clang::RecordDecl *RecordDecl);

  static const RecordInfo &getRecordInfo(const clang::RecordDecl *RecordDecl);

  /// Return true if the declaration type is built in type or a record(class or
  /// struct)
  static bool isScaler(clang::ValueDecl *const ValueDecl);

  /// Return true if all the member and the nested members are scalers
  static bool isCompleteScaler(clang::ValueDecl *const ValueDecl);

  bool VisitCXXRecordDecl(clang::CXXRecordDecl *s);

private:

  /// A table that stores record information
  static std::unordered_map<
      clang::ASTContext *,
      std::unordered_map<const clang::RecordDecl *, RecordInfo *>>
      RecordsInfoGlobalStore;
};

#endif
