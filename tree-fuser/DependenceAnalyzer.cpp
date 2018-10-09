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

DependenceGraph *DependenceAnalyzer::createDependnceGraph(
    const std::vector<clang::CallExpr *> &Calls) {

  std::vector<FunctionAnalyzer *> Temp;
  Temp.resize(Calls.size());
  transform(Calls.begin(), Calls.end(), Temp.begin(),
            [](clang::CallExpr *CallExpr) {
              auto *CalleeDecl =
                  dyn_cast<clang::FunctionDecl>(CallExpr->getCalleeDecl())
                      ->getDefinition();

              return FunctionsFinder::getFunctionInfo(CalleeDecl);
            });
  return createDependnceGraph(Temp);
}

DependenceGraph *DependenceAnalyzer::createDependnceGraph(
    const std::vector<FunctionAnalyzer *> &Traversals) {

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

void DependenceAnalyzer::addIntraTraversalDependecies(
    DependenceGraph *DepGraph, FunctionAnalyzer *Traversal,
    unordered_map<StatementInfo *, DG_Node *> &GraphNodes) {

  for (int i = 0; i < Traversal->getStatements().size(); i++) {
    auto *Stmt1 = Traversal->getStatements()[i];
    assert(Stmt1->getStatementId() == i);

    for (int j = i + 1; j < Traversal->getStatements().size(); j++) {
      auto *Stmt2 = Traversal->getStatements()[j];
      assert(Stmt2->getStatementId() == j);

      if (!ENABLE_CODE_MOTION)
        DepGraph->addDependency(CONTROL_DEP, GraphNodes[Stmt1],
                                GraphNodes[Stmt2]);

      // Add control dependences
      if (Stmt1->hasReturn() &&
          !FSMUtility::isEmpty(Stmt2->getTreeReadsAutomata()))
        DepGraph->addDependency(CONTROL_DEP, GraphNodes[Stmt1],
                                GraphNodes[Stmt2]);

      if (Stmt1->hasReturn())
        if (!FSMUtility::isEmpty(Stmt2->getTreeWritesAutomata()) ||
            !FSMUtility::isEmpty(Stmt2->getLocalWritesAutomata()) ||
            !FSMUtility::isEmpty(Stmt2->getGlobWritesAutomata()))
          DepGraph->addDependency(CONTROL_DEP, GraphNodes[Stmt1],
                                  GraphNodes[Stmt2]);

      if (Stmt2->hasReturn())
        if (!FSMUtility::isEmpty(Stmt1->getTreeWritesAutomata()) ||
            !FSMUtility::isEmpty(Stmt1->getLocalWritesAutomata()) ||
            !FSMUtility::isEmpty(Stmt1->getGlobWritesAutomata()))
          DepGraph->addDependency(CONTROL_DEP, GraphNodes[Stmt1],
                                  GraphNodes[Stmt2]);

      // Add data dependences

      // Check Global conflicts
      if (FSMUtility::hasNonEmptyIntersection(Stmt1->getGlobWritesAutomata(),
                                              Stmt2->getGlobWritesAutomata()) ||

          FSMUtility::hasNonEmptyIntersection(Stmt1->getGlobWritesAutomata(),
                                              Stmt2->getGlobReadsAutomata()) ||

          FSMUtility::hasNonEmptyIntersection(Stmt1->getGlobReadsAutomata(),
                                              Stmt2->getGlobWritesAutomata())) {

        DepGraph->addDependency(GLOBAL_DEP, GraphNodes[Stmt1],
                                GraphNodes[Stmt2]);
      }

      // Check OnTree conflicts
      if (FSMUtility::hasNonEmptyIntersection(Stmt1->getTreeWritesAutomata(),
                                              Stmt2->getTreeWritesAutomata()) ||

          FSMUtility::hasNonEmptyIntersection(Stmt1->getTreeWritesAutomata(),
                                              Stmt2->getTreeReadsAutomata()) ||

          FSMUtility::hasNonEmptyIntersection(Stmt1->getTreeReadsAutomata(),
                                              Stmt2->getTreeWritesAutomata())) {

        DepGraph->addDependency(ONTREE_DEP, GraphNodes[Stmt1],
                                GraphNodes[Stmt2]);
      }

      //  Check local conflicts
      if (FSMUtility::hasNonEmptyIntersection(
              Stmt1->getLocalWritesAutomata(),
              Stmt2->getLocalWritesAutomata()) ||

          FSMUtility::hasNonEmptyIntersection(Stmt1->getLocalWritesAutomata(),
                                              Stmt2->getLocalReadsAutomata()) ||
          FSMUtility::hasNonEmptyIntersection(
              Stmt1->getLocalReadsAutomata(),
              Stmt2->getLocalWritesAutomata())) {

        DepGraph->addDependency(LOCAL_DEP, GraphNodes[Stmt1],
                                GraphNodes[Stmt2]);
      }
    }
  }
}

void DependenceAnalyzer::addInterTraversalDependecies(
    DependenceGraph *DepGraph, FunctionAnalyzer *Traversal1,
    FunctionAnalyzer *Traversal2,
    unordered_map<StatementInfo *, DG_Node *> &GraphNodesT1,
    unordered_map<StatementInfo *, DG_Node *> &GraphNodesT2) {

  for (auto *Stmt1 : Traversal1->getStatements()) {
    // FSMUtility::print(Stmt1->getTreeWritesAutomata(),
    //                   to_string(Stmt1->getStatementId()) + "w", 1);
    // FSMUtility::print(Stmt1->getTreeReadsAutomata(),
    //                   to_string(Stmt1->getStatementId()) + "r", 1);

    for (auto *Stmt2 : Traversal2->getStatements()) {

      // Check Global conflicts
      if (FSMUtility::hasNonEmptyIntersection(Stmt1->getGlobWritesAutomata(),
                                              Stmt2->getGlobWritesAutomata()) ||

          FSMUtility::hasNonEmptyIntersection(Stmt1->getGlobWritesAutomata(),
                                              Stmt2->getGlobReadsAutomata()) ||

          FSMUtility::hasNonEmptyIntersection(Stmt1->getGlobReadsAutomata(),
                                              Stmt2->getGlobWritesAutomata())) {

        DepGraph->addDependency(GLOBAL_DEP, GraphNodesT1[Stmt1],
                                GraphNodesT2[Stmt2]);
      }

      // Check OnTree conflicts
      if (FSMUtility::hasNonEmptyIntersection(Stmt1->getTreeWritesAutomata(),
                                              Stmt2->getTreeWritesAutomata()) ||

          FSMUtility::hasNonEmptyIntersection(Stmt1->getTreeWritesAutomata(),
                                              Stmt2->getTreeReadsAutomata()) ||

          FSMUtility::hasNonEmptyIntersection(Stmt1->getTreeReadsAutomata(),
                                              Stmt2->getTreeWritesAutomata())) {

        DepGraph->addDependency(ONTREE_DEP, GraphNodesT1[Stmt1],
                                GraphNodesT2[Stmt2]);
      }
    }
  }
}
