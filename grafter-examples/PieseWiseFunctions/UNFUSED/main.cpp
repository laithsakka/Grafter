#define __tree_structure__ __attribute__((annotate("tf_tree")))
#define __tree_child__ __attribute__((annotate("tf_child")))
#define __tree_traversal__ __attribute__((annotate("tf_fuse")))
#define __abstract_access__(AccessList)                                        \
  __attribute__((annotate("tf_strict_access" #AccessList)))

#include <cstdlib>
#include <iostream>
#include <vector>
#define DEPTH 5

enum NodeType { LEAF, INNER };
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
  __abstract_access__("(1,'w','local')") inline void assignCoeff(int size) {
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
    float tmp[] = {0};
    arr.insert(arr.begin(), tmp, tmp + 1);
    size++;
  }

  __abstract_access__("(1, 'w', 'local')") inline void setArray(
      std::vector<float> &arr) {
    this->arr = arr;
  }
};

// Tree node
class __tree_structure__ Node {
public:
  float startDom;
  float endDom;
  NodeType type;
  __tree_traversal__ virtual void buildTree(int d, int size, float s,
                                            float e){};
  __tree_traversal__ virtual void addConst(float c){};
  __tree_traversal__ virtual void multConst(float c){};
  __tree_traversal__ virtual void divConst(float c){};
  __tree_traversal__ virtual void differentiate(){};
  __tree_traversal__ virtual void mulVar(){};
  __tree_traversal__ virtual void rangeMulConst(float c, float _s, float _e){};
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
  __tree_traversal__ void rangeMulConst(float c, float _s, float _e) override;
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
  __tree_traversal__ void rangeMulConst(float c, float _s, float _e) override;
  __tree_traversal__ void inline splitLeft(float c, float s);
  __tree_traversal__ void Inner::splitRight(float _s, float _e);
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
    l->type = LEAF;
  }

  if (d == DEPTH - 1) {
    r = new Leaf();
    r->type = LEAF;
  }
  if (d < DEPTH - 1) {
    l = new Inner();
    l->type = INNER;
  }

  if (d < DEPTH - 1) {
    r = new Inner();
    r->type = INNER;
  }

  l->buildTree(d, size, s, (s + e) / 2);
  r->buildTree(d, size, (s + e) / 2, e);
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

__tree_traversal__ void Leaf::differentiate() { coeff->differentiate(); }

__tree_traversal__ void Inner::mulVar() {
  l->mulVar();
  r->mulVar();
}

__tree_traversal__ void Leaf::mulVar() { coeff->mulVar(); }

__tree_traversal__ void Inner::splitLeft(float _s, float _e) {

  // splitting if needed
  if (l->type == LEAF && (_s > l->startDom && _s < l->endDom) ||
      (_e > l->startDom && _e < l->endDom)) {

    // split into 5 nodes
    if ((_s > l->startDom && _s < l->endDom) &&
        (_e > l->startDom && _e < l->endDom)) {
      int s1 = l->startDom;
      int e1 = _s;
      int s2 = e1;
      int e2 = _e;
      int s3 = e2;
      int e3 = l->endDom;

      Leaf *const oldNode = static_cast<Leaf *>(l);
      l = new Inner();
      auto *const newInner1 = static_cast<Inner *>(l);
      newInner1->type = INNER;
      newInner1->startDom = s1;
      newInner1->endDom = e3;

      // create node Inner1 and leaf1
      newInner1->l = new Leaf();
      newInner1->l->type = LEAF;
      newInner1->l->startDom = s1;
      newInner1->l->endDom = e1;
      static_cast<Leaf *>(newInner1->l)->coeff->setArray(oldNode->coeff->arr);

      // creating Inner2 and leaf2, and leaf3
      newInner1->r = new Inner();
      auto *const newInner2 = static_cast<Inner *>(newInner1->l);
      newInner2->type = INNER;
      newInner2->startDom = s2;
      newInner2->endDom = e3;

      newInner2->l = new Leaf();
      newInner2->l->type = LEAF;
      newInner2->l->startDom = s2;
      newInner2->l->endDom = e2;
      static_cast<Leaf *>(newInner2->l)->coeff->setArray(oldNode->coeff->arr);

      newInner2->r = new Leaf();
      newInner2->r->type = LEAF;
      newInner2->r->startDom = s2;
      newInner2->r->endDom = e2;
      static_cast<Leaf *>(newInner2->r)->coeff->setArray(oldNode->coeff->arr);
      delete oldNode;
    }
    // split into three nodes
    else {
      int s1 = l->startDom;
      int e1;
      if (_s > l->startDom && _s < l->endDom)
        e1 = _s;
      else
        e1 = _e;

      int s2 = e1;
      int e2 = l->endDom;

      auto *const oldNode = static_cast<Leaf *>(l);

      // create Inner Node
      l = new Inner();
      l->type = INNER;
      auto *const newInner = static_cast<Inner *>(l);
      newInner->type = INNER;
      newInner->startDom = s1;
      newInner->endDom = e2;

      newInner->l = new Leaf();
      newInner->l->type = LEAF;
      newInner->l->startDom = s1;
      newInner->l->endDom = e1;
      static_cast<Leaf *>(newInner->l)->coeff->setArray(oldNode->coeff->arr);

      newInner->r = new Leaf();
      newInner->r->type = LEAF;
      newInner->r->startDom = s2;
      newInner->r->endDom = e2;
      static_cast<Leaf *>(newInner->r)->coeff->setArray(oldNode->coeff->arr);
      delete oldNode;
    }
  }
}

__tree_traversal__ void Inner::splitRight(float _s, float _e) {

  // splitting if needed
  if (r->type == LEAF && (_s > r->startDom && _s < r->endDom) ||
      (_e > r->startDom && _e < r->endDom)) {

    // split into 5 nodes
    if ((_s > r->startDom && _s < r->endDom) &&
        (_e > r->startDom && _e < r->endDom)) {
      int s1 = r->startDom;
      int e1 = _s;
      int s2 = e1;
      int e2 = _e;
      int s3 = e2;
      int e3 = r->endDom;

      Leaf *const oldNode = static_cast<Leaf *>(r);
      r = new Inner();
      auto *const newInner1 = static_cast<Inner *>(r);
      newInner1->type = INNER;
      newInner1->startDom = s1;
      newInner1->endDom = e3;

      // create node Inner1 and leaf1
      newInner1->l = new Leaf();
      newInner1->l->type = LEAF;
      newInner1->l->startDom = s1;
      newInner1->l->endDom = e1;
      static_cast<Leaf *>(newInner1->l)->coeff->setArray(oldNode->coeff->arr);

      // creating Inner2 and leaf2, and leaf3
      newInner1->r = new Inner();
      auto *const newInner2 = static_cast<Inner *>(newInner1->l);
      newInner2->type = INNER;
      newInner2->startDom = s2;
      newInner2->endDom = e3;

      newInner2->l = new Leaf();
      newInner2->l->type = LEAF;
      newInner2->l->startDom = s2;
      newInner2->l->endDom = e2;
      static_cast<Leaf *>(newInner2->l)->coeff->setArray(oldNode->coeff->arr);

      newInner2->r = new Leaf();
      newInner2->r->type = LEAF;
      newInner2->r->startDom = s2;
      newInner2->r->endDom = e2;
      static_cast<Leaf *>(newInner2->r)->coeff->setArray(oldNode->coeff->arr);
      delete oldNode;
    }
    // split into three nodes
    else {
      int s1 = r->startDom;
      int e1;
      if (_s > r->startDom && _s < r->endDom)
        e1 = _s;
      else
        e1 = _e;

      int s2 = e1;
      int e2 = r->endDom;

      auto *const oldNode = static_cast<Leaf *>(r);

      // create Inner Node
      r = new Inner();
      r->type = INNER;
      auto *const newInner = static_cast<Inner *>(r);
      newInner->type = INNER;
      newInner->startDom = s1;
      newInner->endDom = e2;

      newInner->l = new Leaf();
      newInner->l->type = LEAF;
      newInner->l->startDom = s1;
      newInner->l->endDom = e1;
      static_cast<Leaf *>(newInner->l)->coeff->setArray(oldNode->coeff->arr);

      newInner->r = new Leaf();
      newInner->r->type = LEAF;
      newInner->r->startDom = s2;
      newInner->r->endDom = e2;
      static_cast<Leaf *>(newInner->r)->coeff->setArray(oldNode->coeff->arr);
      delete oldNode;
    }
  }
}

__tree_traversal__ void Inner::rangeMulConst(float c, float _s, float _e) {
  if (_s > endDom | _e < startDom)
    return;

  splitLeft(_s, _e);
  splitRight(_s, _e);
  l->rangeMulConst(c, _s, _e);
  r->rangeMulConst(c, _s, _e);
}

__tree_traversal__ void Leaf::rangeMulConst(float c, float _s, float _e) {
  if (_s > endDom | _e < startDom)
    return;

  coeff->multConst(c);
}

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