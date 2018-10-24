#include "RenderTree.h"

void Document::resolveRelativeWidths() { PageList->resolveRelativeWidths(); }

void PageListEnd::resolveRelativeWidths() { Content->resolveRelativeWidths(); }

void PageListInner::resolveRelativeWidths() {
  NextPage->resolveRelativeWidths();
  Content->resolveRelativeWidths();
}

void Page::resolveRelativeWidths() {
  if (WMode == REL) {
    LogError("ERROR: Cannot have WMode REL for page node");
    return;
  }
  HorizList->resolveRelativeWidths(Width);
}

void HorizontalContainerListEnd::resolveRelativeWidths(int PWidth) {
  Content->resolveRelativeWidths(PWidth);
}

void HorizontalContainerListInner::resolveRelativeWidths(int PWidth) {
  Next->resolveRelativeWidths(PWidth);
  Content->resolveRelativeWidths(PWidth);
}

void HorizontalContainer::resolveRelativeWidths(int PWidth) {
  if (WMode == REL) {
    Width = RelWidth * PWidth;
  }
  ElementsList->resolveRelativeWidths(Width);
}

void ElementListEnd::resolveRelativeWidths(int PWidth) {
  Content->resolveRelativeWidths(PWidth);
}

void ElementListInner::resolveRelativeWidths(int PWidth) {
  Content->resolveRelativeWidths(PWidth);
  Next->resolveRelativeWidths(PWidth);
}

void Element::resolveRelativeWidths(int PWidth) {
  if (WMode == REL) {
    Width = RelWidth * PWidth;
  }
}

void VerticalContainer::resolveRelativeWidths(int PWidth) {
  if (WMode == REL) {
    Width = RelWidth * PWidth;
  }
  HorizList->resolveRelativeWidths(Width);
}
