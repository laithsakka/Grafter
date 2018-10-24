#ifndef RENDER_TREE
#define RENDER_TREE
#define __tree_structure__ __attribute__((annotate("tf_tree")))
#define __tree_child__ __attribute__((annotate("tf_child")))
#define __tree_traversal__ __attribute__((annotate("tf_fuse")))

#define __abstract_access__(AccessList)                                        \
  __attribute__((annotate("tf_strict_access" #AccessList)))

//#include <assert.h>
#include <stdio.h>
#include <string>

using namespace std;

#define NA -1;


__abstract_access__((1,'w','global')) void LogError(string Str) {
  printf("ERROR: %s", &Str[0]);
}
enum MEASURE_MODE { ABS, REL, FLEX };

class Data {
public:
  int PosX = NA;
  int PosY = NA;
  int Height = NA;
  int Width = NA;
  float RelWidth;
  MEASURE_MODE WMode;
  MEASURE_MODE HMode;
  bool HasBoarder;
  int BoarderSize;
  unsigned int BackgroundColor = 0;
  unsigned int Font = 0;
  unsigned int FontColor = 0;
};

class __tree_structure__ Node {
public:
  virtual __tree_traversal__ void setFormatProperties(){};
  virtual __tree_traversal__ void setWidth(){};
  virtual __tree_traversal__ void resolveFlexWidths(){};
};

class Document;
class PageListNode;
class PageListEnd;
class PageListInner;
class Page;
class HorizontalContainerListNode;
class HorizontalContainerListEnd;
class HorizontalContainerListInner;
class HorizontalContainer;

class VerticalContainerListNode;
class VerticalContainerListInner;
class VerticalContainerListEnd;
class VerticalContainer;

class ElementListNode;
class ElementListInner;
class ElementListEnd;
class Element;
class TextBox;
class List;
class Image;

class __tree_structure__ Document : public Node {
public:
  __tree_child__ PageListNode *PageList = nullptr;
  __tree_traversal__ void resolveFlexWidths();
  __tree_traversal__ void resolveRelativeWidths();
};

//-------------- pages  ---------------
class __tree_structure__ PageListNode : public Node {
public:
  __tree_child__ Page *Content;
  virtual __tree_traversal__ void resolveRelativeWidths(){};
};

class __tree_structure__ PageListEnd : public PageListNode {
public:
  __tree_traversal__ void resolveFlexWidths();
  __tree_traversal__ void resolveRelativeWidths();
};

class __tree_structure__ PageListInner : public PageListNode {
public:
  __tree_child__ PageListNode *NextPage;
  __tree_traversal__ void resolveFlexWidths();
  __tree_traversal__ void resolveRelativeWidths();
};

class __tree_structure__ Page : public Node, public Data {
public:
  __tree_child__ HorizontalContainerListNode *HorizList;
  __tree_traversal__ void resolveFlexWidths();
  __tree_traversal__ void resolveRelativeWidths();
};

//-------------- Horizontal Containers  ---------------
class __tree_structure__ HorizontalContainerListNode : public Node {
public:
  int MaxWidth;
  __tree_child__ HorizontalContainer *Content;
  virtual __tree_traversal__ void resolveRelativeWidths(int PWidth){};
};

class __tree_structure__ HorizontalContainerListEnd
    : public HorizontalContainerListNode {
public:
  __tree_traversal__ void resolveFlexWidths();
  __tree_traversal__ void resolveRelativeWidths(int PWidth) override;
};

class __tree_structure__ HorizontalContainerListInner
    : public HorizontalContainerListNode {
public:
  __tree_child__ HorizontalContainerListNode *Next;
  __tree_traversal__ void resolveFlexWidths();
  __tree_traversal__ void resolveRelativeWidths(int PWidth) override;
};

class __tree_structure__ HorizontalContainer : public Node, public Data {
public:
  __tree_child__ ElementListNode *ElementsList;
  __tree_traversal__ void resolveFlexWidths();
  __tree_traversal__ void resolveRelativeWidths(int PWidth);
};

//-------------- Elements List ---------------
class __tree_structure__ ElementListNode : public Node {
public:
  int AccumulatedWidth = 0;
  __tree_child__ Element *Content;
  virtual __tree_traversal__ void resolveRelativeWidths(int PWidth){};
};

class __tree_structure__ ElementListEnd : public ElementListNode {
public:
  __tree_traversal__ void resolveFlexWidths();
  __tree_traversal__ void resolveRelativeWidths(int PWidth) override;
};

class __tree_structure__ ElementListInner : public ElementListNode {
public:
  __tree_child__ ElementListNode *Next;
  __tree_traversal__ void resolveFlexWidths();
  __tree_traversal__ void resolveRelativeWidths(int PWidth) override;
};

//-------------- Elements ---------------
class __tree_structure__ Element : public Node, public Data {
public:
  __tree_traversal__ void resolveFlexWidths() override;
  virtual __tree_traversal__ void resolveRelativeWidths(int PWidth) ;

};

class __tree_structure__ TextBox : public Element {
public:
};

class __tree_structure__ List : public Element {
public:
};

class __tree_structure__ Image : public Element {
public:
};

class __tree_structure__ VerticalContainer : public Element {
public:
  __tree_child__ HorizontalContainerListNode *HorizList;

  __tree_traversal__ void resolveFlexWidths() override;
  __tree_traversal__ void resolveRelativeWidths(int PWidth) override ;

};

//--------------------------------------------
#endif