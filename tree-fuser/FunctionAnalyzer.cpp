//===--- FunctionAnalyzer.cpp ---------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Analyze fuse functions and their access paths
//===----------------------------------------------------------------------===//

#include "FunctionAnalyzer.h"
#include "AccessPath.h"
#include "Logger.h"
#include "RecordAnalyzer.h"

#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

void FunctionAnalyzer::dump() {
  outs() << "isValidFuse:" << isValidFuse() << "\n";
  for (auto *Stmt : Statements) {
    outs() << "[" << Stmt->getStatementId() << "]\n";
    outs() << "reads access paths  count: "
           << Stmt->getAccessPaths().getReadSet().size() << "\n";

    for (auto *AccessPath : Stmt->getAccessPaths().getReadSet())
      AccessPath->dump();

    outs() << "writes access paths count: "
           << Stmt->getAccessPaths().getWriteSet().size() << "\n";
    for (auto *AccessPath : Stmt->getAccessPaths().getWriteSet())
      AccessPath->dump();

    outs() << "replaced nodes access paths count: "
           << Stmt->getAccessPaths().getReplacedSet().size() << "\n";
    for (auto *AccessPath : Stmt->getAccessPaths().getReplacedSet())
      AccessPath->dump();
  }
}

int FunctionAnalyzer::getNumberOfRecursiveCalls() const {
  return CalledChildrenOrderedList.size();
}

clang::FunctionDecl *FunctionAnalyzer::getFunctionDecl() const {
  return this->FuncDeclNode;
}

const clang::VarDecl *FunctionAnalyzer::getTraversedNodeDecl() const {
  return TraversedNodeDecl;
}

const clang::RecordDecl *FunctionAnalyzer::getTraversedTreeTypeDecl() const {
  return TraversedTreeTypeDecl;
}

const vector<clang::FieldDecl *> &
FunctionAnalyzer::getCalledChildrenList() const {
  return CalledChildrenOrderedList;
}

FunctionAnalyzer::FunctionAnalyzer(clang::FunctionDecl *FuncDeclaration) {
  this->FuncDeclNode = FuncDeclaration;
  this->SemanticsSatasified = checkFuseSema();
  return;
}

bool FunctionAnalyzer::isInCalledChildList(clang::FieldDecl *ChildDecl) const {
  for (auto *CalledChild : CalledChildrenOrderedList)
    if (ChildDecl == CalledChild)
      return true;

  return false;
}

bool FunctionAnalyzer::isInChildList(clang::ValueDecl *Decl) const {
  return RecordsAnalyzer::getRecordInfo(TraversedTreeTypeDecl)
      .isChildAccessDecl(Decl);
}

// Handle general statements not sub expression
bool FunctionAnalyzer::collectAccessPath_handleStmt(clang::Stmt *Stmt) {
  Stmt = Stmt->IgnoreImplicit();
  switch (Stmt->getStmtClass()) {
  case clang::Stmt::CompoundStmtClass:
    return collectAccessPath_VisitCompoundStmt(
        dyn_cast<clang::CompoundStmt>(Stmt));

  case clang::Stmt::CallExprClass:
    return collectAccessPath_VisitCallExpr(dyn_cast<clang::CallExpr>(Stmt));

  case clang::Stmt::BinaryOperatorClass:
    return collectAccessPath_VisitBinaryOperator(
        dyn_cast<clang::BinaryOperator>(Stmt));

  case clang::Stmt::IfStmtClass:
    NestedIfDepth++;
    if (!collectAccessPath_VisitIfStmt(dyn_cast<clang::IfStmt>(Stmt))) {
      NestedIfDepth--;
      return false;
    }
    NestedIfDepth--;
    break;

  case clang::Stmt::ReturnStmtClass:
    CurrStatementInfo->setHasReturn(true);
    break;

  case clang::Stmt::Stmt::NullStmtClass:
    break;

  case clang::Stmt::DeclStmtClass:
    return collectAccessPath_VisitDeclsStmt(dyn_cast<clang::DeclStmt>(Stmt));

  case clang::Stmt::CXXMemberCallExprClass:
    return collectAccessPath_VisitCXXMemberCallExpr(
        dyn_cast<clang::CXXMemberCallExpr>(Stmt));

  case clang::Stmt::CXXDeleteExprClass:
    return collectAccessPath_VisitCXXDeleteExpr(
        dyn_cast<clang::CXXDeleteExpr>(Stmt));

  default:
    Logger::getStaticLogger().logError(
        "in FunctionAnalyzer::handleStmt() unsupported statment");
    return false;

    break;
  }

  return true;
}

