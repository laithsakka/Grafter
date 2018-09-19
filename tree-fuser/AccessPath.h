//===--- AccessPath.h -----------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#ifndef TREE_FUSER_ACCESS_PATH_H
#define TREE_FUSER_ACCESS_PATH_H

#include "LLVMDependencies.h"
#include "Logger.h"

class FunctionAnalyzer;
class AccessPath;

struct AccessPathCompare {
  bool operator()(const AccessPath *LHS, const AccessPath *RHS) const;
};

typedef std::set<AccessPath *, AccessPathCompare> AccessPathSet;

class AccessPath {
private:
  bool parseAccessPath(clang::MemberExpr *MemberExpression);

  bool parseAccessPath(clang::DeclRefExpr *DeclRefExpression);

  bool parseAccessPath(clang::CXXStaticCastExpr *CastExpression);

  bool handleNextExpression(clang::Stmt *NextExpression);

  void setValueStartIndex();

  void appendSymbol(clang::ValueDecl *DeclAccess);

  /// An index for the the start index of the value part in splittedAccessPath
  /// array
  int ValueStartIndex = -1;

  bool IsDummy = false;

  /// Indicates that the access is legal
  bool IsLegal = true;

  /// A string representation of the access path
  string AccessPathString;

  /// A pointer to the functions that includes contains the access path
  const FunctionAnalyzer *EnclosingFunction;

  /// Indicates that this AccessPath represents an annotated function call
  bool IsStrictAccessCall = false;

  /// The annotation information for a strict annotated access
  StrictAccessInfo AnnotationInfo;

public:
  /// Creates an AccessPath from an expression
  AccessPath(clang::Expr *SourceExpression, FunctionAnalyzer *Function,
             StrictAccessInfo *AnnotationInfo = nullptr);

  /// Creates an AccessPath from a variable declaration
  AccessPath(clang::VarDecl *VarDeclaration, FunctionAnalyzer *Function);

  /// Creates an AccessPath from a function declaration (for annotated function
  /// accesses)
  AccessPath(clang::FunctionDecl *FunctionDeclaration,
             FunctionAnalyzer *Function);

  /// Print the content of the access path
  void dump() {
    outs() <<"IsLocal:" << isLocal()<<". Value Start:"<<getValueStartIndex()<< " .IsLegal:" << IsLegal << ". Content:" << AccessPathString << "\n";
  }

  bool hasValuePart() const;

  bool isLegal() const;

  bool isOnTree() const;

  bool isOffTree() const;

  bool isLocal() const;

  bool isGlobal() const;

  bool onlyUses(const clang::VarDecl *RootDecl,
                const set<clang::FieldDecl *> &ChildrenSymbols);

  int getValueStartIndex() const;

  int getDepth() const;

  int getValuePathSize() const;

  int getTreeAccessPathSize() const;

  int getTreeAccessPathEndIndex() const;

  const string &getAsStr() const;

  bool isStrictAccessCall() const { return IsStrictAccessCall; }

  clang::ValueDecl *getDeclAtIndex(int Index) const;

  StrictAccessInfo getAnnotationInfo() const { return AnnotationInfo; }

  StrictAccessInfo setAnnotationInfo(StrictAccessInfo &NewValue) {
    AnnotationInfo = NewValue;
  }

  std::vector<pair<string, clang::ValueDecl *>> SplittedAccessPath;
};

class AccessPathContainer {

private:
  AccessPathSet ReadSet;
  AccessPathSet WriteSet;
  AccessPathSet DeleteSet;

public:
  bool insert(AccessPath *AccessPath, bool IsWrite);

  bool insertDeleteAccessPath(AccessPath *AccessPath);

  void freeAccessPaths();

  const AccessPathSet &getReadSet() const {
    return ReadSet;
  }

  const AccessPathSet &getWriteSet() const {
    return WriteSet;
  }

  const AccessPathSet &getDeleteSet() const {
    return DeleteSet;
  }
};

#endif
