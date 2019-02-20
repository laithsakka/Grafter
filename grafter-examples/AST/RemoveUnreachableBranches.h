#include "AST.h"

__tree_traversal__ void Program::removeUnreachableBranches() {
  COUNT
  Functions->removeUnreachableBranches();
}

__tree_traversal__ void StmtListInner::removeUnreachableBranches() {
  COUNT
  Stmt->removeUnreachableBranches();
  Next->removeUnreachableBranches();
}

__tree_traversal__ void StmtListEnd::removeUnreachableBranches() {
  COUNT
  Stmt->removeUnreachableBranches();
}

__tree_traversal__ void Function::removeUnreachableBranches() {
  COUNT
  StmtList->removeUnreachableBranches();
}

__tree_traversal__ void FunctionListInner::removeUnreachableBranches() {
  COUNT
  Content->removeUnreachableBranches();
  Next->removeUnreachableBranches();
}

__tree_traversal__ void FunctionListEnd::removeUnreachableBranches() {
  COUNT
  Content->removeUnreachableBranches();
}

__tree_traversal__ void IfStmt::removeUnreachableBranches() {
  COUNT
  if (Condition->ExpressionType == CONSTANT) {
    auto *const ConstantCond = static_cast<ConstantExpr *>(Condition);
    if (ConstantCond->Value == 0) {
      delete ThenPart;
      ThenPart = new StmtListEnd();
      ThenPart->NodeType = SEQ;
      ThenPart->Stmt = new NullStmt();
      ThenPart->Stmt->NodeType = STMT;
      ThenPart->Stmt->StatementType = NOP;
    }
    if (ConstantCond->Value != 0) {
      delete ElsePart;
      ElsePart = new StmtListEnd();
      ElsePart->NodeType = SEQ;
      ElsePart->Stmt = new NullStmt();
      ElsePart->Stmt->NodeType = STMT;
      ElsePart->Stmt->StatementType = NOP;
    }
  }
  ElsePart->removeUnreachableBranches();
  ThenPart->removeUnreachableBranches();
}