bool FunctionAnalyzer::collectAccessPath_VisitCXXMemberCallExpr(
    clang::CXXMemberCallExpr *Expr) {
  assert(Expr->getStmtClass() == Stmt::CXXMemberCallExprClass);
  auto *CxxMemberCall = dyn_cast<clang::CXXMemberCallExpr>(Expr);

  if (!hasStrictAccessAnnotation(CxxMemberCall->getCalleeDecl())) {
    Logger::getStaticLogger().logError(
        "function calls with no strict access annotation are not allowed");
    return false;
  }

  std::vector<StrictAccessInfo> StrictAccessInfoList =
      getStrictAccessInfo(CxxMemberCall->getCalleeDecl());

  for (auto &AccessInfo : StrictAccessInfoList) {
    AccessPath *NewAccessPath = new AccessPath(Expr, this, &AccessInfo);
    if (!NewAccessPath->isLegal()) {
      delete NewAccessPath;
      return false;
    }
    addAccessPath(NewAccessPath,
                  !NewAccessPath->getAnnotationInfo().IsReadOnly);
  }

  // add access paths of each of the arguments
  for (auto *Argument : Expr->arguments()) {
    Argument = Argument->IgnoreImplicit();

    if (Argument->getStmtClass() == clang::Stmt::MemberExprClass ||
        Argument->getStmtClass() == clang::Stmt::DeclRefExprClass) {
      AccessPath *NewAccessPath = new AccessPath(Argument, this);
      if (!NewAccessPath->isLegal()) {
        delete NewAccessPath;
        return false;
      }
      addAccessPath(NewAccessPath, false);
    } else {

      if (!collectAccessPath_handleSubExpr(Argument)) {
        Logger::getStaticLogger().logError(
            "FunctionAnalyzer unsupported argument type");
        return false;
      }
    }
  }
  return true;
}

bool FunctionAnalyzer::collectAccessPath_handleSubExpr(clang::Expr *Expr) {
  Expr = Expr->IgnoreImplicit();

  switch (Expr->getStmtClass()) {

  case clang::Stmt::BinaryOperatorClass:
    if (!collectAccessPath_VisitBinaryOperator(
            dyn_cast<clang::BinaryOperator>(Expr)))
      return false;
    break;

  case clang::Stmt::CXXConstructExprClass: {
    auto *Constructor = dyn_cast<CXXConstructExpr>(Expr);

    // Return wether if its the default empty constructor
    return Constructor->getConstructor()->isTrivial();
  }

  case clang::Stmt::ParenExprClass: {
    auto *ParenthExpr = dyn_cast<clang::ParenExpr>(Expr);
    return collectAccessPath_VisitParenExpr(ParenthExpr);
  }

  case clang::Stmt::CallExprClass:
    return collectAccessPath_VisitCallExpr(dyn_cast<clang::CallExpr>(Expr));

  case clang::Stmt::CXXMemberCallExprClass:
    return collectAccessPath_VisitCXXMemberCallExpr(
        dyn_cast<clang::CXXMemberCallExpr>(Expr));

  case clang::Stmt::GNUNullExprClass:
  case clang::Stmt::IntegerLiteralClass:
  case clang::Stmt::FloatingLiteralClass:
  case clang::Stmt::CXXBoolLiteralExprClass:
  case clang::Stmt::NullStmtClass:
    return true;
  case clang::Stmt::CXXStaticCastExprClass:
    return collectAccessPath_VisitStaticCastExpr(
        dyn_cast<clang::CXXStaticCastExpr>(Expr));

  default:
    Logger::getStaticLogger().logError(
        "FunctionAnalyzer::handleSubExpr unsupported subexpr :" +
        string(Expr->getStmtClassName()));
    Expr->dump();
    return false;
  }
  return true;
}

