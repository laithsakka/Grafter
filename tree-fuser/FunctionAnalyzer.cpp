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

int FunctionAnalyzer::getNumberOfTraversingCalls() const {
  return TraversingCalls.size();
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

const vector<pair<clang::FunctionDecl *, clang::FieldDecl *>> &
FunctionAnalyzer::getTraversingCalls() const {
  return TraversingCalls;
}

FunctionAnalyzer::FunctionAnalyzer(clang::FunctionDecl *FuncDeclaration) {
  this->FuncDeclNode = FuncDeclaration;
  this->SemanticsSatasified = checkFuseSema();
  return;
}

bool FunctionAnalyzer::isInCalledChildList(clang::FieldDecl *ChildDecl) const {
  for (auto Call : TraversingCalls) {
    clang::FieldDecl *CalledChildDecl = Call.second;
    if (ChildDecl == CalledChildDecl)
      return true;
  }
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

  case clang::Stmt::CXXMemberCallExprClass: {
    auto *CallExpr = dyn_cast<clang::CallExpr>(Stmt);
    if (hasFuseAnnotation(
            dyn_cast<clang::FunctionDecl>(CallExpr->getCalleeDecl()))) {
      return collectAccessPath_VisitCallExpr(CallExpr);
    } else {
      return collectAccessPath_VisitCXXMemberCallExpr(
          dyn_cast<clang::CXXMemberCallExpr>(Stmt));
    }
  }
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
    return Logger::getStaticLogger().logError(
        "function calls with no strict access annotation are not allowed");
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
  if (!FuncDeclNode->isGlobal() && !FuncDeclNode->isCXXClassMember()) {
    Logger::getStaticLogger().logError(
        "fuse traversal must be global or tree class member ");
    return false;
  }

  if (FuncDeclNode->isGlobal()) {
    setGlobal();
  }

  clang::RecordDecl *ContainingRecordDecl;
  if (FuncDeclNode->isCXXClassMember()) {

    ContainingRecordDecl = dyn_cast<CXXMethodDecl>(FuncDeclNode)->getParent();
    if (!RecordsAnalyzer::getRecordInfo(ContainingRecordDecl)
             .isTreeStructure()) {
      return Logger::getStaticLogger().logError(
          "fuse traversal class member must belong to tree structure");
    }
    setCXXMember();
  }

  // Return type must be void
  if (!FuncDeclNode->getReturnType()->isVoidType()) {
    Logger::getStaticLogger().logError("fuse method return type must be void");
    return false;
  }

  // First parameter must be pointer to a tree structure if the function is
  // global
  if (isGlobal()) {
    if (FuncDeclNode->parameters().size() == 0 ||
        !FuncDeclNode->parameters()[0]->getType()->isPointerType()) {
      Logger::getStaticLogger().logError(
          "fuse method first parameter must be pointer to a tree structure");
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
  } else {
    /// The traversed node is the implicit this
    TraversedNodeDecl = nullptr;
    TraversedTreeTypeDecl = ContainingRecordDecl;
  }

  // All other parameters must be complete  scalers
  for (int i = isGlobal() ? 1 : 0; i < FuncDeclNode->parameters().size(); i++) {
    if (!RecordsAnalyzer::isCompleteScaler(FuncDeclNode->parameters()[i])) {
      FuncDeclNode->parameters()[i]->dump();

      return Logger::getStaticLogger().logError(
          "fuse method has non complete-scaler parameter ");
    }
  }

  // Analyze top level traversing calls
  for (auto *Stmt : FuncDeclNode->getBody()->children()) {

    if (Stmt->getStmtClass() != clang::Stmt::CallExprClass &&
        Stmt->getStmtClass() != clang::Stmt::CXXMemberCallExprClass)
      continue;

    auto *Call = dyn_cast<clang::CallExpr>(Stmt);
    if (!hasFuseAnnotation(Call->getCalleeDecl()->getAsFunction())) {
      if (!hasStrictAccessAnnotation(Call->getCalleeDecl())) {
        Logger::getStaticLogger().logError(
            "fuse methods body not allowed to have function calls");
        return false;
      }
    }

    if (hasFuseAnnotation(Call->getCalleeDecl()->getAsFunction())) {

      AccessPath CalledChildAccessPath(
          isGlobal()
              ? Call->getArg(0)
              : dyn_cast<clang::MemberExpr>(
                    Call->child_begin()->child_begin()->IgnoreImplicit()),
          nullptr); // dummmy access path

      if (!CalledChildAccessPath.isLegal() ||
          CalledChildAccessPath.getDepth() != 2) {
        return Logger::getStaticLogger().logError(
            "illegal access path in recursive call first argument");
      }

      if (!CalledChildAccessPath.onlyUses(
              TraversedNodeDecl /* would be null for this*/,
              RecordsAnalyzer::getRecursiveFields(TraversedTreeTypeDecl))) {
        return Logger::getStaticLogger().logError(
            "fuse method recursive call is a not traversing a recognized "
            "child ");
      }

      clang::FieldDecl *ChildDecl = dyn_cast<clang::FieldDecl>(
          CalledChildAccessPath.SplittedAccessPath[1].second);
      assert(ChildDecl != nullptr);

      addTraversingCall(Call->getCalleeDecl()->getAsFunction()->getDefinition(),
                        ChildDecl);
    }
  }

  assert(FuncDeclNode->getBody()->IgnoreImplicit()->getStmtClass() ==
         clang::Stmt::CompoundStmtClass);

  return collectAccessPath_VisitCompoundStmt(
      dyn_cast<clang::CompoundStmt>(FuncDeclNode->getBody()));
}

void FunctionAnalyzer::addAccessPath(AccessPath *AccessPath, bool IsWrite) {
  LLVM_DEBUG(outs() << this->FuncDeclNode->getQualifiedNameAsString()
                    << "adding ap:" << CurrStatementInfo->getStatementId()
                    << "\n";
             AccessPath->dump());

  if (AccessPath->fromAliasing())
    addAccessPath(AccessPath->getAliasingDeclAccessPath(), false);

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
      BinaryExpr->getRHS()->IgnoreImplicit()->getStmtClass() ==
          clang::Stmt::CXXNewExprClass) {

    // LHS
    AccessPath *ReplaceNode =
        new AccessPath(BinaryExpr->getLHS()->IgnoreImplicit(), this);

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
    auto *ConstructorDecl =
        dyn_cast<CXXNewExpr>(BinaryExpr->getRHS()->IgnoreImplicit())
            ->getConstructExpr()
            ->getConstructor();

    if (/*!ConstructorDecl->isTrivial() ||*/ !ConstructorDecl->isImplicit()) {
      ConstructorDecl->dump();
      Logger::getStaticLogger().logError(
          "FunctionAnalyzer::collectAccessPath_VisitBinarƒyOperator: "
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
      return Logger::getStaticLogger().logError(
          "Error1 in  FunctionAnalyzer::collectAccessPath_VisitBinaryOperator "
          ": BIN RHS unsupported stmt class");
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
  if (!hasFuseAnnotation(Expr->getCalleeDecl()->getAsFunction())) {

    if (!hasStrictAccessAnnotation(Expr->getCalleeDecl())) {
      Logger::getStaticLogger().logError(
          "FunctionAnalyzer::collectAccessPath_VisitCallExpr:  this check must "
          "be removed from here , this is not the place for these checks "
          "Error1 in VisitCallExpr ");
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
    return true;
  }

  // Handle calls to functions that have fuse annotations (tree traversals)
  if (hasFuseAnnotation(Expr->getCalleeDecl()->getAsFunction()) &&
      NestedIfDepth != 0) {
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
        return Logger::getStaticLogger().logError(
            "FunctionAnalyzer::collectAccessPath_VisitCallExpr :unsupported "
            "argument type");
      }
    }
  }

  if (Expr->getStmtClass() == clang::Stmt::CXXMemberCallExprClass) {
    AccessPath *NewAccessPath = new AccessPath(
        dyn_cast<clang::MemberExpr>(
            Expr->child_begin()->child_begin()->IgnoreImplicit()),
        this);
    if (!NewAccessPath->isLegal()) {
      delete NewAccessPath;
      return false;
    }
    addAccessPath(NewAccessPath, false);
  }

  return true;
}

bool FunctionAnalyzer::collectAccessPath_VisitCompoundStmt(
    clang::CompoundStmt *Stmt) {
  assert(Stmt);

  // Check each statement alone
  for (auto *ChildStmt : Stmt->children()) {
    // then we are in the main body because we are not inside and if statement
    ChildStmt = ChildStmt->IgnoreImplicit();

    if (NestedIfDepth == 0) {
      bool isTraversingCall =
          (ChildStmt->getStmtClass() == clang::Stmt::CallExprClass ||
           ChildStmt->getStmtClass() == clang::Stmt::CXXMemberCallExprClass) &&
          hasFuseAnnotation(dyn_cast<clang::CallExpr>(ChildStmt)
                                ->getCalleeDecl()
                                ->getAsFunction());

      CurrStatementInfo = new StatementInfo(ChildStmt, this, isTraversingCall,
                                            getStatements().size());
      Statements.push_back(CurrStatementInfo);

      if (isTraversingCall) {
        auto *CalledFunction = dyn_cast<clang::CallExpr>(ChildStmt)
                                   ->getCalleeDecl()
                                   ->getAsFunction()
                                   ->getDefinition();
        if (!CalledFunction) {
          Logger::getStaticLogger().logWarn(
              "A tree traversal is declared but not defined:" +
              dyn_cast<clang::CallExpr>(ChildStmt)
                  ->getCalleeDecl()
                  ->getAsFunction()
                  ->getQualifiedNameAsString());
          return false;
        }
        AccessPath Arg0(
            CalledFunction->isGlobal()
                ? (dyn_cast<clang::CallExpr>(ChildStmt))->getArg(0)
                : dyn_cast<clang::MemberExpr>(ChildStmt->child_begin()
                                                  ->child_begin()
                                                  ->IgnoreImplicit()),
            nullptr); // dummmy access path
        CurrStatementInfo->setCalledChild(
            dyn_cast<clang::FieldDecl>(Arg0.SplittedAccessPath[1].second));
        auto *CalleFuncDecl = dyn_cast<clang::CallExpr>(ChildStmt)
                                  ->getCalleeDecl()
                                  ->getAsFunction()
                                  ->getDefinition();

        CurrStatementInfo->setCalledFunction(CalleFuncDecl);
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
          "FunctionAnalyzer::collectAccessPath_VisitIfStmt : unsupported "
          "stmt "
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

    // Allow aliasing of the form const TreeNode = <On-Tree-Node>
    if (VarDecl->getType()->isPointerType()) {
      clang::QualType Type = VarDecl->getType();

      if (!Type.isConstQualified())
        return Logger::getStaticLogger().logError(
            "FunctionAnalyzer::collectAccessPath_VisitDeclsStmt :Aliasing "
            "statement should define constant pointer");

      AccessPath *NewAccessPath = new AccessPath(VarDecl, this);

      if (!NewAccessPath->isLegal())
        llvm_unreachable("unexpected error");

      addAccessPath(NewAccessPath, true);

      if (!VarDecl->getInit())
        return Logger::getStaticLogger().logError(
            "aliasing statement must have rhs");

      auto *ExprInit = VarDecl->getInit()->IgnoreImpCasts()->IgnoreCasts();

      if (ExprInit->getStmtClass() == Stmt::MemberExprClass ||
          ExprInit->getStmtClass() == Stmt::DeclRefExprClass) {

        AccessPath *NewAccessPath = new AccessPath(ExprInit, this);
        addAccessPath(NewAccessPath, false);

        if (!NewAccessPath->isLegal() || !NewAccessPath->isOnTree() ||
            NewAccessPath->hasValuePart())
          return Logger::getStaticLogger().logError(
              "rhs of aliasing statement not valid");

        addAliasing(VarDecl, NewAccessPath);

      } else {
        return Logger::getStaticLogger().logError(
            "rhs of aliasing statement not valids");
      }
      continue;
    }

    if (!RecordsAnalyzer::isCompleteScaler(VarDecl))
      return Logger::getStaticLogger().logError(
          "FunctionAnalyzer::collectAccessPath_VisitDeclsStmt declaration  "
          "type is not allowed");

    // Add write access path for declaration
    AccessPath *NewAccessPath = new AccessPath(VarDecl, this);
    addAccessPath(NewAccessPath, true);

    if (!NewAccessPath->isLegal())
      return NewAccessPath;

    // Check the initializing part
    if (!VarDecl->getInit())
      continue;

    auto *ExprInit = VarDecl->getInit()->IgnoreImpCasts();
    if (ExprInit->getStmtClass() == Stmt::MemberExprClass ||
        ExprInit->getStmtClass() == Stmt::DeclRefExprClass) {

      AccessPath *NewAccessPath = new AccessPath(ExprInit, this);

      if (!NewAccessPath->isLegal())
        return false;

      addAccessPath(NewAccessPath, false);

    } else {
      if (!collectAccessPath_handleSubExpr(ExprInit))
        return Logger::getStaticLogger().logError(
            "FunctionAnalyzer::collectAccessPath_VisitDeclsStmt : "
            "declaration initialization not allowed ");
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

    for (auto &BaseClass : TopOfStack->bases()) {
      assert(BaseClass.getType()->getAsCXXRecordDecl());
      Stack.push(BaseClass.getType()->getAsCXXRecordDecl());
    }
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
