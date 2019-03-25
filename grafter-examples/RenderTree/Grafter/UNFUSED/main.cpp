#include "ComputeHeights.h"
#include "DocBuilder.h"
#include "Print.h"
#include "RenderTree.h"
#include "ResolveFlexWidths.h"
#include "ResolveRelativeWidth.h"
#include "SetFont.h"
#include "SetPositions.h"
#include <chrono>
#include <stdlib.h>
#include <sys/time.h>

#pragma clang diagnostic ignored "-Wwritable-strings"

#ifdef PAPI
#include <iostream>
using namespace std;
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

void render(Document *Doc) {

#ifdef PAPI
  start_counters();
#endif
  auto t1 = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < 1; i++) {
    Doc->resolveFlexWidths();
    Doc->resolveRelativeWidths();
    Doc->setFont();
    Doc->computeHeights();
    Doc->setPositions();
  }

  auto t2 = std::chrono::high_resolution_clock::now();
#ifdef PAPI
  read_counters();
  print_counters();
#endif

  printf(
      "Runtime: %llu microseconds\n",
      std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count());

  printf("Tree Size :%d\n", Doc->computeTreeSize());
}

int main(int argc, char **argv) {
  int N = atoi(argv[1]);
  int prog = atoi(argv[2]);

  Document *Doc;
  if (prog == 1)
    Doc = BuildDoc1(N);
  else if (prog == 2)
    Doc = BuildDoc2(N);
  else if (prog == 3)
    Doc = BuildDoc3(N , N/5);
#ifndef BUILD_ONLY
  render(Doc);
#endif

#ifdef COUNT_VISITS
  printf("Node Visits: %d\n", _VISIT_COUNTER);
#endif
}
