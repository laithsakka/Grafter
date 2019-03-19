//===--- AccessPath.cpp --------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "AccessPath.h"
#include "FunctionAnalyzer.h"

using namespace clang;

bool AccessPathContainer::insert(AccessPath *AccessPath, bool IsWrite) {

  if (IsWrite)
    return WriteSet.insert(AccessPath).second;
  else
    return ReadSet.insert(AccessPath).second;
}

bool AccessPathContainer::insertReplacedAccessPath(AccessPath *AccessPath) {
  return ReplacedSet.insert(AccessPath).second;
}

void AccessPathContainer::freeAccessPaths() {
  for (auto *Entry : ReadSet)
    delete Entry;

  for (auto *Entry : WriteSet)
    delete Entry;

  for (auto *Entry : ReplacedSet)
    delete Entry;
}

AccessPath::AccessPath(clang::Expr *SourceExpression,
                       FunctionAnalyzer *Function,
                       StrictAccessInfo *AnnotationInfo) {

  SourceExpression = SourceExpression->IgnoreImplicit();
  EnclosingFunction = Function;
  if (Function == nullptr)
    this->IsDummy = true;

  switch (SourceExpression->getStmtClass()) {
  case Stmt::MemberExprClass:
    parseAccessPath(dyn_cast<clang::MemberExpr>(SourceExpression));
    break;
  case Stmt::DeclRefExprClass:
    parseAccessPath(dyn_cast<clang::DeclRefExpr>(SourceExpression));
    break;
  case Stmt::CXXStaticCastExprClass:
    parseAccessPath(dyn_cast<clang::CXXStaticCastExpr>(SourceExpression));
    break;
  case Stmt::CXXMemberCallExprClass: {
    auto *CallExpression = dyn_cast<clang::CXXMemberCallExpr>(SourceExpression);

    assert(hasStrictAccessAnnotation(CallExpression->getCalleeDecl()) &&
           "Member function call not allowed with out annotation");

    assert(AnnotationInfo != nullptr);

    assert(AnnotationInfo->IsLocal && "Invalid annotation");

    this->AnnotationInfo = *AnnotationInfo;
    this->IsStrictAccessCall = true;
    this->AccessPathString =
        "StrictOnTreeAccess" + to_string(AnnotationInfo->Id) + "(" +
        CallExpression->getCalleeDecl()->getAsFunction()->getNameAsString() +
        ")";

    auto &FirstParameter = **(*CallExpression->child_begin())->child_begin();

    // TODO: only one parameter supported for now i assume
    assert(std::next((*CallExpression->child_begin())->child_begin()) ==
           (*CallExpression->child_begin())->child_end());

    if (FirstParameter.IgnoreImplicit()->getStmtClass() ==
        Stmt::MemberExprClass) {
      parseAccessPath(
          dyn_cast<clang::MemberExpr>(FirstParameter.IgnoreImplicit()));
    } else if (FirstParameter.IgnoreImplicit()->getStmtClass() ==
               Stmt::DeclRefExprClass) {
      // Is this possible
      parseAccessPath(
          dyn_cast<clang::DeclRefExpr>(FirstParameter.IgnoreImplicit()));

    } else {
      llvm_unreachable("type not supported");
    }
    break;
  }
  case Stmt::CXXThisExprClass:
    parseAccessPath(dyn_cast<clang::CXXThisExpr>(SourceExpression));
    break;
  default:
    SourceExpression->dump();
    llvm_unreachable("type not supported");
  }

  if (!this->IsLegal) {
    Logger::getStaticLogger().logError(
        "AccessPath::AccessPath :access path is not legal");
  }
  if (IsDummy)
    return;

  // Replace local aliasing
  auto *Aliasing = EnclosingFunction->getLocalAliasing(getDeclAtIndex(0));
  clang::ValueDecl *AliasingLocalDecl = getDeclAtIndex(0);
  if (Aliasing) {
    assert(Aliasing->isOnTree() && !Aliasing->hasValuePart());
    SplittedAccessPath.erase(SplittedAccessPath.begin());
    SplittedAccessPath.insert(SplittedAccessPath.begin(),
                              Aliasing->SplittedAccessPath.begin(),
                              Aliasing->SplittedAccessPath.end());
    FromAliasing = true;
    AccessPathString = "";
    for (auto Entry : SplittedAccessPath) {

      if (AccessPathString.size() != 0)
        AccessPathString += ".";

      AccessPathString += Entry.first;
    }
    AliasingDeclAccessPath =
        new AccessPath(dyn_cast<clang::VarDecl>(AliasingLocalDecl), Function);
  }

  setValueStartIndex();

  // Special checks

  if (IsStrictAccessCall && !AnnotationInfo->IsGlobal && !isOnTree()) {
    Logger::getStaticLogger().logError("Invalid access path");
    IsLegal = false;
  }
  if (IsStrictAccessCall && isOnTree()) {
    if (SplittedAccessPath[0].second !=
        this->EnclosingFunction->TraversedNodeDecl) {
      Logger::getStaticLogger().logError("Invalid access path");
      IsLegal = false;
    }
    for (unsigned I = 1; I < SplittedAccessPath.size(); I++) {
      if ((!EnclosingFunction->isInChildList(SplittedAccessPath[I].second))) {
        Logger::getStaticLogger().logError("Invalid access path");
        IsLegal = false;
      }
    }
  }

  if (isOnTree()) {
    for (int I = 1; I < getValueStartIndex(); I++) {
      if (!hasChildAnnotation(
              dyn_cast<clang::FieldDecl>(SplittedAccessPath[I].second))) {
        Logger::getStaticLogger().logError(
            "on-tree access-path contains invalid tree access symbol");
        SplittedAccessPath[I].second->dump();
        IsLegal = false;
      }
    }
  }

  // is it ok to allow value parts to be pointers ? how do we make sure that the
  // language does not support alia
  if (getValueStartIndex() != -1) {
    for (int I = getValueStartIndex(); I < SplittedAccessPath.size(); I++) {
      if (getDeclAtIndex(I)->getType()->isPointerType() ||
          getDeclAtIndex(I)->getType()->isReferenceType()) {
        this->dump();
        getDeclAtIndex(I)->dump();
        Logger::getStaticLogger().logError(
            "access-path have pointer/reference in value part");
        IsLegal = false;
      }
    }
  }
}

