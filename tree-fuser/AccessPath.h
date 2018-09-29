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

#include "FSMUtility.h"
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
  
  /// A tempory created access-path that does not represent real access
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

  /// Automata representation for writing the access-path
  FSM *WriteAutomata = nullptr;

  /// Automata representation for reading the access-path
  FSM *ReadAutomata = nullptr;

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
    outs() << "IsLocal:" << isLocal()
           << ". Value Start:" << getValueStartIndex()
           << " .IsLegal:" << IsLegal << ".\nContent:" << AccessPathString
           << "\n";
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

  /// Return the length of the access-path
  int getDepth() const;

  /// Return the size of the value part of the access-path
  int getValuePathSize() const;

  /// Return the size of the tree acess part of the access-path
  int getTreeAccessPathSize() const;

  int getTreeAccessPathEndIndex() const;

  const string &getAsStr() const;

  bool isStrictAccessCall() const { return IsStrictAccessCall; }

  clang::ValueDecl *getDeclAtIndex(int Index) const;

  StrictAccessInfo getAnnotationInfo() const { return AnnotationInfo; }

  void setAnnotationInfo(StrictAccessInfo &NewValue) {
    AnnotationInfo = NewValue;
  }

  std::vector<pair<string, clang::ValueDecl *>> SplittedAccessPath;

  /// Return an linear FSM that represents reading the access-path (all states
  /// are final)
  const FSM &getReadAutomata();

  /// Return an linear FSM that represents writing the access-path (only last
  /// state final)
  const FSM &getWriteAutomata();

  ~AccessPath() {
    if (WriteAutomata)
      delete WriteAutomata;

    if (ReadAutomata)
      delete ReadAutomata;
  }
};

class AccessPathContainer {

private:
  AccessPathSet ReadSet;
  AccessPathSet WriteSet;

  /// Stored access paths to nodes that are deleted or replaced (assigned to new
  /// nodes)
  AccessPathSet ReplacedSet;

public:
  /// Insert a normal read, write access path
  bool insert(AccessPath *AccessPath, bool IsWrite);

  /// Insert an access path to a node that is deleted or assigned to a new node
  bool insertReplacedAccessPath(AccessPath *AccessPath);

  /// Delete the dynamically allocated access-paths
  void freeAccessPaths();

  /// Returns the set of the read access-paths
  const AccessPathSet &getReadSet() const { return ReadSet; }

  /// Returns the set of the write access-paths
  const AccessPathSet &getWriteSet() const { return WriteSet; }

  /// Returns the set of the replaced access-paths
  const AccessPathSet &getReplacedSet() const { return ReplacedSet; }
};

#endif
