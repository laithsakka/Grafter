#define __tree_structure__ __attribute__((annotate("tf_tree")))
#define __tree_child__ __attribute__((annotate("tf_child")))
#define __tree_traversal__ __attribute__((annotate("tf_fuse")))
#define __abstract_access__(AccessList)                                        \
  __attribute__((annotate("tf_strict_access" #AccessList)))

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace std;
int _VISIT_COUNTER = 0;

#ifdef PAPI
#include <iostream>
#include <papi.h>
#define SIZE 3
string instance("Original");
int ret;
int _VISIT_COUNTER = 0;
int events[] = {PAPI_L2_TCM, PAPI_L3_TCM, PAPI_TOT_INS};
string defs[] = {"L2 Cache Misses", "L3 Cache Misses ", "Instructions"};

long long values[SIZE];
long long rcyc0, rcyc1, rusec0, rusec1;
long long vcyc0, vcyc1, vusec0, vusec1;

void init_papi() {
  if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
    cout << "PAPI Init Error" << endl;
    exit(1);
  }
  for (int i = 0; i < SIZE; ++i) {
    if (PAPI_query_event(events[i]) != PAPI_OK) {
      cout << "PAPI Event " << i << " does not exist" << endl;
    }
  }
}
void start_counters() {
  // Performance Counters Start
  if (PAPI_start_counters(events, SIZE) != PAPI_OK) {
    cout << "PAPI Error starting counters" << endl;
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

    cout << "PAPI Error reading counters" << endl;
  }
}
void print_counters() {
  for (int i = 0; i < SIZE; ++i)
    cout << defs[i] << " : " << values[i] << "\n";
}
#endif

enum NodeType { LEAF, INNER };
// polynomial representation as an array

// Tree node
class __tree_structure__ Node {
public:
  float startDom;
  float endDom;
  NodeType type;
  float projectVal;

  virtual void print(){};
  __tree_traversal__ virtual void buildTree(int depth, int d, int size, float s,
                                            float e){};
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

class __tree_structure__ Poly {
public:
  // loc1
  std::vector<float> arr;
  int size;

  inline void print() {
    std::cout << arr[0];
    for (int i = 1; i < size; i++) {
      std::cout << " + " << arr[i] << "x^" << i;
    }
    std::cout << std::endl;
  }

  __abstract_access__("(1,'w','local')") inline void assignCoeff(int _size) {
    size = _size;
    arr.resize(size, 0);
    // for(int i = 0; i < size; i++) arr.push_back(1);
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
  __abstract_access__("(1,'w','local')") inline void mulVar() {
    // float tmp[] = {0};
    if (arr.size() == 0)
      arr.push_back(0);
    else
      arr.insert(arr.begin(), 0);
    size++;
  }

  __abstract_access__("(1,'w','local')") inline void setArray(
      std::vector<float> &_arr, int _size) {
    arr = _arr;
    size = _size;
  }

  __abstract_access__("(1,'w','local')") inline float boundedIntegrate(
      float _a, float _b) {
    float ret = 0.0;
    for (int i = 0; i < size; i++) {
      float pa_ = 1.0;
      float pb_ = 1.0;
      for (int j = 0; j < i + 1; j++) {
        pa_ *= _a;
        pb_ *= _b;
      }
      ret += arr[i] * (pb_ - pa_) / (i + 1);
    }
    return ret;
  }

  __abstract_access__("(1,'w','local')") inline float project(float x) {
    float ret = 0.0;
    for (int i = 0; i < size; i++) {
      float p = 1.0;
      for (int j = 0; j < i; j++)
        p *= x;
      ret += arr[i] * p;
    }
    return ret;
  }

  __abstract_access__("(1,'w','local')") inline void square() {
    std::vector<float> sarr;
    int N = size;
    int M = N * N;
    sarr.resize(M, 0);
    // for(int i = 0; i < M; i++) sarr.push_back(0);

    for (int i = 0; i < size; i++) {
      for (int j = 0; j < size; j++) {
        sarr[i + j] += arr[i] * arr[j];
      }
    }
    setArray(sarr, M);
  }
};

class __tree_structure__ Inner : public Node {
public:
  __tree_child__ Node *l;
  __tree_child__ Node *r;
  void print() override;
  __tree_traversal__ void buildTree(int depth, int d, int size, float s,
                                    float e) override;
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

class __tree_structure__ Leaf : public Node {
public:
  __tree_child__ Poly *coeff;
  void print() override;
  __tree_traversal__ void buildTree(int depth, int d, int size, float s,
                                    float e) override;
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

void Inner::print() {
  l->print();
  r->print();
}

void Leaf::print() {
  std::cout << "Range:[ " << startDom << ", " << endDom << "]" << std::endl;
  coeff->print();
}

// build a balanced kd-tree
__tree_traversal__ void Inner::buildTree(int depth, int d, int size, float s,
                                         float e) {
  startDom = s;
  endDom = e;
  projectVal = 0.0;

  if (d == depth - 1) {
    l = new Leaf();
    l->type = LEAF;
  }

  if (d == depth - 1) {
    r = new Leaf();
    r->type = LEAF;
  }
  if (d < depth - 1) {
    l = new Inner();
    l->type = INNER;
  }

  if (d < depth - 1) {
    r = new Inner();
    r->type = INNER;
  }

  l->buildTree(depth, d + 1, size, s, (s + e) / 2);
  r->buildTree(depth, d + 1, size, (s + e) / 2, e);
}

__tree_traversal__ void Leaf::buildTree(int depth, int d, int size, float s,
                                        float e) {
  startDom = s;
  endDom = e;
  projectVal = 0.0;
  coeff = new Poly();
  coeff->assignCoeff(size);
}

__tree_traversal__ void Inner::addConst(float c) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  l->addConst(c);
  r->addConst(c);
}

// adding constant to x^0
__tree_traversal__ void Leaf::addConst(float c) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  coeff->addCons(c);
}

__tree_traversal__ void Inner::multConst(float c) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif

  l->multConst(c);
  r->multConst(c);
}

// multiplying every coeff by the constant
__tree_traversal__ void Leaf::multConst(float c) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  coeff->multConst(c);
}

__tree_traversal__ void Inner::divConst(float c) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  l->divConst(c);
  r->divConst(c);
}

// dividing every coeff by the constant
__tree_traversal__ void Leaf::divConst(float c) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  coeff->divConst(c);
}

