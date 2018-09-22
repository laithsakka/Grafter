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
  default:
    llvm_unreachable("type not supported");
  }

  if (!this->IsLegal) {
    Logger::getStaticLogger().logError(
        "AccessPath::AccessPath :access path is not legal");
  }
  if (IsDummy)
    return;

  setValueStartIndex();
  // special check
  // Too many hacks this is to make sure that if the accespath is onTree
  if (IsStrictAccessCall && !AnnotationInfo->IsGlobal && !isOnTree()) {
    Logger::getStaticLogger().logError("Invalid access path");
    IsLegal = false;
  }

  // Check that the access path is valid
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

  for (int I = 1; I < getValueStartIndex(); I++) {
    if ((!EnclosingFunction->isInChildList(SplittedAccessPath[I].second))) {
      Logger::getStaticLogger().logError(
          "AccessPath::VisitMemberExpr not tree node access");
      IsLegal = false;
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
}

AccessPath::AccessPath(clang::FunctionDecl *FunctionDeclaration,
                       FunctionAnalyzer *Function) {

  EnclosingFunction = Function;

  if (Function == nullptr)
    IsDummy = true;

  // this->AnnotationInfo=getStrictAccessInfo( decl);
  AccessPathString = "strictOffTreeAccess" + to_string(AnnotationInfo.Id) +
                     "(" + FunctionDeclaration->getNameAsString() + ")";
  // this->SplittedAccessPath.push_back(make_pair(decl->getNameAsString(),
  // decl));
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
    if (!ChildrenSymbols.count(
            dyn_cast<clang::FieldDecl>(SplittedAccessPath[i].second))) {
      return false;
    }
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

  for (unsigned I = 0; I < SplittedAccessPath.size(); I++) {
    auto &ValueDecl = SplittedAccessPath[I].second;
    if (EnclosingFunction->TraversedNodeDecl != ValueDecl &&
        !EnclosingFunction->isInChildList(ValueDecl)) {
      ValueStartIndex = I;
    }
  }
}

void AccessPath::appendSymbol(clang::ValueDecl *NodeDecleration) {

  std::string AccessSymbol = NodeDecleration->getNameAsString();

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

bool AccessPath::parseAccessPath(clang::DeclRefExpr *Expression) {
  assert(Expression);
  appendSymbol(Expression->getDecl());
  assert(Expression->child_begin() == Expression->child_end());
  return true;
}

bool AccessPathCompare::operator()(const AccessPath *LHS,
                                   const AccessPath *RHS) const {
  bool AreSame = true;
  if (LHS->SplittedAccessPath.size() != RHS->SplittedAccessPath.size()) {
    AreSame = false;
  } else {
    for (unsigned i = 0; i < LHS->SplittedAccessPath.size(); i++) {
      if (LHS->SplittedAccessPath[i].second !=
          LHS->SplittedAccessPath[i].second)
        AreSame = false;
    }
  }

  if (AreSame) {
    if (LHS->isStrictAccessCall() && RHS->isStrictAccessCall()) {
      if (LHS->isOnTree() && RHS->isOnTree()) {
        if (LHS->getAnnotationInfo().Id != RHS->getAnnotationInfo().Id)
          AreSame = false;
      } else if (LHS->isOffTree() && RHS->isOffTree()) {
        if (LHS->getAnnotationInfo().Id != RHS->getAnnotationInfo().Id)
          AreSame = false;
      } else { // they have different types
        AreSame = false;
      }
    }
    // one of them is strict access
    else if (LHS->isStrictAccessCall() || RHS->isStrictAccessCall())
      AreSame = false;
  }

  if (AreSame)
    return false;
  else
    return LHS < RHS;
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
