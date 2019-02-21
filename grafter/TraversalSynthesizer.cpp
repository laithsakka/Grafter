//===--- TraversalSynthesizer.cpp
//---------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT fo details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "TraversalSynthesizer.h"

#define FUSE_CAP 2
#define diff_CAP 4
using namespace std;

std::map<clang::FunctionDecl *, int> TraversalSynthesizer::FunDeclToNameId =
    std::map<clang::FunctionDecl *, int>();
std::map<std::vector<clang::CallExpr *>, string> TraversalSynthesizer::Stubs =
    std::map<std::vector<clang::CallExpr *>, string>();
int TraversalSynthesizer::Count = 1;

std::string toBinaryString(unsigned Input) {
  string Output;
  while (Input != 0) {
    int Tmp = Input % 2;
    Output = to_string(Tmp) + Output;
    Input = Input / 2;
  }
  if (Output == "")
    Output = "0";
  Output = "0b" + Output;
  return Output;
}

unsigned TraversalSynthesizer::getNumberOfParticipatingTraversals(
    const std::vector<bool> &ParticipatingTraversals) const {
  unsigned Count = 0;
  for (bool Entry : ParticipatingTraversals) {
    if (Entry)
      Count++;
  }
  return Count;
}

int TraversalSynthesizer::getFirstParticipatingTraversal(
    const std::vector<bool> &ParticipatingTraversals) const {
  for (int i = 0; i < ParticipatingTraversals.size(); i++) {
    if (ParticipatingTraversals[i] == true) {
      return i;
    }
  }
  return -1;
}

bool TraversalSynthesizer::isGenerated(
    const vector<clang::CallExpr *> &ParticipatingTraversals, bool HasVirtual,
    const clang::CXXRecordDecl *TraversedType) {
  auto NewFunctionId =
      createName(ParticipatingTraversals, HasVirtual, TraversedType);
  return SynthesizedFunctions.count(NewFunctionId);
}

bool TraversalSynthesizer::isGenerated(
    const vector<clang::FunctionDecl *> &ParticipatingTraversals) {
  auto NewFunctionId = createName(ParticipatingTraversals);
  return SynthesizedFunctions.count(NewFunctionId);
}

std::string TraversalSynthesizer::createName(
    const std::vector<clang::CallExpr *> &ParticipatingTraversals,
    bool HasVirtual, const clang::CXXRecordDecl *TraversedType) {

  std::vector<clang::FunctionDecl *> Temp;
  Temp.resize(ParticipatingTraversals.size());
  transform(ParticipatingTraversals.begin(), ParticipatingTraversals.end(),
            Temp.begin(), [&](clang::CallExpr *CallExpr) {
              auto *CalleeDecl =
                  dyn_cast<clang::FunctionDecl>(CallExpr->getCalleeDecl())
                      ->getDefinition();

              auto *CalleeInfo = FunctionsFinder::getFunctionInfo(CalleeDecl);

              if (CalleeInfo->isVirtual())
                return (clang::FunctionDecl *)CalleeInfo->getDeclAsCXXMethod()
                    ->getCorrespondingMethodInClass(TraversedType)
                    ->getDefinition();
              else
                return CalleeDecl;
            });

  return createName(Temp);
}

std::string TraversalSynthesizer::createName(
    const std::vector<clang::FunctionDecl *> &ParticipatingTraversals) {

  std::string Output = string("_fuse_") + "_";

  for (auto *FuncDecl : ParticipatingTraversals) {
    FuncDecl = FuncDecl->getDefinition();
    if (!FunDeclToNameId.count(FuncDecl)) {
      FunDeclToNameId[FuncDecl] = Count++;
     LLVM_DEBUG( Logger::getStaticLogger().logInfo(
          "Function:" + FuncDecl->getQualifiedNameAsString() + "==>" +
          to_string(Count - 1) + "\n"));
    }

    Output += +"F" + std::to_string(FunDeclToNameId[FuncDecl]);
  }
  return Output;
}

int TraversalSynthesizer::getFunctionId(clang::FunctionDecl *Decl) {
  assert(FunDeclToNameId.count(Decl));

  return FunDeclToNameId[Decl];
}

