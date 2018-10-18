
#include "virtual.h"



__tree_traversal__ void AssignmentNode::foldConstants() {
  AssignedExpr->foldConstants();

  if (AssignedExpr->ExpressionType != BINARY)
    return;

  BinaryExpressionNode *const AssignedExpr_ =
      static_cast<BinaryExpressionNode *>(AssignedExpr);

  if (AssignedExpr_->LHS->ExpressionType != CONSTANT ||
      AssignedExpr_->RHS->ExpressionType != CONSTANT)
    return;

  int NewValue;

  if (AssignedExpr_->Operator == ADD)
    NewValue = static_cast<ConstantNode *>(AssignedExpr_->LHS)->Value +
               static_cast<ConstantNode *>(AssignedExpr_->RHS)->Value;

  if (AssignedExpr_->Operator == SUBTRACT)
    NewValue = static_cast<ConstantNode *>(AssignedExpr_->LHS)->Value -
               static_cast<ConstantNode *>(AssignedExpr_->RHS)->Value;

  // Maybe we can write the delete as a traversal that would be cool
  delete AssignedExpr_->LHS;
  delete AssignedExpr_->RHS;
  delete AssignedExpr;
  AssignedExpr = new ConstantNode();
  AssignedExpr->NodeType = EXPR;
  AssignedExpr->ExpressionType = CONSTANT;
  static_cast<ConstantNode *>(AssignedExpr)->Value = NewValue;
}

__tree_traversal__ void SequenceNode::foldConstants() {
  FirstStmt->foldConstants();
  SecondStmt->foldConstants();
}

__tree_traversal__ void BinaryExpressionNode::foldConstants() {
  LHS->foldConstants();
  RHS->foldConstants();

  // If we allows passing nodes as reference to pointer then we will be able to solve the issue of 
  // replicating this piece of code three times or if we allow return types rather than void
  if (LHS->ExpressionType == BINARY) {
    BinaryExpressionNode *const LHS_ =
        static_cast<BinaryExpressionNode *>(this->LHS);

    if (LHS_->LHS->ExpressionType == CONSTANT &&
        LHS_->RHS->ExpressionType == CONSTANT) {
      int NewValue;
      if (LHS_->Operator == ADD)
        NewValue = static_cast<ConstantNode *>(LHS_->LHS)->Value +
                   static_cast<ConstantNode *>(LHS_->RHS)->Value;

      if (LHS_->Operator == SUBTRACT)
        NewValue = static_cast<ConstantNode *>(LHS_->LHS)->Value -
                   static_cast<ConstantNode *>(LHS_->RHS)->Value;
      delete LHS_->LHS;
      delete LHS_->RHS;
      delete LHS;
      LHS = new ConstantNode();
      LHS->NodeType = EXPR;
      LHS->ExpressionType = CONSTANT;
      static_cast<ConstantNode *>(LHS)->Value = NewValue;
      
    }
  }

  if (RHS->ExpressionType == BINARY) {
    BinaryExpressionNode *const RHS_ = static_cast<BinaryExpressionNode *>(RHS);
    if (RHS_->LHS->ExpressionType == CONSTANT &&
        RHS_->RHS->ExpressionType == CONSTANT) {
      int NewValue;
      if (RHS_->Operator == ADD)
        NewValue = static_cast<ConstantNode *>(RHS_->LHS)->Value +
                   static_cast<ConstantNode *>(RHS_->RHS)->Value;

      if (RHS_->Operator == SUBTRACT)
        NewValue = static_cast<ConstantNode *>(RHS_->LHS)->Value -
                   static_cast<ConstantNode *>(RHS_->RHS)->Value;
      delete RHS_->LHS;
      delete RHS_->RHS;
      delete RHS;
      RHS = new ConstantNode();
      RHS->NodeType = EXPR;
      RHS->ExpressionType = CONSTANT;
      static_cast<ConstantNode *>(RHS)->Value = NewValue;
    }
  }
}

int main() {
  ASTNode *root;
  root->foldConstants();
  root->foldConstants();
}
