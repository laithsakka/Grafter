#define __tree_structure__ __attribute__((annotate("tf_tree")))
#define __tree_child__ __attribute__((annotate("tf_child")))
#define __tree_traversal__ __attribute__((annotate("tf_fuse")))
#define __abstract_access__(AccessList) \
  __attribute__((annotate("tf_strict_access" #AccessList)))

#include <cstdlib>
#include <iostream>
#include <vector>

enum NodeType
{
  LEAF,
  INNER
};
// polynomial representation as an array
class VectorSTD
{
public:
  std::vector<float> arr;
};

class __tree_structure__ Poly
{
public:
  // loc1
  std::vector<float> arr;
  int size;

  inline void print()
  {
    std::cout << arr[0];
    for (int i = 1; i < size; i++)
    {
      std::cout << " + " << arr[i] << "x^" << i;
    }
    std::cout << std::endl;
  }

  __abstract_access__("(1,'w','local')") inline void assignCoeff(int size)
  {
    size = size;
    arr.resize(size, 0);
  }
  __abstract_access__("(1,'w','local')") inline void addCons(float c)
  {
    arr[0] += c;
  }

  __abstract_access__("(1,'w','local')") inline void multConst(float c)
  {
    for (int i = 0; i < size; i++)
    {
      arr[i] *= c;
    }
  }

  __abstract_access__("(1,'w','local')") inline void divConst(float c)
  {
    for (int i = 0; i < size; i++)
    {
      arr[i] /= c;
    }
  }

  __abstract_access__("(1,'w','local')") inline void differentiate()
  {
    for (int i = 0; i < size - 1; i++)
    {
      arr[i] = (i + 1) * arr[i + 1];
    }
    size--;
  }
  __abstract_access__("(1, 'w', 'local')") inline void mulVar()
  {
    float tmp[] = {0};
    arr.insert(arr.begin(), tmp, tmp + 1);
    size++;
  }

  __abstract_access__("(1, 'w', 'local')") inline void setArray(std::vector<float> &_arr, int _size)
  {
    arr = _arr;
    size = _size;
  }

  __abstract_access__("(1, 'w', 'local')") inline float boundedIntegrate(float _a, float _b)
  {
    float ret = 0.0;
    for (int i = 0; i < size; i++)
    {
      float pa_ = 1.0;
      float pb_ = 1.0;
      for (int j = 0; j < i + 1; j++)
      {
        pa_ *= _a;
        pb_ *= _b;
      }
      ret += arr[i] * (pa_ - pb_);
    }
    return ret;
  }

  __abstract_access__("(1, 'w', 'local')") inline float project(float x)
  {
    float ret = 0.0;
    for (int i = 0; i < size; i++)
    {
      float p = 1.0;
      for (int j = 0; j < i; j++)
        p *= x;
      ret += arr[i] * p;
    }
    return ret;
  }

  __abstract_access__("(1, w, 'local')") inline void square()
  {
    std::vector<float> sarr;
    int N = size;
    int M = N * N;
    sarr.resize(M, 0);

    for (int i = 0; i < size; i++)
    {
      for (int j = 0; j < size; j++)
      {
        sarr[i + j] += arr[i] * arr[j];
      }
    }
    setArray(sarr, M);
  }
};

// Tree node
class __tree_structure__ Node
{
public:
  float startDom;
  float endDom;
  NodeType type;
  float projectVal;

  virtual void print(){};
  __tree_traversal__ virtual void buildTree(int depth, int d, int size, float s, float e){};
  __tree_traversal__ virtual void addConst(float c){};
  __tree_traversal__ virtual void multConst(float c){};
  __tree_traversal__ virtual void divConst(float c){};
  __tree_traversal__ virtual void differentiate(){};
  __tree_traversal__ virtual void mulVar(){};
  __tree_traversal__ virtual void rangeMulConst(float c, float _s, float _e){};
  __tree_traversal__ virtual void rangeMulVar(float _s, float _e){};
  __tree_traversal__ virtual void boundedIntegrate(float _a, float _b){};
  __tree_traversal__ virtual void project(float x){};
  __tree_traversal__ virtual void square(){};
};

class __tree_structure__ Inner : public Node
{
public:
  __tree_child__ Node *l;
  __tree_child__ Node *r;
  void print() override;
  __tree_traversal__ void buildTree(int depth, int d, int size, float s, float e) override;
  __tree_traversal__ void addConst(float c) override;
  __tree_traversal__ void multConst(float c) override;
  __tree_traversal__ void divConst(float c) override;
  __tree_traversal__ void differentiate() override;
  __tree_traversal__ void mulVar() override;
  __tree_traversal__ void inline splitLeft(float c, float s);
  __tree_traversal__ void inline splitRight(float _s, float _e);
  __tree_traversal__ void rangeMulConst(float c, float _s, float _e) override;
  __tree_traversal__ void rangeMulVar(float _s, float _e) override;
  __tree_traversal__ void boundedIntegrate(float _a, float _b) override;
  __tree_traversal__ void project(float x) override;
  __tree_traversal__ void square() override;
};

class __tree_structure__ Leaf : public Node
{
public:
  __tree_child__ Poly *coeff;
  void print() override;
  __tree_traversal__ void buildTree(int depth, int d, int size, float s, float e) override;
  __tree_traversal__ void addConst(float c) override;
  __tree_traversal__ void multConst(float c) override;
  __tree_traversal__ void divConst(float c) override;
  __tree_traversal__ void differentiate() override;
  __tree_traversal__ void mulVar() override;
  __tree_traversal__ void rangeMulConst(float c, float _s, float _e) override;
  __tree_traversal__ void rangeMulVar(float _s, float _e) override;
  __tree_traversal__ void boundedIntegrate(float _a, float _b) override;
  __tree_traversal__ void project(float x) override;
  __tree_traversal__ void square() override;
};

void Inner::print()
{
  l->print();
  r->print();
}

void Leaf::print()
{
  std::cout << "Range:[ " << startDom << ", " << endDom << "]" << std::endl;
  coeff->print();
}

// build a balanced kd-tree
__tree_traversal__ void Inner::buildTree(int depth, int d, int size, float s, float e)
{
  startDom = s;
  endDom = e;
  projectVal = 0.0;

  if (d == depth - 1)
  {
    l = new Leaf();
    l->type = LEAF;
  }

  if (d == depth - 1)
  {
    r = new Leaf();
    r->type = LEAF;
  }
  if (d < depth - 1)
  {
    l = new Inner();
    l->type = INNER;
  }

  if (d < depth - 1)
  {
    r = new Inner();
    r->type = INNER;
  }

  l->buildTree(depth, d+1, size, s, (s + e) / 2);
  r->buildTree(depth, d+1, size, (s + e) / 2, e);
}

__tree_traversal__ void Leaf::buildTree(int depth, int d, int size, float s, float e)
{
  startDom = s;
  endDom = e;
  projectVal = 0.0;
  coeff->assignCoeff(size);
}

__tree_traversal__ void Inner::addConst(float c)
{
  l->addConst(c);
  r->addConst(c);
}

// adding constant to x^0
__tree_traversal__ void Leaf::addConst(float c) { coeff->addCons(c); }

__tree_traversal__ void Inner::multConst(float c)
{

  l->multConst(c);
  r->multConst(c);
}

// multiplying every coeff by the constant
__tree_traversal__ void Leaf::multConst(float c) { coeff->multConst(c); }

__tree_traversal__ void Inner::divConst(float c)
{

  l->divConst(c);
  r->divConst(c);
}

// dividing every coeff by the constant
__tree_traversal__ void Leaf::divConst(float c) { coeff->divConst(c); }

__tree_traversal__ void Inner::differentiate()
{

  l->differentiate();
  r->differentiate();
}

// differentiate the polynomial for the range
__tree_traversal__ void Leaf::differentiate()
{
  coeff->differentiate();
}

__tree_traversal__ void Inner::mulVar()
{
  l->mulVar();
  r->mulVar();
}

// multiply the function by x
__tree_traversal__ void Leaf::mulVar() { coeff->mulVar(); }

__tree_traversal__ void Inner::splitLeft(float _s, float _e)
{

  // splitting if needed
  if (l->type == LEAF && ((_s > l->startDom && _s < l->endDom) ||
                          (_e > l->startDom && _e < l->endDom)))
  {

    // split into 5 nodes
    if ((_s > l->startDom && _s < l->endDom) &&
        (_e > l->startDom && _e < l->endDom))
    {
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
      static_cast<Leaf *>(newInner1->l)->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);

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
      static_cast<Leaf *>(newInner2->l)->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);

      newInner2->r = new Leaf();
      newInner2->r->type = LEAF;
      newInner2->r->startDom = s2;
      newInner2->r->endDom = e2;
      static_cast<Leaf *>(newInner2->r)->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);
      delete oldNode;
    }
    // split into three nodes
    else
    {
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
      static_cast<Leaf *>(newInner->l)->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);

      newInner->r = new Leaf();
      newInner->r->type = LEAF;
      newInner->r->startDom = s2;
      newInner->r->endDom = e2;
      static_cast<Leaf *>(newInner->r)->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);
      delete oldNode;
    }
  }
}

