#include "RenderTree.h"

void Document::setFont() { PageList->setFont(FontStyle); }

void PageListEnd::setFont(FontInfo ParentFontStyle) {
  Content->setFont(ParentFontStyle);
}

void PageListInner::setFont(FontInfo ParentFontStyle) {
  Content->setFont(ParentFontStyle);
  NextPage->setFont(ParentFontStyle);
}

void Page::setFont(FontInfo ParentFontStyle) {
  if (FontStyle.Size == (0-1))
    FontStyle.Size = ParentFontStyle.Size;
  if (FontStyle.Color == (0-1))
    FontStyle.Color = ParentFontStyle.Color;
  if (FontStyle.Type == (0-1))
    FontStyle.Type = ParentFontStyle.Type;

  HorizList->setFont(FontStyle);
}

void HorizontalContainerListEnd::setFont(FontInfo ParentFontStyle) {
  Content->setFont(ParentFontStyle);
}

void HorizontalContainerListInner::setFont(FontInfo ParentFontStyle) {
  Content->setFont(ParentFontStyle);
  Next->setFont(ParentFontStyle);
}

void HorizontalContainer::setFont(FontInfo ParentFontStyle) {
  if (FontStyle.Size == (0-1))
    FontStyle.Size = ParentFontStyle.Size;
  if (FontStyle.Color == (0-1))
    FontStyle.Color = ParentFontStyle.Color;
  if (FontStyle.Type == (0-1))
    FontStyle.Type = ParentFontStyle.Type;

  ElementsList->setFont(FontStyle);
}

void ElementListEnd::setFont(FontInfo ParentFontStyle) {
  Content->setFont(ParentFontStyle);
}

void ElementListInner::setFont(FontInfo ParentFontStyle) {
  Content->setFont(ParentFontStyle);
  Next->setFont(ParentFontStyle);
}

void Element::setFont(FontInfo ParentFontStyle) {
  if (FontStyle.Size == (0-1))
    FontStyle.Size = ParentFontStyle.Size;
  if (FontStyle.Color == (0-1))
    FontStyle.Color = ParentFontStyle.Color;
  if (FontStyle.Type == (0-1))
    FontStyle.Type = ParentFontStyle.Type;
}

void VerticalContainer::setFont(FontInfo ParentFontStyle) {
  if (FontStyle.Size == (0-1))
    FontStyle.Size = ParentFontStyle.Size;
  if (FontStyle.Color == (0-1))
    FontStyle.Color = ParentFontStyle.Color;
  if (FontStyle.Type == (0-1))
    FontStyle.Type = ParentFontStyle.Type;

  HorizList->setFont(FontStyle);
}