bool FunctionAnalyzer::checkFuseSema() {
  // The function must be gloabl  be a global function
  if (!FuncDeclNode->isGlobal()) {
    Logger::getStaticLogger().logError("fuse traversal must be global ");
    return false;
  }

  // Return type must be void
  if (!FuncDeclNode->getReturnType()->isVoidType()) {
    Logger::getStaticLogger().logError("fuse method return type must be void");
    return false;
  }

  // First parameter must be pointer to a tree structure
  if (FuncDeclNode->parameters().size() == 0 ||
      !FuncDeclNode->parameters()[0]->getType()->isPointerType()) {
    Logger::getStaticLogger().logError(
        "fuse method first parameter must be pointer to a tree structure");
    return false;
  }

  // Return type must be void
  if (!FuncDeclNode->getReturnType()->isVoidType()) {
    Logger::getStaticLogger().logError("fuse method return type must be void");
    return false;
  }

  TraversedNodeDecl = FuncDeclNode->parameters()[0];
  TraversedTreeTypeDecl =
      TraversedNodeDecl->getType()->getPointeeCXXRecordDecl();

  if (!RecordsAnalyzer::getRecordInfo(TraversedTreeTypeDecl)
           .isTreeStructure()) {
    Logger::getStaticLogger().logError(
        "fuse method first parameter must be pointer to a tree structure");
    return false;
  }

  // All other parameters must be complete  scalers
  for (int i = 1; i < FuncDeclNode->parameters().size(); i++) {
    if (!RecordsAnalyzer::isCompleteScaler(FuncDeclNode->parameters()[i])) {
      Logger::getStaticLogger().logError(
          "fuse method has non complete-scaler parameter ");
      FuncDeclNode->parameters()[i]->dump();

      return false;
    }
  }

  // Must be a recursive function
  for (auto *Stmt : FuncDeclNode->getBody()->children()) {

    if (Stmt->getStmtClass() != clang::Stmt::CallExprClass)
      continue;

    auto *Call = dyn_cast<clang::CallExpr>(Stmt);
    if (Call->getCalleeDecl() != FuncDeclNode) {
      if (!hasStrictAccessAnnotation(Call->getCalleeDecl())) {
        Logger::getStaticLogger().logError(
            "fuse methods body not allowed to have function calls");
        return false;
      }
    }

    if (Call->getCalleeDecl() == FuncDeclNode) {
      AccessPath CalledChildAccessPath(Call->getArg(0),
                                       nullptr); // dummmy access path

      if (!CalledChildAccessPath.isLegal() ||
          CalledChildAccessPath.getDepth() != 2) {
        Logger::getStaticLogger().logError(
            "illegal access path in recursive call first argument");
        return false;
      }

      if (!CalledChildAccessPath.onlyUses(
              TraversedNodeDecl,
              RecordsAnalyzer::getRecursiveFields(TraversedTreeTypeDecl))) {
        Logger::getStaticLogger().logError(
            "fuse method recursive call is a not traversing a recognized "
            "child ");
        return false;
      }

      clang::FieldDecl *ChildDecl = dyn_cast<clang::FieldDecl>(
          CalledChildAccessPath.SplittedAccessPath[1].second);
      assert(ChildDecl != nullptr);

      if (!isInCalledChildList(ChildDecl))
        addCalledChildList(ChildDecl);
    }
  }

  if (this->getNumberOfRecursiveCalls() == 0) {
    Logger::getStaticLogger().logError("fuse method must have recursive calls");
    return false;
  }
  assert(FuncDeclNode->getBody()->IgnoreImplicit()->getStmtClass() ==
         clang::Stmt::CompoundStmtClass);

  return collectAccessPath_VisitCompoundStmt(
      dyn_cast<clang::CompoundStmt>(FuncDeclNode->getBody()));
}

