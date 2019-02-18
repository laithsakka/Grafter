#include "AST.h"
#include <stdio.h>
using namespace std;

 void Program::print() {
  printf("Program\n");
  Functions->print();
}

 void FunctionListEnd::print() { Content->print(); }

 void FunctionListInner::print() {
  Content->print();
  printf("----------------\n");
  Next->print();
}

void Function::print() {
  printf("Function:%s\n", &FunctionName[0]);
  StmtList->print();
}

void StmtListInner::print() {
  Stmt->print();
  Next->print();
}

void StmtListEnd::print() { Stmt->print(); }

void AssignStmt::print() {
  this->Id->print();
  printf("=");
  this->AssignedExpr->print();
  printf(";\n");
}
void BinaryExpr::print() {
  RHS->print();

  if (this->Operator == ADD)
    printf("+");
  if (this->Operator == SUBTRACT)
    printf("-");

  LHS->print();
}

void ConstantExpr::print() { printf("%d", Value); }

void VarRefExpr::print() { printf("MEM[%d]", VarId); }

void IfStmt::print() {
  printf("if ");
  Condition->print();
  printf(":\n");
  ThenPart->print();
  printf("else:\n");
  ElsePart->print();
  printf("endifs\n");
}