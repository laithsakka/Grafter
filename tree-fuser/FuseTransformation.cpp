//===--- FuseTransformation.cpp -------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

#include "FuseTransformation.h"
#include "DependenceAnalyzer.h"
#include "DependenceGraph.h"

bool FusionCandidatesFinder::VisitFunctionDecl(
    const clang::FunctionDecl *FuncDecl) {
  CurrentFuncDecl = FuncDecl;
  return true;
}

bool FusionCandidatesFinder::VisitCompoundStmt(
    const CompoundStmt *CompoundStmt) {

  std::vector<clang::CallExpr *> Candidate;

  for (auto *InnerStmt : CompoundStmt->body()) {

    if (InnerStmt->getStmtClass() != Stmt::CallExprClass) {
      if (Candidate.size() > 1)
        FusionCandidates[CurrentFuncDecl].push_back(Candidate);

      Candidate.clear();
      continue;
    }

    auto *CurrentCallStmt = dyn_cast<clang::CallExpr>(InnerStmt);
    if (Candidate.size() == 0) {
      if (areCompatibleCalls(CurrentCallStmt, CurrentCallStmt)) {
        Candidate.push_back(CurrentCallStmt);
      }
      continue;
    }

    if (areCompatibleCalls(Candidate[0], CurrentCallStmt)) {
      Candidate.push_back(CurrentCallStmt);
    } else {
      if (Candidate.size() > 1)
        FusionCandidates[CurrentFuncDecl].push_back(Candidate);

      Candidate.clear();
    }
  }

  if (Candidate.size() > 1)
    FusionCandidates[CurrentFuncDecl].push_back(Candidate);

  Candidate.clear();
  return true;
}

bool FusionCandidatesFinder::areCompatibleCalls(clang::CallExpr *Call1,
                                                clang::CallExpr *Call2) {

  if (Call1->getCalleeDecl() == nullptr || Call2->getCallee() == nullptr)
    return false;

  auto *Decl1 = dyn_cast<clang::FunctionDecl>(Call1->getCalleeDecl());
  auto *Decl2 = dyn_cast<clang::FunctionDecl>(Call2->getCalleeDecl());

  if (!FunctionsInformation->isValidFuse(Decl1) ||
      !FunctionsInformation->isValidFuse(Decl2))
    return false;

  // Operate on the same tree
  AccessPath TraversalRoot1(Call1->getArg(0), nullptr); // dummmy access path
  AccessPath TraversalRoot2(Call2->getArg(0), nullptr); // dummmy access path

  if (TraversalRoot1.SplittedAccessPath.size() !=
      TraversalRoot2.SplittedAccessPath.size())
    return false;

  for (int i = 0; i < TraversalRoot1.SplittedAccessPath.size(); i++) {
    if (TraversalRoot1.SplittedAccessPath[i].second !=
        TraversalRoot2.SplittedAccessPath[i].second)
      return false;
  }
  return true;
}

FusionTransformer::FusionTransformer(ASTContext *Ctx,
                                     FunctionsFinder *FunctionsInfo) {
  Rewriter.setSourceMgr(Ctx->getSourceManager(), Ctx->getLangOpts());
  this->Ctx = Ctx;
  this->FunctionsInformation = FunctionsInfo;
  this->Synthesizer = new TraversalSynthesizer(Ctx, Rewriter, this);
}

void FusionTransformer::performFusion(
    const vector<clang::CallExpr *> &Candidate, bool IsTopLevel,
    const clang::FunctionDecl
        *EnclosingFunctionDecl /*just needed fo top level*/) {

  // Check if function is generated before
  if (!Synthesizer->isGenerated(Candidate)) {

    Logger::getStaticLogger().logInfo("Creating DG for a candidate");
    DependenceGraph *DepGraph = DepAnalyzer.createDependnceGraph(Candidate);
    DepGraph->dump();

    peformGreedyFusion(DepGraph);

    DepGraph->dumpMergeInfo();

    // Check that fusion was correctly made
    assert(!DepGraph->hasCycle() && "dep graph has cycle");
    assert(!DepGraph->hasWrongFuse() && "dep graph has wrong merging");

    // Generate a topological sort
    Logger::getStaticLogger().logDebug("Generating topological sort");

    std::vector<DG_Node *> ToplogicalOrder = findToplogicalOrder(DepGraph);

    Synthesizer->generateWriteBackInfo(Candidate, ToplogicalOrder);
  }
  if (IsTopLevel) {
    Synthesizer->WriteUpdates(Candidate, EnclosingFunctionDecl);
  }
}

