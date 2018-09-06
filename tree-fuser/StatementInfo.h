//===--- StatementInfo.h ---------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#ifndef TREE_FUSER_STATMENT_INFO
#define TREE_FUSER_STATMENT_INFO

#include "FunctionAnalyzer.h"
#include <stdio.h>

class StatementInfo {
private:
  /// A unique id for the statment within the traversal body
  int StatementId;

  /// Data associated with the function that contains the statment
  FunctionAnalyzer *EnclosingFunction;

  /// Determine if the statment has a return inside it
  bool HasReturn = false;

  /// Determine if the statment is a traversing call
  bool IsCallStmt;

  /// The traversed child for call statmetnts
  clang::FieldDecl *CalledChild;

  /// Stores the accesses of the statments
  AccessPathContainer AccessPaths;

public:
  /// Clang ast node that represents the statment
  const clang::Stmt *Stmt;

  /// Return the statment id
  int getStatementId() {
    return StatementId;
  }

  /// Return true if the statment has return statement
  bool hasReturn() {
    return HasReturn;
  }

  void setHasReturn(bool NewValue) {
    HasReturn = NewValue;
  }

  /// Return true if the statment is recursive call
  bool isCallStmt() {
    return IsCallStmt;
  }

  /// Return the called child of a call statment
  clang::FieldDecl *getCalledChild() {
    assert(IsCallStmt);
    return CalledChild;
  }

  void setCalledChild(clang::FieldDecl *CalledChildNewValue) {
    CalledChild = CalledChildNewValue;
  }

  /// Return the enclosing function
  FunctionAnalyzer *getEnclosingFunction() {
    return EnclosingFunction;
  }

  /// Return the access paths
  AccessPathContainer& getAccessPaths(){
    return AccessPaths;
  }

  StatementInfo(const clang::Stmt *Stmt, FunctionAnalyzer *EnclosingFunction,
               bool IsCallStmt, int StatementId) {
    this->IsCallStmt = IsCallStmt;
    this->EnclosingFunction = EnclosingFunction;
    this->Stmt = Stmt;
    this->StatementId = StatementId;
  }
};

#endif