__tree_traversal__ void Inner::splitRight(float _s, float _e)
{

  // splitting if needed
  if (r->type == LEAF && ((_s > r->startDom && _s < r->endDom) ||
                          (_e > r->startDom && _e < r->endDom)))
  {

    // split into 5 nodes
    if ((_s > r->startDom && _s < r->endDom) &&
        (_e > r->startDom && _e < r->endDom))
    {
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
      static_cast<Leaf *>(newInner1->l)->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);

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
      static_cast<Leaf *>(newInner2->l)->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);

      newInner2->r = new Leaf();
      newInner2->r->type = LEAF;
      newInner2->r->startDom = s2;
      newInner2->r->endDom = e2;
      static_cast<Leaf *>(newInner2->r)->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);
      delete oldNode;
    }
    // split into three nodes
    else
    {
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
      static_cast<Leaf *>(newInner->l)->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);

      newInner->r = new Leaf();
      newInner->r->type = LEAF;
      newInner->r->startDom = s2;
      newInner->r->endDom = e2;
      static_cast<Leaf *>(newInner->r)->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);
      delete oldNode;
    }
  }
}

__tree_traversal__ void Inner::rangeMulConst(float c, float _s, float _e)
{
  if (_s > endDom || _e < startDom)
    return;

  splitLeft(_s, _e);
  splitRight(_s, _e);
  l->rangeMulConst(c, _s, _e);
  r->rangeMulConst(c, _s, _e);
}

