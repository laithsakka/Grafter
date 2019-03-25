#define __tree_structure__ __attribute__((annotate("tf_tree")))
#define __tree_child__ __attribute__((annotate("tf_child")))
#define __tree_traversal__ __attribute__((annotate("tf_fuse")))
#define __abstract_access__(AccessList)\
__attribute__((annotate("tf_strict_access" #AccessList)))

#include <cstdlib>
#include <iostream>
#include <vector>
#define DEPTH 5
// polynomial representation as an array
class VectorSTD {
public:
  std::vector<float> arr;
};

class __tree_structure__ Poly {
public:
  // loc1
  std::vector<float> arr;
  int size;
  __abstract_access__("(1,'w','local')")  inline void assignCoeff(int size) {
    size = size;
    arr.resize(size, 0);
  }
  __abstract_access__("(1,'w','local')") inline void addCons(float c) {
    arr[0] += c;
  }

  __abstract_access__("(1,'w','local')") inline void multConst(float c) {
    for (int i = 0; i < size; i++) {
      arr[i] *= c;
    }
  }

  __abstract_access__("(1,'w','local')") inline void divConst(float c) {
    for (int i = 0; i < size; i++) {
      arr[i] /= c;
    }
  }

  __abstract_access__("(1,'w','local')") inline void differentiate() {
    for (int i = 0; i < size - 1; i++) {
      arr[i] = (i + 1) * arr[i + 1];
    }
    size--;
  }
  __abstract_access__("(1, 'w', 'local')") inline void mulVar() {
    float tmp [] = {0};
    arr.insert(arr.begin(), tmp, tmp+1);
    size++;
  }
};

// Tree node
class __tree_structure__ Node {
public:
  float startDom;
  float endDom;
  __tree_traversal__ virtual void buildTree(int d, int size, float s, float e){};
  __tree_traversal__ virtual void addConst(float c){};
  __tree_traversal__ virtual void multConst(float c){};
  __tree_traversal__ virtual void divConst(float c){};
  __tree_traversal__ virtual void differentiate(){};
  __tree_traversal__ virtual void mulVar(){};
};

class __tree_structure__ Leaf : public Node {
public:
  __tree_child__ Poly *coeff;
  __tree_traversal__ void buildTree(int d, int size, float s, float e) override;
  __tree_traversal__ void addConst(float c) override;
  __tree_traversal__ void multConst(float c) override;
  __tree_traversal__ void divConst(float c) override;
  __tree_traversal__ void differentiate() override;
  __tree_traversal__ void mulVar() override;
};
class __tree_structure__ Inner : public Node {
public:
  __tree_child__ Node *l;
  __tree_child__ Node *r;
  __tree_traversal__ void buildTree(int d, int size, float s, float e) override;
  __tree_traversal__ void addConst(float c) override;
  __tree_traversal__ void multConst(float c) override;
  __tree_traversal__ void divConst(float c) override;
  __tree_traversal__ void differentiate() override;
  __tree_traversal__ void mulVar() override;
};

__tree_traversal__ void Leaf::buildTree(int d, int size, float s, float e) {
  startDom = s;
  endDom = e;
  coeff->assignCoeff(size);
}

// build a balanced kd-tree
__tree_traversal__ void Inner::buildTree(int d, int size, float s, float e) {
  startDom = s;
  endDom = e;
  if (d == DEPTH - 1) {
    l = new Leaf();
  }

  if (d == DEPTH - 1) {
    r = new Leaf();
  }
  if (d < DEPTH - 1) {
    l = new Inner();
  }

  if (d < DEPTH - 1) {
    r = new Inner();
  }

  l->buildTree(d, size, s, (s+e)/2);
  r->buildTree(d, size, (s+e)/2, e);
}

// adding constant to x^0
__tree_traversal__ void Inner::addConst(float c) {
  l->addConst(c);
  r->addConst(c);
}

__tree_traversal__ void Leaf::addConst(float c) { coeff->addCons(c); }

// multiplying every coeff by the constantvoid
__tree_traversal__ void Inner::multConst(float c) {

  l->multConst(c);
  r->multConst(c);
}

__tree_traversal__ void Leaf::multConst(float c) { coeff->multConst(c); }

// multiplying every coeff by the constantvoid
__tree_traversal__ void Inner::divConst(float c) {

  l->divConst(c);
  r->divConst(c);
}

__tree_traversal__ void Leaf::divConst(float c) { coeff->divConst(c); }

__tree_traversal__ void Inner::differentiate() {

  l->differentiate();
  r->differentiate();
}

__tree_traversal__ void Leaf::mulVar() { coeff->mulVar(); }

__tree_traversal__ void Inner::mulVar() {
  l->mulVar();
  r->mulVar();
}

__tree_traversal__ void Leaf::differentiate() { coeff->differentiate(); }

int main() {
  Node *root = new Inner();
  root->buildTree(0, 1, 0, 1);
  // f =((f+1)*x*x+10)' f(x) = x^2 + 10
  root->addConst(1);
  root->mulVar();
  root->mulVar();
  root->addConst(10);
  root->differentiate();
  return 0;
};