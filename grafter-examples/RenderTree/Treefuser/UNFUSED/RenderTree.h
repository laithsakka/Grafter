#ifndef RENDER_TREE
#define RENDER_TREE
#define __tree_structure__ __attribute__((annotate("tf_tree")))
#define __tree_child__ __attribute__((annotate("tf_child")))
#define __tree_traversal__ __attribute__((annotate("tf_fuse")))
#define __abstract_access__(AccessList)                                        \
  __attribute__((annotate("tf_strict_access" #AccessList)))

#include <stdio.h>
#include <string>
#include <vector>
#ifdef COUNT_VISITS
int   _VISIT_COUNTER=0;
#endif
using namespace std;

__abstract_access__("(1,'w','global')") inline void LogError1() {
  printf("ERROR: leaf element cant have WMode = FLEX");
}

enum MEASURE_MODE { ABS, REL, FLEX };
struct String {
  char *Content;
  int Length = 0;
};

struct ListItems {
public:
  String *Items;
  int Size = 0;
};

struct FontInfo {
public:
  unsigned int Type = -1;
  unsigned int Size = -1;
  unsigned int Color = -1;
};

enum NodeType {
  Document_,
  PageListEnd_,
  PageListInner_,
  Page_,
  HorizontalContainerListEnd_,
  HorizontalContainerListInner_,
  HorizontalContainer_,

  VerticalContainerListInner_,
  VerticalContainerListEnd_,
  VerticalContainer_,

  ElementListInner_,
  ElementListEnd_,
  TextBox_,
  List_,
  Image_
};

class __tree_structure__ Node {
public:
  NodeType NType;
  __tree_child__ Node *PageList = nullptr;
  __tree_child__ Node *ContentPageList = nullptr;
  __tree_child__ Node *NextPage = nullptr;
  __tree_child__ Node *HorizListInPage = nullptr;
  __tree_child__ Node *HorizListInVC = nullptr;

  __tree_child__ Node *ContentHorizContainer = nullptr;
  __tree_child__ Node *NextHoriz = nullptr;
  __tree_child__ Node *ContentElement = nullptr;
  __tree_child__ Node *NextElement = nullptr;
  __tree_child__ Node *ElementsList = nullptr;

  int MaxWidth;
  int AggregatedHeight;

  int AccumulatedWidth = 0;
  int MaxHeight = 0;

  String ContentText;

  ListItems Items;
  int ItemMargin;

  string path_to_image;
  float ImageOriginalWidth;
  float ImageOriginalHeight;
  String ImageURL;

  int PosX = 0;
  int PosY = 0;
  int Height = 0;
  int Width = 0;
  float RelWidth;
  MEASURE_MODE WMode;
  FontInfo FontStyle;
  unsigned int BackgroundColor = 0;
};
__abstract_access__("(1,'w','global')") inline void LogError2() {
  printf("ERROR: Cannot have WMode REL for page node");
}

__tree_traversal__ void resolveFlexWidths(Node *n) {
  if (n == NULL)
    return;


    #ifdef COUNT_VISITS
      _VISIT_COUNTER++;
    #endif

  // Document
  resolveFlexWidths(n->PageList);

  // Page List
  resolveFlexWidths(n->ContentPageList);
  resolveFlexWidths(n->NextPage);

  // Page
  resolveFlexWidths(n->HorizListInPage);
  if (n->NType == Page_) {
    if (n->WMode == FLEX) {
      n->Width = n->HorizListInPage->MaxWidth;
    }
  }

  // HorizontalContainerList
  resolveFlexWidths(n->ContentHorizContainer);
  resolveFlexWidths(n->NextHoriz);
  if (n->NType == HorizontalContainerListEnd_) {
    n->MaxWidth = n->ContentHorizContainer->Width;
  }
  if (n->NType == HorizontalContainerListInner_) {
    if (n->ContentHorizContainer->Width > n->NextHoriz->MaxWidth)
      n->MaxWidth = n->ContentHorizContainer->Width;
    else
      n->MaxWidth = n->NextHoriz->MaxWidth;
  }

  // Horizontal Container
  resolveFlexWidths(n->ElementsList);
  if (n->NType == HorizontalContainer_) {
    if (n->WMode == FLEX) {
      n->Width = n->ElementsList->AccumulatedWidth;
    }
  }

  // ElementList
  resolveFlexWidths(n->ContentElement);
  resolveFlexWidths(n->NextElement);
  if (n->NType == ElementListInner_) {
    n->AccumulatedWidth =
        n->ContentElement->Width + n->NextElement->AccumulatedWidth;
  }
  if (n->NType == ElementListEnd_) {
    n->AccumulatedWidth = n->ContentElement->Width;
  }

  // element except vertical container
  if (n->NType == Image_ || n->NType == List_ || n->NType == TextBox_) {
    if (n->WMode == FLEX) {
      LogError1();
    }
  }

  // vertical container
  resolveFlexWidths(n->HorizListInVC);
  if (n->NType == VerticalContainer_) {
    if (n->WMode == FLEX) {
      n->Width = n->HorizListInVC->MaxWidth;
    }
  }
}

__tree_traversal__ void resolveRelativeWidths(Node *n, int PWidth) {
  if (n == NULL)
    return;

    #ifdef COUNT_VISITS
      _VISIT_COUNTER++;
    #endif

  // Document
  resolveRelativeWidths(n->PageList, 0);

  // Page List
  resolveRelativeWidths(n->ContentPageList, 0);
  resolveRelativeWidths(n->NextPage, 0);

  // Page
  if (n->NType == Page_) {
    if (n->WMode == REL) {
      LogError2();
      return;
    }
  }
  resolveRelativeWidths(n->HorizListInPage, n->Width);

  // HorizontalContainerList
  resolveRelativeWidths(n->ContentHorizContainer, PWidth);
  resolveRelativeWidths(n->NextHoriz, PWidth);

  // HorizontalContainer
  if (n->NType == HorizontalContainer_) {
    if (n->WMode == REL) {
      n->Width = n->RelWidth * PWidth;
    }
  }
  resolveRelativeWidths(n->ElementsList, n->Width);

  // ElementList
  resolveRelativeWidths(n->ContentElement, PWidth);
  resolveRelativeWidths(n->NextElement, PWidth);

  // Element except vertical container
  if (n->NType == Image_ || n->NType == List_ || n->NType == TextBox_) {
    if (n->WMode == REL) {
      n->Width = n->RelWidth * PWidth;
    }
  }

  // Vertical Container
  if (n->NType == VerticalContainer_) {
    if (n->WMode == REL) {
      n->Width = n->RelWidth * PWidth;
    }
  }
  resolveRelativeWidths(n->HorizListInVC, n->Width);
}
__abstract_access__("(1,'w','global')") inline void LogErrorFontNotSet() {
  printf("Font Size Not Set");
}
__abstract_access__("(0,'r','global')") inline int computeTextHorizSpace(
    int Length, int FontSize) {

  // we can read the info from the map or compute it
  return Length * (FontSize);
}
__abstract_access__("(0,'r','global')") inline int getCharHeight(int FontSize) {
  // we can read the info from the map or compute it
  return (FontSize * 2);
}

// simply represent a pure function (reads are handled as parameters in this
// case)
__abstract_access__("(0,'r','global')") inline int computeListHeight(
    ListItems List, int FontSize, int ItemMargin, int Width) {
  int Height = 0;
  for (int i = 0; i < List.Size; i++) {
    int CharsTotalSpace = computeTextHorizSpace(List.Items[i].Length, FontSize);
    CharsTotalSpace += ItemMargin;
    Height += (CharsTotalSpace / Width);
    if (CharsTotalSpace % Width) {
      Height++;
    }
  }
  return Height;
}

__tree_traversal__ void computeHeight(Node *n) {
  if (n == NULL)
    return;


    #ifdef COUNT_VISITS
      _VISIT_COUNTER++;
    #endif

  // Document
  computeHeight(n->PageList);

  // PageList
  computeHeight(n->ContentPageList);
  computeHeight(n->NextPage);

  // Page
  computeHeight(n->HorizListInPage);
  if (n->NType == Page_) {
    n->Height = n->HorizListInPage->AggregatedHeight;
  }

  // HorizontalContainerList
  computeHeight(n->ContentHorizContainer);
  computeHeight(n->NextHoriz);
  if (n->NType == HorizontalContainerListEnd_) {
    n->AggregatedHeight = n->ContentHorizContainer->Height;
  }
  if (n->NType == HorizontalContainerListInner_) {
    n->AggregatedHeight = 0;
    n->AggregatedHeight =
        n->ContentHorizContainer->Height + n->NextHoriz->AggregatedHeight;
  }

  // HorizontalContainer
  computeHeight(n->ElementsList);
  if (n->NType == HorizontalContainer_) {
    n->Height = n->ElementsList->MaxHeight;
  }

  // element List
  computeHeight(n->ContentElement);
  computeHeight(n->NextElement);

  if (n->NType == ElementListEnd_) {
    n->MaxHeight = n->ContentElement->Height;
  }

  if (n->NType == ElementListInner_) {
    if (n->ContentElement->Height >= n->NextElement->MaxHeight) {
      n->MaxHeight = n->ContentElement->Height;
    } else {
      n->MaxHeight = n->NextElement->MaxHeight;
    }
  }

  if (n->NType == TextBox_) {
    int CharsTotalSpace;
    if (n->FontStyle.Size == (0 - 1))
      LogErrorFontNotSet();

    CharsTotalSpace =
        computeTextHorizSpace(n->ContentText.Length, n->FontStyle.Size);
    n->Height = (CharsTotalSpace / n->Width);
    if (CharsTotalSpace % n->Width) {
      n->Height = n->Height + 1;
    }
    n->Height = n->Height * getCharHeight(n->FontStyle.Size);
  }

  if (n->NType == List_) {
    if (n->FontStyle.Size == (0 - 1))
      LogErrorFontNotSet();
    n->Height =
        computeListHeight(n->Items, n->FontStyle.Size, n->ItemMargin, n->Width);
  }

  if (n->NType == Image_) {
    // keep the same ratio of the original image
    n->Height = n->Width * (n->ImageOriginalHeight / n->ImageOriginalWidth);
  }

  computeHeight(n->HorizListInVC);
  if (n->NType == VerticalContainer_)
    n->Height = n->HorizListInVC->AggregatedHeight;
}

__tree_traversal__ void setFont(Node *n, FontInfo ParentFontStyle) {
  // Doc
  if (n == NULL)
    return;


    #ifdef COUNT_VISITS
      _VISIT_COUNTER++;
    #endif

  setFont(n->PageList, n->FontStyle);

  // page list
  setFont(n->ContentPageList, ParentFontStyle);
  setFont(n->NextPage, ParentFontStyle);

  // page
  if (n->NType == Page_) {
    if (n->FontStyle.Size == (0 - 1))
      n->FontStyle.Size = ParentFontStyle.Size;
    if (n->FontStyle.Color == (0 - 1))
      n->FontStyle.Color = ParentFontStyle.Color;
    if (n->FontStyle.Type == (0 - 1))
      n->FontStyle.Type = ParentFontStyle.Type;
  }
  setFont(n->HorizListInPage, n->FontStyle);

  // HorizontalContainerList
  setFont(n->ContentHorizContainer, ParentFontStyle);
  setFont(n->NextHoriz, ParentFontStyle);

  // HorizontalContainer
  if (n->NType == HorizontalContainer_) {
    if (n->FontStyle.Size == (0 - 1))
      n->FontStyle.Size = ParentFontStyle.Size;
    if (n->FontStyle.Color == (0 - 1))
      n->FontStyle.Color = ParentFontStyle.Color;
    if (n->FontStyle.Type == (0 - 1))
      n->FontStyle.Type = ParentFontStyle.Type;
  }
  setFont(n->ElementsList, n->FontStyle);

  // element list
  setFont(n->ContentElement, ParentFontStyle);
  setFont(n->NextElement, ParentFontStyle);

  if (n->NType == Image_ || n->NType == List_ || n->NType == TextBox_ ||
      n->NType == VerticalContainer_) {
    if (n->FontStyle.Size == (0 - 1))
      n->FontStyle.Size = ParentFontStyle.Size;
    if (n->FontStyle.Color == (0 - 1))
      n->FontStyle.Color = ParentFontStyle.Color;
    if (n->FontStyle.Type == (0 - 1))
      n->FontStyle.Type = ParentFontStyle.Type;
  }
  setFont(n->HorizListInVC, n->FontStyle);
}

__tree_traversal__ void setPositions(Node *n, int CurrX, int CurrY) {
  if (n == NULL)
    return;

    #ifdef COUNT_VISITS
      _VISIT_COUNTER++;
    #endif

  // document
  setPositions(n->PageList, 0, 0);

  // page list
  setPositions(n->ContentPageList, 0, 0);
  setPositions(n->NextPage, 0, 0);

  // Page
  if (n->NType == Page_) {
    n->PosX = 0;
    n->PosY = 0;
  }
  setPositions(n->HorizListInPage, n->PosX, n->PosY);

  // HorizontalContainerList
  setPositions(n->ContentHorizContainer, CurrX, CurrY);
  int arg;
  if (n->NType == HorizontalContainerListInner_) {
    arg = n->ContentHorizContainer->Height;
  }
  setPositions(n->NextHoriz, CurrX, CurrY + arg);

  // Horizontal Container
  if (n->NType == HorizontalContainer_) {
    n->PosX = CurrX;
    n->PosY = CurrY;
  }
  setPositions(n->ElementsList, CurrX, CurrY);

  // element list
  int arg2;
  setPositions(n->ContentElement, CurrX, CurrY);
  if (n->NType == ElementListInner_) {
    arg2 = n->ContentElement->Width;
  }
  setPositions(n->NextElement, CurrX + arg2, CurrY);

  // element

  if (n->NType == Image_ || n->NType == List_ || n->NType == TextBox_ ||
      n->NType == VerticalContainer_) {
    n->PosX = CurrX;
    n->PosY = CurrY;
  }
  setPositions(n->HorizListInVC, CurrX, CurrY);
}

void print(Node *n) {
  if (n == NULL)
    return;

  if (n->NType == Document_) {
    printf("Document:\n");
    print(n->PageList);
  }

  if (n->NType == PageListEnd_) {
    print(n->ContentPageList);
  }
  if (n->NType == PageListInner_) {
    print(n->ContentPageList);
    print(n->NextPage);
  }

  if (n->NType == Page_) {
    printf("Page==> W:%d, H:%d ,PosX:%d, PosY:%d, FontSize:%d\n", n->Width,
           n->Height, n->PosX, n->PosY, n->FontStyle.Size);
    print(n->HorizListInPage);
  }

  if (n->NType == HorizontalContainerListInner_) {
    print(n->ContentHorizContainer);
    print(n->NextHoriz);
  }
  if (n->NType == HorizontalContainerListEnd_) {
    print(n->ContentHorizContainer);
  }
  if (n->NType == HorizontalContainer_) {
    printf("Horiz Container==> W:%d, H:%d ,PosX:%d, PosY:%d, FontSize:%d\n",
           n->Width, n->Height, n->PosX, n->PosY, n->FontStyle.Size);
    print(n->ElementsList);
  }

  if (n->NType == ElementListInner_) {
    print(n->ContentElement);
    print(n->NextElement);
  }
  if (n->NType == ElementListEnd_) {
    print(n->ContentElement);
  }
  if (n->NType == VerticalContainer_) {
    printf("VerticalContainer==> W:%d, H:%d ,PosX:%d, PosY:%d, FontSize:%d\n",
           n->Width, n->Height, n->PosX, n->PosY, n->FontStyle.Size);
    print(n->HorizListInVC);
  }

  if (n->NType == Image_) {
    printf("Image ==> W:%d, H:%d ,PosX:%d, PosY:%d, FontSize:%d\n", n->Width,
           n->Height, n->PosX, n->PosY, n->FontStyle.Size);
  }
  if (n->NType == List_) {
    printf("List==> W:%d, H:%d ,PosX:%d, PosY:%d, FontSize:%d\n", n->Width,
           n->Height, n->PosX, n->PosY, n->FontStyle.Size);
  }
  if (n->NType == TextBox_) {
    printf("Textbox==> W:%d, H:%d ,PosX:%d, PosY:%d, FontSize:%d\n", n->Width,
           n->Height, n->PosX, n->PosY, n->FontStyle.Size);
  }
}

int computeTreeSize(Node *n) {
  if (n == NULL)
    return 0;

  if (n->NType == Document_) {
    return sizeof(*n) + computeTreeSize(n->PageList);
  }

  if (n->NType == PageListEnd_) {
    return sizeof(*n) + computeTreeSize(n->ContentPageList);
  }
  if (n->NType == PageListInner_) {
    return sizeof(*n) + computeTreeSize(n->ContentPageList) +
           computeTreeSize(n->NextPage);
  }

  if (n->NType == Page_) {

    return sizeof(*n) + computeTreeSize(n->HorizListInPage);
  }

  if (n->NType == HorizontalContainerListInner_) {
    return sizeof(*n) + computeTreeSize(n->ContentHorizContainer) +
           computeTreeSize(n->NextHoriz);
  }
  if (n->NType == HorizontalContainerListEnd_) {
    return sizeof(*n) + computeTreeSize(n->ContentHorizContainer);
  }
  if (n->NType == HorizontalContainer_) {
    return sizeof(*n) + computeTreeSize(n->ElementsList);
  }

  if (n->NType == ElementListInner_) {
    return sizeof(*n) + computeTreeSize(n->ContentElement) +
           computeTreeSize(n->NextElement);
  }

  if (n->NType == ElementListEnd_) {
    return sizeof(*n) + computeTreeSize(n->ContentElement);
  }

  if (n->NType == VerticalContainer_) {
    return sizeof(*n) + computeTreeSize(n->HorizListInVC);
  }

  if (n->NType == Image_) {
    return sizeof(*n);
  }
  if (n->NType == List_) {
    return sizeof(*n);
  }
  if (n->NType == TextBox_) {
    return sizeof(*n);
  }
}
#endif
