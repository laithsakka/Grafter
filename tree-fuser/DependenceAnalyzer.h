//===--- DependenceAnalyzer.h ---------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This class is responsible to do build the dependence graph.
// this process is the most time consuming during in the transformation
//  and it is not written in the most efficient way!.
//===----------------------------------------------------------------------===//

#ifndef TREE_FUSER_DEPENDENCE_ANALYZER
#define TREE_FUSER_DEPENDENCE_ANALYZER

#include "AccessPath.h"
#include "DependenceGraph.h"
#include "FunctionAnalyzer.h"
#include <stdio.h>

class DependenceAnalyzer {

public:
  DependenceGraph *createDependnceGraph(vector<FunctionAnalyzer *> Traversals);

private:
  /// Analyze and add dependences between nodes within the same traversal
  void addIntraTraversalDependecies(
      DependenceGraph *Graph, FunctionAnalyzer *Traversal,
      unordered_map<StatementInfo *, DG_Node *> &StmtToGraphNode);

  /// Analyze and add dependences between nodes from different traversal
  void addInterTraversalDependecies(
      DependenceGraph *Graph, FunctionAnalyzer *Traversal1,
      FunctionAnalyzer *Traversal2,
      unordered_map<StatementInfo *, DG_Node *> &StmtToGNodeT1,
      unordered_map<StatementInfo *, DG_Node *> &StmtToGNodeT2);

  /// Add a dependence between two statement nodes with out considering possible
  /// extended accesses
  void addStmtToStmtDependencies(DependenceGraph *Graph, DG_Node *S1Node,
                                 DG_Node *S2Node,
                                 const AccessPathSet &S1Accesses,
                                 const AccessPathSet &S2Accesses, bool Intra);

  /// Return true if two access paths access have the same value part(even if
  /// the accessed object is on different nodes)
  // bool haveSameValuePart(AccessPath *AP1, AccessPath *AP2);

  /// Add dependence between from a call to a statement node considering all
  /// extended accesses of the call
  void addStmtToCallDeps(DependenceGraph *Graph, DG_Node *StmtNode,
                         DG_Node *CallNode);

  /// Add dependence between from a statement to a call node considering all
  /// extended accesses of the call
  void addCallToStmtDeps(DependenceGraph *Graph, DG_Node *CallNode,
                         DG_Node *StmtNode);

  /// Add dependence between two call statements considering all extended
  /// accesses of the calls
  void addCallToCallDeps(DependenceGraph *Graph, DG_Node *C1Node,
                         DG_Node *C2Node, bool WithinSameTraversal);
};
#endif /* DependenceAnalyzer_hpp */
