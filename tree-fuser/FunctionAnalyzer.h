//===--- FuncrionFinder.h -------------------------------------------------===//
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
  ///  Declaration of the analyzed functions
  clang::FunctionDecl *FuncDeclNode;

  /// Declaration of the traversed node argument
  clang::VarDecl *TraversedNodeDecl;

  /// Declaration of the travesed tree type
  const clang::RecordDecl *TraversedTreeTypeDecl;

  /// Pointer to the currently analyzed statment within the function
  StatementInfo *CurrStatementInfo = nullptr;

  int NestedIfDepth = 0;

  /// List of called recursivly visited children in original order
  std::vector<clang::FieldDecl *> CalledChildrenOrderedList;

  /// Store the result of the semantics check
  bool SemanticsSatasified = true;

  /// Add an access path to the currently traversed statement information
  void addAccessPath(AccessPath *AccessPath, bool IsRead);

  /// Add a delete access path to the traversed statment
  void addDeleteAccessPath(AccessPath *AccessPath);

  /// Perform checks that confirms that the body of the function with treefuser
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
  FunctionAnalyzer(clang::FunctionDecl *FuncDeclaration);

  /// Return the top level statements in the body of the function
  std::vector<StatementInfo *> &getStatements() {
    return Statements;
  }

  /// Dump information about the analyzed function
  void dump();

  /// Return true if the field is recursively visited by the function
  bool isInCalledChildList(clang::FieldDecl *ChildDecl) const;

  /// Return true
  bool isInChildList(clang::ValueDecl *Decl) const;

  bool isValidFuse() const { return SemanticsSatasified; }

  int getNumberOfRecursiveCalls() const;

  clang::FunctionDecl *getFunctionDecl() const;

  const clang::VarDecl *getTraversedNodeDecl() const;

  const clang::RecordDecl *getTraversedTreeTypeDecl() const;

  const vector<clang::FieldDecl *> &getCalledChildrenList() const;

  void addCalledChildList(clang::FieldDecl *childDecl) {
    CalledChildrenOrderedList.push_back(childDecl);
  }

  ~FunctionAnalyzer();
};

#endif
