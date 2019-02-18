#include "RenderTree.h"

// this is a pure function reads from 0

__abstract_access__("(0,'r','global')") inline int computeTextHorizSpace(
    int Length, int FontSize) {

  // we can read the info from the map or compute it
  return Length * (FontSize);
}
__abstract_access__("(0,'r','global')") inline int getCharHeight(int FontSize) {
  // we can read the info from the map or compute it
  return (FontSize * 2);
}

__tree_traversal__ void Document::computeHeights() {
  COUNT
  PageList->computeHeights();
}

__tree_traversal__ void PageListEnd::computeHeights() {
  COUNT
  Content->computeHeights();
}

__tree_traversal__ void PageListInner::computeHeights() {
  COUNT
  Content->computeHeights();
  NextPage->computeHeights();
}

__tree_traversal__ void Page::computeHeights() {
  COUNT
  HorizList->computeHeights();
  Height = HorizList->AggregatedHeight;
}

__tree_traversal__ void HorizontalContainerListEnd::computeHeights() {
  COUNT
  Content->computeHeights();
  AggregatedHeight = Content->Height;
}

__tree_traversal__ void HorizontalContainerListInner::computeHeights() {
  COUNT
  Content->computeHeights();
  Next->computeHeights();
  AggregatedHeight = 0;
  AggregatedHeight = AggregatedHeight + Content->Height;
  AggregatedHeight = AggregatedHeight + Next->AggregatedHeight;
}

__tree_traversal__ void HorizontalContainer::computeHeights() {
  COUNT
  ElementsList->computeHeights();
  Height = ElementsList->MaxHeight;
}

__tree_traversal__ void ElementListEnd::computeHeights() {
  COUNT
  Content->computeHeights();
  MaxHeight = Content->Height;
}

__tree_traversal__ void ElementListInner::computeHeights() {
  COUNT
  Content->computeHeights();
  Next->computeHeights();

  if (Content->Height >= Next->MaxHeight) {
    MaxHeight = Content->Height;
  } else {
    MaxHeight = Next->MaxHeight;
  }
}
__abstract_access__("(1,'w','global')") inline void LogErrorFontNotSet() {
  printf("Font Size Not Set");
}

__tree_traversal__ void TextBox::computeHeights() {
  COUNT
  // make sure passing a reference is workign right lol
  int CharsTotalSpace;
  if (FontStyle.Size == (0 - 1))
    LogErrorFontNotSet();

  CharsTotalSpace = computeTextHorizSpace(ContentText.Length, FontStyle.Size);
  Height = (CharsTotalSpace / Width);
  if (CharsTotalSpace % Width) {
    Height = Height + 1;
  }
  Height = Height * getCharHeight(FontStyle.Size);
}

// simply represent a pure function (reads are handled as parameters in this
// case)
__abstract_access__("(0,'r','global')") inline int computeListHeight(
    ListItems List, int FontSize, int ItemMargin, int Width) {
  int Height = 0;
  for (int i = 0; i < List.Size; i++) {
    int CharsTotalSpace = computeTextHorizSpace(List.Items[i].Length, FontSize);
    CharsTotalSpace += ItemMargin;
    Height += (CharsTotalSpace / Width);
    if (CharsTotalSpace % Width) {
      Height++;
    }
  }
  return Height;
}

__tree_traversal__ void List::computeHeights() {
  if (FontStyle.Size == (0 - 1))
    LogErrorFontNotSet();
  Height = computeListHeight(Items, FontStyle.Size, ItemMargin, Width);
}

__tree_traversal__ void Image::computeHeights() {
  // keep the same ratio of the original image
  Height = Width * (ImageOriginalHeight / ImageOriginalWidth);
}

__tree_traversal__ void VerticalContainer::computeHeights() {
  COUNT
  HorizList->computeHeights();
  Height = HorizList->AggregatedHeight;
}