void FunctionAnalyzer::addAccessPath(AccessPath *AccessPath, bool IsWrite) {
  CurrStatementInfo->getAccessPaths().insert(AccessPath, IsWrite);
}

void FunctionAnalyzer::addReplacedNodeAccessPath(AccessPath *AccessPath) {
  CurrStatementInfo->getAccessPaths().insertReplacedAccessPath(AccessPath);
}

bool FunctionAnalyzer::collectAccessPath_VisitStaticCastExpr(
    clang::CXXStaticCastExpr *Expr) {
  auto *SubExpr = Expr->getSubExpr()->IgnoreImplicit();

  if (SubExpr->getStmtClass() == clang::Stmt::MemberExprClass) {
    AccessPath *NewAccessPath = new AccessPath(SubExpr, this);
    if (!NewAccessPath->isLegal()) {
      delete NewAccessPath;
      return false;
    }
    addAccessPath(NewAccessPath, false);
    return true;
  } else if (!collectAccessPath_handleSubExpr(SubExpr)) {
    Logger::getStaticLogger().logError(
        "error in FunctionAnalyzer::collectAccessPath_VisitStaticCastExpr ");
    return false;
  }
}

bool FunctionAnalyzer::collectAccessPath_VisitBinaryOperator(
    clang::BinaryOperator *BinaryExpr) {

  // Handle new statement: <tree-node> = new <tree-structure>()
  // Rightnow the current only supported format is <tree-node> = new Type();
  // No user defined constructors are allowed
  if (BinaryExpr->isAssignmentOp() &&
      BinaryExpr->getRHS()->getStmtClass() == clang::Stmt::CXXNewExprClass) {

    // LHS
    AccessPath *ReplaceNode = new AccessPath(BinaryExpr->getLHS(), this);

    if (!ReplaceNode->isLegal() || !ReplaceNode->getValuePathSize() == 0 ||
        !ReplaceNode->isOnTree() ||
        !ReplaceNode->onlyUses(
            getTraversedNodeDecl(),
            RecordsAnalyzer::getRecordInfo(getTraversedTreeTypeDecl())
                .getRecursiveFields())) {

      Logger::getStaticLogger().logError(
          "FunctionAnalyzer::collectAccessPath_VisitBinaryOperator: "
          "new statement LHS is not legal");
      delete ReplaceNode;
      return false;
    }
    // Make sure no constructor is called
    auto *ConstructorDecl = dyn_cast<CXXNewExpr>(BinaryExpr->getRHS())
                                ->getConstructExpr()
                                ->getConstructor();

    if (!ConstructorDecl->isTrivial() || !ConstructorDecl->isImplicit()) {
      Logger::getStaticLogger().logError(
          "FunctionAnalyzer::collectAccessPath_VisitBinaryOperator: "
          "constructor not supported");
      delete ReplaceNode;
      return false;
    }
    addReplacedNodeAccessPath(ReplaceNode);
    return true;
  }

  // RHS
  if (BinaryExpr->getRHS()->IgnoreImplicit()->getStmtClass() ==
          clang::Stmt::MemberExprClass ||
      BinaryExpr->getRHS()->IgnoreImplicit()->getStmtClass() ==
          clang::Stmt::DeclRefExprClass) {

    AccessPath *NewAccessPath = new AccessPath(BinaryExpr->getRHS(), this);
    if (!NewAccessPath->isLegal()) {
      delete NewAccessPath;
      return false;
    }
    addAccessPath(NewAccessPath, false);

  } else {
    if (!collectAccessPath_handleSubExpr(BinaryExpr->getRHS())) {

      Logger::getStaticLogger().logError(
          "Error1 in  FunctionAnalyzer::collectAccessPath_VisitBinaryOperator "
          ": BIN RHS unsupported stmt class");
      return false;
    }
  }

  // LHS
  if (BinaryExpr->getLHS()->IgnoreImplicit()->getStmtClass() ==
          clang::Stmt::MemberExprClass ||
      BinaryExpr->getLHS()->IgnoreImplicit()->getStmtClass() ==
          clang::Stmt::DeclRefExprClass) {

    AccessPath *NewAccessPath = new AccessPath(BinaryExpr->getLHS(), this);
    if (!NewAccessPath->isLegal()) {
      delete NewAccessPath;
      return false;
    }

    if (BinaryExpr->isAssignmentOp())
      addAccessPath(NewAccessPath, true);
    else
      addAccessPath(NewAccessPath, false);

    // TODO : create enum what is -1!
    if (BinaryExpr->isAssignmentOp()) {
      if (NewAccessPath->isOnTree() &&
          NewAccessPath->getValueStartIndex() == -1) {
        Logger::getStaticLogger().logError(
            "collectAccessPath_VisitBinaryOperator: writing to tree nodes not "
            "allowed ");
        return false;
      }
    }

  } else {
    if (!collectAccessPath_handleSubExpr(BinaryExpr->getLHS())) {
      Logger::getStaticLogger().logError(
          "FunctionAnalyzer::collectAccessPath_VisitBinaryOperator in LHS "
          "unsupported stmt class");
      return false;
    }
  }
  return true;
}

