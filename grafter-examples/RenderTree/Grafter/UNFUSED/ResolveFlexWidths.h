#include "RenderTree.h"
 __abstract_access__("(1,'w','global')") inline void LogError1() {
  printf("ERROR: leaf element cant have WMode = FLEX" );
}


__tree_traversal__ void Document::resolveFlexWidths() {

#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif

  PageList->resolveFlexWidths();
}

__tree_traversal__ void PageListEnd::resolveFlexWidths() {

#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif

  Content->resolveFlexWidths();
}

__tree_traversal__ void PageListInner::resolveFlexWidths() {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  Content->resolveFlexWidths();
  NextPage->resolveFlexWidths();
}

__tree_traversal__ void Page::resolveFlexWidths() {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  HorizList->resolveFlexWidths();
  if (WMode == FLEX) {
    Width = HorizList->MaxWidth;
  }
}

__tree_traversal__ void HorizontalContainerListInner::resolveFlexWidths() {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  Content->resolveFlexWidths();
  Next->resolveFlexWidths();

  if (Content->Width > Next->MaxWidth)
    MaxWidth = Content->Width;
  else
    MaxWidth = Next->MaxWidth;
}

__tree_traversal__ void HorizontalContainerListEnd::resolveFlexWidths() {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  Content->resolveFlexWidths();
  MaxWidth = Content->Width;
}

__tree_traversal__ void HorizontalContainer::resolveFlexWidths() {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  ElementsList->resolveFlexWidths();
  if (WMode == FLEX) {
    Width = ElementsList->AccumulatedWidth;
  }
}

__tree_traversal__ void ElementListEnd::resolveFlexWidths() {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  Content->resolveFlexWidths();
  AccumulatedWidth = Content->Width;
}

__tree_traversal__ void ElementListInner::resolveFlexWidths() {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  Next->resolveFlexWidths();
  Content->resolveFlexWidths();
  AccumulatedWidth = Content->Width + Next->AccumulatedWidth;
}

__tree_traversal__ void VerticalContainer::resolveFlexWidths() {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  HorizList->resolveFlexWidths();
  if (WMode == FLEX) {
    Width = HorizList->MaxWidth;
  }
}

__tree_traversal__ void Element::resolveFlexWidths() {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  if (WMode == FLEX) {
    LogError1();
  }
}
