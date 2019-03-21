
#include "AST.h"
#include <chrono>
#include <iostream>
#include <iterator>
#include <set>
#include <stdlib.h>
#include <vector>

#define X rand() % 5
#define Y 0

VarRefExpr *createVarRef(int VarRefId) {
  VarRefExpr *ret = new VarRefExpr();
  ret->NodeType = EXPR;
  ret->ExpressionType = VARREF;
  ret->VarId = VarRefId;
  return ret;
}

ConstantExpr *createConstantExpr(int val) {
  ConstantExpr *ret = new ConstantExpr();
  ret->NodeType = EXPR;
  ret->ExpressionType = CONSTANT;
  ret->Value = val;
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

IfStmt *createIf(int sz);
StmtListInner *createListOfStmt(int N, bool AddIf = false) {
  auto *ret = new StmtListInner();
  ret->NodeType = ASTNodeType::SEQ;

  ret->Stmt = createExprAssignment(X, createVarRef(X));
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
  if (AddIf)
    ((StmtListInner *)currStmt)->Next->Stmt = createIf(5);
  else {
    ((StmtListInner *)currStmt)->Next->Stmt = createExprAssignment(
        X, createAddExpr(
               createAddExpr(createAddExpr(createAddExpr(createVarRef(X),
                                                         createVarRef(X)),
                                           createVarRef(X)),
                             createVarRef(X)),
               createVarRef(X)));
  }
  return ret;
}

IfStmt *createIf(int sz) {
  IfStmt *ret = new IfStmt();
  ret->NodeType = STMT;
  ret->StatementType = IF;
  ret->Condition = createAddExpr(createVarRef(X), createVarRef(X));
  ret->ThenPart = createListOfStmt(sz);
  ret->ElsePart = createListOfStmt(sz);
  return ret;
}

IncrStmt *createIncr() {
  IncrStmt *ret = new IncrStmt();
  ret->NodeType = STMT;
  ret->StatementType = INC;
  ret->Id = createVarRef(Y);
  return ret;
}

IncrStmt *createIncr(int id) {
  IncrStmt *ret = new IncrStmt();
  ret->NodeType = STMT;
  ret->StatementType = INC;
  ret->Id = createVarRef(id);
  return ret;
}

Function *createFunction(int i) {
  Function *ret = new Function();
  ret->FunctionName = "F" + std::to_string((long long)i);
  ret->NodeType = ASTNodeType::FUNCTION;
  ret->StmtList = new StmtListInner();
  ret->StmtList->Stmt = createExprAssignment(Y, createConstantExpr(10));

  ((StmtListInner *)(ret->StmtList))->Next = new StmtListInner();
  ((StmtListInner *)(ret->StmtList))->Next->Stmt = createIncr();

  ((StmtListInner *)(((StmtListInner *)(ret->StmtList))->Next))->Next =
      createListOfStmt(5, true);
  return ret;
}

// This is one used for the main evaluation it uses different constructs of
// language to a create a function and replicates it FCount times.
Program *createProgram1(int FCount) {
  auto ret = new Program();
  ret->Functions = new FunctionListInner();
  auto *CurrF = ret->Functions;
  for (int i = 0; i < FCount - 2; i++) {
    CurrF->Content = createFunction(i);
    ((FunctionListInner *)CurrF)->Next = new FunctionListInner();
    CurrF = ((FunctionListInner *)CurrF)->Next;
  }
  CurrF->Content = createFunction(FCount);
  ((FunctionListInner *)CurrF)->Next = new FunctionListEnd();
  ((FunctionListInner *)CurrF)->Next->Content = createFunction(FCount + 1);
  return ret;
}

#define SZ 20
Function *createLargeFunction(int StmtCount) {
  Function *ret = new Function();
  ret->FunctionName = "largeFunction";
  ret->NodeType = ASTNodeType::FUNCTION;
  ret->StmtList = new StmtListInner();
  int i = 0;
  auto *currStmt = ret->StmtList;
  while (i != StmtCount) {
    switch (rand() % 4) {
    case 0:
      currStmt->Stmt =
          createExprAssignment(rand() % SZ, createVarRef(rand() % SZ));
      break;
    case 1:
      currStmt->Stmt =
          createExprAssignment(rand() % SZ, createConstantExpr(rand() % 1000));
      break;
    case 2:
      currStmt->Stmt = createIncr(rand() % SZ);
      break;
    case 3:
      currStmt->Stmt = createIf(1);
      break;
    }

    ((StmtListInner *)(currStmt))->Next = new StmtListInner();
    currStmt = ((StmtListInner *)(currStmt))->Next;
    i++;
  }
  currStmt->Stmt = createIncr();
  ((StmtListInner *)(currStmt))->Next = new StmtListEnd();
  ((StmtListInner *)(currStmt))->Next->Stmt = createIncr();
  return ret;
}

Program *createLargeFunctionProg(int stmtCount) {
    srand(0);

  auto ret = new Program();
  ret->Functions = new FunctionListEnd();
  ret->Functions->Content = createLargeFunction(stmtCount);
  return ret;
}

#define SZ2 1000000000
// with  a lot of propagation opportunities 
Function *createLongLiveRangeFunc(int stmtCount) {

  Function *ret = new Function();
  ret->FunctionName = "largeFunction";
  ret->NodeType = ASTNodeType::FUNCTION;
  ret->StmtList = new StmtListInner();
  int i = 0;
  std::set<int> usedLHS;
  auto *currStmt = ret->StmtList;
  while (i != stmtCount) {
    switch (rand() % 4) {
    case 0: {
      int lhs = rand() % SZ2;
      usedLHS.insert(lhs);
      int rhs = rand() % usedLHS.size();
      auto it = usedLHS.begin();
      std::advance(it, rhs);
      currStmt->Stmt = createExprAssignment(lhs, createVarRef(*it));
      break;
    }
    case 1:
    {
      int lhs = rand() % SZ2;
      currStmt->Stmt =
          createExprAssignment(lhs, createConstantExpr(rand() % 1000));
      usedLHS.insert(lhs);
      break;
    }
    case 2:
      currStmt->Stmt = createIncr(rand() % SZ2);
      break;
    case 3:
      currStmt->Stmt = createIf(1);
      break;
    }

    ((StmtListInner *)(currStmt))->Next = new StmtListInner();
    currStmt = ((StmtListInner *)(currStmt))->Next;
    i++;
  }
  currStmt->Stmt = createIncr();
  ((StmtListInner *)(currStmt))->Next = new StmtListEnd();
  ((StmtListInner *)(currStmt))->Next->Stmt = createIncr();
  return ret;
}

Program *createLongLiveRangeProg(int FCount, int stmtCount) {
  srand(0);
  auto ret = new Program();
  ret->Functions = new FunctionListInner();
  auto *CurrF = ret->Functions;
  for (int i = 0; i < FCount - 2; i++) {
    CurrF->Content = createLongLiveRangeFunc(200);
    ((FunctionListInner *)CurrF)->Next = new FunctionListInner();
    CurrF = ((FunctionListInner *)CurrF)->Next;
  }
  CurrF->Content = createLongLiveRangeFunc(200);
  ((FunctionListInner *)CurrF)->Next = new FunctionListEnd();
  ((FunctionListInner *)CurrF)->Next->Content =
      createLongLiveRangeFunc(FCount + 1);
  return ret;
}