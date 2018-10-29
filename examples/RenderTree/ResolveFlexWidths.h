#include "RenderTree.h"
 __abstract_access__("(1,'w','global')") inline void LogError1() {
  printf("ERROR: leaf element cant have WMode = FLEX" );
}

 
__tree_traversal__ void Document::resolveFlexWidths() {
  COUNT
  PageList->resolveFlexWidths();
}

__tree_traversal__ void PageListEnd::resolveFlexWidths() {
  COUNT
  Content->resolveFlexWidths();
}

__tree_traversal__ void PageListInner::resolveFlexWidths() {
  COUNT
  Content->resolveFlexWidths();
  NextPage->resolveFlexWidths();
}

__tree_traversal__ void Page::resolveFlexWidths() {
  COUNT
  HorizList->resolveFlexWidths();
  if (WMode == FLEX) {
    Width = HorizList->MaxWidth; 
  }
}

__tree_traversal__ void HorizontalContainerListInner::resolveFlexWidths() {
  COUNT
  Content->resolveFlexWidths();
  Next->resolveFlexWidths();

  if (Content->Width > Next->MaxWidth)
    MaxWidth = Content->Width;
  else
    MaxWidth = Next->MaxWidth;
}

__tree_traversal__ void HorizontalContainerListEnd::resolveFlexWidths() {
  COUNT
  Content->resolveFlexWidths();
  MaxWidth = Content->Width;
}

__tree_traversal__ void HorizontalContainer::resolveFlexWidths() {
  COUNT
  ElementsList->resolveFlexWidths();
  if (WMode == FLEX) {
    Width = ElementsList->AccumulatedWidth;
   
  }
}

__tree_traversal__ void ElementListEnd::resolveFlexWidths() {
  COUNT
  Content->resolveFlexWidths();
  AccumulatedWidth = Content->Width;
}

__tree_traversal__ void ElementListInner::resolveFlexWidths() {
  COUNT
  Next->resolveFlexWidths();
  Content->resolveFlexWidths();
  AccumulatedWidth = Content->Width + Next->AccumulatedWidth;
}

__tree_traversal__ void VerticalContainer::resolveFlexWidths() {
  COUNT
  HorizList->resolveFlexWidths();
  if (WMode == FLEX) {
    Width = HorizList->MaxWidth;
  }
}

__tree_traversal__ void Element::resolveFlexWidths() {
 COUNT
  if (WMode == FLEX) {
    LogError1();
  }
}
