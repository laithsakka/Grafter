#include <assert.h>
#include <chrono>
#include <stddef.h>
#include <stdio.h>

#define __tree_structure__ __attribute__((annotate("tf_tree")))
#define __tree_child__ __attribute__((annotate("tf_child")))
#define __tree_traversal__ __attribute__((annotate("tf_fuse")))
#define _Bool bool

enum NodeType { VAL_NODE, NULL_NODE };

class __tree_structure__ Node {
public:
  bool Found = false;
  NodeType Type;
  __tree_traversal__ virtual void search(int Key, bool ValidCall) {}

  __tree_traversal__ virtual void insert(int Key, bool ValidCall) {}
};

class __tree_structure__ NullNode : public Node {

  __tree_traversal__ virtual void search(int Key, bool ValidCall) {
    Found = false;
  }

  __tree_traversal__ virtual void insert(int Key, bool ValidCall) {}
};

class __tree_structure__ ValueNode : public Node {
public:
  int Value;
  __tree_child__ Node *Left, *Right;

  __tree_traversal__ void search(int Key, bool ValidCall) override {

    if (ValidCall == false )
      return;
    
    Found = false;
    if (Key == Value) {
      Found = true;
      return;
    }
    Left->search(Key, Key < Value);
    Right->search(Key, Key >= Value);
    Found = Left->Found || Right->Found;
  }

  __tree_traversal__ void insert(int NewValue, bool ValidCall) {
    if (ValidCall == false)
      return;
      
    if (NewValue < Value && Left->Type == NULL_NODE) {
      delete Left;
      Left = new ValueNode();
      ValueNode *const Left_ = static_cast<ValueNode *>(Left);
      Left_->Value = NewValue;
      Left_->Type = VAL_NODE;

      Left_->Left = new NullNode();
      Left_->Left->Type = NULL_NODE;

      Left_->Right = new NullNode();
      Left_->Right->Type = NULL_NODE;

      return;
    }
    if (NewValue >= Value && Right->Type == NULL_NODE) {
      delete Right;
      Right = new ValueNode();
      ValueNode *const Right_ = static_cast<ValueNode *>(Right);
      Right_->Value = NewValue;
      Right_->Type = VAL_NODE;

      Right_->Left = new NullNode();
      Right_->Left->Type = NULL_NODE;

      Right_->Right = new NullNode();
      Right_->Right->Type = NULL_NODE;
      return;
    }

    Left->insert(NewValue, NewValue < Value);
    Right->insert(NewValue, NewValue >= Value);
  }
  };

  Node *createTree() {
    Node* Root = new ValueNode();
    ValueNode *const Root_ = static_cast<ValueNode *>(Root);
    Root_->Value = 5;
    Root_->Type = VAL_NODE;

    Root_->Left = new NullNode();
    Root_->Left->Type = NULL_NODE;

    Root_->Right = new NullNode();
    Root_->Right->Type = NULL_NODE;

    return Root;
  }

  int main() {
    Node *Root = createTree(); 
    Root->insert(10, true);
    Root->insert(20, true);
    Root->search(10, true);
    if(Root->Found)
      printf("10 is found\n"); 
    else
      printf("10 is not found\n"); 

  }