__tree_traversal__ void Inner::differentiate() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  l->differentiate();
  r->differentiate();
}

// differentiate the polynomial for the range
__tree_traversal__ void Leaf::differentiate() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif

  coeff->differentiate();
}

__tree_traversal__ void Inner::mulVar() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  l->mulVar();
  r->mulVar();
}

// multiply the function by x
__tree_traversal__ void Leaf::mulVar() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  coeff->mulVar();
}

__tree_traversal__ void Inner::splitLeft(float _s, float _e) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  // splitting if needed
  if (l->type == LEAF && ((_s > l->startDom && _s < l->endDom) ||
                          (_e > l->startDom && _e < l->endDom))) {

    // split into 5 nodes
    if ((_s > l->startDom && _s < l->endDom) &&
        (_e > l->startDom && _e < l->endDom)) {
      float s1 = l->startDom;
      float e1 = _s;
      float s2 = e1;
      float e2 = _e;
      float s3 = e2;
      float e3 = l->endDom;

      Leaf *const oldNode = static_cast<Leaf *>(l);
      l = new Inner();
      auto *const newInner1 = static_cast<Inner *>(l);
      newInner1->type = INNER;
      newInner1->startDom = s1;
      newInner1->endDom = e3;

      // create node Inner1 and leaf1
      newInner1->l = new Leaf();
      static_cast<Leaf *>(newInner1->l)->coeff = new Poly();

      newInner1->l->type = LEAF;
      newInner1->l->startDom = s1;
      newInner1->l->endDom = e1;
      static_cast<Leaf *>(newInner1->l)
          ->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);

      // creating Inner2 and leaf2, and leaf3
      newInner1->r = new Inner();
      auto *const newInner2 = static_cast<Inner *>(newInner1->r);
      newInner2->type = INNER;
      newInner2->startDom = s2;
      newInner2->endDom = e3;

      newInner2->l = new Leaf();
      static_cast<Leaf *>(newInner2->l)->coeff = new Poly();

      newInner2->l->type = LEAF;
      newInner2->l->startDom = s2;
      newInner2->l->endDom = e2;
      static_cast<Leaf *>(newInner2->l)
          ->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);

      newInner2->r = new Leaf();
      static_cast<Leaf *>(newInner2->r)->coeff = new Poly();

      newInner2->r->type = LEAF;
      newInner2->r->startDom = s3;
      newInner2->r->endDom = e3;
      static_cast<Leaf *>(newInner2->r)
          ->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);
      delete oldNode;
    }
    // split into three nodes
    else {
      float s1 = l->startDom;
      float e1;
      if (_s > l->startDom && _s < l->endDom)
        e1 = _s;
      else
        e1 = _e;

      float s2 = e1;
      float e2 = l->endDom;

      auto *const oldNode = static_cast<Leaf *>(l);

      // create Inner Node
      l = new Inner();
      l->type = INNER;
      auto *const newInner = static_cast<Inner *>(l);
      newInner->type = INNER;
      newInner->startDom = s1;
      newInner->endDom = e2;

      newInner->l = new Leaf();
      static_cast<Leaf *>(newInner->l)->coeff = new Poly();

      newInner->l->type = LEAF;
      newInner->l->startDom = s1;
      newInner->l->endDom = e1;
      static_cast<Leaf *>(newInner->l)
          ->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);

      newInner->r = new Leaf();
      static_cast<Leaf *>(newInner->r)->coeff = new Poly();

      newInner->r->type = LEAF;
      newInner->r->startDom = s2;
      newInner->r->endDom = e2;
      static_cast<Leaf *>(newInner->r)
          ->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);
      delete oldNode;
    }
  }
}