AccessPath::AccessPath(clang::VarDecl *VarDeclaration,
                       FunctionAnalyzer *Function) {

  EnclosingFunction = Function;

  if (Function == nullptr)
    IsDummy = true;

  AccessPathString = VarDeclaration->getNameAsString();
  SplittedAccessPath.push_back(
      make_pair(VarDeclaration->getNameAsString(), VarDeclaration));

  ValueStartIndex = 0;
  // Recognize the symbol by the automata generator
  FSMUtility::addSymbol(VarDeclaration);
}

// build an access path for a global strict access
AccessPath::AccessPath(clang::FunctionDecl *FunctionDeclaration,
                       FunctionAnalyzer *Function) {

  EnclosingFunction = Function;

  if (Function == nullptr)
    IsDummy = true;
  AccessPathString = "strictOffTreeAccess" + to_string(AnnotationInfo.Id) +
                     "(" + FunctionDeclaration->getNameAsString() + ")";

  // this->SplittedAccessPath.push_back(make_pair(FunctionDeclaration->getNameAsString(),
  //  Dummy));
  ValueStartIndex = 0;
  IsStrictAccessCall = true;
}

bool AccessPath::hasValuePart() const { return ValueStartIndex != -1; }

bool AccessPath::isLegal() const { return IsLegal; }

int AccessPath::getValueStartIndex() const { return ValueStartIndex; }

bool AccessPath::isOnTree() const { return IsLegal && ValueStartIndex != 0; }

bool AccessPath::isOffTree() const { return IsLegal && ValueStartIndex == 0; }

// TODO : SplittedAccessPath[0].second->getDeclContext()->isFunctionOrMethod()
// why this check
bool AccessPath::isLocal() const {
  return !IsStrictAccessCall && IsLegal && isOffTree() &&
         SplittedAccessPath[0].second->getDeclContext()->isFunctionOrMethod();
}

bool AccessPath::isGlobal() const {
  return IsLegal && isOffTree() && !isLocal();
}

bool AccessPath::onlyUses(const clang::VarDecl *RootDecl,
                          const set<clang::FieldDecl *> &ChildrenSymbols) {

  if ((SplittedAccessPath[0].second != RootDecl))
    return false;

  for (int i = 1; i < SplittedAccessPath.size(); i++) {
    if (!hasChildAnnotation(
            dyn_cast<clang::FieldDecl>(SplittedAccessPath[i].second)))
      return false;
  }

  return true;
}

