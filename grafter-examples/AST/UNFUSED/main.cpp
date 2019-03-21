#include "AST.h"
#include "ASTBuilder.h"
#include "ConstantFolding.h"
#include "ConstantPropagationAssigment.h"
#include "DesugarDec.h"
#include "DesugarInc.h"
#include "Print.h"
#include "RemoveUnreachableBranches.h"
#include "computeSize.h"
#include <chrono>
#include <stdlib.h>
#include <sys/time.h>
#include <vector>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

using namespace std;

#ifdef PAPI
#include <iostream>
#include <papi.h>
#define SIZE 3
string instance("Original");
int ret;
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
    //   f->print();
  }
  auto t2 = std::chrono::high_resolution_clock::now();
#ifdef PAPI
  read_counters();
  print_counters();
#endif
  printf(
      "Runtime: %llu microseconds\n",
      std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count());
  printf("Tree Size:%d\n", ls[0]->computeSize());
}

int main(int argc, char **argv) {
  // number of functions
  int FCount = atoi(argv[1]);

  int Prog = atoi(argv[2]);
  // how many tree to create
  int Itters = 1;
  vector<Program *> ls;
  ls.resize(Itters);

  for (int i = 0; i < Itters; i++) {
    switch (Prog) {
    case 1: // random function of regular size repeated many times (horiz AST)
      ls[i] = createProgram1(FCount);
      break;
    case 2: // the ast is a really large function (vert AST)
      ls[i] = createLargeFunctionProg(FCount);
      break;
    case 3: // the ast is a really large function (vert AST)
      ls[i] = createLongLiveRangeProg(FCount, FCount);
      break;
    default:
      break;
    }
  }
#ifndef BUILD_ONLY
  optimize(ls);
#endif

#ifdef COUNT_VISITS
  printf("Node Visits: %d\n", _VISIT_COUNTER);
#endif
}