// multiply the range by a constant
__tree_traversal__ void Leaf::rangeMulConst(float c, float _s, float _e)
{
  if (_s > endDom || _e < startDom)
    return;

  coeff->multConst(c);
}

__tree_traversal__ void Inner::rangeMulVar(float _s, float _e)
{
  if (_s > endDom || _e < startDom)
    return;

  splitLeft(_s, _e);
  splitRight(_s, _e);
  l->rangeMulVar(_s, _e);
  r->rangeMulVar(_s, _e);
}

// multiply the range by x
__tree_traversal__ void Leaf::rangeMulVar(float _s, float _e)
{

  if (_s > endDom || _e < startDom)
    return;

  coeff->mulVar();
}

__tree_traversal__ void Inner::boundedIntegrate(float _a, float _b)
{

  if (_a > endDom || _b < startDom)
  {
    projectVal = 0.0;
    return;
  }

  float a_;
  a_ = _a;
  float b_;
  b_ = _b;

  if (_a < startDom)
    a_ = startDom;
  if (_b > endDom)
    b_ = endDom;

  l->boundedIntegrate(a_, b_);
  r->boundedIntegrate(a_, b_);

  projectVal = l->projectVal + r->projectVal;
}

// integrate the polynomial for the range
__tree_traversal__ void Leaf::boundedIntegrate(float _a, float _b)
{
  if (_a > endDom || _b < startDom)
  {
    projectVal = 0.0;
    return;
  }

  float a_;
  a_ = _a;
  float b_;
  b_ = _b;

  if (_a < startDom)
    a_ = startDom;
  if (_b > endDom)
    b_ = endDom;

  projectVal = coeff->boundedIntegrate(a_, b_);
}

// compute the function at a given point
__tree_traversal__ void Inner::project(float x)
{
  if (x < startDom || x > endDom)
    return;

  l->project(x);
  r->project(x);

  if (x >= l->startDom && x < l->endDom)
    projectVal = l->projectVal;
  if (x >= r->startDom && x < r->endDom)
    projectVal = r->projectVal;
}

__tree_traversal__ void Leaf::project(float x)
{
  if (x < startDom || x > endDom)
    return;

  projectVal = coeff->project(x);
}

// square the domain of the function
__tree_traversal__ void Inner::square()
{
  l->square();
  r->square();
}

__tree_traversal__ void Leaf::square() { coeff->square(); }

int main()
{
  Node *root = new Inner();
  root->buildTree(5, 0, 1, 0, 1);
  // f =((f+1)*x*x+10)' f(x) = x^2 + 10
  root->addConst(1);
  root->mulVar();
  root->mulVar();
  root->addConst(10);
  root->differentiate();
  root->print();

  //Experiment 1
  //Experiment 2
  //Experiment 3
  return 0;
};