string StringReplace(std::string str, const std::string &from,
                     const std::string &to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return str;
  str.replace(start_pos, from.length(), to);
  return str;
}
void TraversalSynthesizer::
    setBlockSubPart(/*
string &Decls,*/ std::string &BlockPart,
                    const std::vector<clang::FunctionDecl *>
                        &ParticipatingTraversalsDecl,
                    const int BlockId,
                    std::unordered_map<int, vector<DG_Node *>> &Statements,
                    bool HasCXXCall) {
  StatementPrinter Printer;

  for (int TraversalIndex = 0;
       TraversalIndex < ParticipatingTraversalsDecl.size(); TraversalIndex++) {
    std::string Declarations = "";

    if (!Statements.count(TraversalIndex))
      continue;

    auto *Decl = ParticipatingTraversalsDecl[TraversalIndex];

    string NextLabel = "_label_B" + to_string(BlockId) + +"F" +
                       to_string(TraversalIndex) + "_Exit";

    bool DumpNextLabel = false;
    string BlockBody = "";
    for (DG_Node *Statement : Statements[TraversalIndex]) {
      // toplevel declaration statements need to be seen by the whole function
      // since we are conditionally executing the block, we need to move the
      // declarations before the if condition but keep initializations in its
      // place

      if (Statement->getStatementInfo()->Stmt->getStmtClass() ==
          clang::Stmt::DeclStmtClass) {

        auto *CurrentDecl =
            dyn_cast<clang::DeclStmt>(Statement->getStatementInfo()->Stmt);
        for (auto *D : CurrentDecl->decls()) {
          auto *VarDecl = dyn_cast<clang::VarDecl>(D);

          // add the  declaration at the top of the block body
          Declarations +=
              StringReplace(VarDecl->getType().getAsString(), "const", "") +
              " ";
          Declarations += "_f" + to_string(TraversalIndex) + "_" +
                          VarDecl->getNameAsString() + ";\n";

          if (VarDecl->hasInit()) {
            BlockBody +=
                "_f" + to_string(TraversalIndex) + "_" +
                VarDecl->getNameAsString() + "=" +
                Printer.printStmt(
                    VarDecl->getInit(), ASTCtx->getSourceManager(),
                    FunctionsFinder::getFunctionInfo(Decl)->isGlobal()
                        ? Decl->getParamDecl(0)
                        : nullptr,
                    NextLabel, TraversalIndex, HasCXXCall, HasCXXCall) +
                ";\n";
          }
        }

      } else {
        // Nullptr is passed as root decl TODO:
        BlockBody += Printer.printStmt(
            Statement->getStatementInfo()->Stmt, ASTCtx->getSourceManager(),
            FunctionsFinder::getFunctionInfo(Decl)->isGlobal()
                ? Decl->getParamDecl(0)
                : nullptr,
            NextLabel, TraversalIndex, HasCXXCall, HasCXXCall);
      }
    }
    BlockPart += Declarations;
    if (BlockBody.compare("") != 0) {
      BlockPart += "if (truncate_flags &" +
                   toBinaryString((1 << TraversalIndex)) + ") {\n";
      BlockPart += BlockBody;
      BlockPart += "}\n";
      DumpNextLabel = true;
    }
    if (DumpNextLabel)
      BlockPart += NextLabel + ":\n";
  }
}