void FusionTransformer::peformGreedyFusion(DependenceGraph *DepGraph) {
  unordered_map<clang::FieldDecl *, vector<DG_Node *>> ChildToCallers;
  for (auto *Node : DepGraph->getNodes()) {
    if (Node->getStatementInfo()->isCallStmt()) {
      ChildToCallers[Node->getStatementInfo()->getCalledChild()].push_back(
          Node);
    }
  }

  LLVM_DEBUG(for (auto &Entry
                  : ChildToCallers) {
    outs() << Entry.first->getNameAsString() << ":" << Entry.second.size()
           << "\n";
  });
  vector<unordered_map<clang::FieldDecl *, vector<DG_Node *>>::iterator>
      IteratorsList;

  for (auto It = ChildToCallers.begin(); It != ChildToCallers.end(); It++)
    IteratorsList.push_back(It);

  srand(time(nullptr));
  // random_shuffle(itList.begin(), itList.end());
  // random_shuffle(itList.begin(), itList.end());
  // random_shuffle(itList.begin(), itList.end());

  for (auto It : IteratorsList) {
    vector<DG_Node *> &CallNodes = It->second;

    // reverse(CallNodes.begin(), CallNodes.end());
    // random_shuffle(nodeLst.begin(), nodeLst.end());
    // random_shuffle(nodeLst.begin(), nodeLst.end());
    // random_shuffle(nodeLst.begin(), nodeLst.end());

    for (int i = 0; i < CallNodes.size(); i++) {
      if (CallNodes[i]->isMerged())
        continue;

      for (int j = i + 1; j < CallNodes.size(); j++) {
        if (CallNodes[j]->isMerged())
          continue;

        DepGraph->merge(CallNodes[i], CallNodes[j]);

        if (DepGraph->hasCycle() ||
            DepGraph->hasWrongFuse(CallNodes[i]->getMergeInfo())) {
          LLVM_DEBUG(outs()
                     << "rollback on merge, " << DepGraph->hasCycle() << ","
                     << DepGraph->hasWrongFuse(CallNodes[i]->getMergeInfo())
                     << "\n");

          DepGraph->unmerge(CallNodes[j]);
        }
      }
    }
  }
}

void FusionTransformer::findToplogicalOrderRec(
    vector<DG_Node *> &TopOrder, unordered_map<DG_Node *, bool> &Visited,
    DG_Node *Node) {
  if (!Node->allPredesVisited(Visited))
    return;

  TopOrder.push_back(Node);
  if (!Node->isMerged()) {
    Visited[Node] = true;
    for (auto &SuccDep : Node->getSuccessors()) {
      if (!Visited[SuccDep.first])
        findToplogicalOrderRec(TopOrder, Visited, SuccDep.first);
    }
    return;
  }

  // Handle merged node
  for (auto *MergedNode : Node->getMergeInfo()->MergedNodes) {
    assert(!Visited[MergedNode]);
    Visited[MergedNode] = true;
  }

  for (auto *MergedNode : Node->getMergeInfo()->MergedNodes) {
    for (auto &SuccDep : MergedNode->getSuccessors()) {

      if (Node->getMergeInfo()->isInMergedNodes(SuccDep.first))
        continue;

      // WRONG ASSERTION
      if (!Visited[SuccDep.first])
        findToplogicalOrderRec(TopOrder, Visited, SuccDep.first);
    }
  }
}

std::vector<DG_Node *>
FusionTransformer::findToplogicalOrder(DependenceGraph *DepGraph) {
  std::unordered_map<DG_Node *, bool> Visited;
  std::vector<DG_Node *> Order;

  bool AllVisited = false;
  while (!AllVisited) {
    AllVisited = true;
    for (auto *Node : DepGraph->getNodes()) {
      if (!Visited[Node]) {
        AllVisited = false;
        findToplogicalOrderRec(Order, Visited, Node);
      }
    }
  }
  return Order;
}
