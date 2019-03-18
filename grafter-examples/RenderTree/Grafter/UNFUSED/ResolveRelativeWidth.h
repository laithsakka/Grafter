#include "RenderTree.h"

__tree_traversal__ void Document::resolveRelativeWidths() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif

 PageList->resolveRelativeWidths();
}

__tree_traversal__ void PageListEnd::resolveRelativeWidths() {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif
 Content->resolveRelativeWidths();
}

__tree_traversal__ void PageListInner::resolveRelativeWidths() {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  NextPage->resolveRelativeWidths();
  Content->resolveRelativeWidths();
}

__abstract_access__("(1,'w','global')") inline void LogError2() {
  printf("ERROR: Cannot have WMode REL for page node");
}
__tree_traversal__ void Page::resolveRelativeWidths() {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  if (WMode == REL) {
    LogError2();
    return;
  }
  HorizList->resolveRelativeWidths(Width);
}

__tree_traversal__ void
HorizontalContainerListEnd::resolveRelativeWidths(int PWidth) {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  Content->resolveRelativeWidths(PWidth);
}

__tree_traversal__ void
HorizontalContainerListInner::resolveRelativeWidths(int PWidth) {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  Next->resolveRelativeWidths(PWidth);
  Content->resolveRelativeWidths(PWidth);
}

__tree_traversal__ void HorizontalContainer::resolveRelativeWidths(int PWidth) {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  if (WMode == REL) {
    Width = RelWidth * PWidth;
  }
  ElementsList->resolveRelativeWidths(Width);
}

__tree_traversal__ void ElementListEnd::resolveRelativeWidths(int PWidth) {

#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif

  Content->resolveRelativeWidths(PWidth);
}

__tree_traversal__ void ElementListInner::resolveRelativeWidths(int PWidth) {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif
  Content->resolveRelativeWidths(PWidth);
  Next->resolveRelativeWidths(PWidth);
}

__tree_traversal__ void Element::resolveRelativeWidths(int PWidth) {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  if (WMode == REL) {
    Width = RelWidth * PWidth;
  }
}

__tree_traversal__ void VerticalContainer::resolveRelativeWidths(int PWidth) {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  if (WMode == REL) {
    Width = RelWidth * PWidth;
  }
  HorizList->resolveRelativeWidths(Width);
}
