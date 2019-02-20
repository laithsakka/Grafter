#include "AST.h"
#include "ConstantFolding.h"
#include "ConstantPropagationAssigment.h"
#include "DesugarDec.h"
#include "DesugarInc.h"
#include "Print.h"
#include "RemoveUnreachableBranches.h"
#include <chrono>

#ifdef PAPI
#include <iostream>
using namespace std;
#include <papi.h>
#define SIZE 1
string instance("Original");
int ret;
int events[] = {PAPI_L2_DCA};
// string defs[] = {"cycles",
//                  "Instruction Count",
//                  "L2 Data Access Count",
//                  "L2 Data Miss Count",
//                  "Instruction Access Count",
//                  "L2 Instruction Miss Count",
//                  "L3 Total Access Count",
//                  "L3 Total Miss Count"};

string defs[] = {"isntr", "PAPI_INT_INS", "PAPI_L1_DCA"};
long long values[SIZE];
long long rcyc0, rcyc1, rusec0, rusec1;
long long vcyc0, vcyc1, vusec0, vusec1;

void init_papi() {
  if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
    cerr << "PAPI Init Error" << endl;
    exit(1);
  }
  for (int i = 0; i < SIZE; ++i) {
    if (PAPI_query_event(events[i]) != PAPI_OK) {
      cerr << "PAPI Event " << i << " does not exist" << endl;
    }
  }
}
void start_counters() {
  // Performance Counters Start
  if (PAPI_start_counters(events, SIZE) != PAPI_OK) {
    cerr << "PAPI Error starting counters" << endl;
  }
}
void read_counters() {
  // Performance Counters Read
  ret = PAPI_stop_counters(values, SIZE);
  if (ret != PAPI_OK) {
    if (ret == PAPI_ESYS) {
      cout << "error inside PAPI call" << endl;
    } else if (ret == PAPI_EINVAL) {
      cout << "error with arguments" << endl;
    }

    cerr << "PAPI Error reading counters" << endl;
  }
}
void print_counters() {
  for (int i = 0; i < SIZE; ++i)
    cout << defs[i] << " : " << values[i] << "\n";
}
#endif

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

// ConstantExpr *createConstantExpr(int Value) {
//   auto *ret = new ConstantExpr();
//   ret->NodeType = EXPR;
//   ret->ExpressionType = CONSTANT;
//   ret->Value = Value;
//   return ret;
// }

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
IfStmt *createIf();
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
    ((StmtListInner *)currStmt)->Next->Stmt = createIf();
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

IfStmt *createIf() {
  IfStmt *ret = new IfStmt();
  ret->NodeType = STMT;
  ret->StatementType = IF;
  ret->Condition = createAddExpr(createVarRef(X), createVarRef(X));
  ret->ThenPart = createListOfStmt(5);
  ret->ElsePart = createListOfStmt(5);
  return ret;
}

IncrStmt *createIncr() {
  IncrStmt *ret = new IncrStmt();
  ret->NodeType = STMT;
  ret->StatementType = INC;
  ret->Id = createVarRef(Y);
  return ret;
}

Function *createFunction(int i) {
  Function *ret = new Function();
  ret->FunctionName = "F" + std::to_string((long long)i);
  ret->NodeType = ASTNodeType::FUNCTION;
  ret->StmtList = new StmtListInner();
  ret->StmtList->Stmt = createExprAssignment(Y, createVarRef(X));

  ((StmtListInner *)(ret->StmtList))->Next = new StmtListInner();
  ((StmtListInner *)(ret->StmtList))->Next->Stmt = createIncr();

  ((StmtListInner *)(((StmtListInner *)(ret->StmtList))->Next))->Next =
      createListOfStmt(5, true);
  return ret;
}

Program *createProgram(int FCount) {
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
void optimize(vector<Program *> &ls) {

#ifdef PAPI
  start_counters();
#endif
  auto t1 = std::chrono::high_resolution_clock::now();
  for (auto *f : ls) {
    // f->print();
    f->desugarDecr();
    f->desugarInc();
    f->propagateConstantsAssignments();
    f->foldConstants();
    f->removeUnreachableBranches();
    //  printf("after:\n");
    // f->print();
  }
  auto t2 = std::chrono::high_resolution_clock::now();
#ifdef PAPI
  read_counters();
  print_counters();
#endif
  printf("duration iss %llu\n",
         std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count());
}

int main(int argc, char **argv) {
  // number of functions
  int FCount = atoi(argv[1]);
  int Itters = atoi(argv[2]);
  vector<Program *> ls;
  ls.resize(Itters);

  for (int i = 0; i < Itters; i++)
    ls[i] = createProgram(FCount);

#ifndef BUILD_ONLY
  optimize(ls);
#endif

  printf("visit=%d\n", c);
}
