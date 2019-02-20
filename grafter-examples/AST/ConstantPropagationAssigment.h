
#include "AST.h"
#include <stdio.h>

using namespace std;
// this runs in O(N^2) in the worst case, when fused with linear traversals will
// hide the speed up since it dominates the work, for that purpose it was
// restricted to top level assignments
__tree_traversal__ void Program::propagateConstantsAssignments() {
  COUNT
  Functions->propagateConstantsAssignments();
}

__tree_traversal__ void FunctionListEnd::propagateConstantsAssignments() {
  COUNT
  Content->propagateConstantsAssignments();
}

__tree_traversal__ void FunctionListInner::propagateConstantsAssignments() {
  COUNT
  Content->propagateConstantsAssignments();
  Next->propagateConstantsAssignments();
}

__tree_traversal__ void Function::propagateConstantsAssignments() {
  COUNT
  StmtList->propagateConstantsAssignments();
}

__tree_traversal__ void StmtListInner::propagateConstantsAssignments() {
  COUNT
  int VarRefId;
  VarRefId = 0 - 1;
  int Value;
  VarRefId = 0 - 1;

  if (Stmt->StatementType == ASSIGNMENT) {
    if (static_cast<AssignStmt *>(Stmt)->AssignedExpr->ExpressionType ==
        CONSTANT) {
      VarRefId =
          static_cast<VarRefExpr *>(static_cast<AssignStmt *>(Stmt)->Id)->VarId;

      Value = static_cast<ConstantExpr *>(
                  static_cast<AssignStmt *>(Stmt)->AssignedExpr)
                  ->Value;
    }
  }

  Stmt->propagateConstantsAssignments();

  Next->replaceVarRefWithConst(VarRefId, Value);
  ;
  Next->propagateConstantsAssignments();
}

__tree_traversal__ void StmtListEnd::propagateConstantsAssignments() {
  COUNT
  Stmt->propagateConstantsAssignments();
}
__tree_traversal__ void IfStmt::propagateConstantsAssignments() {
  COUNT
  ThenPart->propagateConstantsAssignments();
  ElsePart->propagateConstantsAssignments();
}

__tree_traversal__ void StmtListInner::replaceVarRefWithConst(int VarRefId,
                                                              int Val) {
  COUNT
  if (VarRefId == (0 - 1))
    return;

  Stmt->replaceVarRefWithConst(VarRefId, Val);

  if (Stmt->StatementType == IF ||
      (Stmt->StatementType == ASSIGNMENT &&
       static_cast<VarRefExpr *>(static_cast<AssignStmt *>(Stmt)->Id)->VarId ==
           VarRefId))
    return;

  Next->replaceVarRefWithConst(VarRefId, Val);
}

__tree_traversal__ void StmtListEnd::replaceVarRefWithConst(int VarRefId,
                                                            int Val) {
  COUNT
  if (VarRefId == (0 - 1))
    return;

  Stmt->replaceVarRefWithConst(VarRefId, Val);
}

__tree_traversal__ void AssignStmt::replaceVarRefWithConst(int VarRefId,
                                                           int Val) {

  COUNT
  if (AssignedExpr->ExpressionType == VARREF &&
      static_cast<VarRefExpr *>(AssignedExpr)->VarId == VarRefId) {

    delete AssignedExpr;
    AssignedExpr = new ConstantExpr();
    static_cast<ConstantExpr *>(AssignedExpr)->ExpressionType = CONSTANT;
    static_cast<ConstantExpr *>(AssignedExpr)->Value = Val;
  }
  AssignedExpr->replaceVarRefWithConst(VarRefId, Val);
}

__tree_traversal__ void BinaryExpr::replaceVarRefWithConst(int VarRefId,
                                                           int Val) {
  COUNT
  LHS->replaceVarRefWithConst(VarRefId, Val);
  RHS->replaceVarRefWithConst(VarRefId, Val);

  if (LHS->ExpressionType == VARREF &&
      static_cast<VarRefExpr *>(LHS)->VarId == VarRefId) {

    delete LHS;
    LHS = new ConstantExpr();
    static_cast<ConstantExpr *>(LHS)->ExpressionType = CONSTANT;
    static_cast<ConstantExpr *>(LHS)->Value = Val;
  }

  if (RHS->ExpressionType == VARREF &&
      static_cast<VarRefExpr *>(RHS)->VarId == VarRefId) {
    delete RHS;
    RHS = new ConstantExpr();
    static_cast<ConstantExpr *>(RHS)->ExpressionType = CONSTANT;
    static_cast<ConstantExpr *>(RHS)->Value = Val;
  }
}

__tree_traversal__ void IfStmt::replaceVarRefWithConst(int VarRefId, int Val) {
  COUNT
  Condition->replaceVarRefWithConst(VarRefId, Val);
  ThenPart->replaceVarRefWithConst(VarRefId, Val);
  ElsePart->replaceVarRefWithConst(VarRefId, Val);
}