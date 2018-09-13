//===--- RecordsAnalyzer.cpp ----------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "RecordAnalyzer.h"
#include "Logger.h"
#include <set>
#include <stack>
RecordsStore RecordsAnalyzer::RecordsInfoGlobalStore = RecordsStore();

const set<clang::FieldDecl *> &
RecordsAnalyzer::getChildAccessDecls(const clang::RecordDecl *RecordDecl) {
  ASTContext *Ctx = &RecordDecl->getASTContext();
  auto *RecordDeclInfo =
      RecordsAnalyzer::RecordsInfoGlobalStore[Ctx][RecordDecl];
  assert(RecordDeclInfo->IsTreeStructure);
  return RecordDeclInfo->getChildAccessDecls();
}

const RecordInfo &
RecordsAnalyzer::getRecordInfo(const clang::RecordDecl *RecordDecl) {
  ASTContext *Ctx = &RecordDecl->getASTContext();
  return *RecordsAnalyzer::RecordsInfoGlobalStore[Ctx][RecordDecl];
}

bool RecordsAnalyzer::isCompleteScaler(clang::ValueDecl *const Decl) {
  if (Decl->getType()->isBuiltinType())
    return true;

  if (!Decl->getType()->isClassType())
    return false;

  auto *Ctx = &Decl->getASTContext();

  auto *RecordDecl = Decl->getType()->getAsCXXRecordDecl();
  auto *RecordInfo = RecordsAnalyzer::RecordsInfoGlobalStore[Ctx][RecordDecl];

  if (RecordInfo && RecordInfo->IsCompleteScaler != -1)
    return RecordInfo->IsCompleteScaler;

  auto It = RecordDecl->field_begin();
  while (It != RecordDecl->field_end()) {
    if (!isCompleteScaler(*It)) {
      return RecordInfo->IsCompleteScaler = false;
    }
    It++;
  }
  return RecordInfo->IsCompleteScaler = true;
}

bool RecordsAnalyzer::isScaler(clang::ValueDecl *const Decl) {

  if (Decl->getType()->isBuiltinType())
    return true;

  else if (Decl->getType()->isClassType() ||
           Decl->getType()->isStructureType()) {
    return true;
  } else
    return false;
}

bool RecordsAnalyzer::VisitCXXRecordDecl(clang::CXXRecordDecl *RecordDecl) {

  RecordInfo *RecordInformation = new RecordInfo();

  if (!hasTreeAnnotation(RecordDecl)) {
    RecordInformation->IsTreeStructure = false;
    return true;
  }

  Logger::getStaticLogger().logInfo("analyzing " +
                                    RecordDecl->getNameAsString() + "\n");

  ASTContext *Ctx = &RecordDecl->getASTContext();
  if (RecordsAnalyzer::RecordsInfoGlobalStore[Ctx].count(RecordDecl) &&
      RecordsAnalyzer::RecordsInfoGlobalStore[Ctx][RecordDecl]) {
    Logger::getStaticLogger().logWarn(
        "RecordsAnalyzer::VisitCXXRecordDecl : record already analyzed");
    return true;
  }

  RecordsAnalyzer::RecordsInfoGlobalStore[Ctx][RecordDecl] = RecordInformation;

  for (auto *Field : RecordDecl->fields()) {
    if (!hasChildAnnotation(Field))
      continue;

    if (!Field->getType()->isPointerType()) {
      Logger::getStaticLogger().logError(
          "child must be pointer  annotation is dropped");
      Field->dropAttr<clang::AnnotateAttr>();
      abort();
    }

    // Check if the type of the field is recursive

    std::set<const clang::CXXRecordDecl *> DeclTypes;

    std::stack<const clang::CXXRecordDecl *> Stack;
    Stack.push(RecordDecl);

    while (!Stack.empty()) {
      const clang::CXXRecordDecl *TopOfStack = Stack.top();
      Stack.pop();
      DeclTypes.insert(TopOfStack);

      for (auto &BaseClass : TopOfStack->bases())
        Stack.push(BaseClass.getType()->getAsCXXRecordDecl());
    }

    bool IsRecursive = false;
    //  Stack = std::stack<clang::CXXRecordDecl *const>();
    assert(Stack.empty());

    Stack.push(Field->getType()->getPointeeCXXRecordDecl());
    while (!Stack.empty()) {
      const clang::CXXRecordDecl *TopOfStack = Stack.top();
      Stack.pop();
      if (DeclTypes.count(TopOfStack)) {
        IsRecursive = true;
        break;
      }

      for (auto &BaseClass : TopOfStack->bases())
        Stack.push(BaseClass.getType()->getAsCXXRecordDecl());
    }

    if (!IsRecursive) {
      Logger::getStaticLogger().logError(
          "child must be recursive annotation is dropped");
      abort();
    }

    RecordInformation->RecursiveDeclarations.insert(Field);
  }

  RecordInformation->IsTreeStructure = 1;
  Logger::getStaticLogger().logInfo("record " + RecordDecl->getNameAsString() +
                                    " is a tree structure");
  return true;
}

void RecordsAnalyzer::analyzeRecordsDeclarations(const ASTContext &Context) {
  TraverseDecl(Context.getTranslationUnitDecl());
}
