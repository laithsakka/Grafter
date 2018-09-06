//===--- DependenceAnalyzer.h ---------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "DependenceAnalyzer.h"

// TODO change this to input configuration
#define ENABLE_CODE_MOTION 1

bool haveSameValuePart(AccessPath *AP1, AccessPath *AP2) {

  if (AP1->isStrictAccessCall() && AP2->isStrictAccessCall())
    return AP1->getAnnotationInfo().Id == AP2->getAnnotationInfo().Id;

  if (AP1->isStrictAccessCall() || AP2->isStrictAccessCall())
    return false;

  // If they both refer to node  this function will assume true for
  // this case TODO : hmmmm
  if (AP1->hasValuePart() == false && AP2->hasValuePart() == false)
    return true;

  if (AP1->hasValuePart() == false && AP2->hasValuePart() == true)
    return false;

  if (AP1->hasValuePart() == true && AP2->hasValuePart() == false)
    return false;

  // If  one of them is prefix of the other then there is conflict there

  auto *Longer = AP1->getValuePathSize() >= AP2->getValuePathSize() ? AP1 : AP2;
  auto *Shorter = Longer == AP1 ? AP2 : AP1;
  auto ShorterIdx = Shorter->getValueStartIndex();
  auto LongerIdx = Longer->getValueStartIndex();
  auto ShorterLastIndex =
      Shorter->getValueStartIndex() + Shorter->getValuePathSize() - 1;

  while (ShorterIdx <= ShorterLastIndex) {
    if (Shorter->getDeclAtIndex(ShorterIdx) !=
        Longer->getDeclAtIndex(LongerIdx))
      return false;

    ShorterIdx++;
    LongerIdx++;
  }
  return true;
}

bool mayConflict(AccessPath *AP1, AccessPath *AP2) {

  // if (AP1->isGlobal() && !AP2->isGlobal())
  //   continue;
  if (AP1->isGlobal() != AP2->isGlobal())
    return false;

  // if (AP1->isOnTree() && !(*AP2)->isOnTree())
  //   continue;
  if (AP1->isOnTree() != AP2->isOnTree())
    return false;

  if (!haveSameValuePart(AP1, AP2))
    return false;

  return true;
}

// P1 == {child}*AP2
bool conflictWithExtenededAccessPath(AccessPath *Absolute, AccessPath *Extended,
                                     FunctionAnalyzer *ExtendedTraversal) {

  // if (AP1->getTreeAccessPathSize() > AP2->getTreeAccessPathSize())
  //   return false;
  if (Absolute->getTreeAccessPathSize() < Extended->getTreeAccessPathSize())
    return false;

  // AP2 must be suffix of AP1
  int Idx1 = Absolute->getTreeAccessPathEndIndex();
  int Idx2 = Extended->getTreeAccessPathEndIndex();
  for (; Idx2 != 0; Idx2--, Idx1--) {
    if (Absolute->SplittedAccessPath[Idx1].second !=
        Extended->SplittedAccessPath[Idx2].second)
      return false;
  }

  // The remaining part(AP1 -AP2) must be in AP2Traversal children
  while (Idx1 >= 1) {
    if (!ExtendedTraversal->isInCalledChildList(dyn_cast<clang::FieldDecl>(
            Absolute->SplittedAccessPath[Idx1].second))) {
      return false;
    }
    Idx1--;
  }
  return true;
}

// APInStmt == CalledChild.{child}*.APInCall
void addCallToStmtDeps_Helper(DependenceGraph *DepGraph, AccessPath *APInStmt,
                              DG_Node *StmtNode, AccessPath *APInCall,
                              DG_Node *CallNode) {
  if (!mayConflict(APInStmt, APInCall))
    return;

  // ignore locals (each itteration has its own local scope)
  if (APInStmt->isLocal() || APInCall->isLocal())
    return;

  if (APInStmt->isOffTree()) {
    assert(APInCall->isOffTree());
    DepGraph->addDependency(GLOBAL_DEP, CallNode, StmtNode);
    return;
  }

  if (APInCall->getTreeAccessPathSize() >= APInStmt->getTreeAccessPathSize())
    return;

  // APInStmt must start with the called child for
  if (APInStmt->SplittedAccessPath[1].second !=
      CallNode->getStatementInfo()->getCalledChild())
    return;

  if (conflictWithExtenededAccessPath(
          APInStmt, APInCall,
          CallNode->getStatementInfo()->getEnclosingFunction()))
    DepGraph->addDependency(ONTREE_DEP, CallNode, StmtNode);
}

