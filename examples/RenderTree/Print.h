#include "RenderTree.h"

void Document::print() {
  printf("Document:\n");
  this->PageList->print();
}

void PageListEnd::print() { Content->print(); }

void PageListInner::print() {
  Content->print();
  NextPage->print();
}

void Page::print() {
  printf("Page==> W:%d, H:%d ,PosX:%d, PosY:%d, FontSize:%d\n", Width, Height,
         PosX, PosY, FontStyle.Size);
  HorizList->print();
}

void HorizontalContainerListInner::print() {
  Content->print();
  Next->print();
}

void HorizontalContainerListEnd::print() { Content->print(); }

void HorizontalContainer::print() {
  printf("Horiz Container==> W:%d, H:%d ,PosX:%d, PosY:%d, FontSize:%d\n", Width,
         Height, PosX, PosY, FontStyle.Size);
  ElementsList->print();
}

void ElementListEnd::print() { Content->print(); }

void ElementListInner::print() {
  Content->print();
  Next->print();
}

void VerticalContainer::print() {
  printf("Vert Container==> W:%d, H:%d ,PosX:%d, PosY:%d, FontSize:%d\n", Width,
         Height, PosX, PosY, FontStyle.Size);
  HorizList->print();
}

void TextBox::print() {
  printf("TextBox==> W:%d, H:%d ,PosX:%d, PosY:%d, FontSize:%d\n", Width, Height,
         PosX, PosY, FontStyle.Size);
}
void Image::print() {
  printf("Image ==> W:%d, H:%d ,PosX:%d, PosY:%d, FontSize:%d\n", Width, Height,
         PosX, PosY, FontStyle.Size);
}

void List::print() {
  printf("List ==> W:%d, H:%d ,PosX:%d, PosY:%d, FontSize:%d\n", Width, Height,
         PosX, PosY, FontStyle.Size);
}
