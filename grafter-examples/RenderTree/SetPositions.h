#include "RenderTree.h"

void Document::setPositions() { PageList->setPositions(); }

void PageListEnd::setPositions() { Content->setPositions(); }

void PageListInner::setPositions() {
  Content->setPositions();
  NextPage->setPositions();
}

void Page::setPositions() {
  PosX = 0;
  PosY = 0;
  HorizList->setPositions(PosX, PosY);
}

void HorizontalContainerListEnd::setPositions(int CurrX, int CurrY) {
  Content->setPositions(CurrX, CurrY);
}

void HorizontalContainerListInner::setPositions(int CurrX, int CurrY) {
  Content->setPositions(CurrX, CurrY);
  Next->setPositions(CurrX, CurrY + Content->Height);
}

void HorizontalContainer::setPositions(int CurrX, int CurrY) {
  PosX = CurrX;
  PosY = CurrY;
  ElementsList->setPositions(CurrX, CurrY);
}

void ElementListInner::setPositions(int CurrX, int CurrY) {
  Content->setPositions(CurrX, CurrY);
  Next->setPositions(CurrX + Content->Width, CurrY);
}

void ElementListEnd::setPositions(int CurrX, int CurrY) {
  Content->setPositions(CurrX, CurrY);
}

void Element::setPositions(int CurrX, int CurrY) {
  PosX = CurrX;
  PosY = CurrY;
}

void VerticalContainer::setPositions(int CurrX, int CurrY) {
  PosX = CurrX;
  PosY = CurrY;
  HorizList->setPositions(CurrX, CurrY);
}