// APInStmt == CalledChild.{child}*.APInCall
void addStmtToCallDeps_Helper(DependenceGraph *DepGraph, AccessPath *APInStmt,
                              DG_Node *StmtNode, AccessPath *APInCall,
                              DG_Node *CallNode) {

  // C.{child in CallTraversal}*.{AP1 in CallTraversal}= AP2 iff ontree
  // AP1=AP2 if offtree

  if (!mayConflict(APInStmt, APInCall))
    return;

  // Ignore locals each since iteration has its own local scope
  if (APInStmt->isLocal() || APInCall->isLocal())
    return;

  if (APInStmt->isOffTree()) {
    assert(APInCall->isOffTree());
    DepGraph->addDependency(GLOBAL_DEP, StmtNode, CallNode);
    return;
  }

  if (APInCall->getTreeAccessPathSize() >= APInStmt->getTreeAccessPathSize())
    return;

  // APInStmt must start with the called child for
  if (APInStmt->SplittedAccessPath[1].second !=
      CallNode->getStatementInfo()->getCalledChild())
    return;

  if (conflictWithExtenededAccessPath(
          APInStmt, APInCall,
          CallNode->getStatementInfo()->getEnclosingFunction()))
    DepGraph->addDependency(ONTREE_DEP, StmtNode, CallNode);
}

void addCallToCallDeps_Helper(DependenceGraph *DepGraph, AccessPath *AP1,
                              DG_Node *CallNode1, AccessPath *AP2,
                              DG_Node *CallNode2, bool WithinSameTraversal) {
  assert(CallNode1->getStatementInfo()->isCallStmt() &&
         CallNode2->getStatementInfo()->isCallStmt());

  auto *Traversal1 = CallNode1->getStatementInfo()->getEnclosingFunction();
  auto *Traversal2 = CallNode2->getStatementInfo()->getEnclosingFunction();

  if (!mayConflict(AP1, AP2))
    return;

  // Ignore locals since each iteration has its own local scope
  if (AP1->isLocal() || AP2->isLocal())
    return;

  if (AP1->isOffTree()) {
    DepGraph->addDependency(GLOBAL_DEP, CallNode1, CallNode2);
    return;
  }

  if (CallNode1->getStatementInfo()->getCalledChild() !=
      CallNode2->getStatementInfo()->getCalledChild())
    return;

  if (conflictWithExtenededAccessPath(AP1, AP2, Traversal2) ||
      conflictWithExtenededAccessPath(AP2, AP1, Traversal1)) {
    if (!WithinSameTraversal)
      DepGraph->addDependency(ONTREE_DEP_FUSABLE, CallNode1, CallNode2);
    else{
      outs()<<"here\n";
      DepGraph->addDependency(ONTREE_DEP, CallNode1, CallNode2);
    }
  }
}

DependenceGraph *DependenceAnalyzer::createDependnceGraph(
    std::vector<FunctionAnalyzer *> Traversals) {

  // Lookup graph nodes using traversal index and StatementInfo*
  std::unordered_map<int, std::unordered_map<StatementInfo *, DG_Node *>>
      GraphNodeLookup;

  DependenceGraph *DepGraph = new DependenceGraph();
  for (int i = 0; i < Traversals.size(); i++) {
    for (auto *Stmt : Traversals[i]->getStatements()) {
      auto Pair = make_pair(Stmt, i);
      GraphNodeLookup[i][Stmt] = DepGraph->createNode(Pair);
    }
  }

  for (int i = 0; i < Traversals.size(); i++) {
    addIntraTraversalDependecies(DepGraph, Traversals[i], GraphNodeLookup[i]);
  }
  for (int i = 0; i < Traversals.size(); i++) {
    for (int j = i + 1; j < Traversals.size(); j++)
      addInterTraversalDependecies(DepGraph, Traversals[i], Traversals[j],
                                   GraphNodeLookup[i], GraphNodeLookup[j]);
  }

  return DepGraph;
}