bool FunctionAnalyzer::collectAccessPath_VisitCallExpr(clang::CallExpr *Expr) {
  if (FuncDeclNode != Expr->getCalleeDecl()) {

    if (!hasStrictAccessAnnotation(Expr->getCalleeDecl())) {
      Logger::getStaticLogger().logError(
          "FunctionAnalyzer::collectAccessPath_VisitCallExpr:  this check must "
          "be removed from here , this is not the place for these checks "
          "Error1 in VisitCallExpr "); //?? why
      return false;
    }

    // A special type of access paths is added
    std::vector<StrictAccessInfo> AnnotatedAccesses =
        getStrictAccessInfo(Expr->getDirectCallee());

    for (auto &AnnotatedAccess : AnnotatedAccesses) {
      AccessPath *NewAccessPath = new AccessPath(Expr->getDirectCallee(), this);
      NewAccessPath->setAnnotationInfo(AnnotatedAccess);
      if (NewAccessPath->isLegal() == false) {
        delete NewAccessPath;
        return false;
      }
      if (!NewAccessPath->getAnnotationInfo().IsGlobal) {
        Expr->dump();
        Logger::getStaticLogger().logError(
            "FunctionAnalyzer::collectAccessPath_VisitCallExpr:  strict "
            "access call must be global for for non CXXMemberCallExpr ");
        return false;
      }
      bool IsWrite = false;

      if (!NewAccessPath->getAnnotationInfo().IsReadOnly)
        IsWrite = true;

      addAccessPath(NewAccessPath, IsWrite);
    }
  }

  if (FuncDeclNode == Expr->getCalleeDecl() && NestedIfDepth != 0) {
    Logger::getStaticLogger().logError(
        "FunctionAnalyzer::collectAccessPath_VisitCallExpr: not allowed to "
        "have conditioned recursive call ");
    return false;
  }

  for (auto *Argument : Expr->arguments()) {
    Argument = Argument->IgnoreImplicit();
    if (Argument->getStmtClass() == clang::Stmt::MemberExprClass ||
        Argument->getStmtClass() == clang::Stmt::DeclRefExprClass) {

      AccessPath *NewAccessPath = new AccessPath(Argument, this);

      if (!NewAccessPath->isLegal()) {
        delete NewAccessPath;
        return false;
      }
      addAccessPath(NewAccessPath, false);

    } else {
      if (!collectAccessPath_handleSubExpr(Argument)) {
        Logger::getStaticLogger().logError(
            "FunctionAnalyzer::collectAccessPath_VisitCallExpr :unsupported "
            "argument type");
        return false;
      }
    }
  }

  return true;
}

