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
  DependenceGraph *createDependenceGraph(const vector<clang::CallExpr *> &Calls,
                                        bool HasVirtualCall,
                                        const clang::CXXRecordDecl *TraversedType);

private:
  DependenceGraph *
  createDependenceGraph(const vector<FunctionAnalyzer *> &Traversals);

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
};
#endif /* DependenceAnalyzer_hpp */
