#include "RenderTree.h"
#include "ResolveFlexWidths.h"
#include "ResolveRelativeWidth.h"
#include <sys/time.h>
long long currentTimeInMilliseconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

Page *buildPage() {
  auto *P = new Page();
  P->Width = 200;
  P->WMode = ABS;
  P->HorizList = new HorizontalContainerListInner();

  // H1
  auto *H1 = (HorizontalContainerListInner *)P->HorizList;
  H1->Content = new HorizontalContainer();
  H1->Next = new HorizontalContainerListEnd();
  H1->Content->WMode = FLEX;

  // H2
  auto *H2 = H1->Next;
  H2->Content = new HorizontalContainer();
  H2->Content->WMode = FLEX;

  // building H2
  H2->Content->ElementsList = new ElementListEnd();

  H2->Content->ElementsList->Content = new TextBox();

  //// building H1 Content
  auto *H1E1 = H1->Content->ElementsList = new ElementListInner();
  auto *H1E2 = ((ElementListInner *)H1->Content->ElementsList)->Next =
      new ElementListEnd();

  H1E1->Content = new List();
  H1E1->Content->WMode = ABS;
  H1E1->Content->Width = 50;

  H1E2->Content = new VerticalContainer();
  H1E2->Content->WMode = FLEX;

  // build H1E2 Vertical Container
  auto *H1E2H1 = ((VerticalContainer *)H1E2->Content)->HorizList =
      new HorizontalContainerListInner();
  H1E2H1->Content = new HorizontalContainer();
  H1E2H1->Content->WMode = FLEX;
  H1E2H1->Content->ElementsList = new ElementListEnd();
  auto *TxtBox = H1E2H1->Content->ElementsList->Content = new TextBox();
  TxtBox->Width = 150;
  TxtBox->WMode = ABS;

  auto *H1E2H2 = ((HorizontalContainerListInner *)H1E2H1)->Next =
      new HorizontalContainerListEnd();

  H1E2H2->Content = new HorizontalContainer();
  H1E2H2->Content->WMode = FLEX;
  H1E2H2->Content->ElementsList = new ElementListEnd();
  auto *Img = H1E2H2->Content->ElementsList->Content = new Image();
  Img->WMode = ABS;
  Img->Width = 100;
  return P;
}

Document *BuildDoc() {
  Document *Doc = new Document();
  auto *PageListNode = Doc->PageList = new PageListInner();

  for (int i = 0; i < 10000; i++) {
    PageListNode->Content = buildPage();
    static_cast<PageListInner *>(PageListNode)->NextPage = new PageListInner();
    PageListNode = static_cast<PageListInner *>(PageListNode)->NextPage;
  }
  PageListNode->Content = buildPage();
  static_cast<PageListInner *>(PageListNode)->NextPage = new PageListEnd();
  static_cast<PageListInner *>(PageListNode)->NextPage->Content = buildPage();
  return Doc;
}

int main() {
  Document *Doc = BuildDoc();
  auto t1 = currentTimeInMilliseconds();
  Doc->resolveFlexWidths();
  Doc->resolveRelativeWidths();
  auto t2 = currentTimeInMilliseconds();
  printf("duration is %llu\n", t2 - t1);
}
