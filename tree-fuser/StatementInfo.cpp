//===--- StatementInfo.cpp-------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//
#include <StatementInfo.h>

#define DEBUG_TYPE "stmt-info"

const FSM &StatementInfo::getLocalWritesAutomata() {
  if (!LocalWritesAutomata) {
    LocalWritesAutomata = new FSM();
    for (auto *AccessPath : getAccessPaths().getWriteSet()) {
      if (AccessPath->isLocal())
        fst::Union(LocalWritesAutomata, AccessPath->getWriteAutomata());
    }
    fst::ArcSort(LocalWritesAutomata, fst::ILabelCompare<fst::StdArc>());
  }
  return *LocalWritesAutomata;
}

const FSM &StatementInfo::getLocalReadsAutomata() {
  if (!LocalReadsAutomata) {
    LocalReadsAutomata = new FSM();

    for (auto *AccessPath : getAccessPaths().getReadSet()) {
      if (AccessPath->isLocal())
        fst::Union(LocalReadsAutomata, AccessPath->getReadAutomata());
    }

    for (auto *AccessPath : getAccessPaths().getWriteSet()) {
      if (AccessPath->isLocal())
        fst::Union(LocalReadsAutomata, AccessPath->getReadAutomata());
    }
    fst::ArcSort(LocalReadsAutomata, fst::ILabelCompare<fst::StdArc>());
  }
  return *LocalReadsAutomata;
}

const FSM &StatementInfo::getGlobWritesAutomata(bool IncludeExtended) {
  if (!BaseGlobalWritesAutomata) {
    BaseGlobalWritesAutomata = new FSM();
    for (auto *AccessPath : getAccessPaths().getWriteSet()) {
      if (AccessPath->isGlobal())
        fst::Union(BaseGlobalWritesAutomata, AccessPath->getWriteAutomata());
    }
    fst::ArcSort(BaseGlobalWritesAutomata, fst::ILabelCompare<fst::StdArc>());
  }
  if (isCallStmt() && IncludeExtended)
    return getExtendedGlobWritesAutomata(); // the extended include the basic
  else
    return *BaseGlobalWritesAutomata;
}

const FSM &StatementInfo::getGlobReadsAutomata(bool IncludeExtended) {
  if (!BaseGlobalReadsAutomata) {
    BaseGlobalReadsAutomata = new FSM();

    for (auto *AccessPath : getAccessPaths().getReadSet()) {
      if (AccessPath->isGlobal())
        fst::Union(BaseGlobalReadsAutomata, AccessPath->getReadAutomata());
    }

    for (auto *AccessPath : getAccessPaths().getWriteSet()) {
      if (AccessPath->isGlobal())
        fst::Union(BaseGlobalReadsAutomata, AccessPath->getReadAutomata());
    }
    fst::ArcSort(BaseGlobalReadsAutomata, fst::ILabelCompare<fst::StdArc>());
  }
  if (isCallStmt() && IncludeExtended)
    return getExtendedGlobReadsAutomata(); // the extended include the basic
  else
    return *BaseGlobalReadsAutomata;
}

const FSM &StatementInfo::getTreeReadsAutomata(bool IncludeExtended) {
  if (!BaseTreeReadsAutomata) {
    BaseTreeReadsAutomata = new FSM();

    for (auto *AccessPath : getAccessPaths().getReadSet()) {
      if (AccessPath->isOnTree())
        fst::Union(BaseTreeReadsAutomata, AccessPath->getReadAutomata());
    }

    for (auto *AccessPath : getAccessPaths().getWriteSet()) {
      if (AccessPath->isOnTree())
        fst::Union(BaseTreeReadsAutomata, AccessPath->getReadAutomata());
    }

    for (auto *AccessPath : getAccessPaths().getReplacedSet()) {
      assert(AccessPath->isOnTree());
      fst::Union(BaseTreeReadsAutomata, AccessPath->getReadAutomata());
    }
    fst::ArcSort(BaseTreeReadsAutomata, fst::ILabelCompare<fst::StdArc>());
  }
  if (isCallStmt() && IncludeExtended)
    return getExtendedTreeReadsAutomata();
  else
    return *BaseTreeReadsAutomata;
}

