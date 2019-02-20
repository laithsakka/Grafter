#ifndef AST
#define AST
#define __tree_structure__ __attribute__((annotate("tf_tree")))
#define __tree_child__ __attribute__((annotate("tf_child")))
#define __tree_traversal__ __attribute__((annotate("tf_fuse")))
#include <string>
enum ASTNodeType { STMT, EXPR, FUNCTION, SEQ };
enum ASTStmtType { ASSIGNMENT, IF, NOP, INC, DECR };
enum ASTExprType { CONSTANT, BINARY, VARREF };
enum ExprOperator { ADD, SUBTRACT };

int c =0; 

#define COUNT 

class StatementNode;

class __tree_structure__ ASTNode {
public:
  ASTNodeType NodeType;
  __tree_traversal__ virtual void foldConstants() {}
  __tree_traversal__ virtual void replaceVarRefWithConst(int VarRefId,
                                                         int Val){};
  __tree_traversal__ virtual void propagateConstantsAssignments(){};
  __tree_traversal__ virtual void removeUnreachableBranches(){};
  __tree_traversal__ virtual void deleteChildren(){};
  __tree_traversal__ virtual void desugarInc(){};
  __tree_traversal__ virtual void desugarDecr(){};

  virtual void print(){};
  bool ChangedFolding = false;
  bool ChangedPropagation = false;
};

class __tree_structure__ StmtListNode : public ASTNode {
public:
  __tree_child__ StatementNode *Stmt;
};

class __tree_structure__ StmtListInner : public StmtListNode {
public:
  __tree_child__ StmtListNode *Next;
  __tree_traversal__ void foldConstants() override;
  __tree_traversal__ void propagateConstantsAssignments() override;
  __tree_traversal__ void replaceVarRefWithConst(int VarRefId,
                                                 int Val) override;
  __tree_traversal__ void removeUnreachableBranches() override;
  __tree_traversal__ void desugarInc() override;
  __tree_traversal__ void desugarDecr() override;

  void print() override;
};

class __tree_structure__ StmtListEnd : public StmtListNode {
public:
  __tree_traversal__ void foldConstants() override;
  __tree_traversal__ void replaceVarRefWithConst(int VarRefId,
                                                 int Val) override;
  __tree_traversal__ void propagateConstantsAssignments() override;
  __tree_traversal__ void removeUnreachableBranches() override;
  __tree_traversal__ void desugarInc() override;
  __tree_traversal__ void desugarDecr() override;

  void print() override;
};

class __tree_structure__ Function : public ASTNode {
public:
  std::string FunctionName = "not-set";
  __tree_child__ StmtListNode *StmtList;
  __tree_traversal__ void foldConstants() override;
  __tree_traversal__ void propagateConstantsAssignments() override;
  __tree_traversal__ void removeUnreachableBranches() override;
  __tree_traversal__ void desugarInc() override;
  __tree_traversal__ void desugarDecr() override;

  void print() override;
};

class __tree_structure__ FunctionList : public ASTNode {
public:
  __tree_child__ Function *Content;
};

class __tree_structure__ FunctionListInner : public FunctionList {
public:
  __tree_child__ FunctionList *Next;
  __tree_traversal__ void foldConstants() override;
  __tree_traversal__ void propagateConstantsAssignments() override;
  __tree_traversal__ void removeUnreachableBranches() override;
  __tree_traversal__ void desugarInc() override;
  __tree_traversal__ void desugarDecr() override;

  void print() override;
};

class __tree_structure__ FunctionListEnd : public FunctionList {
public:
  __tree_traversal__ void foldConstants() override;
  __tree_traversal__ void propagateConstantsAssignments() override;
  __tree_traversal__ void removeUnreachableBranches() override;
  __tree_traversal__ void desugarInc() override;
  __tree_traversal__ void desugarDecr() override;

  void print() override;
};

class __tree_structure__ Program : public FunctionList {
public:
  __tree_child__ FunctionList *Functions;
  __tree_traversal__ void foldConstants() override;
  __tree_traversal__ void propagateConstantsAssignments() override;
  __tree_traversal__ void removeUnreachableBranches() override;
  __tree_traversal__ void desugarInc() override;
  __tree_traversal__ void desugarDecr() override;

  void print() override;
};

class __tree_structure__ ExpressionNode : public ASTNode {
public:
  ASTExprType ExpressionType;
};

class __tree_structure__ BinaryExpr : public ExpressionNode {
public:
  __tree_child__ ExpressionNode *LHS = 0;
  __tree_child__ ExpressionNode *RHS = 0;
  ExprOperator Operator;
  __tree_traversal__ void foldConstants() override;
  __tree_traversal__ void replaceVarRefWithConst(int VarRefId,
                                                 int Val) override;
  void print() override;
};

class __tree_structure__ VarRefExpr : public ExpressionNode {
public:
  int VarId;
  void print() override;
};

class __tree_structure__ ConstantExpr : public ExpressionNode {
public:
  int Value;
  void print() override;
};
class __tree_structure__ StatementNode : public ASTNode {
public:
  ASTStmtType StatementType;
};

class __tree_structure__ AssignStmt : public StatementNode {
public:
  __tree_child__ VarRefExpr *Id;
  bool IsMutable;
  __tree_child__ ExpressionNode *AssignedExpr;
  __tree_traversal__ void foldConstants() override;
  __tree_traversal__ void replaceVarRefWithConst(int VarRefId,
                                                 int Val) override;

  void print() override;
};

class __tree_structure__ IncrStmt : public StatementNode {
public:
  __tree_child__ VarRefExpr *Id;
  void print() override;
};

class __tree_structure__ DecrStmt : public StatementNode {
public:
  __tree_child__ VarRefExpr *Id;
  void print() override;
};

class __tree_structure__ IfStmt : public StatementNode {
public:
  __tree_child__ ExpressionNode *Condition;
  __tree_child__ StmtListNode *ThenPart;
  __tree_child__ StmtListNode *ElsePart;

  __tree_traversal__ void foldConstants() override;
  __tree_traversal__ void replaceVarRefWithConst(int VarRefId,
                                                 int Val) override;
  __tree_traversal__ void propagateConstantsAssignments() override;

  __tree_traversal__ void removeUnreachableBranches() override;
  __tree_traversal__ void desugarInc() override;
  __tree_traversal__ void desugarDecr() override;

  void print() override;
};

class __tree_structure__ NullStmt : public StatementNode {
public:
};

#endif