bool FunctionAnalyzer::collectAccessPath_VisitCompoundStmt(
    clang::CompoundStmt *Stmt) {
  assert(Stmt);

  // Check each statement alone
  for (auto *ChildStmt : Stmt->children()) {
    // then we are in the main body becouse we are not inside and if statement
    ChildStmt = ChildStmt->IgnoreImplicit();
    if (NestedIfDepth == 0) {

      bool IsRecursiveCall =
          ChildStmt->getStmtClass() == clang::Stmt::CallExprClass &&
          dyn_cast<clang::CallExpr>(ChildStmt)->getCalleeDecl() == FuncDeclNode;

      CurrStatementInfo = new StatementInfo(ChildStmt, this, IsRecursiveCall,
                                            getStatements().size());
      this->getStatements().push_back(CurrStatementInfo);

      if (IsRecursiveCall) {
        AccessPath Arg0((dyn_cast<clang::CallExpr>(ChildStmt))->getArg(0),
                        nullptr); // dummmy access path
        CurrStatementInfo->setCalledChild(
            dyn_cast<clang::FieldDecl>(Arg0.SplittedAccessPath[1].second));
      }
    }

    if (!collectAccessPath_handleStmt(ChildStmt))
      return false;
  }

  return true;
}

bool FunctionAnalyzer::collectAccessPath_VisitIfStmt(clang::IfStmt *Stmt) {

  // Check the condition part first
  if (Stmt->getCond()) {
    auto *Cond = Stmt->getCond()->IgnoreImplicit();

    if (Cond->getStmtClass() == clang::Stmt::MemberExprClass ||
        Cond->getStmtClass() == clang::Stmt::DeclRefExprClass) {

      AccessPath *NewAccessPath = new AccessPath(Cond, this);
      if (!NewAccessPath->isLegal()) {
        delete NewAccessPath;
        return false;
      }

      addAccessPath(NewAccessPath, false);

    } else {
      if (!collectAccessPath_handleSubExpr(Cond)) {
        Logger::getStaticLogger().logError(
            "FunctionAnalyzer::collectAccessPath_VisitIfStmt  unsupported "
            "expression inside condition part");
        return false;
      }
    }
  }
  // Check then part
  if (Stmt->getThen()) {
    auto *ThenPart = Stmt->getThen()->IgnoreImplicit();

    if (!collectAccessPath_handleStmt(ThenPart)) {
      Logger::getStaticLogger().logError(
          "FunctionAnalyzer::collectAccessPath_ VisitIfStmt : unsupported "
          "statement type in then part");
      return false;
    }
  }

  // Check else part
  if (Stmt->getElse()) {
    auto *ElsePart = Stmt->getElse()->IgnoreImplicit();

    if (!collectAccessPath_handleStmt(ElsePart)) {
      Logger::getStaticLogger().logError(
          "FunctionAnalyzer::collectAccessPath_VisitIfStmt : unsupported stmt "
          "type in else part");
      return false;
    }
  }
  return true;
}

bool FunctionAnalyzer::collectAccessPath_VisitParenExpr(
    clang::ParenExpr *Expr) {

  if (Expr->getSubExpr()->getStmtClass() == Stmt::MemberExprClass) {
    AccessPath *NewAccessPath = new AccessPath(Expr->getSubExpr(), this);

    if (!NewAccessPath->isLegal()) {
      delete NewAccessPath;
      return false;
    }
    addAccessPath(NewAccessPath, false);

  } else {
    if (!collectAccessPath_handleSubExpr(Expr->getSubExpr())) {
      Logger::getStaticLogger().logError(
          "FunctionAnalyzer::collectAccessPath_VisitParenExpr not allowed "
          "subexpression");
      return false;
    }
  }
  return true;
}

