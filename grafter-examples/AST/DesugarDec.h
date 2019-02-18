#include "AST.h"

__tree_traversal__ void Program::desugarDecr() { Functions->desugarDecr(); }

__tree_traversal__ void FunctionListInner::desugarDecr() {
  Content->desugarDecr();
  Next->desugarDecr();
}

__tree_traversal__ void FunctionListEnd::desugarDecr() {
  Content->desugarDecr();
}
__tree_traversal__ void Function::desugarDecr() { StmtList->desugarDecr(); }

__tree_traversal__ void StmtListInner::desugarDecr() {
  Stmt->desugarDecr();

  if (Stmt->StatementType == DECR) {
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
    BinExp->Operator = SUBTRACT;

    BinExp->LHS = new VarRefExpr();

    BinExp->LHS->NodeType = EXPR;
    BinExp->LHS->ExpressionType = VARREF;
    static_cast<VarRefExpr *>(BinExp->LHS)->VarId = Variable;

    BinExp->RHS = new ConstantExpr();
    BinExp->RHS->NodeType = EXPR;
    BinExp->RHS->ExpressionType = CONSTANT;
    static_cast<ConstantExpr *>(BinExp->RHS)->Value = 1;
  }

  Next->desugarDecr();
}
__tree_traversal__ void StmtListEnd::desugarDecr() { Stmt->desugarDecr(); }

__tree_traversal__ void IfStmt::desugarDecr() {
  ThenPart->desugarDecr();
  ElsePart->desugarDecr();
}
