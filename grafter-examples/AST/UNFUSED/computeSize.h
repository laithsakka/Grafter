#include "AST.h"
#include <stdio.h>
using namespace std;
;
int Program::computeSize() {
  return sizeof(*this) + this->Functions->computeSize();
}

int FunctionListEnd::computeSize() {
  return sizeof(*this) + Content->computeSize();
}

int FunctionListInner::computeSize() {
  return sizeof(*this) + Content->computeSize() + Next->computeSize();
}
int Function::computeSize() { return sizeof(*this) + StmtList->computeSize(); }

int StmtListInner::computeSize() {
  return sizeof(*this) + Stmt->computeSize() + Next->computeSize();
}

int StmtListEnd::computeSize() { return sizeof(*this) + Stmt->computeSize(); }

int AssignStmt::computeSize() {
  return sizeof(*this) + this->Id->computeSize() + AssignedExpr->computeSize();
}

int BinaryExpr::computeSize() {
  return sizeof(*this) + RHS->computeSize() + LHS->computeSize();
}

int ConstantExpr::computeSize() { return sizeof(*this); }

int VarRefExpr::computeSize() { return sizeof(*this); }

int IfStmt::computeSize() {
  return sizeof(*this) + Condition->computeSize() + ThenPart->computeSize() +
         ElsePart->computeSize();
}

int IncrStmt::computeSize() { return sizeof(*this) + Id->computeSize(); }

int DecrStmt::computeSize() { return sizeof(*this); }