bool FunctionAnalyzer::collectAccessPath_VisitDeclsStmt(clang::DeclStmt *Stmt) {
  for (auto *Decl : Stmt->decls()) {
    auto *VarDecl = dyn_cast<clang::VarDecl>(Decl);

    assert(VarDecl);

    if (!RecordsAnalyzer::isCompleteScaler(VarDecl)) {
      Logger::getStaticLogger().logError(
          "FunctionAnalyzer::collectAccessPath_VisitDeclsStmt declaration  "
          "type is not allowed");
      return false;
    }

    // Add write access path for declaration
    AccessPath *NewAccessPath = new AccessPath(VarDecl, this);
    if (!NewAccessPath->isLegal()) {
      delete NewAccessPath;
      return false;
    }
    addAccessPath(NewAccessPath, true);

    // Check the initializing part
    auto *ExprInit = VarDecl->getInit();

    if (ExprInit) {
      ExprInit = VarDecl->getInit()->IgnoreImpCasts();
      if (ExprInit->getStmtClass() == Stmt::MemberExprClass ||
          ExprInit->getStmtClass() == Stmt::DeclRefExprClass) {

        AccessPath *NewAccessPath = new AccessPath(ExprInit, this);

        if (!NewAccessPath->isLegal()) {
          delete NewAccessPath;
          return false;
        }
        addAccessPath(NewAccessPath, false);

      } else {
        if (!collectAccessPath_handleSubExpr(ExprInit)) {
          Logger::getStaticLogger().logError(
              "FunctionAnalyzer::collectAccessPath_VisitDeclsStmt : "
              "declaration initialization not allowed ");
          return false;
        }
      }
    }
  }
  return true;
}

bool FunctionAnalyzer::collectAccessPath_VisitCXXDeleteExpr(
    clang::CXXDeleteExpr *Expr) {

  auto *DeleteDecl = Expr->getOperatorDelete();

  if (DeleteDecl->hasBody() || Expr->isArrayForm() || Expr->isGlobalDelete()) {
    Logger::getStaticLogger().logError(
        "FunctionAnalyzer::collectAccessPath_VisitCXXDeleteExpr : "
        "not supported delete");
    return false;
  }

  // Make sure there are no destructors
  std::stack<const clang::CXXRecordDecl *> Stack;
  Stack.push(Expr->getDestroyedType()->getAsCXXRecordDecl());

  while (!Stack.empty()) {
    const clang::CXXRecordDecl *TopOfStack = Stack.top();
    Stack.pop();

    if (TopOfStack->hasUserDeclaredDestructor()) {
      Logger::getStaticLogger().logError(
          "FunctionAnalyzer::collectAccessPath_VisitCXXDeleteExpr : "
          "user declared destructors not unsupported");
      return false;
    }

    for (auto &BaseClass : TopOfStack->bases())
      Stack.push(BaseClass.getType()->getAsCXXRecordDecl());
  }

  AccessPath *NewAccessPath = new AccessPath(Expr->getArgument(), this);
  if (!NewAccessPath->isLegal() || !NewAccessPath->isOnTree() ||
      !NewAccessPath->getValuePathSize() == 0 ||
      !NewAccessPath->onlyUses(
          getTraversedNodeDecl(),
          RecordsAnalyzer::getRecordInfo(getTraversedTreeTypeDecl())
              .getRecursiveFields())) {
    Logger::getStaticLogger().logError(
        "FunctionAnalyzer::collectAccessPath_VisitCXXDeleteExpr : "
        "not supported delete argument");
    delete NewAccessPath;
    return false;
  }

  addReplacedNodeAccessPath(NewAccessPath);

  return true;
}
FunctionAnalyzer::~FunctionAnalyzer() {
  for (auto *StmtInfo : Statements) {
    delete StmtInfo;
  }
}

//*****************************************************