const FSM &StatementInfo::getTreeWritesAutomata(bool IncludeExtended) {
  if (!BaseTreeWritesAutomata) {
    BaseTreeWritesAutomata = new FSM();

    for (auto *AccessPath : getAccessPaths().getWriteSet()) {
      if (AccessPath->isOnTree())
        fst::Union(BaseTreeWritesAutomata, AccessPath->getWriteAutomata());
    }

    // Adding node mutations accesses
    for (auto *AccessPath : getAccessPaths().getReplacedSet()) {
      assert(AccessPath->isOnTree());
      fst::Union(BaseTreeWritesAutomata, AccessPath->getWriteAutomata());
    }
    fst::ArcSort(BaseTreeWritesAutomata, fst::ILabelCompare<fst::StdArc>());
  }
  if (isCallStmt() && IncludeExtended)
    return getExtendedTreeWritesAutomata();
  else
    return *BaseTreeWritesAutomata;
}

// Helper function used during the build of extended accesses only for on-tree
// accesses

void buildFromAccessPath(FSM *FSMachine, int CurrState, AccessPath *AP,
                         bool Reads) {
  bool First = true;
  for (auto &Entry : AP->SplittedAccessPath) {

    // ignore the first one since this function only called through the
    if (First && AP->isOnTree()) {
      First = false;
      continue;
    }
    int NewState = FSMachine->AddState();
    FSMUtility::addTransition(*FSMachine, CurrState, NewState, Entry.second);
    CurrState = NewState;
    if (Reads)
      FSMachine->SetFinal(NewState, 0);
  }
  FSMachine->SetFinal(CurrState, 0);
}

// Helper function used during the build of extended accesses only for on-tree
// accesses
void buildFromSimpleStmt(FSM *FSMachine, int CurrState, StatementInfo *Stmt,
                         bool Reads) {
  if (Reads) {
    for (auto *AP : Stmt->getAccessPaths().getReadSet()) {
      if (AP->isOnTree())
        buildFromAccessPath(FSMachine, CurrState, AP, true);
    }
    for (auto *AP : Stmt->getAccessPaths().getWriteSet()) {
      if (AP->isOnTree())
        buildFromAccessPath(FSMachine, CurrState, AP, true);
    }
    for (auto *AP : Stmt->getAccessPaths().getReplacedSet()) {
      if (AP->isOnTree())
        buildFromAccessPath(FSMachine, CurrState, AP, true);
    }
  } else {
    for (auto *AP : Stmt->getAccessPaths().getWriteSet()) {
      if (AP->isOnTree())
        buildFromAccessPath(FSMachine, CurrState, AP, false);
    }
    for (auto *AP : Stmt->getAccessPaths().getReplacedSet()) {
      if (AP->isOnTree())
        buildFromAccessPath(FSMachine, CurrState, AP, false);
    }
  }
}

// Helper function used during the build of extended accesses only for on-tree
// accesses
void buildFromCall(
    FSM *FSMachine, int CurrState, StatementInfo *CallStmt,
    std::unordered_map<FunctionAnalyzer *, int> FunctionToStateId,
    bool ForReads) {

  buildFromSimpleStmt(FSMachine, CurrState, CallStmt, ForReads);

  auto *ChildRecord =
      CallStmt->getCalledChild()->getType()->getPointeeCXXRecordDecl();

  auto CalledChildRecordInfo = RecordsAnalyzer::getRecordInfo(ChildRecord);

  auto *CalledFunctionInfo =
      FunctionsFinder::getFunctionInfo(CallStmt->getCalledFunction());

  std::set<FunctionAnalyzer *> PossiblyCalledFunctions;
  if (CalledFunctionInfo->isCXXMember() &&
      dyn_cast<CXXMethodDecl>(CallStmt->getCalledFunction())->isVirtual()) {
    PossiblyCalledFunctions.insert(CalledFunctionInfo);

    // For each possible derived type add the corresponding called method
    for (auto *DerivedRecord : RecordsAnalyzer::DerivedRecords[ChildRecord]) {
      auto *CalledMethod =
          dyn_cast<CXXMethodDecl>(CallStmt->getCalledFunction())
              ->getCorrespondingMethodInClass(DerivedRecord)
              ->getDefinition();

      PossiblyCalledFunctions.insert(
          FunctionsFinder::getFunctionInfo(CalledMethod));
    }

  } else {
    PossiblyCalledFunctions.insert(CalledFunctionInfo);
  }
  for (auto *Function : PossiblyCalledFunctions) {
    if (!FunctionToStateId.count(Function)) {
      int NewState = FSMachine->AddState();
      LLVM_DEBUG(cout << "add function mapping:"
                      << Function->getFunctionDecl()->getNameAsString() << ":"
                      << NewState << "\n");

      FunctionToStateId[Function] = NewState;

      for (auto *Stmt : Function->getStatements()) {
        if (Stmt->isCallStmt())
          buildFromCall(FSMachine, NewState, Stmt, FunctionToStateId, ForReads);
        else
          buildFromSimpleStmt(FSMachine, NewState, Stmt, ForReads);
      }
      if (ForReads)
        FSMachine->SetFinal(NewState, 0);
    }
    FSMUtility::addTransition(*FSMachine, CurrState,
                              FunctionToStateId[Function],
                              CallStmt->getCalledChild());
  }
}

