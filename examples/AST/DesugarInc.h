#include "AST.h"

__tree_traversal__ void Program::desugarInc() { Functions->desugarInc(); }

__tree_traversal__ void FunctionListInner::desugarInc() {
  Content->desugarInc();
  Next->desugarInc();
}

__tree_traversal__ void FunctionListEnd::desugarInc() { Content->desugarInc(); }
__tree_traversal__ void Function::desugarInc() { StmtList->desugarInc(); }

__tree_traversal__ void StmtListInner::desugarInc() {
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
__tree_traversal__ void StmtListEnd::desugarInc() { Stmt->desugarInc(); }

__tree_traversal__ void IfStmt::desugarInc() {
  ThenPart->desugarInc();
  ElsePart->desugarInc();
}
