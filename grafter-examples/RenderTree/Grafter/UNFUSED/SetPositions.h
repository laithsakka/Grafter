#include "RenderTree.h"

void Document::setPositions() {

#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif

  PageList->setPositions();
}

void PageListEnd::setPositions() {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  Content->setPositions();
}

void PageListInner::setPositions() {

  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  Content->setPositions();
  NextPage->setPositions();
}

void Page::setPositions() {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif

  PosX = 0;
  PosY = 0;
  HorizList->setPositions(PosX, PosY);
}

void HorizontalContainerListEnd::setPositions(int CurrX, int CurrY) {
#ifdef COUNT_VISITS
  _VISIT_COUNTER++;
#endif

  Content->setPositions(CurrX, CurrY);
}

void HorizontalContainerListInner::setPositions(int CurrX, int CurrY) {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  Content->setPositions(CurrX, CurrY);
  Next->setPositions(CurrX, CurrY + Content->Height);
}

void HorizontalContainer::setPositions(int CurrX, int CurrY) {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  PosX = CurrX;
  PosY = CurrY;
  ElementsList->setPositions(CurrX, CurrY);
}

void ElementListInner::setPositions(int CurrX, int CurrY) {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  Content->setPositions(CurrX, CurrY);
  Next->setPositions(CurrX + Content->Width, CurrY);
}

void ElementListEnd::setPositions(int CurrX, int CurrY) {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  Content->setPositions(CurrX, CurrY);
}

void Element::setPositions(int CurrX, int CurrY) {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  PosX = CurrX;
  PosY = CurrY;
}

void VerticalContainer::setPositions(int CurrX, int CurrY) {
  #ifdef COUNT_VISITS
    _VISIT_COUNTER++;
  #endif

  PosX = CurrX;
  PosY = CurrY;
  HorizList->setPositions(CurrX, CurrY);
}