void TraversalSynthesizer::setCallPart(
    std::string &CallPartText,
    const std::vector<clang::CallExpr *> &ParticipatingCallExpr,
    const std::vector<clang::FunctionDecl *> &ParticipatingTraversalsDecl,
    DG_Node *CallNode, FusedTraversalWritebackInfo *WriteBackInfo,
    bool HasCXXCall) {
  CallPartText = "";
  StatementPrinter Printer;

  std::vector<DG_Node *> NextCallNodes;
  if (CallNode->isMerged())
    NextCallNodes = CallNode->getMergeInfo()->getCallsOrdered();
  else
    NextCallNodes.push_back(CallNode);

  if (!NextCallNodes.size())
    return;

  // The call should be executed iff one of at least of the participating nodes
  // is active
  unsigned int ConditionBitMask = 0;

  for (DG_Node *Node : NextCallNodes)
    ConditionBitMask |=
        (1 << Node->getTraversalId() /*should return the index*/);

  string CallConditionText = "if ( (truncate_flags & " +
                             toBinaryString(ConditionBitMask) + ") )/*call*/";
  CallPartText += CallConditionText + "{\n\t";
  // Adjust truncate flags of the new called function

  string AdjustedFlagCode = "unsigned int AdjustedTruncateFlags = 0 ;\n";

  for (auto it = NextCallNodes.rbegin(); it!=NextCallNodes.rend();++it) {
    AdjustedFlagCode += "AdjustedTruncateFlags <<= 1;\n";
    AdjustedFlagCode += "AdjustedTruncateFlags |=("
                        " 0b01 & (truncate_flags >>" +
                        to_string((*it)->getTraversalId()) + "));\n";
    ;
  }

  // add an argument to enable disable this optimization
  if (NextCallNodes.size() == 1) {
    int CalledTraversalId = NextCallNodes[0]->getTraversalId();

    auto *RootDecl =
        CallNode->getStatementInfo()->getEnclosingFunction()->isGlobal()
            ? CallNode->getStatementInfo()
                  ->getEnclosingFunction()
                  ->getFunctionDecl()
                  ->getParamDecl(0)
            : nullptr;

    CallPartText += Printer.printStmt(
        NextCallNodes[0]->getStatementInfo()->Stmt, ASTCtx->getSourceManager(),
        RootDecl, "not used", CallNode->getTraversalId(),
        /*replace this*/ HasCXXCall, HasCXXCall);

    CallPartText += "\n}";
    return;
  }

  // dd adjusted flags code
  CallPartText += AdjustedFlagCode;

  std::vector<clang::CallExpr *> NexTCallExpressions;

  for (auto *Node : NextCallNodes)
    NexTCallExpressions.push_back(
        dyn_cast<clang::CallExpr>(Node->getStatementInfo()->Stmt));

  bool HasVirtual = false;
  bool HasCXXMethod = false;

  for (auto *Call : NexTCallExpressions) {
    auto *CalleeInfo = FunctionsFinder::getFunctionInfo(
        Call->getCalleeDecl()->getAsFunction()->getDefinition());
    if (CalleeInfo->isCXXMember())
      HasCXXMethod = true;
    if (CalleeInfo->isVirtual())
      HasVirtual = true;
  }

  std::string NextCallName;
  string NextCallParamsText;

  if (!HasVirtual)
    NextCallName = createName(NexTCallExpressions, false, nullptr);
  else
    NextCallName = getVirtualStub(NexTCallExpressions);

  // if (NextCallName == "__virtualStub14")
  //   assert(false);

  Transformer->performFusion(NexTCallExpressions, /*IsTopLevel*/ false,
                             nullptr);

  auto *RootDeclCallNode =
      CallNode->getStatementInfo()->getEnclosingFunction()->isGlobal()
          ? CallNode->getStatementInfo()
                ->getEnclosingFunction()
                ->getFunctionDecl()
                ->getParamDecl(0)
          : nullptr;

  // Create the call
  if (!HasVirtual) {

    CallPartText += NextCallName + "(";

    if (CallNode->getStatementInfo()->Stmt->getStmtClass() ==
        clang::Stmt::CallExprClass) {
      auto FirstArgument =
          dyn_cast<clang::CallExpr>(CallNode->getStatementInfo()->Stmt)
              ->getArg(0);
      NextCallParamsText += Printer.printStmt(
          FirstArgument, ASTCtx->getSourceManager(), RootDeclCallNode, "",
          CallNode->getTraversalId(), HasCXXCall, HasCXXCall);

    } else if (CallNode->getStatementInfo()->Stmt->getStmtClass() ==
               clang::Stmt::CXXMemberCallExprClass) {
      NextCallParamsText +=
          Printer.printStmt(CallNode->getStatementInfo()
                                ->Stmt->child_begin()
                                ->child_begin()
                                ->IgnoreImplicit(),
                            ASTCtx->getSourceManager(), RootDeclCallNode, "",
                            CallNode->getTraversalId(), HasCXXCall, HasCXXCall);
    }

  } else {
    if (CallNode->getStatementInfo()->Stmt->getStmtClass() ==
        clang::Stmt::CXXMemberCallExprClass) {
      CallPartText += Printer.printStmt(
                          CallNode->getStatementInfo()
                              ->Stmt->child_begin()
                              ->child_begin()
                              ->IgnoreImplicit(),
                          ASTCtx->getSourceManager(), RootDeclCallNode, "",
                          CallNode->getTraversalId(), HasCXXCall, HasCXXCall) +
                      "->" + NextCallName + "(";
    } else if (CallNode->getStatementInfo()->Stmt->getStmtClass() ==
               clang::Stmt::CallExprClass) {
      auto FirstArgument =
          dyn_cast<clang::CallExpr>(CallNode->getStatementInfo()->Stmt)
              ->getArg(0);

      CallPartText += NextCallName + "(";
      NextCallParamsText += Printer.printStmt(
          FirstArgument, ASTCtx->getSourceManager(), RootDeclCallNode, "",
          CallNode->getTraversalId(), HasCXXCall, HasCXXCall);
    } else {
      llvm_unreachable("unexpected");
    }
  }

  for (auto *CallNode : NextCallNodes) {
    auto *CallExpr =
        dyn_cast<clang::CallExpr>(CallNode->getStatementInfo()->Stmt);
    auto *RootDecl =
        CallNode->getStatementInfo()->getEnclosingFunction()->isGlobal()
            ? CallNode->getStatementInfo()
                  ->getEnclosingFunction()
                  ->getFunctionDecl()
                  ->getParamDecl(0)
            : nullptr;
    for (int ArgIdx =
             CallNode->getStatementInfo()->getEnclosingFunction()->isGlobal()
                 ? 1
                 : 0;
         ArgIdx < CallExpr->getNumArgs(); ArgIdx++) {
      NextCallParamsText +=
          (NextCallParamsText == "" ? "" : ", ") +
          Printer.printStmt(CallExpr->getArg(ArgIdx),
                            ASTCtx->getSourceManager(), RootDecl /* not used*/,
                            "not-used", CallNode->getTraversalId(), HasCXXCall,
                            HasCXXCall);
    }
  }

  NextCallParamsText += (NextCallParamsText == "" ? "AdjustedTruncateFlags"
                                                  : ", AdjustedTruncateFlags");

  CallPartText += NextCallParamsText;
  CallPartText += ");";
  CallPartText += "\n}";
  return;
}

