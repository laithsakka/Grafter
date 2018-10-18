#define __tree_structure__ __attribute__((annotate("tf_tree")))
#define __tree_child__ __attribute__((annotate("tf_child")))
#define __tree_traversal__ __attribute__((annotate("tf_fuse")))

enum ASTNodeType { STMT, EXPR };
enum ASTStmtType { ASSIGNMENT, SEQ };
enum ASTExprType { CONSTANT, BINARY, VARREF };
enum ExprOperator { ADD, SUBTRACT };

class __tree_structure__ ASTNode {
public:
  ASTNodeType NodeType;
  __tree_traversal__ virtual void foldConstants(){}
};

class __tree_structure__ StatementNode : public ASTNode {
public:
  ASTStmtType StatementType;
};

class __tree_structure__ ExpressionNode : public ASTNode {
public:
  ASTExprType ExpressionType;
};

class __tree_structure__ VarRefNode : public ExpressionNode {
public:
};

class __tree_structure__ ConstantNode : public ExpressionNode {
public:
  int Value;
};

class __tree_structure__ BinaryExpressionNode : public ExpressionNode {
public:
  __tree_child__ ExpressionNode *LHS;
  __tree_child__ ExpressionNode *RHS;
  ExprOperator Operator;
  __tree_traversal__ void foldConstants() override;
};

class __tree_structure__ AssignmentNode : public StatementNode {
public:
  __tree_child__ VarRefNode *Id;
  __tree_child__ ExpressionNode *AssignedExpr;
  __tree_traversal__ void foldConstants() override;
};

class __tree_structure__ SequenceNode : public StatementNode {
public:
  // SequenceNode() : StatementNode(ASTStmtType::SEQ) {}

  __tree_child__ StatementNode *FirstStmt;
  __tree_child__ StatementNode *SecondStmt;
  __tree_traversal__ void foldConstants() override;
};