__tree_traversal__ void Inner::splitRight(float _s, float _e) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  // splitting if needed
  if (r->type == LEAF && ((_s > r->startDom && _s < r->endDom) ||
                          (_e > r->startDom && _e < r->endDom))) {

    // split into 5 nodes
    if ((_s > r->startDom && _s < r->endDom) &&
        (_e > r->startDom && _e < r->endDom)) {
      float s1 = r->startDom;
      float e1 = _s;
      float s2 = e1;
      float e2 = _e;
      float s3 = e2;
      float e3 = r->endDom;

      Leaf *const oldNode = static_cast<Leaf *>(r);
      r = new Inner();
      auto *const newInner1 = static_cast<Inner *>(r);
      newInner1->type = INNER;
      newInner1->startDom = s1;
      newInner1->endDom = e3;

      // create node Inner1 and leaf1
      newInner1->l = new Leaf();
      static_cast<Leaf *>(newInner1->l)->coeff = new Poly();

      newInner1->l->type = LEAF;
      newInner1->l->startDom = s1;
      newInner1->l->endDom = e1;
      static_cast<Leaf *>(newInner1->l)
          ->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);

      // creating Inner2 and leaf2, and leaf3
      newInner1->r = new Inner();
      auto *const newInner2 = static_cast<Inner *>(newInner1->r);
      newInner2->type = INNER;
      newInner2->startDom = s2;
      newInner2->endDom = e3;

      newInner2->l = new Leaf();
      static_cast<Leaf *>(newInner2->l)->coeff = new Poly();

      newInner2->l->type = LEAF;
      newInner2->l->startDom = s2;
      newInner2->l->endDom = e2;
      static_cast<Leaf *>(newInner2->l)
          ->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);

      newInner2->r = new Leaf();
      static_cast<Leaf *>(newInner2->r)->coeff = new Poly();

      newInner2->r->type = LEAF;
      newInner2->r->startDom = s3;
      newInner2->r->endDom = e3;
      static_cast<Leaf *>(newInner2->r)
          ->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);
      delete oldNode;
    }
    // split into three nodes
    else {
      float s1 = r->startDom;
      float e1;
      if (_s > r->startDom && _s < r->endDom)
        e1 = _s;
      else
        e1 = _e;

      float s2 = e1;
      float e2 = r->endDom;

      auto *const oldNode = static_cast<Leaf *>(r);

      // create Inner Node
      r = new Inner();
      r->type = INNER;
      auto *const newInner = static_cast<Inner *>(r);
      newInner->type = INNER;
      newInner->startDom = s1;
      newInner->endDom = e2;

      newInner->l = new Leaf();
      static_cast<Leaf *>(newInner->l)->coeff = new Poly();

      newInner->l->type = LEAF;
      newInner->l->startDom = s1;
      newInner->l->endDom = e1;
      static_cast<Leaf *>(newInner->l)
          ->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);

      newInner->r = new Leaf();
      static_cast<Leaf *>(newInner->r)->coeff = new Poly();

      newInner->r->type = LEAF;
      newInner->r->startDom = s2;
      newInner->r->endDom = e2;
      static_cast<Leaf *>(newInner->r)
          ->coeff->setArray(oldNode->coeff->arr, oldNode->coeff->size);
      delete oldNode;
    }
  }
}

void Inner::rangeMulConst(float c, float _s, float _e) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  if (_s >= endDom || _e <= startDom)
    return;

  splitLeft(_s, _e);
  ;
  splitRight(_s, _e);

  l->rangeMulConst(c, _s, _e);
  r->rangeMulConst(c, _s, _e);
}

// multiply the range by a constant
__tree_traversal__ void Leaf::rangeMulConst(float c, float _s, float _e) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  if (_s >= endDom || _e <= startDom)
    return;

  coeff->multConst(c);
}

