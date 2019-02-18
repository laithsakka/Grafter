//===--- Annotations.h ---------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "LLVMDependencies.h"
#include "Logger.h"
#include <stdio.h>

bool hasFuseAnnotation(clang::FunctionDecl *Declaration) {
  return Declaration->hasAttr<clang::AnnotateAttr>() &&
         (Declaration->getAttr<clang::AnnotateAttr>()
              ->getAnnotation()
              .str()
              .compare("tf_fuse") == 0);
}

bool hasTreeAnnotation(const clang::CXXRecordDecl *Declaration) {

  return Declaration->hasAttr<clang::AnnotateAttr>() &&
         Declaration->getAttr<clang::AnnotateAttr>()
                 ->getAnnotation()
                 .str()
                 .compare("tf_tree") == 0;
}

bool hasChildAnnotation(clang::FieldDecl *Declaration) {

  return Declaration->hasAttr<clang::AnnotateAttr>() &&
         Declaration->getAttr<clang::AnnotateAttr>()
                 ->getAnnotation()
                 .str()
                 .compare("tf_child") == 0;
}

bool hasStrictAccessAnnotation(clang::Decl *Declaration) {

  return Declaration->hasAttr<clang::AnnotateAttr>() &&
         Declaration->getAttr<clang::AnnotateAttr>()
             ->getAnnotation()
             .startswith("tf_strict_access");
  ;
}

vector<StrictAccessInfo> getStrictAccessInfo(clang::Decl *Declaration) {
  StringRef Annotation =
      Declaration->getAttr<clang::AnnotateAttr>()->getAnnotation();

  std::vector<StrictAccessInfo> StrictAccessInfoVector;

  std::pair<StringRef, StringRef> SplittedPair = Annotation.split('|');
  while (true) {
    StringRef RefTmp = SplittedPair.first.split('(').second;
    StringRef Par1 = RefTmp.split(',').first;

    RefTmp = RefTmp.split(',').second;
    StringRef Par2 = RefTmp.split(',').first;
    RefTmp = RefTmp.split(',').second;
    StringRef Par3 = RefTmp.split(')').first;

    StrictAccessInfo StrictAccessInfoEntry;

    if (Par2.compare("'r'") == 0)
      StrictAccessInfoEntry.IsReadOnly = true;
    else if (Par2.compare("'w'") == 0)
      StrictAccessInfoEntry.IsReadOnly = false;
    else {
      llvm_unreachable(
          ("Invalid strict access annotation:" + Par2.str()).c_str());
    }
    StrictAccessInfoEntry.Id = stoi(Par1.str());

    if (Par3.compare("'local'") == 0) {
      StrictAccessInfoEntry.IsLocal = true;
      StrictAccessInfoEntry.IsGlobal = false;

    } else if (Par3.compare("'global'") == 0) {
      StrictAccessInfoEntry.IsGlobal = true;
      StrictAccessInfoEntry.IsLocal = false;

    } else {
      llvm_unreachable("Invalid strict access annotation");
    }

    StrictAccessInfoVector.push_back(StrictAccessInfoEntry);

    if (SplittedPair.second.str().compare("") == 0)
      break;
    else
      SplittedPair = SplittedPair.second.split('|');
  }
  return StrictAccessInfoVector;
}