const clang::CXXRecordDecl *
extractDeclTraversedType(clang::FunctionDecl *FuncDecl) {
  auto *FunctionInfo =
      FunctionsFinder::getFunctionInfo(FuncDecl->getDefinition());
  return dyn_cast<CXXRecordDecl>(FunctionInfo->getTraversedTreeTypeDecl());
}

// return th

const clang::CXXRecordDecl *getHighestCommonTraversedType(
    std::vector<clang::FunctionDecl *> &ParticipatingFunctions) {
  auto *HighestCommon = extractDeclTraversedType(ParticipatingFunctions[0]);
  for (int i = 1; i < ParticipatingFunctions.size(); i++) {
    const clang::CXXRecordDecl *Candidate =
        extractDeclTraversedType(ParticipatingFunctions[i]);
    if (Candidate == HighestCommon)
      continue;
    if (std::find(RecordsAnalyzer::DerivedRecords[HighestCommon].begin(),
                  RecordsAnalyzer::DerivedRecords[HighestCommon].end(),
                  Candidate) !=
        RecordsAnalyzer::DerivedRecords[HighestCommon].end()) {
      HighestCommon = Candidate;
    } else if (std::find(RecordsAnalyzer::DerivedRecords[Candidate].begin(),
                         RecordsAnalyzer::DerivedRecords[Candidate].end(),
                         HighestCommon) !=
               RecordsAnalyzer::DerivedRecords[Candidate].end()) {
      // do nothing
    } else {
      llvm_unreachable("not supposed to happen !");
    }
  }
  return HighestCommon;
}
void TraversalSynthesizer::generateWriteBackInfo(
    const std::vector<clang::CallExpr *> &ParticipatingCalls,
    const std::vector<DG_Node *> &ToplogicalOrder, bool HasVirtual,
    bool HasCXXCall, const CXXRecordDecl *DerivedType) {

  StatementPrinter Printer;

  // generate the name of the new function
  string idName = createName(ParticipatingCalls, HasVirtual, DerivedType);

  // check if already generated and assert on It
  assert(!isGenerated(ParticipatingCalls, HasVirtual, DerivedType));

  vector<clang::FunctionDecl *> TraversalsDeclarationsList;
  TraversalsDeclarationsList.resize(ParticipatingCalls.size());
  transform(ParticipatingCalls.begin(), ParticipatingCalls.end(),
            TraversalsDeclarationsList.begin(), [&](clang::CallExpr *CallExpr) {
              auto *CalleeDecl =
                  dyn_cast<clang::FunctionDecl>(CallExpr->getCalleeDecl())
                      ->getDefinition();

              auto *CalleeInfo = FunctionsFinder::getFunctionInfo(CalleeDecl);

              if (CalleeInfo->isVirtual())
                return (clang::FunctionDecl *)CalleeInfo->getDeclAsCXXMethod()
                    ->getCorrespondingMethodInClass(DerivedType)
                    ->getDefinition();
              else
                return CalleeDecl;
            });

  // create keback info
  FusedTraversalWritebackInfo *WriteBackInfo =
      new FusedTraversalWritebackInfo();

  SynthesizedFunctions[idName] = WriteBackInfo;

  WriteBackInfo->ParticipatingCalls = ParticipatingCalls;
  WriteBackInfo->FunctionName = idName;

  // create forward declaration
  WriteBackInfo->ForwardDeclaration = "void " + idName + "(";

  // Adding the type of the traversed node as the first argument
  // Actually this should be hmm
  WriteBackInfo->ForwardDeclaration +=
      getHighestCommonTraversedType(TraversalsDeclarationsList)
          ->getNameAsString() +
      "*" + " _r";

  // append the arguments of each method and rename locals  by adding _fx_ only
  // participating traversals
  int Idx = -1;
  for (auto *Decl : TraversalsDeclarationsList) {
    bool First = true;
    Idx++;
    for (auto *Param : Decl->parameters()) {
      if (FunctionsFinder::getFunctionInfo(Decl)->isGlobal() && First) {
        First = false;
        continue;
      }

      WriteBackInfo->ForwardDeclaration +=
          "," + string(Param->getType().getAsString()) + " _f" +
          to_string(Idx) + "_" + Param->getDeclName().getAsString();
    }
  }

  WriteBackInfo->ForwardDeclaration += ", unsigned int truncate_flags)";

  string RootCasting = "";
  if (HasCXXCall) {
    for (int i = 0; i < TraversalsDeclarationsList.size(); i++) {
      auto *Decl = TraversalsDeclarationsList[i];
      CXXRecordDecl *CastedToType = nullptr;

      if (FunctionsFinder::getFunctionInfo(Decl)->isGlobal())
        CastedToType = Decl->getParamDecl(0)->getType()->getAsCXXRecordDecl();
      else
        CastedToType = dyn_cast<clang::CXXMethodDecl>(Decl)->getParent();

      RootCasting += CastedToType->getNameAsString() + " *" + string("_r") +
                     "_f" + to_string(i) + " = " + "(" +
                     CastedToType->getNameAsString() + "*)(_r);\n";
    }
  }

  string VisitsCounting = "\n#ifdef COUNT_VISITS \n _VISIT_COUNTER++;\n #endif \n";

  WriteBackInfo->Body += VisitsCounting;
  WriteBackInfo->Body += RootCasting;

  unordered_map<int, vector<DG_Node *>> StamentsOderedByTId;

  int CurBlockId = 0;

  for (auto *DG_Node : ToplogicalOrder) {
    if (DG_Node->getStatementInfo()->isCallStmt()) {
      CurBlockId++;
      // the block part
      // WriteBackInfo->Body += "//block " + to_string(CurBlockId) + "\n";

      string blockSubPart = "";
      setBlockSubPart(/*Decls,*/ blockSubPart, TraversalsDeclarationsList,
                      CurBlockId, StamentsOderedByTId, HasCXXCall);
      WriteBackInfo->Body += blockSubPart;

      string CallPartText = "";
      this->setCallPart(CallPartText, ParticipatingCalls,
                        TraversalsDeclarationsList, DG_Node, WriteBackInfo,
                        HasCXXCall);
      // callect call expression (only for participating traversals)
      WriteBackInfo->Body += CallPartText;

      StamentsOderedByTId.clear();
    } else {
      StamentsOderedByTId[DG_Node->getTraversalId()].push_back(DG_Node);
    }
  }
  CurBlockId++;
  // WriteBackInfo->Body += "//block " + to_string(CurBlockId) + "\n";

  string blockSubPart = "";

  this->setBlockSubPart(/*Decls, */ blockSubPart, TraversalsDeclarationsList,
                        CurBlockId, StamentsOderedByTId, HasCXXCall);

  WriteBackInfo->Body += blockSubPart;

  std::string CallPartText = "return ;\n";
  // callect call expression (only for participating traversals)
  WriteBackInfo->Body += CallPartText;
  WriteBackInfo->Body = /* Decls + */ WriteBackInfo->Body;

  // string fullFun = "//****** this fused method is generated by PLCL\n " +
  //                  WriteBackInfo->ForwardDeclaration + "{\n" +
  //                  "//first level declarations\n" + "\n//Body\n" +
  //                  WriteBackInfo->Body + "\n}\n\n";
}