void DependenceAnalyzer::addCallToStmtDeps(DependenceGraph *DepGraph,
                                           DG_Node *CallNode,
                                           DG_Node *StmtNode) {
  assert(CallNode->getStatementInfo()->isCallStmt());

  StatementInfo *StmtInfo = StmtNode->getStatementInfo();
  FunctionAnalyzer *CallTraversal =
      CallNode->getStatementInfo()->getEnclosingFunction();

  for (auto *StmtInCall : CallTraversal->getStatements()) {

    for (auto *APInStmt : StmtInfo->getAccessPaths().getReadSet())
      for (auto *APInCall : StmtInCall->getAccessPaths().getWriteSet())
        addCallToStmtDeps_Helper(DepGraph, APInStmt, StmtNode, APInCall,
                                 CallNode);

    // W-W,R
    for (auto *APInStmt : StmtInfo->getAccessPaths().getWriteSet()) {

      for (auto *APInCall : StmtInCall->getAccessPaths().getReadSet())
        addCallToStmtDeps_Helper(DepGraph, APInStmt, StmtNode, APInCall,
                                 CallNode);

      for (auto APInCall : StmtInCall->getAccessPaths().getWriteSet())
        addCallToStmtDeps_Helper(DepGraph, APInStmt, StmtNode, APInCall,
                                 CallNode);
    }
  }
}

void DependenceAnalyzer::addStmtToCallDeps(DependenceGraph *DepGraph,
                                           DG_Node *StmtNode,
                                           DG_Node *CallNode) {
  assert(CallNode->getStatementInfo()->isCallStmt());

  StatementInfo *StmtInfo = StmtNode->getStatementInfo();
  FunctionAnalyzer *CallTraversal =
      CallNode->getStatementInfo()->getEnclosingFunction();

  for (auto *StmtInCall : CallTraversal->getStatements()) {

    for (auto *APInStmt : StmtInfo->getAccessPaths().getReadSet()) {
      for (auto *APInCall : StmtInCall->getAccessPaths().getWriteSet())
        addStmtToCallDeps_Helper(DepGraph, APInStmt, StmtNode, APInCall,
                                 CallNode);
    }
  }

  for (auto *APInStmt : StmtInfo->getAccessPaths().getWriteSet()) {
    for (auto *StmtInCall : CallTraversal->getStatements()) {

      for (auto *APInCall : StmtInCall->getAccessPaths().getReadSet())
        addStmtToCallDeps_Helper(DepGraph, APInStmt, StmtNode, APInCall,
                                 CallNode);

      for (auto APInCall : StmtInCall->getAccessPaths().getWriteSet())
        addStmtToCallDeps_Helper(DepGraph, APInStmt, StmtNode, APInCall,
                                 CallNode);
    }
  }
}