void Inner::rangeMulVar(float _s, float _e) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  if (_s >= endDom || _e <= startDom)
    return;

  splitLeft(_s, _e);
  ;
  splitRight(_s, _e);
  l->rangeMulVar(_s, _e);
  r->rangeMulVar(_s, _e);
}

// multiply the range by x
__tree_traversal__ void Leaf::rangeMulVar(float _s, float _e) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  if (_s >= endDom || _e <= startDom)
    return;

  coeff->mulVar();
}

__tree_traversal__ void Inner::boundedIntegrate(float _a, float _b) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  if (_a > endDom || _b < startDom) {
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
__tree_traversal__ void Leaf::boundedIntegrate(float _a, float _b) {

#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  if (_a > endDom || _b < startDom) {
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
__tree_traversal__ void Inner::project(float x) {

#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  if (x < startDom || x > endDom)
    return;

  l->project(x);
  r->project(x);

  if (x >= l->startDom && x < l->endDom)
    projectVal = l->projectVal;
  if (x >= r->startDom && x < r->endDom)
    projectVal = r->projectVal;
}

__tree_traversal__ void Leaf::project(float x) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  if (x < startDom || x > endDom)
    return;

  projectVal = coeff->project(x);
}

// square the domain of the function
__tree_traversal__ void Inner::square() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  l->square();
  r->square();
}

__tree_traversal__ void Leaf::square() {

#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif
  coeff->square();
}

void runExp2(Node *n) {

  n->differentiate();
  n->differentiate();
  n->differentiate();
  n->differentiate();
  n->differentiate();
  n->project(0);
}

void runExp1(Node *n) {

  n->differentiate();
  n->differentiate();
  n->square();
  n->square();
  n->addConst(1);
  n->mulVar();
  n->addConst(1);
  n->mulVar();
  n->addConst(1);
  n->mulVar();
  n->addConst(1);
  n->mulVar();
}

void runExp3(Node *n) {

  n->addConst(3.5);
  n->square();
  n->rangeMulConst(0, -10000, 0);
  n->mulVar();
  n->mulVar();
  n->mulVar();
  n->boundedIntegrate(-100000, 100000);
}

// void runExp1(int n) {
//   Node *exp1 = new Inner();
//   exp1->type = INNER;
//   exp1->buildTree(1, 0, 4, 0, n);
//   exp1->addConst(1);

//   for (int i = 0; i < n; i += 7) {
//     exp1->rangeMulConst(i, i - 1, i);
//     exp1->rangeMulConst(i + 1, i, i + 1);
//     exp1->rangeMulConst(i + 2, i + 1, i + 2);
//     exp1->rangeMulConst(i + 3, i + 2, i + 3);
//     exp1->rangeMulConst(i + 4, i + 3, i + 4);
//     exp1->rangeMulConst(i + 5, i + 1, i + 5);
//     exp1->rangeMulConst(i + 6, i + 2, i + 6);
//     exp1->rangeMulConst(i + 7, i + 3, i + 7);
//   }
//   exp1->mulVar();
//   exp1->mulVar();

//   Node *exp2 = new Inner();
//   exp2->type = INNER;
//   exp2->buildTree(1, 0, 1, 0, n);
//   exp2->addConst(1);

//   for (int i = 0; i < n; i += 7) {
//     exp2->rangeMulConst(i, i - 1, i);
//     exp2->rangeMulConst(i + 1, i, i + 1);
//     exp2->rangeMulConst(i + 2, i + 1, i + 2);
//     exp2->rangeMulConst(i + 3, i + 2, i + 3);
//     exp2->rangeMulConst(i + 4, i + 3, i + 4);
//     exp2->rangeMulConst(i + 5, i + 1, i + 5);
//     exp2->rangeMulConst(i + 6, i + 2, i + 6);
//     exp2->rangeMulConst(i + 7, i + 3, i + 7);
//   }
//   exp2->mulVar();
//   exp2->boundedIntegrate(0, n);

//   auto res = exp1->projectVal + exp2->projectVal;
// }

int main(int argc, char **argv) {

  int n = atoi(argv[1]);
  int prog = atoi(argv[2]);

  auto T = new Inner();
  T->type = INNER;
  T->buildTree(n, 0, 4, -100000, 100000);

#ifdef PAPI
  start_counters();
#endif
  auto t1 = std::chrono::high_resolution_clock::now();
  if (prog == 1)
    runExp1(T);
  else if (prog == 2)
    runExp2(T);
  else if (prog == 3)
    runExp3(T);

  auto t2 = std::chrono::high_resolution_clock::now();
#ifdef PAPI
  read_counters();
  print_counters();
#endif
  printf(
      "Runtime: %llu microseconds\n",
      std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count());

  return 0;
};