extern AccessPath extractVisitedChild(clang::CallExpr *Call);

void TraversalSynthesizer::WriteUpdates(
    const std::vector<clang::CallExpr *> CallsExpressions,
    clang::FunctionDecl *EnclosingFunctionDecl) {

  for (auto *CallExpr : CallsExpressions)
    Rewriter.InsertText(CallExpr->getBeginLoc(), "//");

  // add forward declarations
  for (auto &SynthesizedFunction : SynthesizedFunctions) {
    Rewriter.InsertText(
        EnclosingFunctionDecl->getTypeSourceInfo()->getTypeLoc().getBeginLoc(),
        (SynthesizedFunction.second->ForwardDeclaration) + string(";\n"));
  }

  for (auto &SynthesizedFunction : SynthesizedFunctions) {
    Rewriter.InsertText(
        EnclosingFunctionDecl->getTypeSourceInfo()->getTypeLoc().getBeginLoc(),
        (SynthesizedFunction.second->ForwardDeclaration + "\n{\n" +
         SynthesizedFunction.second->Body + "\n};\n"));
  }

  StatementPrinter Printer;
  // 2-build the new function call and add It.
  bool IsMemberCall = CallsExpressions[0]->getStmtClass() ==
                      clang::Stmt::CXXMemberCallExprClass;

  bool HasVirtual = false;
  bool HasCXXMethod = false;

  for (auto *Call : CallsExpressions) {
    auto *CalleeInfo = FunctionsFinder::getFunctionInfo(
        Call->getCalleeDecl()->getAsFunction()->getDefinition());
    if (CalleeInfo->isCXXMember())
      HasCXXMethod = true;
    if (CalleeInfo->isVirtual())
      HasVirtual = true;
  }
  std::string NextCallName;
  if (!HasVirtual)
    NextCallName = createName(CallsExpressions, false, nullptr);
  else
    NextCallName = getVirtualStub(CallsExpressions);

  string NewCall = "";

  string Params = "";

  if (!HasVirtual) {
    NewCall += NextCallName + "(";

    if (CallsExpressions[0]->getStmtClass() == clang::Stmt::CallExprClass) {
      auto FirstArgument =
          dyn_cast<clang::CallExpr>(CallsExpressions[0])->getArg(0);
      Params += Printer.printStmt(FirstArgument, ASTCtx->getSourceManager(),
                                  nullptr, "", -1);
    } else if (CallsExpressions[0]->getStmtClass() ==
               clang::Stmt::CXXMemberCallExprClass) {
      Params += Printer.printStmt(
          CallsExpressions[0]->child_begin()->child_begin()->IgnoreImplicit(),
          ASTCtx->getSourceManager(), nullptr, "", -1);
    }

  } else {
    if (CallsExpressions[0]->getStmtClass() ==
        clang::Stmt::CXXMemberCallExprClass) {
      NewCall += Printer.printStmt(CallsExpressions[0]
                                       ->child_begin()
                                       ->child_begin()
                                       ->IgnoreImplicit(),
                                   ASTCtx->getSourceManager(), nullptr, "", -1,
                                   false) +
                 "->" + NextCallName + "(";
    } else if (CallsExpressions[0]->getStmtClass() ==
               clang::Stmt::CallExprClass) {

      auto FirstArgument =
          dyn_cast<clang::CallExpr>(CallsExpressions[0])->getArg(0);

      NewCall += NextCallName + "(";
      Params += Printer.printStmt(FirstArgument, ASTCtx->getSourceManager(),
                                  nullptr, "", -1);
    } else {
      llvm_unreachable("unexpected");
    }
  }

  // 3-append arguments of all methods in the same oƒrder and build default
  // params
  // hack
  for (auto *CallExpr : CallsExpressions) {

    for (int ArgIdx =
             CallExpr->getCalleeDecl()->getAsFunction()->isGlobal() ? 1 : 0;
         ArgIdx < CallExpr->getNumArgs(); ArgIdx++) {
      Params += ((Params.size() == 0) ? "" : ", ") +
                Printer.stmtTostr(CallExpr->getArg(ArgIdx),
                                  ASTCtx->getSourceManager());
    }
  }

  // add initial truncate flags
  unsigned int x = 0;
  for (int i = 0; i < CallsExpressions.size(); i++)
    x |= (1 << i);

  Params += ((Params.size() == 0) ? "" : ", ") + toBinaryString(x) + ");";
  NewCall += Params;
  Rewriter.InsertTextAfter(
      Lexer::findLocationAfterToken(
          CallsExpressions[CallsExpressions.size() - 1]->getLocEnd(),
          tok::TokenKind::semi, ASTCtx->getSourceManager(),
          ASTCtx->getLangOpts(), true),
      "\n\t//added by fuse transformer \n\t" + NewCall + "\n");

  // add virtual stubs
  for (auto &Entry : Stubs) {
    auto &Calls = Entry.first;
    auto &StubName = Entry.second;

    AccessPath AP = extractVisitedChild(Calls[0]);

    auto *CalledChildType = AP.getDeclAtIndex(AP.SplittedAccessPath.size() - 1)
                                ->getType()
                                ->getPointeeCXXRecordDecl();

    vector<clang::FunctionDecl *> TraversalsDeclarationsList;
    TraversalsDeclarationsList.resize(Calls.size());
    transform(Calls.begin(), Calls.end(), TraversalsDeclarationsList.begin(),
              [&](clang::CallExpr *CallExpr) {
                auto *CalleeDecl =
                    dyn_cast<clang::FunctionDecl>(CallExpr->getCalleeDecl())
                        ->getDefinition();

                auto *CalleeInfo = FunctionsFinder::getFunctionInfo(CalleeDecl);

                if (CalleeInfo->isVirtual())
                  return (clang::FunctionDecl *)CalleeInfo->getDeclAsCXXMethod()
                      ->getDefinition();
                else
                  return CalleeDecl;
              });

    // append the arguments of each method and rename locals  by adding _fx_
    // only participating traversals
    string Params = "";
    string Args = "this";


    int Idx = -1;
    for (auto *Decl : TraversalsDeclarationsList) {
      bool First = true;
      Idx++;
      for (auto *Param : Decl->parameters()) {
        if (FunctionsFinder::getFunctionInfo(Decl)->isGlobal() && First) {
          First = false;
          continue;
        }

        Params += (Params == "" ? "" : ", ") +
                  string(Param->getType().getAsString()) + " _f" +
                  to_string(Idx) + "_" + Param->getDeclName().getAsString();
        Args += (Args == "" ? "" : ", ") + string(" _f") + to_string(Idx) +
                "_" + Param->getDeclName().getAsString();
      }
    }

    Params +=
        (Params == "" ? "" : ", ") + string("unsigned int truncate_flags");
    Args += ", truncate_flags" ;
    auto LambdaFun = [&](const CXXRecordDecl *DerivedType) {
      assert(Rewriter::isRewritable(DerivedType->getLocEnd()));
      Rewriter.InsertText(
          DerivedType->getDefinition()->getLocEnd(),
          (DerivedType == CalledChildType ? "virtual" : "") + string(" void ") +
              StubName + "(" + Params + ")" +
              (DerivedType == CalledChildType ? "" : "override") + ";\n");

      Rewriter.InsertTextAfter(EnclosingFunctionDecl->getAsFunction()
                                   ->getDefinition()
                                   ->getTypeSourceInfo()
                                   ->getTypeLoc()
                                   .getBeginLoc(),
                               "void " + DerivedType->getNameAsString() +
                                   "::" + StubName + "(" + Params + "){" +
                                   createName(Calls, true, DerivedType) + "(" +
                                   Args +
                                   ");"
                                   "}\n");

      return;
    };
    LambdaFun(CalledChildType);
    for (auto *DerivedType : RecordsAnalyzer::DerivedRecords[CalledChildType]) {
      LambdaFun(DerivedType);
    }
  }
}

