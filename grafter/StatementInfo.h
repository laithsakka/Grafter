//===--- StatementInfo.h --------------------------------------------------===//
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
#include<stack>

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
  clang::FieldDecl *CalledChild = nullptr;

  /// Data associated with the function that is called
  clang::FunctionDecl *CalledFunction = nullptr;

  /// Stores the accesses of the statments
  AccessPathContainer AccessPaths;

  /// Automata that represents the local write access-paths in the statement
  FSM *LocalWritesAutomata = nullptr;

  /// Automata that represents the local read access-paths in the statement
  FSM *LocalReadsAutomata = nullptr;

  /// Automata that represents the global write access-paths in the statement
  FSM *BaseGlobalWritesAutomata = nullptr;

  /// Automata that represents the global read access-paths in the statement
  FSM *BaseGlobalReadsAutomata = nullptr;

  /// Automata that represents the on-tree write access-paths in the statements
  FSM *BaseTreeWritesAutomata = nullptr;

  /// Automata that represents the on-tree read access-paths in the statement
  FSM *BaseTreeReadsAutomata = nullptr;

  /// Automata that includes the on-tree read access-paths during the
  /// invocations of call statement
  FSM *ExtendedTreeReadsAutomata = nullptr;

  /// Automata that includes the on-tree write access-paths during the
  /// invocations of call statement
  FSM *ExtendedTreeWritesAutomata = nullptr;

  /// Automata that includes the global read access-paths during the
  /// invocations of call statement
  FSM *ExtendedGlobalReadsAutomata = nullptr;

  /// Automata that includes the global write access-paths during the
  /// invocations of call statement
  FSM *ExtendedGlobalWritesAutomata = nullptr;

  const FSM &getExtendedTreeReadsAutomata();

  const FSM &getExtendedTreeWritesAutomata();

  const FSM &getExtendedGlobReadsAutomata();

  const FSM &getExtendedGlobWritesAutomata();

public:
  clang::FunctionDecl * getCalledFunction() const{
    return CalledFunction;
  }
  /// Set the called function
  void setCalledFunction(clang::FunctionDecl * CalledFunc){
    CalledFunction = CalledFunc->getDefinition();
  }

  /// Clang ast node that represents the statement
  clang::Stmt *Stmt;

  /// Return the statement id
  int getStatementId() { return StatementId; }

  /// Return true if the statement has return statement
  bool hasReturn() { return HasReturn; }

  void setHasReturn(bool NewValue) { HasReturn = NewValue; }

  /// Return true if the statement is recursive call
  bool isCallStmt() { return IsCallStmt; }

  /// Return the called child of a call statement
  clang::FieldDecl *getCalledChild() {
    assert(IsCallStmt);
    return CalledChild;
  }

  void setCalledChild(clang::FieldDecl *CalledChildNewValue) {
    CalledChild = CalledChildNewValue;
  }

  /// Return the enclosing function
  FunctionAnalyzer *getEnclosingFunction() { return EnclosingFunction; }

  /// Return the access paths
  AccessPathContainer &getAccessPaths() { return AccessPaths; }

  StatementInfo(clang::Stmt *Stmt, FunctionAnalyzer *EnclosingFunction,
                bool IsCallStmt, int StatementId) {
    this->IsCallStmt = IsCallStmt;
    this->EnclosingFunction = EnclosingFunction;
    this->Stmt = Stmt;
    this->StatementId = StatementId;
  }

  const FSM &getLocalWritesAutomata();

  const FSM &getLocalReadsAutomata();

  const FSM &getGlobWritesAutomata(bool IncludeExtended = true);

  const FSM &getGlobReadsAutomata(bool IncludeExtended = true);

  const FSM &getTreeReadsAutomata(bool IncludeExtended = true);

  const FSM &getTreeWritesAutomata(bool IncludeExtended = true);
};

#endif
