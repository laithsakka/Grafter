#include "AST.h"
#include "ConstantFolding.h"
#include "ConstantPropagationAssigment.h"
#include "Print.h"
#include "RemoveUnreachableBranches.h"
#include "DesugarInc.h"
#include "DesugarDec.h"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <stdlib.h>
#include <vector>
#define X rand() % 5
#define Y 0

#include <sys/time.h>
long long currentTimeInMilliseconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

VarRefExpr *createVarRef(int VarRefId) {
  VarRefExpr *ret = new VarRefExpr();
  ret->NodeType = EXPR;
  ret->ExpressionType = VARREF;
  ret->VarId = VarRefId;
  return ret;
}

ConstantExpr *createConstantExpr(int Value) {
  auto *ret = new ConstantExpr();
  ret->NodeType = EXPR;
  ret->ExpressionType = CONSTANT;
  ret->Value = Value;
  return ret;
}

AssignStmt *createExprAssignment(int VarRefId, ExpressionNode *expr) {
  auto *ret = new AssignStmt();
  ret->NodeType = ASTNodeType::STMT;
  ret->StatementType = ASSIGNMENT;
  ret->AssignedExpr = expr;
  ret->Id = new VarRefExpr();
  ((VarRefExpr *)ret->Id)->NodeType = EXPR;
  ((VarRefExpr *)ret->Id)->ExpressionType = VARREF;
  ((VarRefExpr *)ret->Id)->VarId = VarRefId;
  return ret;
}

BinaryExpr *createAddExpr(ExpressionNode *lhs, ExpressionNode *rhs) {
  auto *ret = new BinaryExpr();
  ret->NodeType = ASTNodeType::STMT;
  ret->ExpressionType = BINARY;
  ret->LHS = lhs;
  ret->RHS = rhs;
  ret->Operator = ADD;
  return ret;
}
StmtListInner *createListOfStmt(int N);

IfStmt *createIf() {
  IfStmt *ret = new IfStmt();
  ret->NodeType = STMT;
  ret->StatementType = IF;
  ret->Condition = createAddExpr(createVarRef(X), createVarRef(X));
  ret->ThenPart = createListOfStmt(10);
  ret->ElsePart = createListOfStmt(10);
  return ret;
}

StmtListInner *createListOfStmt(int N) {
  auto *ret = new StmtListInner();
  ret->NodeType = ASTNodeType::SEQ;

  ret->Stmt = createExprAssignment(X, createConstantExpr(10));
  StmtListNode *currStmt = ret;
  for (int i = 0; i < N; i++) {

    ((StmtListInner *)currStmt)->Next = new StmtListInner();
    ((StmtListInner *)currStmt)->Next->NodeType = ASTNodeType::SEQ;

    ((StmtListInner *)currStmt)->Next->Stmt = createExprAssignment(
        X, createAddExpr(
               createAddExpr(createAddExpr(createAddExpr(createVarRef(X),
                                                         createVarRef(X)),
                                           createVarRef(X)),
                             createVarRef(X)),
               createVarRef(X)));
    currStmt = ((StmtListInner *)currStmt)->Next;
  }
  ((StmtListInner *)currStmt)->Next = new StmtListEnd();
  ((StmtListInner *)currStmt)->Next->Stmt = createExprAssignment(
      X,
      createAddExpr(createAddExpr(createAddExpr(createAddExpr(createVarRef(X),
                                                              createVarRef(X)),
                                                createVarRef(X)),
                                  createVarRef(X)),
                    createVarRef(X)));
  return ret;
}

Function *createFunction(int i) {
  Function *ret = new Function();
  ret->FunctionName = "F" + to_string(i);
  ret->NodeType = ASTNodeType::FUNCTION;
  ret->StmtList = new StmtListInner();
  ret->StmtList->Stmt = createExprAssignment(X, createConstantExpr(10));
  ((StmtListInner *)(ret->StmtList))->Next = new StmtListInner();
  ((StmtListInner *)(ret->StmtList))->Next->Stmt = createIf();
  ((StmtListInner *)(((StmtListInner *)(ret->StmtList))->Next))->Next =
      createListOfStmt(10);
  return ret;
}

Program *createProgram(int FCount) {
  auto ret = new Program();
  ret->Functions = new FunctionListInner();
  auto *CurrF = ret->Functions;
  for (int i = 0; i < FCount; i++) {
    CurrF->Content = createFunction(i);
    ((FunctionListInner *)CurrF)->Next = new FunctionListInner();
    CurrF = ((FunctionListInner *)CurrF)->Next;
  }
  CurrF->Content = createFunction(FCount);
  ((FunctionListInner *)CurrF)->Next = new FunctionListEnd();
  ((FunctionListInner *)CurrF)->Next->Content = createFunction(FCount + 1);
  return ret;
}

int main() {
  vector<Program *> ls;
  const int N = 1;
  ls.resize(N);
  for (int i = 0; i < N; i++)
    ls[i] = createProgram(300000);

  auto t1 = currentTimeInMilliseconds();

  for (auto *f : ls) {
   // f->print();
    f->desugarInc();
    f->desugarDecr();
    f->propagateConstantsAssignments();
    f->foldConstants();
    f->removeUnreachableBranches();

    // printf("after:\n");
    // f->print();
  }
  auto t2 = currentTimeInMilliseconds();
  printf("duration is %llu\n", t2 - t1);

  // printf("after:\n");
  //  F->print();
}