void StatementPrinter::print_handleStmt(const clang::Stmt *Stmt,
                                        SourceManager &SM) {
  Stmt = Stmt->IgnoreImplicit();
  switch (Stmt->getStmtClass()) {

  case Stmt::CompoundStmtClass: {
    for (auto *ChildStmt : Stmt->children())
      print_handleStmt(ChildStmt, SM);
    break;
  }
  case Stmt::CallExprClass: {
    auto *CallExpr = dyn_cast<clang::CallExpr>(Stmt);
    Output += CallExpr->getDirectCallee()->getNameAsString() + "(";

    if (CallExpr->getNumArgs()) {
      auto *LastArgument = CallExpr->getArg(CallExpr->getNumArgs() - 1);
      for (auto *Argument : CallExpr->arguments()) {
        print_handleStmt(Argument, SM);
        if (Argument != LastArgument)
          Output += ",";
      }
    }
    Output += ")";

    if (NestedExpressionDepth == 0)
      Output += ";";
    break;
  }
  case Stmt::BinaryOperatorClass: {
    // print the lhs ,
    NestedExpressionDepth++;
    auto *BinaryOperator = dyn_cast<clang::BinaryOperator>(Stmt);

    if (BinaryOperator->isAssignmentOp())
      Output += "\t";
    print_handleStmt(BinaryOperator->getLHS(), SM);

    // print op
    Output += BinaryOperator->getOpcodeStr();

    // print lhs
    print_handleStmt(BinaryOperator->getRHS(), SM);
    if (BinaryOperator->isAssignmentOp())
      Output += ";\n";

    NestedExpressionDepth--;
    break;
  }
  case Stmt::DeclRefExprClass: {
    auto *DeclRefExpr = dyn_cast<clang::DeclRefExpr>(Stmt);
    if (DeclRefExpr->getDecl() == this->RootNodeDecl)
      Output +=
          RootCasedPerTraversals ? "_r_f" + to_string(TraversalIndex) : "_r";
    else if (!DeclRefExpr->getDecl()->isDefinedOutsideFunctionOrMethod())
      // only change prefix local decl with _f(TID)_ if TID>= 0
      Output += TraversalIndex >= 0
                    ? "_f" + to_string(this->TraversalIndex) + "_" +
                          DeclRefExpr->getDecl()->getNameAsString()
                    : DeclRefExpr->getDecl()->getNameAsString();
    else
      Output += stmtTostr(Stmt, SM);
    break;
  }
  case Stmt::ImplicitCastExprClass: {
    Output += "|>";
    print_handleStmt(dyn_cast<clang::ImplicitCastExpr>(Stmt)->getSubExpr(), SM);
    break;
  }
  case Stmt::MemberExprClass: {
    auto *MemberExpression = dyn_cast<clang::MemberExpr>(Stmt);
    print_handleStmt(*MemberExpression->child_begin(), SM);

    if (MemberExpression->isArrow())
      Output += "->" + MemberExpression->getMemberDecl()->getNameAsString();
    else
      Output += "." + MemberExpression->getMemberDecl()->getNameAsString();

    break;
  }
  case Stmt::IfStmtClass: {
    auto &IfStmt = *dyn_cast<clang::IfStmt>(Stmt);
    // check the condition part first
    if (IfStmt.getCond() != nullptr) {
      NestedExpressionDepth++;
      Output += "\t if (";
      auto *IfStmtCondition = IfStmt.getCond()->IgnoreImplicit();
      print_handleStmt(IfStmtCondition, SM);
      Output += ")";
      NestedExpressionDepth--;
    }

    if (IfStmt.getThen() != nullptr) {
      auto *IfStmtThenPart = IfStmt.getThen()->IgnoreImplicit();
      Output += "{\n";
      print_handleStmt(IfStmtThenPart, SM);
      Output += "\t}";
    } else {
      Output += "{}\n";
    }

    if (IfStmt.getElse() != nullptr) {
      auto *IfStmtElsePart = IfStmt.getElse()->IgnoreImplicit();
      Output += "else {\n\t";
      print_handleStmt(IfStmtElsePart, SM);
      Output += "\n\t}";
    } else {
      Output += "\n";
    }
    break;
  }
  case Stmt::ReturnStmtClass: {
    unsigned int x = 0;
    for (int i = 0; i < (TraversalsCount + 1); i++)
      x |= (1 << i);
    Output += "\t truncate_flags&=" +
              toBinaryString(x & (~(unsigned int)(1 << (TraversalIndex)))) +
              "; goto " + NextLabel + " ;\n";

    break;
  }
  case Stmt::Stmt::NullStmtClass:
    Output += "\t\t;\n";
    break;
  case Stmt::DeclStmtClass: {
    auto *DeclStmt = dyn_cast<clang::DeclStmt>(Stmt);
    for (auto *Decl : DeclStmt->decls()) {
      auto *VarDecl = dyn_cast<clang::VarDecl>(Decl);
      assert(VarDecl != nullptr);
      Output += "\t" + VarDecl->getType().getAsString() + " " + "_f" +
                to_string(this->TraversalIndex) + "_" +
                VarDecl->getNameAsString() + " ";
      if (VarDecl->getInit() != nullptr) {
        Output += "=";
        print_handleStmt(VarDecl->getInit(), SM);
        Output += ";\n";
      } else
        Output += ";\n";
    }
    break;
  }
  case Stmt::ParenExprClass: {
    auto *ParenExpr = dyn_cast<clang::ParenExpr>(Stmt);
    Output += "(";
    print_handleStmt(ParenExpr->getSubExpr(), SM);
    Output += ")";
  } break;
  case Stmt::CXXMemberCallExprClass: {
    auto *CallExpr = dyn_cast<clang::CXXMemberCallExpr>(Stmt);
    print_handleStmt(*(CallExpr->child_begin())->child_begin(), SM);
    Output += "->" +
              CallExpr->getCalleeDecl()->getAsFunction()->getNameAsString() +
              "(";
    if (CallExpr->getNumArgs()) {
      auto *LastArgument = CallExpr->getArg(CallExpr->getNumArgs() - 1);
      for (auto *Argument : CallExpr->arguments()) {
        print_handleStmt(Argument, SM);
        if (Argument != LastArgument)
          Output += ",";
      }
    }
    Output += ")";
    if (NestedExpressionDepth == 0)
      Output += ";";
    break;
  }
  case Stmt::CXXStaticCastExprClass: {
    auto *CastStmt = dyn_cast<CXXStaticCastExpr>(Stmt);
    Output +=
        "static_cast<" + CastStmt->getTypeAsWritten().getAsString() + ">(";
    print_handleStmt(CastStmt->getSubExpr(), SM);
    Output += ")";
    break;
  }
  case Stmt::CXXDeleteExprClass: {
    auto *DeleteArgument = dyn_cast<CXXDeleteExpr>(Stmt)->getArgument();
    Output += "delete ";
    print_handleStmt(DeleteArgument, SM);
    Output += ";";
    break;
  }
  case Stmt::CXXThisExprClass: {

    if (!ReplaceThis)
      Output += "this";
    else
      Output +=
          RootCasedPerTraversals ? "_r_f" + to_string(TraversalIndex) : "_r";
    break;
  }
  case Stmt::UnaryOperatorClass: {
    auto *UnaryOpExp = dyn_cast<clang::UnaryOperator>(Stmt);
    Output += UnaryOpExp->getOpcodeStr(UnaryOpExp->getOpcode()).str();
    print_handleStmt(UnaryOpExp->getSubExpr(), SM);
    break;
  }
  case Stmt::IntegerLiteralClass: {
    auto *IntegerLit = dyn_cast<clang::IntegerLiteral>(Stmt);
    Output += IntegerLit->getValue().toString(10, true);
    break;
  }
  case Stmt::CXXConstructExprClass: {
    auto *ConstructExpr = dyn_cast<clang::CXXConstructExpr>(Stmt);
    if (ConstructExpr->getNumArgs())
      print_handleStmt(ConstructExpr->getArg(0), SM);
    break;
  }
  default:
   LLVM_DEBUG( Stmt->dump());
    Output += stmtTostr(Stmt, SM);
  }
  return;
}
