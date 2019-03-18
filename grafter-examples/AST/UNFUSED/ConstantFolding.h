#include "AST.h"

__tree_traversal__ void Program::foldConstants() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif

  Functions->foldConstants();
}

__tree_traversal__ void FunctionListEnd::foldConstants() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif

  Content->foldConstants();
}

__tree_traversal__ void FunctionListInner::foldConstants() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif

  Content->foldConstants();
  Next->foldConstants();
}

__tree_traversal__ void Function::foldConstants() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif

  StmtList->foldConstants();
}

__tree_traversal__ void AssignStmt::foldConstants() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  AssignedExpr->foldConstants();

  if (AssignedExpr->ExpressionType != BINARY)
    return;

  BinaryExpr *const AssignedExpr_ = static_cast<BinaryExpr *>(AssignedExpr);

  if (AssignedExpr_->LHS->ExpressionType != CONSTANT ||
      AssignedExpr_->RHS->ExpressionType != CONSTANT)
    return;

  int NewValue;

  if (AssignedExpr_->Operator == ADD)
    NewValue = static_cast<ConstantExpr *>(AssignedExpr_->LHS)->Value +
               static_cast<ConstantExpr *>(AssignedExpr_->RHS)->Value;

  if (AssignedExpr_->Operator == SUBTRACT)
    NewValue = static_cast<ConstantExpr *>(AssignedExpr_->LHS)->Value -
               static_cast<ConstantExpr *>(AssignedExpr_->RHS)->Value;

  // Maybe we can write the delete as a traversal that would be cool
  delete AssignedExpr_->LHS;
  delete AssignedExpr_->RHS;
  delete AssignedExpr;
  AssignedExpr = new ConstantExpr();
  AssignedExpr->NodeType = ASTNodeType::EXPR;
  AssignedExpr->ExpressionType = ASTExprType::CONSTANT;
  static_cast<ConstantExpr *>(AssignedExpr)->Value = NewValue;
}

__tree_traversal__ void StmtListInner::foldConstants() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  Stmt->foldConstants();
  Next->foldConstants();
}

__tree_traversal__ void StmtListEnd::foldConstants() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  Stmt->foldConstants();
}

__tree_traversal__ void BinaryExpr::foldConstants() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  LHS->foldConstants();
  RHS->foldConstants();

  // If we allows passing nodes as reference to pointer then we will be able to
  // solve the issue of replicating this piece of code three times or if we
  // allow return types rather than void
  if (LHS->ExpressionType == BINARY) {
    BinaryExpr *const LHS_ = static_cast<BinaryExpr *>(this->LHS);

    if (LHS_->LHS->ExpressionType == CONSTANT &&
        LHS_->RHS->ExpressionType == CONSTANT) {
      int NewValue;
      if (LHS_->Operator == ADD)
        NewValue = static_cast<ConstantExpr *>(LHS_->LHS)->Value +
                   static_cast<ConstantExpr *>(LHS_->RHS)->Value;

      if (LHS_->Operator == SUBTRACT)
        NewValue = static_cast<ConstantExpr *>(LHS_->LHS)->Value -
                   static_cast<ConstantExpr *>(LHS_->RHS)->Value;
      delete LHS_->LHS;
      delete LHS_->RHS;
      delete LHS;
      LHS = new ConstantExpr();
      LHS->NodeType = ASTNodeType::EXPR;
      LHS->ExpressionType = ASTExprType::CONSTANT;
      static_cast<ConstantExpr *>(LHS)->Value = NewValue;
    }
  }

  if (RHS->ExpressionType == BINARY) {
    BinaryExpr *const RHS_ = static_cast<BinaryExpr *>(RHS);
    if (RHS_->LHS->ExpressionType == CONSTANT &&
        RHS_->RHS->ExpressionType == CONSTANT) {
      int NewValue;
      if (RHS_->Operator == ADD)
        NewValue = static_cast<ConstantExpr *>(RHS_->LHS)->Value +
                   static_cast<ConstantExpr *>(RHS_->RHS)->Value;

      if (RHS_->Operator == SUBTRACT)
        NewValue = static_cast<ConstantExpr *>(RHS_->LHS)->Value -
                   static_cast<ConstantExpr *>(RHS_->RHS)->Value;
      delete RHS_->LHS;
      delete RHS_->RHS;
      delete RHS;
      RHS = new ConstantExpr();
      RHS->NodeType = ASTNodeType::EXPR;
      RHS->ExpressionType = ASTExprType::CONSTANT;
      static_cast<ConstantExpr *>(RHS)->Value = NewValue;
    }
  }
}

__tree_traversal__ void IfStmt::foldConstants() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  Condition->foldConstants();
  ThenPart->foldConstants();
  ElsePart->foldConstants();

  // If we allows passing nodes as reference to pointer then we will be able to
  // solve the issue of replicating this piece of code three times or if we
  // allow return types rather than void
  if (Condition->ExpressionType == BINARY) {
    BinaryExpr *const Condition_ = static_cast<BinaryExpr *>(this->Condition);

    if (Condition_->LHS->ExpressionType == CONSTANT &&
        Condition_->RHS->ExpressionType == CONSTANT) {
      int NewValue;
      if (Condition_->Operator == ADD)
        NewValue = static_cast<ConstantExpr *>(Condition_->LHS)->Value +
                   static_cast<ConstantExpr *>(Condition_->RHS)->Value;

      if (Condition_->Operator == SUBTRACT)
        NewValue = static_cast<ConstantExpr *>(Condition_->LHS)->Value -
                   static_cast<ConstantExpr *>(Condition_->RHS)->Value;
      delete Condition_->LHS;
      delete Condition_->RHS;
      delete Condition_;
      Condition = new ConstantExpr();
      Condition->NodeType = ASTNodeType::EXPR;
      Condition->ExpressionType = ASTExprType::CONSTANT;
      static_cast<ConstantExpr *>(Condition)->Value = NewValue;
    }
  }
}
