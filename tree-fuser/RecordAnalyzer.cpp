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

  ASTContext *Ctx = &RecordDecl->getASTContext();

  RecordInfo *RecordInformation = new RecordInfo();
  if (RecordsAnalyzer::RecordsInfoGlobalStore[Ctx].count(RecordDecl) &&
      RecordsAnalyzer::RecordsInfoGlobalStore[Ctx][RecordDecl]) {
    Logger::getStaticLogger().logWarn(
        "RecordsAnalyzer::VisitCXXRecordDecl : record allready analysed");
    return true;
  }

  RecordsAnalyzer::RecordsInfoGlobalStore[Ctx][RecordDecl] = RecordInformation;

  if (!hasTreeAnnotation(RecordDecl)) {
    RecordInformation->IsTreeStructure = false;
    return true;
  }

  for (auto *Field : RecordDecl->fields()) {
    if (!hasChildAnnotation(Field))
      continue;

    if (!Field->getType()->isPointerType()) {
      Logger::getStaticLogger().logError(
          "child must be poniter and recursive annotation is dropped");
      Field->dropAttr<clang::AnnotateAttr>();
      abort();
    }

    if (Field->getType()->getPointeeCXXRecordDecl() != RecordDecl) {
      bool Found = false;
      for (auto &BaseClass : RecordDecl->bases()) {
        if (Field->getType()->getPointeeCXXRecordDecl() ==
            BaseClass.getType()->getAsCXXRecordDecl())
          Found = true;
      }
      if (!Found) {
        Logger::getStaticLogger().logError(
            "child must be poniter and recursive annotation is dropped");
        abort();
      }
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
