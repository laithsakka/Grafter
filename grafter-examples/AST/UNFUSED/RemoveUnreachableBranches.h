#include "AST.h"

__tree_traversal__ void Program::removeUnreachableBranches() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  Functions->removeUnreachableBranches();
}

__tree_traversal__ void StmtListInner::removeUnreachableBranches() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  Stmt->removeUnreachableBranches();
  Next->removeUnreachableBranches();
}

__tree_traversal__ void StmtListEnd::removeUnreachableBranches() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  Stmt->removeUnreachableBranches();
}

__tree_traversal__ void Function::removeUnreachableBranches() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  StmtList->removeUnreachableBranches();
}

__tree_traversal__ void FunctionListInner::removeUnreachableBranches() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  Content->removeUnreachableBranches();
  Next->removeUnreachableBranches();
}

__tree_traversal__ void FunctionListEnd::removeUnreachableBranches() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  Content->removeUnreachableBranches();
}

__tree_traversal__ void IfStmt::removeUnreachableBranches() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
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