const FSM &StatementInfo::getExtendedTreeReadsAutomata() {
  assert(isCallStmt());
  if (!ExtendedTreeReadsAutomata) {
    ExtendedTreeReadsAutomata = new FSM();
    ExtendedTreeReadsAutomata->AddState();
    ExtendedTreeReadsAutomata->SetStart(0);
    ExtendedTreeReadsAutomata->AddState();
    FSMUtility::addTraversedNodeTransition(*ExtendedTreeReadsAutomata, 0, 1);
    ExtendedTreeReadsAutomata->SetFinal(1, 0);

    std::unordered_map<FunctionAnalyzer *, int> EmptyTable;
    buildFromCall(ExtendedTreeReadsAutomata, 1, this, EmptyTable, true);

    fst::ArcSort(ExtendedTreeReadsAutomata, fst::ILabelCompare<fst::StdArc>());
  }
  return *ExtendedTreeReadsAutomata;
}

const FSM &StatementInfo::getExtendedTreeWritesAutomata() {
  assert(isCallStmt());
  if (!ExtendedTreeWritesAutomata) {
    ExtendedTreeWritesAutomata = new FSM();
    ExtendedTreeWritesAutomata->AddState();
    ExtendedTreeWritesAutomata->SetStart(0);
    ExtendedTreeWritesAutomata->AddState();
    FSMUtility::addTraversedNodeTransition(*ExtendedTreeWritesAutomata, 0, 1);

    std::unordered_map<FunctionAnalyzer *, int> EmtyTable;

    buildFromCall(ExtendedTreeWritesAutomata, 1, this, EmtyTable, false);

    fst::ArcSort(ExtendedTreeWritesAutomata, fst::ILabelCompare<fst::StdArc>());
  }

  return *ExtendedTreeWritesAutomata;
}

const FSM &StatementInfo::getExtendedGlobReadsAutomata() {
  assert(isCallStmt());
  if (!ExtendedGlobalReadsAutomata) {
    ExtendedGlobalReadsAutomata = new FSM();

    for (auto *Stmt : EnclosingFunction->getStatements())
      fst::Union(ExtendedGlobalReadsAutomata,
                 Stmt->getGlobReadsAutomata(false));
    fst::ArcSort(ExtendedGlobalReadsAutomata,
                 fst::ILabelCompare<fst::StdArc>());
  }
  return *ExtendedGlobalReadsAutomata;
}

const FSM &StatementInfo::getExtendedGlobWritesAutomata() {
  assert(isCallStmt());
  if (!ExtendedGlobalWritesAutomata) {
    ExtendedGlobalWritesAutomata = new FSM();

    for (auto *Stmt : EnclosingFunction->getStatements()) {
      fst::Union(ExtendedGlobalWritesAutomata,
                 Stmt->getGlobWritesAutomata(false));
    }
    fst::ArcSort(ExtendedGlobalWritesAutomata,
                 fst::ILabelCompare<fst::StdArc>());
  }
  return *ExtendedGlobalWritesAutomata;
}
