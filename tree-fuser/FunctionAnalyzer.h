//===--- FunctionAnalyzer.h -----------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Analyze fuse functions and their access paths
//===----------------------------------------------------------------------===//

#ifndef TREE_FUSER_FUNCTION_ANALYZER
#define TREE_FUSER_FUNCTION_ANALYZER

#include "AccessPath.h"
#include "FunctionsFinder.h"
#include "LLVMDependencies.h"
#include "RecordAnalyzer.h"
#include "StatementInfo.h"
#include <string>

using namespace clang;
using namespace std;

struct AccessPathCompare;
class StatementInfo;
class AccessPath;

class FunctionAnalyzer {
  friend class AccessPath;

private:
  /// Maps local defintion to their aliased tree locations
  std::map<clang::ValueDecl *, AccessPath *> LocalAliasingMap;

  /// Determine weather the function is virtual member or global (the only two
  /// allowed versions)
  bool IsGlobal = true;

  ///  Declaration of the analyzed functions
  clang::FunctionDecl *FuncDeclNode;

  /// Declaration of the traversed node argument
  clang::VarDecl *TraversedNodeDecl;

  /// Declaration of the travesed tree type
  const clang::RecordDecl *TraversedTreeTypeDecl;

  /// Pointer to the currently analyzed statment within the function
  StatementInfo *CurrStatementInfo = nullptr;

  int NestedIfDepth = 0;

  /// List of called recursivly traversed children in original order
  std::vector<pair<clang::FunctionDecl *, clang::FieldDecl *>> TraversingCalls;

  /// Store the result of the semantics check
  bool SemanticsSatasified = true;

  /// Add an access path to the currently traversed statement information
  void addAccessPath(AccessPath *AccessPath, bool IsRead);

  /// Add an access path to a node that is deleted or assigned to a new node
  void addReplacedNodeAccessPath(AccessPath *AccessPath);

  /// Perform checks that confirms that the body of the function with tree-fuser
  /// semantics
  bool checkFuseSema();

  /// Stores the statments information of the function
  std::vector<StatementInfo *> Statements;

  bool collectAccessPath_handleStmt(clang::Stmt *Stmt);

  bool collectAccessPath_handleSubExpr(clang::Expr *Expr);

  bool collectAccessPath_VisitCompoundStmt(clang::CompoundStmt *Stmt);

  bool collectAccessPath_VisitIfStmt(clang::IfStmt *Stmt);

  bool collectAccessPath_VisitDeclsStmt(clang::DeclStmt *Stmt);

  bool collectAccessPath_VisitParenExpr(clang::ParenExpr *Expr);

  bool collectAccessPath_VisitCXXMemberCallExpr(clang::CXXMemberCallExpr *Expr);

  bool collectAccessPath_VisitCallExpr(clang::CallExpr *Expr);

  bool collectAccessPath_VisitBinaryOperator(clang::BinaryOperator *Stmt);

  bool collectAccessPath_VisitStaticCastExpr(clang::CXXStaticCastExpr *Expr);

  bool collectAccessPath_VisitCXXDeleteExpr(clang::CXXDeleteExpr *Expr);

public:
  bool isVirtual() {
    if (isGlobal())
      return false;
    if (isCXXMember()) {
      return dyn_cast<clang::CXXMethodDecl>(FuncDeclNode)->isVirtual();
    }
  }

  clang::CXXMethodDecl *getDeclAsCXXMethod() {
    if (isGlobal())
      return nullptr;
    if (isCXXMember())
      return dyn_cast<clang::CXXMethodDecl>(FuncDeclNode);
  }

  void addAliasing(clang::ValueDecl *Decl, AccessPath *Ap) {
    LocalAliasingMap[Decl] = Ap;
  }

  AccessPath *getLocalAliasing(clang::ValueDecl *Decl) const {
    if (!LocalAliasingMap.count(Decl))
      return nullptr;
    else
      return LocalAliasingMap.find(Decl)->second;
  }

  FunctionAnalyzer(clang::FunctionDecl *FuncDeclaration);

  bool isCXXMember() const { return !IsGlobal; };

  bool isGlobal() const { return IsGlobal; };

  void setCXXMember() { IsGlobal = false; };

  void setGlobal() { IsGlobal = true; };

  /// Return the top level statements in the body of the function
  std::vector<StatementInfo *> &getStatements() { return Statements; }

  /// Dump information about the analyzed function
  void dump();

  /// Return true if the field is recursively visited by the function
  bool isInCalledChildList(clang::FieldDecl *ChildDecl) const;

  /// Return true
  bool isInChildList(clang::ValueDecl *Decl) const;

  bool isValidFuse() const { return SemanticsSatasified; }

  void setValidFuse(bool IsValid) { SemanticsSatasified = IsValid; }

  int getNumberOfTraversingCalls() const;

  clang::FunctionDecl *getFunctionDecl() const;

  const clang::VarDecl *getTraversedNodeDecl() const;

  const clang::RecordDecl *getTraversedTreeTypeDecl() const;

  const vector<pair<clang::FunctionDecl *, clang::FieldDecl *>> &
  getTraversingCalls() const;

  void addTraversingCall(clang::FunctionDecl *CalledFunction,
                         clang::FieldDecl *CalledChild) {
    TraversingCalls.push_back(make_pair(CalledFunction, CalledChild));
  }

  ~FunctionAnalyzer();
};

#endif
