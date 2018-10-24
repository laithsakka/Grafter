#include "RenderTree.h"

__tree_traversal__ void Document::resolveFlexWidths() {
  PageList->resolveFlexWidths();
}

__tree_traversal__ void PageListEnd::resolveFlexWidths() {
  Content->resolveFlexWidths();
}

__tree_traversal__ void PageListInner::resolveFlexWidths() {
  Content->resolveFlexWidths();
  NextPage->resolveFlexWidths();
}

__tree_traversal__ void Page::resolveFlexWidths() {
  HorizList->resolveFlexWidths();
  if (WMode == FLEX) {
    Width = HorizList->MaxWidth;
    
  }
}

__tree_traversal__ void HorizontalContainerListInner::resolveFlexWidths() {

  Content->resolveFlexWidths();
  Next->resolveFlexWidths();

  if (Content->Width > Next->MaxWidth)
    MaxWidth = Content->Width;
  else
    MaxWidth = Next->MaxWidth;
}

__tree_traversal__ void HorizontalContainerListEnd::resolveFlexWidths() {
  Content->resolveFlexWidths();
  MaxWidth = Content->Width;
}

__tree_traversal__ void HorizontalContainer::resolveFlexWidths() {
  ElementsList->resolveFlexWidths();
  if (WMode == FLEX) {
    Width = ElementsList->AccumulatedWidth;
   
  }
}

__tree_traversal__ void ElementListEnd::resolveFlexWidths() {
  Content->resolveFlexWidths();
  AccumulatedWidth = Content->Width;
}

__tree_traversal__ void ElementListInner::resolveFlexWidths() {
  Next->resolveFlexWidths();
  Content->resolveFlexWidths();
  AccumulatedWidth = Content->Width + Next->AccumulatedWidth;
}

__tree_traversal__ void VerticalContainer::resolveFlexWidths() {
  HorizList->resolveFlexWidths();
  if (WMode == FLEX) {
    Width = HorizList->MaxWidth;
  }
}

__tree_traversal__ void Element::resolveFlexWidths() {
  if (WMode == FLEX) {
    LogError("ERROR: Flex not for leaf element");
  }
}
