#include "RenderTree.h"
#include <stdlib.h>

Page *buildPageDoc1() {
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

// a list of pages (book)
Document *BuildDoc1(int N) {
  Document *Doc = new Document();
  auto *PageListNode = Doc->PageList = new PageListInner();

  for (int i = 0; i < N - 2; i++) {
    PageListNode->Content = buildPageDoc1();
    static_cast<PageListInner *>(PageListNode)->NextPage = new PageListInner();
    PageListNode = static_cast<PageListInner *>(PageListNode)->NextPage;
  }
  PageListNode->Content = buildPageDoc1();
  static_cast<PageListInner *>(PageListNode)->NextPage = new PageListEnd();
  static_cast<PageListInner *>(PageListNode)->NextPage->Content =
      buildPageDoc1();
  return Doc;
}

List *createList() {
  auto ret = new List();
  ret->WMode = ABS;
  ret->Width = 50;
  {
    ret->ItemMargin = 3;
    ret->Items.Size = 3;
    ret->Items.Items = new String[3];
    ret->Items.Items[0].Length = 100;
    ret->Items.Items[0].Content = "this is item one";

    ret->Items.Items[1].Length = 150;
    ret->Items.Items[1].Content = "this is item two";

    ret->Items.Items[2].Length = 120;
    ret->Items.Items[2].Content = "this is item three";
  }
  return ret;
}

TextBox *createTextBox() {
  auto *ret = new TextBox();
  ret->WMode = REL;
  ret->RelWidth = 1;
  ret->ContentText.Length = 1000;
  return ret;
}
HorizontalContainer *createHorizContainer(int ElementsCount, int depth);

VerticalContainer *createVerticalContainer(int N, int depth) {
  auto *ret = new VerticalContainer();
  ret = new VerticalContainer();
  ret->WMode = ABS;
  ret->Width = 150;

  // build H1E2 Vertical Container
  if (N == 0)
    ret->HorizList = new HorizontalContainerListEnd();
  else
    ret->HorizList = new HorizontalContainerListInner();

  auto *curContainer = ret->HorizList;
  for (int i = 0; i < N; i++) {
    curContainer->Content = createHorizContainer(
        N ? rand() % N : N, depth ? rand() % depth : depth);
    if (i == N - 1)
      ((HorizontalContainerListInner *)curContainer)->Next =
          new HorizontalContainerListEnd();
    else
      ((HorizontalContainerListInner *)curContainer)->Next =
          new HorizontalContainerListInner();

    curContainer = ((HorizontalContainerListInner *)curContainer)->Next;
  }
  curContainer->Content =
      createHorizContainer(N ? rand() % N : N, depth ? rand() % depth : depth);
  return ret;
}

Image *createImage() {
  auto *Img = new Image();
  Img->WMode = REL;
  Img->RelWidth = 1;
  static_cast<Image *>(Img)->ImageOriginalHeight = 100;
  static_cast<Image *>(Img)->ImageOriginalWidth = 200;
  return Img;
}

Element *createElement(int N, int depth) {

  while (true) {
    switch (rand() % 4) {
    case 0:
      if (depth == 0)
        continue;
      return createVerticalContainer(N, depth);
      break;
    case 1:
      return createTextBox();
      break;
    case 2:
      return createImage();
      break;
    case 3:
      return createList();
      break;
    }
  }
}

// depth and number of elements
HorizontalContainer *createHorizContainer(int N, int depth) {
  auto *ret = new HorizontalContainer();
  ret->WMode = REL;
  ret->Width = -1; // not set
  ret->RelWidth = 1;
  if (N == 0)
    ret->ElementsList = new ElementListEnd();
  else
    ret->ElementsList = new ElementListInner();

  auto *curElement = (ret->ElementsList);
  for (int i = 0; i < N; i++) {
    curElement->Content =
        createElement(N ? rand() % N : N, depth ? rand() % depth : depth);
    if (i == N - 1)
      ((ElementListInner *)(curElement))->Next = new ElementListEnd();
    else
      ((ElementListInner *)(curElement))->Next = new ElementListInner();
    curElement = ((ElementListInner *)(curElement))->Next;
  }
  curElement->Content =
      createElement(N ? rand() % N : N, depth ? rand() % depth : depth);
  return ret;
}

Page *buildPageDoc2(int N, int depth) {
  Page *P = new Page();
  P->Width = -1; // not set
  P->WMode = FLEX;
  P->HorizList = new HorizontalContainerListInner();
  auto *curContainer = P->HorizList;
  for (int i = 0; i < N; i++) {
    curContainer->Content = createHorizContainer(rand() % N, rand() % depth);
    if (i == N - 1)
      ((HorizontalContainerListInner *)curContainer)->Next =
          new HorizontalContainerListEnd();
    else
      ((HorizontalContainerListInner *)curContainer)->Next =
          new HorizontalContainerListInner();

    curContainer = ((HorizontalContainerListInner *)curContainer)->Next;
  }
  curContainer->Content = createHorizContainer(rand() % N, rand() % depth);
  P->FontStyle.Type = 1;
  P->FontStyle.Size = 1;
  P->FontStyle.Size = 1;
  return P;
}

// one page with deep nested components (depth = N)
Document *BuildDoc2(int N) {
  Document *Doc = new Document();
  Doc->PageList = new PageListEnd();
  Doc->PageList->Content = buildPageDoc2(N, N);

  return Doc;
}

Document *BuildDoc3(int pageCount, int sizeMax) {
  Document *Doc = new Document();

  Doc->PageList = new PageListInner();
  auto *curPage = Doc->PageList;
  for (int i = 0; i < pageCount; i++) {
    curPage->Content = buildPageDoc2((rand()%sizeMax) +1, (rand()%sizeMax) +1);
    if (i == pageCount - 1)
      ((PageListInner *)curPage)->NextPage = new PageListEnd();
    else
      ((PageListInner *)curPage)->NextPage = new PageListInner();
    curPage = ((PageListInner *)curPage)->NextPage;
  }
  curPage->Content = buildPageDoc2((rand()%sizeMax) +1, (rand()%sizeMax) +1);

  return Doc;
}