int AccessPath::getValuePathSize() const {
  if (ValueStartIndex == -1)
    return 0;
  else
    return SplittedAccessPath.size() - ValueStartIndex;
}

int AccessPath::getTreeAccessPathSize() const {
  if (ValueStartIndex == -1)
    return SplittedAccessPath.size();
  else
    return ValueStartIndex;
}

int AccessPath::getTreeAccessPathEndIndex() const {
  if (ValueStartIndex == -1)
    return SplittedAccessPath.size() - 1;
  else
    return ValueStartIndex - 1;
}

clang::ValueDecl *AccessPath::getDeclAtIndex(int i) const {
  return SplittedAccessPath[i].second;
}

const string &AccessPath::getAsStr() const { return AccessPathString; }

int AccessPath::getDepth() const { return SplittedAccessPath.size(); }

void AccessPath::setValueStartIndex() {

  // a child a
  for (unsigned I = 0; I < SplittedAccessPath.size(); I++) {
    auto *ValueDecl = SplittedAccessPath[I].second;
    if (EnclosingFunction->TraversedNodeDecl != ValueDecl) {
      clang::FieldDecl *Field = dyn_cast<clang::FieldDecl>(ValueDecl);
      if (Field && hasChildAnnotation(Field))
        continue;
      ValueStartIndex = I;
      break;
    }
  }
}

void AccessPath::appendSymbol(clang::ValueDecl *NodeDecleration) {

  // This is a hack (this) is represented by nullptr everywhere in tree-fuser
  // for now
  std::string AccessSymbol =
      NodeDecleration ? NodeDecleration->getNameAsString() : "^";

  if (!IsDummy && EnclosingFunction->TraversedNodeDecl == NodeDecleration)
    AccessSymbol = "^";

  if (SplittedAccessPath.size() != 0 || IsStrictAccessCall) {
    AccessPathString = AccessSymbol + "." + AccessPathString;
    SplittedAccessPath.insert(SplittedAccessPath.begin(),
                              make_pair(AccessSymbol, NodeDecleration));

  } else {
    AccessPathString = AccessSymbol;
    SplittedAccessPath.insert(SplittedAccessPath.begin(),
                              make_pair(AccessSymbol, NodeDecleration));
  }

  // Recognize the symbol by the automata generator
  FSMUtility::addSymbol(NodeDecleration);
}

bool AccessPath::parseAccessPath(clang::MemberExpr *Expression) {
  if (Expression->getMemberDecl()->isFunctionOrFunctionTemplate()) {
    Logger::getStaticLogger().logError(
        "Function calls not allowed as part of access path");
    return IsLegal = false;
  }

  // change to post order for better performance
  appendSymbol(Expression->getMemberDecl());
  auto *NextExpression = (*Expression->child_begin())->IgnoreImplicit();
  assert(std::next(Expression->child_begin()) == Expression->child_end());
  return handleNextExpression(NextExpression);
}

bool AccessPath::parseAccessPath(clang::CXXStaticCastExpr *Expression) {
  assert(Expression);
  return handleNextExpression(Expression->getSubExpr()->IgnoreImplicit());
}

bool AccessPath::parseAccessPath(clang::CXXThisExpr *Expression) {
  assert(Expression);
  appendSymbol(nullptr);
}

bool AccessPath::parseAccessPath(clang::DeclRefExpr *Expression) {
  assert(Expression);
  appendSymbol(Expression->getDecl());
  assert(Expression->child_begin() == Expression->child_end());
  return true;
}

bool AccessPathCompare::operator()(const AccessPath *LHS,
                                   const AccessPath *RHS) const {
  return LHS->SplittedAccessPath < RHS->SplittedAccessPath;
}

bool AccessPath::handleNextExpression(clang::Stmt *NextExpression) {
  switch (NextExpression->getStmtClass()) {
  case Stmt::MemberExprClass:
    return parseAccessPath(dyn_cast<clang::MemberExpr>(NextExpression));

  case Stmt::DeclRefExprClass:
    return parseAccessPath(dyn_cast<clang::DeclRefExpr>(NextExpression));

  case Stmt::CXXStaticCastExprClass:
    return parseAccessPath(dyn_cast<clang::CXXStaticCastExpr>(NextExpression));

  case Stmt::ParenExprClass:
    return handleNextExpression(
        (dyn_cast<clang::ParenExpr>(NextExpression))->getSubExpr());
  case Stmt::CXXThisExprClass:
    return parseAccessPath(dyn_cast<clang::CXXThisExpr>(NextExpression));
  default:
    Logger::getStaticLogger().logError(
        "AccessPath::handleNextExpression unsupported type "
        ">>" +
        string(NextExpression->getStmtClassName()));
    NextExpression->dump();
    IsLegal = false;
    return false;
  }
  return true;
}

