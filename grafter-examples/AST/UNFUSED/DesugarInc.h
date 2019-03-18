#include "AST.h"

__tree_traversal__ void Program::desugarInc() {
 #ifdef COUNT_VISITS
 _VISIT_COUNTER++;
 #endif
  Functions->desugarInc();
}

__tree_traversal__ void FunctionListInner::desugarInc() {
 #ifdef COUNT_VISITS
  _VISIT_COUNTER++;
  #endif
  Content->desugarInc();
  Next->desugarInc();
}

__tree_traversal__ void FunctionListEnd::desugarInc() {
 #ifdef COUNT_VISITS
 _VISIT_COUNTER++;
  #endif
   Content->desugarInc();
}
__tree_traversal__ void Function::desugarInc() {
  #ifdef COUNT_VISITS
  _VISIT_COUNTER++;
  #endif
  StmtList->desugarInc(); }

__tree_traversal__ void StmtListInner::desugarInc() {
 #ifdef COUNT_VISITS
  _VISIT_COUNTER++;
   #endif
  Stmt->desugarInc();

  if (Stmt->StatementType == INC) {
    int Variable =
        static_cast<VarRefExpr *>(static_cast<IncrStmt *>(Stmt)->Id)->VarId;
    delete Stmt;
    Stmt = new AssignStmt();
    AssignStmt *const Assignment = static_cast<AssignStmt *>(Stmt);
    Assignment->StatementType = ASSIGNMENT;
    Assignment->NodeType = STMT;
    Assignment->AssignedExpr = new BinaryExpr();
    Assignment->Id = new VarRefExpr();
    Assignment->Id->NodeType = EXPR;
    Assignment->Id->ExpressionType = VARREF;

    Assignment->Id->ExpressionType = VARREF;
    static_cast<VarRefExpr *>(Assignment->Id)->VarId = Variable;

    BinaryExpr *const BinExp =
        static_cast<BinaryExpr *>(Assignment->AssignedExpr);
    BinExp->ExpressionType = BINARY;
    BinExp->NodeType = EXPR;
    BinExp->Operator = ADD;

    BinExp->LHS = new VarRefExpr();

    BinExp->LHS->NodeType = EXPR;
    BinExp->LHS->ExpressionType = VARREF;
    static_cast<VarRefExpr *>(BinExp->LHS)->VarId = Variable;

    BinExp->RHS = new ConstantExpr();
    BinExp->RHS->NodeType = EXPR;
    BinExp->RHS->ExpressionType = CONSTANT;
    static_cast<ConstantExpr *>(BinExp->RHS)->Value = 1;
  }

  Next->desugarInc();
}
__tree_traversal__ void StmtListEnd::desugarInc() {
 #ifdef COUNT_VISITS
 _VISIT_COUNTER++;
  #endif
  Stmt->desugarInc();
}

__tree_traversal__ void IfStmt::desugarInc() {
 #ifdef COUNT_VISITS
 _VISIT_COUNTER++;
 #endif
  ThenPart->desugarInc();
  ElsePart->desugarInc();
}
