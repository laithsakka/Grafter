#ifndef RENDER_TREE
#define RENDER_TREE
#define __tree_structure__ __attribute__((annotate("tf_tree")))
#define __tree_child__ __attribute__((annotate("tf_child")))
#define __tree_traversal__ __attribute__((annotate("tf_fuse")))
#define __abstract_access__(AccessList)                                        \
  __attribute__((annotate("tf_strict_access" #AccessList)))

#ifdef COUNT_VISITS
int  _VISIT_COUNTER =0;
#endif

#include "Types.h"
#include <stdio.h>
#include <string>
#include <vector>
using namespace std;


class __tree_structure__ Node {
public:
  virtual void print(){};
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
  FontInfo FontStyle;

  __tree_child__ PageListNode *PageList = nullptr;
  __tree_traversal__ void resolveFlexWidths();
  __tree_traversal__ void resolveRelativeWidths();
  __tree_traversal__ void computeHeights();
  __tree_traversal__ void setFont();
  __tree_traversal__ void setPositions();
  int computeTreeSize() ;
  void print() override;
};

//-------------- pages  ---------------
// this is an abstract class
class __tree_structure__ PageListNode : public Node {
public:
  __tree_child__ Page *Content;
  virtual __tree_traversal__ void resolveRelativeWidths(){};
  virtual __tree_traversal__ void resolveFlexWidths(){};
  virtual __tree_traversal__ void computeHeights(){};
  virtual __tree_traversal__ void setFont(FontInfo FontStyle){};
  virtual __tree_traversal__ void setPositions(){};
  virtual int computeTreeSize() = 0;

};

class __tree_structure__ PageListEnd : public PageListNode {
public:
  __tree_traversal__ void resolveFlexWidths() override;
  __tree_traversal__ void resolveRelativeWidths() override;
  __tree_traversal__ void computeHeights() override;
  __tree_traversal__ void setFont(FontInfo FontStyle) override;
  __tree_traversal__ __tree_traversal__ void setPositions() override;
  void print() override;
  int computeTreeSize() override;
};

class __tree_structure__ PageListInner : public PageListNode {
public:
  __tree_child__ PageListNode *NextPage;
  __tree_traversal__ void resolveFlexWidths() override;
  __tree_traversal__ void resolveRelativeWidths() override;
  __tree_traversal__ void computeHeights() override;
  __tree_traversal__ void setFont(FontInfo FontStyle) override;
  __tree_traversal__ void setPositions() override;
  void print() override;
  int computeTreeSize() override;
};

class __tree_structure__ Page : public Node, public Data {
public:
  __tree_child__ HorizontalContainerListNode *HorizList;
  __tree_traversal__ void resolveRelativeWidths();
  __tree_traversal__ void resolveFlexWidths();
  __tree_traversal__ void computeHeights();
  __tree_traversal__ void setFont(FontInfo FontStyle);
  __tree_traversal__ void setPositions();
  void print() override;
  int computeTreeSize() ;
};

//-------------- Horizontal Containers  ---------------
class __tree_structure__ HorizontalContainerListNode : public Node {
public:
  int MaxWidth;
  int AggregatedHeight;
  __tree_child__ HorizontalContainer *Content;
  virtual __tree_traversal__ void resolveRelativeWidths(int PWidth){};
  virtual __tree_traversal__ void resolveFlexWidths(){};
  virtual __tree_traversal__ void computeHeights(){};
  virtual __tree_traversal__ void setFont(FontInfo FontStyle){};
  virtual __tree_traversal__ void setPositions(int CurrX, int CurrY){};
  virtual int computeTreeSize() =0;
};

class __tree_structure__ HorizontalContainerListEnd
    : public HorizontalContainerListNode {
public:
  __tree_traversal__ void resolveFlexWidths() override;
  __tree_traversal__ void resolveRelativeWidths(int PWidth) override;
  __tree_traversal__ void computeHeights() override;
  __tree_traversal__ void setFont(FontInfo FontStyle) override;
  __tree_traversal__ void setPositions(int CurrX, int CurrY) override;
  void print() override;
  int computeTreeSize() override;
};

class __tree_structure__ HorizontalContainerListInner
    : public HorizontalContainerListNode {
public:
  __tree_child__ HorizontalContainerListNode *Next;
  __tree_traversal__ void resolveFlexWidths() override;
  __tree_traversal__ void resolveRelativeWidths(int PWidth) override;
  __tree_traversal__ void computeHeights() override;
  __tree_traversal__ void setFont(FontInfo FontStyle) override;
  __tree_traversal__ void setPositions(int CurrX, int CurrY) override;
  void print() override;
  int computeTreeSize() override;
};

class __tree_structure__ HorizontalContainer : public Node, public Data {
public:
  __tree_child__ ElementListNode *ElementsList;
  __tree_traversal__ void resolveFlexWidths();
  __tree_traversal__ void resolveRelativeWidths(int PWidth);
  __tree_traversal__ void computeHeights();
  __tree_traversal__ void setFont(FontInfo FontStyle);
  __tree_traversal__ void setPositions(int CurrX, int CurrY);
  void print() override;
  int computeTreeSize() ;
};

//-------------- Elements List ---------------
class __tree_structure__ ElementListNode : public Node {
public:
  int AccumulatedWidth = 0;
  int MaxHeight = 0;
  __tree_child__ Element *Content;
  virtual __tree_traversal__ void resolveRelativeWidths(int PWidth){};
  virtual __tree_traversal__ void resolveFlexWidths(){};
  virtual __tree_traversal__ void computeHeights(){};
  virtual __tree_traversal__ void setFont(FontInfo FontStyle){};
  virtual __tree_traversal__ void setPositions(int CurrX, int CurrY){};
  virtual int computeTreeSize()= 0;
};

class __tree_structure__ ElementListEnd : public ElementListNode {
public:
  __tree_traversal__ void resolveFlexWidths() override;
  __tree_traversal__ void resolveRelativeWidths(int PWidth) override;
  __tree_traversal__ void computeHeights() override;
  __tree_traversal__ void setFont(FontInfo FontStyle) override;
  __tree_traversal__ void setPositions(int CurrX, int CurrY) override;
  void print() override;
  int computeTreeSize() override;
};

class __tree_structure__ ElementListInner : public ElementListNode {
public:
  __tree_child__ ElementListNode *Next;
  __tree_traversal__ void resolveFlexWidths() override;
  __tree_traversal__ void resolveRelativeWidths(int PWidth) override;
  __tree_traversal__ void computeHeights() override;
  __tree_traversal__ void setFont(FontInfo FontStyle) override;
  __tree_traversal__ void setPositions(int CurrX, int CurrY) override;
  void print() override;
  int computeTreeSize() override;
};

//-------------- Elements ---------------
class __tree_structure__ Element : public Node, public Data {
public:
  virtual __tree_traversal__ void resolveFlexWidths();
  virtual __tree_traversal__ void resolveRelativeWidths(int PWidth);
  virtual __tree_traversal__ void computeHeights(){};
  virtual __tree_traversal__ void setFont(FontInfo FontStyle);
  virtual __tree_traversal__ void setPositions(int CurrX, int CurrY);
  virtual int computeTreeSize() =0;

};

class __tree_structure__ TextBox : public Element {

public:
  String ContentText;
  __tree_traversal__ void computeHeights() override;
  void print() override;
    int computeTreeSize() override;

};

class __tree_structure__ List : public Element {
public:
  ListItems Items;
  int ItemMargin;
  __tree_traversal__ void computeHeights() override;
  void print() override;
  int computeTreeSize() override;

};

class __tree_structure__ Image : public Element {
public:
  string path_to_image;
  float ImageOriginalWidth;
  float ImageOriginalHeight;
  String ImageURL;
  __tree_traversal__ void computeHeights() override;
  void print() override;
  int computeTreeSize() override;

};

class __tree_structure__ VerticalContainer : public Element {
public:
  __tree_child__ HorizontalContainerListNode *HorizList;
  __tree_traversal__ void resolveFlexWidths() override;
  __tree_traversal__ void resolveRelativeWidths(int PWidth) override;
  __tree_traversal__ void computeHeights() override;
  __tree_traversal__ void setFont(FontInfo FontStyle) override;
  __tree_traversal__ void setPositions(int CurrX, int CurrY) override;
  void print() override;
  int computeTreeSize() override;

};

//--------------------------------------------
#endif