const FSM &AccessPath::getWriteAutomata() {
  if (WriteAutomata == nullptr) {
    WriteAutomata = new FSM();
    int StateId = WriteAutomata->AddState();
    WriteAutomata->SetStart(StateId /*0*/);

    // Handle global strict access
    if (IsStrictAccessCall) {
      if (AnnotationInfo.IsGlobal) {
        StateId = WriteAutomata->AddState();
        FSMUtility::addTransitionOnAbstractAccess(*WriteAutomata, StateId - 1,
                                                  StateId, AnnotationInfo.Id);
        WriteAutomata->SetFinal(StateId, 0);
        return *WriteAutomata;
      } else
        llvm_unreachable("not supported in Treefuser2 yet!");
    }

    bool First = true;
    for (auto &Entry : SplittedAccessPath) {

      if (First && isOnTree()) {
        StateId = WriteAutomata->AddState();
        FSMUtility::addTraversedNodeTransition(*WriteAutomata, StateId - 1,
                                               StateId);
        First = false;
        continue;
      }
      StateId = WriteAutomata->AddState();
      FSMUtility::addTransition(*WriteAutomata, StateId - 1, StateId,
                                Entry.second);
    }
    WriteAutomata->SetFinal(StateId, 0);

    // if a written access path ends with struct or node then add any transition
    // we cant do delete this or this = new so we exclude those
    auto *LastField = SplittedAccessPath[SplittedAccessPath.size() - 1].second;

    bool NonDecreasingCall = LastField == nullptr;
    if (!NonDecreasingCall && !RecordsAnalyzer::isPrimitiveScaler(LastField)) {
      StateId = WriteAutomata->AddState();
      FSMUtility::addAnyTransition(*WriteAutomata, StateId - 1, StateId);
      FSMUtility::addEpsTransition(*WriteAutomata, StateId, StateId - 1);
      WriteAutomata->SetFinal(StateId, 0);
    }
  }
  return *WriteAutomata;
}

const FSM &AccessPath::getReadAutomata() {
  if (ReadAutomata == nullptr) {
    ReadAutomata = new FSM();
    int StateId = ReadAutomata->AddState();
    ReadAutomata->SetStart(StateId /*0*/);

    // Handle global strict access
    if (IsStrictAccessCall) {
      if (AnnotationInfo.IsGlobal) {
        StateId = ReadAutomata->AddState();
        FSMUtility::addTransitionOnAbstractAccess(*ReadAutomata, StateId - 1,
                                                  StateId, AnnotationInfo.Id);

        ReadAutomata->SetFinal(StateId, 0);
        return *ReadAutomata;
      } else
        llvm_unreachable("not supported in Treefuser2 yet!");
    }

    bool First = true;
    for (auto &Entry : SplittedAccessPath) {

      if (First && isOnTree()) {
        StateId = ReadAutomata->AddState();
        FSMUtility::addTraversedNodeTransition(*ReadAutomata, StateId - 1,
                                               StateId);
        ReadAutomata->SetFinal(StateId, 0);
        First = false;
        continue;
      }
      StateId = ReadAutomata->AddState();
      FSMUtility::addTransition(*ReadAutomata, StateId - 1, StateId,
                                Entry.second);
      ReadAutomata->SetFinal(StateId, 0);
    }

    // if a value access path ends with struct then add any transition
    auto *LastField = SplittedAccessPath[SplittedAccessPath.size() - 1].second;
    bool NonDecreasingCall = LastField == nullptr;
    if (hasValuePart() && !NonDecreasingCall &&
        !RecordsAnalyzer::isPrimitiveScaler(LastField)) {
      StateId = ReadAutomata->AddState();
      FSMUtility::addAnyTransition(*ReadAutomata, StateId - 1, StateId);
      FSMUtility::addEpsTransition(*ReadAutomata, StateId, StateId - 1);
      ReadAutomata->SetFinal(StateId, 0);
    }
  }
  return *ReadAutomata;
}
