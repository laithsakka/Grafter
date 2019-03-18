#include "ComputeHeights.h"
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

Page *buildPage() {
  Page *P = new Page();
  P->Width = -1; // not set
  P->WMode = FLEX;
  P->HorizList = new HorizontalContainerListInner();
  P->FontStyle.Type = 1;
  P->FontStyle.Size = 1;
  P->FontStyle.Size = 1;

  // H1
  auto *H1 = (HorizontalContainerListInner *)P->HorizList;
  H1->Content = new HorizontalContainer();
  H1->Next = new HorizontalContainerListEnd();
  H1->Content->WMode = FLEX;

  // H2
  auto *H2 = H1->Next;
  H2->Content = new HorizontalContainer();
  H2->Content->WMode = REL;
  H2->Content->Width = -1; // not set
  H2->Content->RelWidth = 1;

  // building H2
  H2->Content->ElementsList = new ElementListEnd();

  H2->Content->ElementsList->Content = new TextBox();
  {
    auto *TxtBox = (TextBox *)H2->Content->ElementsList->Content;
    TxtBox->WMode = REL;
    TxtBox->RelWidth = 1;
    TxtBox->ContentText.Length = 1000;
  }
  //// building H1 Content
  auto *H1E1 = H1->Content->ElementsList = new ElementListInner();
  auto *H1E2 = ((ElementListInner *)H1->Content->ElementsList)->Next =
      new ElementListEnd();

  H1E1->Content = new List();
  H1E1->Content->WMode = ABS;
  H1E1->Content->Width = 50;
  {
    auto *Lst = (List *)(H1E1->Content);
    Lst->ItemMargin = 3;
    Lst->Items.Size = 3;
    Lst->Items.Items = new String[3];
    Lst->Items.Items[0].Length = 100;
    Lst->Items.Items[0].Content = "this is item one";

    Lst->Items.Items[1].Length = 150;
    Lst->Items.Items[1].Content = "this is item two";

    Lst->Items.Items[2].Length = 120;
    Lst->Items.Items[2].Content = "this is item three";
  }

  H1E2->Content = new VerticalContainer();
  H1E2->Content->WMode = ABS;
  H1E2->Content->Width = 150;

  // build H1E2 Vertical Container
  auto *H1E2H1 = ((VerticalContainer *)H1E2->Content)->HorizList =
      new HorizontalContainerListInner();
  H1E2H1->Content = new HorizontalContainer();
  H1E2H1->Content->WMode = REL;
  H1E2H1->Content->RelWidth = 1;

  H1E2H1->Content->ElementsList = new ElementListEnd();
  auto *TxtBox = H1E2H1->Content->ElementsList->Content = new TextBox();
  TxtBox->Width = -1;
  TxtBox->WMode = REL;
  TxtBox->RelWidth = 1;

  ((TextBox *)TxtBox)->ContentText.Length = 1000;
  ((TextBox *)TxtBox)->ContentText.Content = "bla bla bla";

  auto *H1E2H2 = ((HorizontalContainerListInner *)H1E2H1)->Next =
      new HorizontalContainerListEnd();

  H1E2H2->Content = new HorizontalContainer();
  H1E2H2->Content->WMode = REL;
  H1E2H2->Content->RelWidth = .75;

  H1E2H2->Content->ElementsList = new ElementListEnd();
  auto *Img = H1E2H2->Content->ElementsList->Content = new Image();
  Img->WMode = REL;
  Img->RelWidth = 1;
  static_cast<Image *>(Img)->ImageOriginalHeight = 100;
  static_cast<Image *>(Img)->ImageOriginalWidth = 200;

  return P;
}

Document *BuildDoc(int N) {
  Document *Doc = new Document();
  auto *PageListNode = Doc->PageList = new PageListInner();

  for (int i = 0; i < N - 2; i++) {
    PageListNode->Content = buildPage();
    static_cast<PageListInner *>(PageListNode)->NextPage = new PageListInner();
    PageListNode = static_cast<PageListInner *>(PageListNode)->NextPage;
  }
  PageListNode->Content = buildPage();
  static_cast<PageListInner *>(PageListNode)->NextPage = new PageListEnd();
  static_cast<PageListInner *>(PageListNode)->NextPage->Content = buildPage();
  return Doc;
}

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
}

int main(int argc, char **argv) {
  int N = atoi(argv[1]);
  Document *Doc = BuildDoc(N);

#ifndef BUILD_ONLY
  render(Doc);
#endif

#ifdef COUNT_VISITS
  printf("Node Visits: %d\n", _VISIT_COUNTER);
#endif
}