// this is wrong guys
void DependenceAnalyzer::addCallToCallDeps(DependenceGraph *DepGraph,
                                           DG_Node *CallNode1,
                                           DG_Node *CallNode2,
                                           bool WithinSameTraversal) {

  auto *Traversal1 = CallNode1->getStatementInfo()->getEnclosingFunction();
  auto *Traversal2 = CallNode2->getStatementInfo()->getEnclosingFunction();

  for (auto *Stmt1 : Traversal1->getStatements()) {
    for (auto *Stmt2 : Traversal2->getStatements()) {

      // W-W,R
      for (auto *AP1 : Stmt1->getAccessPaths().getWriteSet()) {
        for (auto *AP2 : Stmt2->getAccessPaths().getWriteSet())
          addCallToCallDeps_Helper(DepGraph, AP1, CallNode1, AP2, CallNode2,
                                   WithinSameTraversal);

        for (auto *AP2 : Stmt2->getAccessPaths().getReadSet())
          addCallToCallDeps_Helper(DepGraph, AP1, CallNode1, AP2, CallNode2,
                                   WithinSameTraversal);
      }

      for (auto *AP1 : Stmt1->getAccessPaths().getReadSet()) {
        for (auto *AP2 : Stmt2->getAccessPaths().getWriteSet())
          addCallToCallDeps_Helper(DepGraph, AP1, CallNode1, AP2, CallNode2,
                                   WithinSameTraversal);
      }
    }
  }
}

void DependenceAnalyzer::addStmtToStmtDependencies(
    DependenceGraph *DepGraph, DG_Node *StmtNode1, DG_Node *StmtNode2,
    const AccessPathSet &Stmt1Accesses, const AccessPathSet &Stmt2Accesses,
    bool WithinSameTraversal) {

  for (AccessPath *AP1 : Stmt1Accesses) {
    for (AccessPath *AP2 : Stmt2Accesses) {

      if (!mayConflict(AP1, AP2))
        continue;

      if (!WithinSameTraversal & (AP1->isLocal() || AP2->isLocal()))
        continue;

      if (AP1->isOffTree()) {
        DepGraph->addDependency(GLOBAL_DEP, StmtNode1, StmtNode2);
        continue;
      }

      // Check that tree access path is the same
      if (AP1->getTreeAccessPathSize() != AP2->getTreeAccessPathSize())
        continue;

      bool HaveSameTreeAccessPath = true;
      for (int Idx = 1; Idx <= AP1->getTreeAccessPathEndIndex(); Idx++) {
        if (AP1->SplittedAccessPath[Idx].second !=
            AP2->SplittedAccessPath[Idx].second) {
          HaveSameTreeAccessPath = false;
          break;
        }
      }
      if (HaveSameTreeAccessPath)
        DepGraph->addDependency(ONTREE_DEP, StmtNode1, StmtNode2);
    }
  }
}

