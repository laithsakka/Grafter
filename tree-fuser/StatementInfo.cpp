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

FSM *StatementInfo::createExtendedAutomataPrefix(bool AllAccept) {
  FSM *Prefix = new FSM();

  Prefix->AddState();
  Prefix->SetStart(0);

  Prefix->AddState();
  FSMUtility::addTraversedNodeTransition(*Prefix, 0, 1);

  // Add the transition to the called child
  Prefix->AddState();
  FSMUtility::addTransition(*Prefix, 1, 2, getCalledChild());

  // Child*
  for (auto *Child : EnclosingFunction->getCalledChildrenList())
    FSMUtility::addTransition(*Prefix, 2, 2, Child);


  if (AllAccept) {
    Prefix->SetFinal(1, 0);
    Prefix->SetFinal(2, 0);
  } else {
    Prefix->SetFinal(2, 0);
  }
  return Prefix;
}

const FSM &StatementInfo::getExtendedTreeReadsAutomata() {
  assert(isCallStmt());
  if (!ExtendedTreeReadsAutomata) {
    auto *PrefixAcceptedAll = createExtendedAutomataPrefix(true);
    auto *PrefixAcceptedLast = createExtendedAutomataPrefix(false);

    FSM AllReads;
    // Followed by any read access in the body of the traversal
    for (auto *Stmt : EnclosingFunction->getStatements()) {
      auto *RootRemoved =
          FSMUtility::CopyRootRemoved(Stmt->getTreeReadsAutomata(false));

      fst::Union(&AllReads, *RootRemoved);

      delete RootRemoved;
    }

    fst::Concat(PrefixAcceptedLast, AllReads);
    for (int i = 1; i <= 3; i++) {
      PrefixAcceptedLast->SetFinal(i,0);
    }

    ExtendedTreeReadsAutomata = new FSM();
    fst::Union(ExtendedTreeReadsAutomata, *PrefixAcceptedLast);
    fst::ArcSort(ExtendedTreeReadsAutomata, fst::ILabelCompare<fst::StdArc>());

    delete PrefixAcceptedAll;
    delete PrefixAcceptedLast;
  }
  return *ExtendedTreeReadsAutomata;
}

const FSM &StatementInfo::getExtendedTreeWritesAutomata() {
  assert(isCallStmt());
  if (!ExtendedTreeWritesAutomata) {
    ExtendedTreeWritesAutomata = createExtendedAutomataPrefix(false);

    FSM AllWrites;
    // Followed by any read access in the body of the traversal
    for (auto *Stmt : EnclosingFunction->getStatements()) {
      auto *RootRemoved =
          FSMUtility::CopyRootRemoved(Stmt->getTreeWritesAutomata(false));

      fst::Union(&AllWrites, *RootRemoved);
    }

    fst::Concat(ExtendedTreeWritesAutomata, AllWrites);
    fst::Union(ExtendedTreeWritesAutomata, getTreeWritesAutomata(false));
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
