//===--- ToolMain.cpp -----------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "FunctionAnalyzer.h"
#include "FunctionsFinder.h"
#include "FuseTransformation.h"
#include "LLVMDependencies.h"
#include "Logger.h"
#include "RecordAnalyzer.h"

#include <assert.h>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

llvm::cl::OptionCategory TreeFuserCategory("TreeFuser options:");

static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("");

int main(int argc, const char **argv) {
  clang::tooling::CommonOptionsParser OptionsParser(argc, argv,
                                                    TreeFuserCategory);
  clang::tooling::ClangTool ClangTool(OptionsParser.getCompilations(),
                                      OptionsParser.getSourcePathList());

  std::vector<std::unique_ptr<ASTUnit>> ASTList;
  ClangTool.buildASTs(ASTList);

  if (ClangTool.run(
          newFrontendActionFactory<clang::SyntaxOnlyAction>().get())) {
    errs() << "ERROR: input source files have a compilation error";
    return 0;
  }

  RecordsAnalyzer RecordAnalyserInstance;
  FunctionsFinder FunctionsInfo;

  outs() << ("INFO: anlyzing records\n");

  for (auto &ASTUnit : ASTList)
    RecordAnalyserInstance.analyzeRecordsDeclarations(
        ASTUnit.get()->getASTContext());

  outs() << ("INFO: analyzing functions\n");

  for (auto &ASTUnit : ASTList)
    FunctionsInfo.findFunctions(ASTUnit.get()->getASTContext());

  outs() << ("INFO: running transformation\n");

  for (auto &ASTUnit : ASTList) {
    auto *Ctx = &ASTUnit.get()->getASTContext();
    FusionCandidatesFinder CandidatesFinder(Ctx, &FunctionsInfo);

    // Find candidates
    CandidatesFinder.findCandidates();

    // Perform fusion
    for (auto &Entry : CandidatesFinder.getFusionCandidates()) {
      auto *EnclosingFunctionDecl = Entry.first;
      for (auto &Candidate : Entry.second) {
        // Must be defined locally to avoid duplicate functions definitions
        FusionTransformer Transformer(Ctx, &FunctionsInfo);
        Transformer.performFusion(Candidate, true, EnclosingFunctionDecl);
        // Commit source file changes
        Transformer.overwriteChangedFiles();
      }
    }
  }
  return 1;
}
