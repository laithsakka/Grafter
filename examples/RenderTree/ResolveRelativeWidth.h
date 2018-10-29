#include "RenderTree.h"

__tree_traversal__ void Document::resolveRelativeWidths() {
  COUNT PageList->resolveRelativeWidths();
}

__tree_traversal__ void PageListEnd::resolveRelativeWidths() {
  COUNT Content->resolveRelativeWidths();
}

__tree_traversal__ void PageListInner::resolveRelativeWidths() {
  COUNT
  NextPage->resolveRelativeWidths();
  Content->resolveRelativeWidths();
}

__abstract_access__("(1,'w','global')") inline void LogError2() {
  printf("ERROR: Cannot have WMode REL for page node");
}
__tree_traversal__ void Page::resolveRelativeWidths() {
  COUNT
  if (WMode == REL) {
    LogError2();
    return;
  }
  HorizList->resolveRelativeWidths(Width);
}

__tree_traversal__ void
HorizontalContainerListEnd::resolveRelativeWidths(int PWidth) {
  COUNT
  Content->resolveRelativeWidths(PWidth);
}

__tree_traversal__ void
HorizontalContainerListInner::resolveRelativeWidths(int PWidth) {
  COUNT
  Next->resolveRelativeWidths(PWidth);
  Content->resolveRelativeWidths(PWidth);
}

__tree_traversal__ void HorizontalContainer::resolveRelativeWidths(int PWidth) {
  COUNT
  if (WMode == REL) {
    Width = RelWidth * PWidth;
  }
  ElementsList->resolveRelativeWidths(Width);
}

__tree_traversal__ void ElementListEnd::resolveRelativeWidths(int PWidth) {
  COUNT;
  Content->resolveRelativeWidths(PWidth);
}

__tree_traversal__ void ElementListInner::resolveRelativeWidths(int PWidth) {
  COUNT
  Content->resolveRelativeWidths(PWidth);
  Next->resolveRelativeWidths(PWidth);
}

__tree_traversal__ void Element::resolveRelativeWidths(int PWidth) {
  COUNT
  if (WMode == REL) {
    Width = RelWidth * PWidth;
  }
}

__tree_traversal__ void VerticalContainer::resolveRelativeWidths(int PWidth) {
  COUNT
  if (WMode == REL) {
    Width = RelWidth * PWidth;
  }
  HorizList->resolveRelativeWidths(Width);
}
