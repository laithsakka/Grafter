#include "RenderTree.h"
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

Node *buildPage() {
  Node *P = new Node();
  P->NType = Page_;
  P->Width = -1; // not set
  P->WMode = FLEX;
  P->FontStyle.Type = 1;
  P->FontStyle.Size = 1;
  P->FontStyle.Size = 1;

  P->HorizListInPage = new Node();
  P->HorizListInPage->NType = HorizontalContainerListInner_;

  // H1
  auto *H1 = P->HorizListInPage;
  H1->ContentHorizContainer = new Node();
  H1->ContentHorizContainer->NType = HorizontalContainer_;
  H1->ContentHorizContainer->WMode = FLEX;

  H1->NextHoriz = new Node();
  H1->NextHoriz->NType = HorizontalContainerListEnd_;

  // H2
  auto *H2 = H1->NextHoriz;
  H2->ContentHorizContainer = new Node();
  H2->ContentHorizContainer->NType = HorizontalContainer_;

  H2->ContentHorizContainer->WMode = REL;
  H2->ContentHorizContainer->Width = -1; // not set
  H2->ContentHorizContainer->RelWidth = 1;

  // building H2
  H2->ContentHorizContainer->ElementsList = new Node();
  H2->ContentHorizContainer->ElementsList->NType = ElementListEnd_;

  H2->ContentHorizContainer->ElementsList->ContentElement = new Node();
  H2->ContentHorizContainer->ElementsList->ContentElement->NType = TextBox_;
  {
    auto *TxtBox = H2->ContentHorizContainer->ElementsList->ContentElement;
    TxtBox->WMode = REL;
    TxtBox->RelWidth = 1;
    TxtBox->ContentText.Length = 1000;
  }
  //// building H1 Content
  auto *H1E1 = H1->ContentHorizContainer->ElementsList = new Node();
  H1E1->NType = ElementListInner_;

  auto *H1E2 = H1E1->NextElement = new Node();
  H1E2->NType = ElementListEnd_;

  H1E1->ContentElement = new Node();
  H1E1->ContentElement->NType = List_;

  H1E1->ContentElement->WMode = ABS;
  H1E1->ContentElement->Width = 50;
  {
    auto *Lst = H1E1->ContentElement;
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

  H1E2->ContentElement = new Node();
  H1E2->ContentElement->NType = VerticalContainer_;

  H1E2->ContentElement->WMode = ABS;
  H1E2->ContentElement->Width = 150;

  // build H1E2 Vertical Container
  auto *H1E2H1 = H1E2->ContentElement->HorizListInVC = new Node();
  H1E2H1->NType = HorizontalContainerListInner_;

  H1E2H1->ContentHorizContainer = new Node();
  H1E2H1->ContentHorizContainer->NType = HorizontalContainer_;
  H1E2H1->ContentHorizContainer->WMode = REL;
  H1E2H1->ContentHorizContainer->RelWidth = 1;

  H1E2H1->ContentHorizContainer->ElementsList = new Node();
  H1E2H1->ContentHorizContainer->ElementsList->NType = ElementListEnd_;

  auto *TxtBox = H1E2H1->ContentHorizContainer->ElementsList->ContentElement =
      new Node();
  TxtBox->NType = TextBox_;

  TxtBox->Width = -1;
  TxtBox->WMode = REL;
  TxtBox->RelWidth = 1;

  TxtBox->ContentText.Length = 1000;
  TxtBox->ContentText.Content = "bla bla bla";

  auto *H1E2H2 = H1E2H1->NextHoriz = new Node();
  H1E2H2->NType = HorizontalContainerListEnd_;

  H1E2H2->ContentHorizContainer = new Node();
  H1E2H2->ContentHorizContainer->NType = HorizontalContainer_;

  H1E2H2->ContentHorizContainer->WMode = REL;
  H1E2H2->ContentHorizContainer->RelWidth = .75;

  H1E2H2->ContentHorizContainer->ElementsList = new Node();
  H1E2H2->ContentHorizContainer->ElementsList->NType = ElementListEnd_;

  auto *Img = H1E2H2->ContentHorizContainer->ElementsList->ContentElement =
      new Node();
  Img->NType = Image_;
  Img->WMode = REL;
  Img->RelWidth = 1;
  Img->ImageOriginalHeight = 100;
  Img->ImageOriginalWidth = 200;

  return P;
}

Node *BuildDoc(int N) {
  Node *Doc = new Node();
  Doc->NType = Document_;

  auto *PageListNode = Doc->PageList = new Node();
  PageListNode->NType = PageListInner_;

  for (int i = 0; i < N - 2; i++) {
    PageListNode->ContentPageList = buildPage();
    PageListNode->NextPage = new Node();
    PageListNode->NextPage->NType = PageListInner_;
    PageListNode = PageListNode->NextPage;
  }
  PageListNode->ContentPageList = buildPage();
  PageListNode->NextPage = new Node();
  PageListNode->NextPage->NType = PageListEnd_;
  PageListNode->NextPage->ContentPageList = buildPage();
  return Doc;
}

void render(Node *n) {
  #ifdef PAPI
    start_counters();
  #endif
  auto t1 = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < 1; i++) {
    FontInfo d;
    resolveFlexWidths(n);
    resolveRelativeWidths(n, 0);
    setFont(n, d);
    computeHeight(n);
    setPositions(n, 0, 0);
  }
  auto t2 = std::chrono::high_resolution_clock::now();
  #ifdef PAPI
    read_counters();
    print_counters();
  #endif
printf("Runtime: %llu microseconds\n",
         std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count());
}

int main(int argc, char **argv) {
  int N = atoi(argv[1]);
  Node *n = BuildDoc(N);

#ifndef BUILD_ONLY
  render(n);
#endif


#ifdef COUNT_VISITS
  printf("Node Visits: %d\n", _VISIT_COUNTER);
#endif
}