void DependenceAnalyzer::addIntraTraversalDependecies(
    DependenceGraph *DepGraph, FunctionAnalyzer *Traversal,
    unordered_map<StatementInfo *, DG_Node *> &GraphNodes) {

  for (int i = 0; i < Traversal->getStatements().size(); i++) {
    auto *Stmt1 = Traversal->getStatements()[i];
    assert(Stmt1->getStatementId() == i);

    bool TraversalPerformsWrite = false;
    for (auto *Stmt : Traversal->getStatements()) {
      if (Stmt->getAccessPaths().getWriteSet().size() != 0) {
        TraversalPerformsWrite = true;
        break;
      }
    }

    for (int j = i + 1; j < Traversal->getStatements().size(); j++) {
      auto *Stmt2 = Traversal->getStatements()[j];
      assert(Stmt2->getStatementId() == j);

      if (ENABLE_CODE_MOTION == false) {
        DepGraph->addDependency(CONTROL_DEP, GraphNodes[Stmt1],
                                GraphNodes[Stmt2]);
      }

      bool Stmt2ReadsPtr = false; // OR read of pointer

      for (auto *AccessPath : Stmt2->getAccessPaths().getReadSet()) {
        if (AccessPath->isOnTree()) {
          Stmt2ReadsPtr = true;
          break;
        }
      }

      // Add control dependences
      if (Stmt1->hasReturn() && Stmt2ReadsPtr)
        DepGraph->addDependency(CONTROL_DEP, GraphNodes[Stmt1],
                                GraphNodes[Stmt2]);

      if (Stmt1->hasReturn() && !Stmt2->isCallStmt() &&
          Stmt2->getAccessPaths().getWriteSet().size() != 0)
        DepGraph->addDependency(CONTROL_DEP, GraphNodes[Stmt1],
                                GraphNodes[Stmt2]);

      if (Stmt2->hasReturn() && !Stmt1->isCallStmt() &&
          Stmt1->getAccessPaths().getWriteSet().size() != 0)
        DepGraph->addDependency(CONTROL_DEP, GraphNodes[Stmt1],
                                GraphNodes[Stmt2]);

      if (Stmt1->hasReturn() && Stmt2->isCallStmt() && TraversalPerformsWrite)
        DepGraph->addDependency(CONTROL_DEP, GraphNodes[Stmt1],
                                GraphNodes[Stmt2]);

      if (Stmt2->hasReturn() && Stmt1->isCallStmt() && TraversalPerformsWrite)
        DepGraph->addDependency(CONTROL_DEP, GraphNodes[Stmt1],
                                GraphNodes[Stmt2]);

      // Add data dependences
      addStmtToStmtDependencies(DepGraph, GraphNodes[Stmt1], GraphNodes[Stmt2],
                                Stmt1->getAccessPaths().getReadSet(),
                                Stmt2->getAccessPaths().getWriteSet(), true);

      addStmtToStmtDependencies(DepGraph, GraphNodes[Stmt1], GraphNodes[Stmt2],
                                Stmt1->getAccessPaths().getWriteSet(),
                                Stmt2->getAccessPaths().getReadSet(), true);

      addStmtToStmtDependencies(DepGraph, GraphNodes[Stmt1], GraphNodes[Stmt2],
                                Stmt1->getAccessPaths().getWriteSet(),
                                Stmt2->getAccessPaths().getWriteSet(), true);

      if (Stmt2->isCallStmt())
        addStmtToCallDeps(DepGraph, GraphNodes[Stmt1], GraphNodes[Stmt2]);

      if (Stmt1->isCallStmt())
        addCallToStmtDeps(DepGraph, GraphNodes[Stmt1], GraphNodes[Stmt2]);

      if (Stmt1->isCallStmt() && Stmt2->isCallStmt())
        addCallToCallDeps(DepGraph, GraphNodes[Stmt1], GraphNodes[Stmt2], true);
    }
  }
}

void DependenceAnalyzer::addInterTraversalDependecies(
    DependenceGraph *DepGraph, FunctionAnalyzer *Traversal1,
    FunctionAnalyzer *Traversal2,
    unordered_map<StatementInfo *, DG_Node *> &GraphNodesT1,
    unordered_map<StatementInfo *, DG_Node *> &GraphNodesT2) {

  for (auto *Stmt1 : Traversal1->getStatements()) {
    for (auto *Stmt2 : Traversal2->getStatements()) {

      addStmtToStmtDependencies(DepGraph, GraphNodesT1[Stmt1],
                                GraphNodesT2[Stmt2],
                                Stmt1->getAccessPaths().getReadSet(),
                                Stmt2->getAccessPaths().getWriteSet(), false);

      addStmtToStmtDependencies(DepGraph, GraphNodesT1[Stmt1],
                                GraphNodesT2[Stmt2],
                                Stmt1->getAccessPaths().getWriteSet(),
                                Stmt2->getAccessPaths().getReadSet(), false);

      addStmtToStmtDependencies(DepGraph, GraphNodesT1[Stmt1],
                                GraphNodesT2[Stmt2],
                                Stmt1->getAccessPaths().getWriteSet(),
                                Stmt2->getAccessPaths().getWriteSet(), false);

      if (Stmt2->isCallStmt())
        addStmtToCallDeps(DepGraph, GraphNodesT1[Stmt1], GraphNodesT2[Stmt2]);

      if (Stmt1->isCallStmt())
        addCallToStmtDeps(DepGraph, GraphNodesT1[Stmt1], GraphNodesT2[Stmt2]);

      if (Stmt1->isCallStmt() && Stmt2->isCallStmt())
        addCallToCallDeps(DepGraph, GraphNodesT1[Stmt1], GraphNodesT2[Stmt2],
                          false);
    }
  }
}
