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

extern llvm::cl::OptionCategory TreeFuserCategory;
namespace opts {
llvm::cl::opt<unsigned>
    MaxMergedInstances("max-merged-f",
                       cl::desc("a maximum number of calls for the same "
                                "function that can be fused together"),
                       cl::init(5), cl::ZeroOrMore, cl::cat(TreeFuserCategory));
llvm::cl::opt<unsigned>
    MaxMergedNodes("max-merged-n",
                   cl::desc("a maximum number of  that can be fused together"),
                   cl::init(5), cl::ZeroOrMore, cl::cat(TreeFuserCategory));
} // namespace opts

bool FusionCandidatesFinder::VisitFunctionDecl(clang::FunctionDecl *FuncDecl) {
  CurrentFuncDecl = FuncDecl;
  return true;
}

bool FusionCandidatesFinder::VisitCompoundStmt(
    const CompoundStmt *CompoundStmt) {

  std::vector<clang::CallExpr *> Candidate;

  for (auto *InnerStmt : CompoundStmt->body()) {

    if (InnerStmt->getStmtClass() != Stmt::CallExprClass &&
        InnerStmt->getStmtClass() != Stmt::CXXMemberCallExprClass) {

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

AccessPath extractVisitedChild(clang::CallExpr *Call) {
  if (Call->getStmtClass() == clang::Stmt::CXXMemberCallExprClass) {
    auto *ExprCallRemoved =
        Call->child_begin()->child_begin()->IgnoreImplicit();

    if (ExprCallRemoved->getStmtClass() == clang::Stmt::DeclRefExprClass)
      return AccessPath(dyn_cast<clang::DeclRefExpr>(ExprCallRemoved), nullptr);
    else if (ExprCallRemoved->getStmtClass() == clang::Stmt::MemberExprClass)
      return AccessPath(dyn_cast<clang::MemberExpr>(ExprCallRemoved), nullptr);
    else
      llvm_unreachable("unsupported case");

  } else {
    return AccessPath(Call->getArg(0), nullptr);
  }
}

bool FusionCandidatesFinder::areCompatibleCalls(clang::CallExpr *Call1,
                                                clang::CallExpr *Call2) {

  if (Call1->getCalleeDecl() == nullptr || Call2->getCallee() == nullptr)
    return false;

  auto *Decl1 = dyn_cast<clang::FunctionDecl>(Call1->getCalleeDecl());
  auto *Decl2 = dyn_cast<clang::FunctionDecl>(Call2->getCalleeDecl());

  if (!FunctionsInformation->isValidFuse(Decl1->getDefinition()) ||
      !FunctionsInformation->isValidFuse(Decl2->getDefinition()))
    return false;

  // visiting the same child
  AccessPath TraversalRoot1 = extractVisitedChild(Call1);
  AccessPath TraversalRoot2 = extractVisitedChild(Call2);

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
    clang::FunctionDecl *EnclosingFunctionDecl /*just needed fo top level*/) {

  bool HasVirtual = false;
  bool HasCXXMethod = false;

  for (auto *Call : Candidate) {
    auto *CalleeInfo = FunctionsFinder::getFunctionInfo(
        Call->getCalleeDecl()->getAsFunction()->getDefinition());
    if (CalleeInfo->isCXXMember())
      HasCXXMethod = true;
    if (CalleeInfo->isVirtual())
      HasVirtual = true;
  }
  AccessPath AP = extractVisitedChild(Candidate[0]);
  auto *TraversedType = AP.getDeclAtIndex(AP.SplittedAccessPath.size() - 1)
                            ->getType()
                            ->getPointeeCXXRecordDecl();

  auto fuseFunctions = [&](const CXXRecordDecl *DerivedType) {
    if (!Synthesizer->isGenerated(Candidate, HasVirtual, DerivedType)) {
      Logger::getStaticLogger().logInfo(
          "Generating Code for function " +
          Synthesizer->createName(Candidate, HasVirtual, DerivedType));

      Logger::getStaticLogger().logInfo("Creating DG for a candidate");

      DependenceGraph *DepGraph =
          DepAnalyzer.createDependenceGraph(Candidate, HasVirtual, DerivedType);

     // DepGraph->dump();

      performGreedyFusion(DepGraph);

      LLVM_DEBUG(DepGraph->dumpMergeInfo());

      // Check that fusion was correctly made
      assert(!DepGraph->hasCycle() && "dep graph has cycle");
      assert(!DepGraph->hasWrongFuse() && "dep graph has wrong merging");

      std::vector<DG_Node *> ToplogicalOrder = findToplogicalOrder(DepGraph);

      Synthesizer->generateWriteBackInfo(Candidate, ToplogicalOrder, HasVirtual,
                                         HasCXXMethod, DerivedType);
      Logger::getStaticLogger().logDebug("Code Generation Done ");
    }
  };
  if (HasVirtual) {
    fuseFunctions(TraversedType);
    for (auto *DerivedType : RecordsAnalyzer::DerivedRecords[TraversedType]) {
      fuseFunctions(DerivedType);
    }
  } else
    fuseFunctions(nullptr);

  if (IsTopLevel) {

    Synthesizer->WriteUpdates(Candidate, EnclosingFunctionDecl);
  }
}

void FusionTransformer::performGreedyFusion(DependenceGraph *DepGraph) {
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

        auto ReachMaxMerged = [&](MergeInfo *Info) {
          unordered_map<FunctionDecl *, int> Counter;
          for (auto *Node : Info->MergedNodes) {
            Counter[Node->getStatementInfo()
                        ->getCalledFunction()
                        ->getDefinition()]++;
            auto Count = Counter[Node->getStatementInfo()
                                     ->getCalledFunction()
                                     ->getDefinition()];

            if (Count > opts::MaxMergedInstances) {
              return true;
            }
          }
          return false;
        };

        if (CallNodes[i]->getMergeInfo()->MergedNodes.size() >
                opts::MaxMergedNodes ||
            ReachMaxMerged(CallNodes[i]->getMergeInfo()) ||
            DepGraph->hasCycle() ||
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
