//===--- CodeWriter.cpp ---------------------------------------------------===//
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

int TraversalSynthesizer::FunctionCounter = 0;

std::string toBinaryString(unsigned Input) {
  string Output;
  while (Input != 0) {
    int Tmp = Input % 2;
    Output = to_string(Tmp) + Output;
    Input = Input / 2;
  }
  Output = "0b" + Output;
  return Output;
}

TraversalSynthesizer::TraversalSynthesizer(
    const std::vector<clang::CallExpr *> &TraversalsCallExpressions,
    const clang::FunctionDecl *EnclosingFunctionDecl,
    clang::ASTContext *ASTContext, clang::Rewriter &Rewriter_,
    std::vector<DG_Node *> &StatmentsTopologicalOrder)
    : Rewriter(Rewriter_), StatmentsTopologicalOrder(StatmentsTopologicalOrder),
      TraversalsCallExpressionsList(TraversalsCallExpressions) {

  this->EnclosingFunctionDecl = EnclosingFunctionDecl;
  this->ASTContext = ASTContext;
}

unsigned TraversalSynthesizer::getNumberOfParticipatingTraversals(
    std::vector<bool> &ParticipatingTraversals) {
  unsigned Count = 0;
  for (bool Entry : ParticipatingTraversals) {
    if (Entry)
      Count++;
  }
  return Count;
}

int TraversalSynthesizer::getFirstParticipatingTraversal(
    std::vector<bool> &ParticipatingTraversals) {
  for (int i = 0; i < ParticipatingTraversals.size(); i++) {
    if (ParticipatingTraversals[i] == true) {
      return i;
    }
  }
  return -1;
}

bool TraversalSynthesizer::isGenerated(vector<bool> &ParticipatingTraversals) {
  string NewFunctionId = createName(ParticipatingTraversals);
  return SynthesizedFunctions.count(NewFunctionId);
}

std::string
TraversalSynthesizer::createName(vector<bool> &ParticipatingTraversals) {
  string Output = "_fuse_" + to_string(FunctionId) + "_";

  for (int i = 0; i < ParticipatingTraversals.size(); i++) {
    if (ParticipatingTraversals[i] == true)
      Output += +"F" + std::to_string(i + 1);
  }
  return Output;
}

void TraversalSynthesizer::setBlockSubPart(
    string &Decls, std::string &BlockPart,
    std::vector<bool> &ParticipatingTraversals, int BlockId_,
    std::vector<vector<DG_Node *>> Statements) {

  StatmentPrinter Printer;
  for (int TraversalIndex = 0; TraversalIndex < ParticipatingTraversals.size();
       TraversalIndex++) {

    if (ParticipatingTraversals[TraversalIndex] == false)
      continue;

    string FuncId = to_string(TraversalIndex + 1);
    string BlockId = to_string(BlockId_);

    string NextLabel = "_label_B" + BlockId + "F" + FuncId + "_Exit";
    string Temp = "";

    for (DG_Node *Statement : Statements[TraversalIndex]) {
      if (Statement->getStatementInfo()->Stmt->getStmtClass() ==
          clang::Stmt::DeclStmtClass) {
        Decls +=
            "\t" +
            Printer.printStmt(
                Statement->getStatementInfo()->Stmt,
                ASTContext->getSourceManager(),
                TraversalsDeclarationsList[TraversalIndex]->getParamDecl(0),
                NextLabel, TraversalIndex + 1) +
            ";\n";

      } else {
        Temp += Printer.printStmt(
            Statement->getStatementInfo()->Stmt, ASTContext->getSourceManager(),
            TraversalsDeclarationsList[TraversalIndex]->getParamDecl(0),
            NextLabel, TraversalIndex + 1);
      }
    }
    if (Temp.compare("") != 0) {
      BlockPart += "if (truncate_flags &" +
                   toBinaryString((1 << TraversalIndex)) + ") {\n";
      BlockPart += Temp;
      BlockPart += "}\n" + NextLabel + ":\n";
    }
  }
}

void TraversalSynthesizer::setCallPart(
    std::string &CallPartText, std::vector<bool> &ParticipatingTraversals,
    DG_Node *CallNode, FusedTraversalWritebackInfo *WriteBackInfo) {

  StatmentPrinter Printer;
  std::unordered_map<int, const clang::CallExpr *> TraversalIdToCallExpr;

  std::vector<bool> ParticipatingTraversalsForNextCall;

  ParticipatingTraversalsForNextCall.resize(TraversalsDeclarationsList.size());

  fill(ParticipatingTraversalsForNextCall.begin(),
       ParticipatingTraversalsForNextCall.end(), false);

  if (CallNode->isMerged()) {

    for (auto It = CallNode->getMergeInfo()->MergedNodes.begin();
         It != CallNode->getMergeInfo()->MergedNodes.end(); It++) {

      if (ParticipatingTraversals[(*It)->getTraversalId()] == true) {
        TraversalIdToCallExpr[(*It)->getTraversalId()] =
            dyn_cast<clang::CallExpr>((*It)->getStatementInfo()->Stmt);
        ParticipatingTraversalsForNextCall[(*It)->getTraversalId()] = true;
      }
    }

  } else {
    if (ParticipatingTraversals[CallNode->getTraversalId()] == true) {
      TraversalIdToCallExpr[CallNode->getTraversalId()] =
          dyn_cast<clang::CallExpr>(CallNode->getStatementInfo()->Stmt);

      ParticipatingTraversalsForNextCall[CallNode->getTraversalId()] = true;
    }
  }
  int ActiveCallsCount =
      getNumberOfParticipatingTraversals(ParticipatingTraversalsForNextCall);

  if (ActiveCallsCount == 0) {
    CallPartText = "";
    return;
  }

  unsigned int ConditionBitMask = 0;

  string CallConditionText = "if ( (truncate_flags & ";
  for (int StatementIndex = 0;
       StatementIndex < ParticipatingTraversalsForNextCall.size();
       StatementIndex++) {
    if (ParticipatingTraversalsForNextCall[StatementIndex] == false)
      continue;

    ConditionBitMask |= (1 << StatementIndex);
  }
  CallConditionText += toBinaryString(ConditionBitMask) + ") )";
  if (ActiveCallsCount == 1) {

    int TraversalId =
        getFirstParticipatingTraversal(ParticipatingTraversalsForNextCall);
    string NextCallName =
        TraversalsDeclarationsList[TraversalId]->getNameAsString();
    CallPartText += CallConditionText + "{\n\t";
    CallPartText +=
        NextCallName + "(" + "_r->" +
        CallNode->getStatementInfo()->getCalledChild()->getNameAsString();

    auto *callExpr = (TraversalIdToCallExpr[TraversalId]);

    string NextCallParamsText;
    for (int ArgIndex = 1; ArgIndex < callExpr->getNumArgs(); ArgIndex++) {

      NextCallParamsText +=
          ", " + Printer.printStmt(
                     callExpr->getArg(ArgIndex), ASTContext->getSourceManager(),
                     TraversalsDeclarationsList[TraversalId]->getParamDecl(0),
                     "dummy", TraversalId + 1);
    }
    CallPartText += NextCallParamsText;
    CallPartText += ");";
    CallPartText += "\n}";
    return;
  }

  string NextCallName;
  bool CallingSame = false;
  if (ActiveCallsCount > 1) {
    int CurrentActivaCallCount =
        getNumberOfParticipatingTraversals(ParticipatingTraversals);

    if (/*CurrentActivaCallCount > FUSE_CAP */ true) {
      //  if( (CurrentActivaCallCount  -  ActiveCallsCount) > diff_CAP ){
      NextCallName = createName(ParticipatingTraversalsForNextCall);
      if (CurrentActivaCallCount == 2)
        assert((CurrentActivaCallCount == ActiveCallsCount) && "why!");
      if (this->isGenerated(ParticipatingTraversalsForNextCall) == false) {
        this->generateWriteBackInfo(ParticipatingTraversalsForNextCall,
                                    TraversalIdToCallExpr);
      }
    } else {
      // assert(false);
      NextCallName = WriteBackInfo->FunctionName;
      CallingSame = true;
    }
  }

  CallPartText += CallConditionText + "{\n\t";

  // Special handling for call with static cast
  auto FirstArgument =
      dyn_cast<clang::CallExpr>(CallNode->getStatementInfo()->Stmt)->getArg(0);

  auto *RootDecl = CallNode->getStatementInfo()
                       ->getEnclosingFunction()
                       ->getFunctionDecl()
                       ->getParamDecl(0);

  CallPartText +=
      NextCallName + "(" +
      Printer.printStmt(FirstArgument, ASTContext->getSourceManager(), RootDecl,
                        "", -1);

  if (CallingSame) {
    string NextCallParamsText;
    for (int StatementIndex = 0;
         StatementIndex < ParticipatingTraversals.size(); StatementIndex++) {

      if (ParticipatingTraversals[StatementIndex] == false)
        continue;

      const clang::CallExpr *callExpr =
          ParticipatingTraversalsForNextCall[StatementIndex] == true
              ? TraversalIdToCallExpr[StatementIndex]
              : WriteBackInfo->ParticipatingTraversalsAndCalls[StatementIndex];

      for (int h = 1; h < callExpr->getNumArgs(); h++) {

        NextCallParamsText +=
            ", " +
            Printer.printStmt(
                callExpr->getArg(h), ASTContext->getSourceManager(),
                TraversalsDeclarationsList[StatementIndex]->getParamDecl(0),
                "dummy", StatementIndex + 1);
      }
    }

    NextCallParamsText +=
        ",truncate_flags & " + toBinaryString(ConditionBitMask);
    CallPartText += NextCallParamsText;
    CallPartText += ");";
    CallPartText += "\n}";
    return;

  } else {
    string NextCallParamsText;
    for (int StatementIndex = 0;
         StatementIndex < ParticipatingTraversalsForNextCall.size();
         StatementIndex++) {

      if (ParticipatingTraversalsForNextCall[StatementIndex] == false)
        continue;

      auto *callExpr = (TraversalIdToCallExpr[StatementIndex]);

      for (int h = 1; h < callExpr->getNumArgs(); h++) {

        NextCallParamsText +=
            ", " +
            Printer.printStmt(
                callExpr->getArg(h), ASTContext->getSourceManager(),
                TraversalsDeclarationsList[StatementIndex]->getParamDecl(0),
                "dummy", StatementIndex + 1);
      }
    }

    NextCallParamsText +=
        ",truncate_flags & " + toBinaryString(ConditionBitMask);
    CallPartText += NextCallParamsText;
    CallPartText += ");";
    CallPartText += "\n}";
    return;
  }
}

void TraversalSynthesizer::generateWriteBackInfo(
    vector<bool> &ParticipatingTraversals,
    unordered_map<int, const clang::CallExpr *>
        ParticipatingTraversalsAndCalls_) {

  StatmentPrinter Printer;

  // generate the name of the new function
  string idName = createName(ParticipatingTraversals);

  // check if already generated and assert on It
  assert(!isGenerated(ParticipatingTraversals));

  // create writeback info
  FusedTraversalWritebackInfo *writeBackInfo = new FusedTraversalWritebackInfo;
  SynthesizedFunctions[idName] = writeBackInfo;

  writeBackInfo->ParticipatingTraversalsAndCalls =
      ParticipatingTraversalsAndCalls_;
  writeBackInfo->FunctionName = idName;

  // create forward declaration
  writeBackInfo->ForwardDeclaration = "void " + idName + "(";

  writeBackInfo->ForwardDeclaration +=
      string(this->TraversalsDeclarationsList[0]
                 ->getParamDecl(0)
                 ->getType()
                 .getAsString()) +
      " _r";

  // append the arguments of each method and rename locals  by adding _fx_ only
  // participating traversals
  for (int i = 0; i < TraversalsDeclarationsList.size(); i++) {

    if (ParticipatingTraversals[i] == false)
      continue;

    for (int j = 1; j < TraversalsDeclarationsList[i]->parameters().size();
         j++) {
      writeBackInfo->ForwardDeclaration += "," +
                                           string(TraversalsDeclarationsList[i]
                                                      ->parameters()[j]
                                                      ->getType()
                                                      .getAsString()) +
                                           " _f" + to_string(i + 1) + "_" +
                                           TraversalsDeclarationsList[i]
                                               ->parameters()[j]
                                               ->getDeclName()
                                               .getAsString();
    }
  }

  writeBackInfo->ForwardDeclaration += ",unsigned int truncate_flags)";

  // build the Body of the traversal

  // store all top level declarations here (seen across blocks , calls only
  // occure on top level (relative to method Body !!)
  string decls;

  vector<vector<DG_Node *>> stamentsOderedByTId;
  stamentsOderedByTId.resize(TraversalsDeclarationsList.size());

  int blockId_ = 0;

  for (int i = 0; i < StatmentsTopologicalOrder.size(); i++) {

    if (StatmentsTopologicalOrder[i]->getStatementInfo()->isCallStmt()) {
      blockId_++;
      // the block part
      writeBackInfo->Body += "//block " + to_string(blockId_) + "\n";

      string blockSubPart = "";
      this->setBlockSubPart(decls, blockSubPart, ParticipatingTraversals,
                            blockId_, stamentsOderedByTId);
      writeBackInfo->Body += blockSubPart;

      //  call part
      string CallPartText = "";
      this->setCallPart(CallPartText, ParticipatingTraversals,
                        StatmentsTopologicalOrder[i], writeBackInfo);
      // callect call expression (only for participating traversals)
      writeBackInfo->Body += CallPartText;

      stamentsOderedByTId.clear();
      stamentsOderedByTId.resize(TraversalsDeclarationsList.size());

    } else {
      if (ParticipatingTraversals[StatmentsTopologicalOrder[i]
                                      ->getTraversalId()] == false)
        continue;
      else {
        stamentsOderedByTId[StatmentsTopologicalOrder[i]->getTraversalId()]
            .push_back(StatmentsTopologicalOrder[i]);
      }
    }
  }

  // last block ( no read is needed)
  blockId_++;
  writeBackInfo->Body += "//block " + to_string(blockId_) + "\n";

  string blockSubPart = "";
  this->setBlockSubPart(decls, blockSubPart, ParticipatingTraversals, blockId_,
                        stamentsOderedByTId);
  writeBackInfo->Body += blockSubPart;

  string CallPartText = "";
  CallPartText += "return ;\n";
  // callect call expression (only for participating traversals)
  writeBackInfo->Body += CallPartText;
  writeBackInfo->Body = decls + writeBackInfo->Body;

  string fullFun = "//****** this fused method is generated by PLCL\n " +
                   writeBackInfo->ForwardDeclaration + "{\n" +
                   "//first level declarations\n" + decls + "\n//Body\n" +
                   writeBackInfo->Body + "\n}\n\n";
}

void TraversalSynthesizer::writeFusedVersion() {
  // generate decl list
  TraversalsDeclarationsList.clear();
  for (int i = 0; i < TraversalsCallExpressionsList.size(); i++) {
    TraversalsDeclarationsList.push_back(
        TraversalsCallExpressionsList[i]->getCalleeDecl()->getAsFunction());
  }
  // set fuse counter
  TraversalSynthesizer::FunctionCounter++;
  FunctionId = FunctionCounter;

  // 1-comment all old calls
  for (int i = 0; i < TraversalsCallExpressionsList.size(); i++) {
    Rewriter.InsertText(TraversalsCallExpressionsList[i]->getExprLoc(), "//");
  }
  // build the wholse set of new traversals

  std::vector<bool> participatingTraversalsStart;
  std::unordered_map<int, const clang::CallExpr *> TraversalIdToCallExpr;
  for (int i = 0; i < this->TraversalsCallExpressionsList.size(); i++) {
    TraversalIdToCallExpr[i] = TraversalsCallExpressionsList[i];
  }
  participatingTraversalsStart.resize(TraversalsCallExpressionsList.size());
  fill(participatingTraversalsStart.begin(), participatingTraversalsStart.end(),
       true);

  this->generateWriteBackInfo(participatingTraversalsStart,
                              TraversalIdToCallExpr);

  for (unordered_map<string, FusedTraversalWritebackInfo *>::iterator It =
           SynthesizedFunctions.begin();
       It != SynthesizedFunctions.end(); It++) {
    Rewriter.InsertText(this->EnclosingFunctionDecl->getLocStart(),
                        (It->second->ForwardDeclaration + "\n{\n" +
                         It->second->Body + "\n};\n"));
  }

  // add the new call]
  StatmentPrinter Printer;
  // 2-build the new function call and add It.
  string newCall =
      createName(participatingTraversalsStart) + "(" +
      Printer.stmtTostr(TraversalsCallExpressionsList[0]->getArg(0),
                        ASTContext->getSourceManager());

  // 3-append arguments of all methods in the same order and build defualt
  // params

  for (int i = 0; i < TraversalsCallExpressionsList.size(); i++) {

    for (int j = 1; j < TraversalsCallExpressionsList[i]->getNumArgs(); j++) {

      newCall +=
          "," + Printer.stmtTostr(TraversalsCallExpressionsList[i]->getArg(j),
                                  ASTContext->getSourceManager());
    }
  }
  // add doFunX for each traversals
  unsigned int x = 0;
  for (int i = 0; i < TraversalsCallExpressionsList.size(); i++) {
    x |= (1 << i);
  }
  newCall += "," + toBinaryString(x) + ");";
  Rewriter.InsertTextAfter(
      Lexer::findLocationAfterToken(
          TraversalsCallExpressionsList[TraversalsCallExpressionsList.size() -
                                        1]
              ->getLocEnd(),
          tok::TokenKind::semi, ASTContext->getSourceManager(),
          ASTContext->getLangOpts(), true),
      "\n\t//added by fuse transformer \n\t" + newCall + "\n");
}

void StatmentPrinter::print_handleStmt(const clang::Stmt *Stmt,
                                       SourceManager &SM) {
  Stmt = Stmt->IgnoreImplicit();
  switch (Stmt->getStmtClass()) {

  case Stmt::CompoundStmtClass: {
    for (auto *ChildStmt : Stmt->children())
      print_handleStmt(ChildStmt, SM);
    break;
  }
  case Stmt::StmtClass::CallExprClass: {
    auto *CallExpr = dyn_cast<clang::CallExpr>(Stmt);
    Output += CallExpr->getDirectCallee()->getNameAsString() + "(";

    auto *LastArgument = CallExpr->getArg(CallExpr->getNumArgs() - 1);
    for (auto *Argument : CallExpr->arguments()) {
      print_handleStmt(Argument, SM);
      if (Argument != LastArgument)
        Output += ",";
    }
    Output += ")";

    if (NestedExpressionDepth == 0)
      Output += ";";
    break;
  }
  case Stmt::StmtClass::BinaryOperatorClass: {
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
  case Stmt::StmtClass::DeclRefExprClass: {
    auto *DeclRefExpr = dyn_cast<clang::DeclRefExpr>(Stmt);
    if (DeclRefExpr->getDecl() == this->RootNodeDecl)
      Output += "_r";
    else if (!DeclRefExpr->getDecl()->isDefinedOutsideFunctionOrMethod())
      Output += "_f" + to_string(this->FunctionId) + "_" +
                DeclRefExpr->getDecl()->getNameAsString();
    else
      Output += stmtTostr(Stmt, SM);
    break;
  }
  case Stmt::StmtClass::ImplicitCastExprClass: {
    Output += "|>";
    print_handleStmt(dyn_cast<clang::ImplicitCastExpr>(Stmt)->getSubExpr(), SM);
    break;
  }
  case Stmt::StmtClass::MemberExprClass: {
    auto *MemberExpression = dyn_cast<clang::MemberExpr>(Stmt);
    print_handleStmt(*MemberExpression->child_begin(), SM);

    if (MemberExpression->isArrow())
      Output += "->" + MemberExpression->getMemberDecl()->getNameAsString();
    else
      Output += "." + MemberExpression->getMemberDecl()->getNameAsString();

    break;
  }
  case Stmt::StmtClass::IfStmtClass: {
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
  case Stmt::StmtClass::ReturnStmtClass:
    Output += "\t truncate_flags&=" +
              toBinaryString(~(unsigned int)(1 << (FunctionId - 1))) +
              "; goto " + NextLabel + " ;\n";

    break;
  case Stmt::Stmt::NullStmtClass:
    Output += "\t\t;\n";
    break;
  case Stmt::StmtClass::DeclStmtClass: {
    auto *DeclStmt = dyn_cast<clang::DeclStmt>(Stmt);
    for (auto *Decl : DeclStmt->decls()) {
      auto *VarDecl = dyn_cast<clang::VarDecl>(Decl);
      assert(VarDecl != nullptr);
      Output += "\t" + VarDecl->getType().getAsString() + " " + "_f" +
                to_string(this->FunctionId) + "_" + VarDecl->getNameAsString() +
                " ";
      if (VarDecl->getInit() != nullptr) {
        Output += "=";
        print_handleStmt(VarDecl->getInit(), SM);
        Output += ";\n";
      } else
        Output += ";\n";
    }
    break;
  }
  case Stmt::StmtClass::ParenExprClass: {
    auto *ParenExpr = dyn_cast<clang::ParenExpr>(Stmt);
    Output += "(";
    print_handleStmt(ParenExpr->getSubExpr(), SM);
    Output += ")";
  } break;
  case Stmt::StmtClass::CXXMemberCallExprClass: {
    auto *CallExpr = dyn_cast<clang::CXXMemberCallExpr>(Stmt);
    print_handleStmt(*(CallExpr->child_begin())->child_begin(), SM);
    Output += "->" +
              CallExpr->getCalleeDecl()->getAsFunction()->getNameAsString() +
              "(";

    auto *LastArgument = CallExpr->getArg(CallExpr->getNumArgs() - 1);
    for (auto *Argument : CallExpr->arguments()) {
      print_handleStmt(Argument, SM);
      if (Argument != LastArgument)
        Output += ",";
    }
    Output += ")";
    if (NestedExpressionDepth == 0)
      Output += ";";
    break;
  }
  case Stmt::StmtClass::CXXStaticCastExprClass: {
    auto *CastStmt = dyn_cast<CXXStaticCastExpr>(Stmt);
    Output +=
        "static_cast<" + CastStmt->getTypeAsWritten().getAsString() + ">(";
    print_handleStmt(CastStmt->getSubExpr(), SM);
    Output += ")";
    break;
  }
  default:
    Output += stmtTostr(Stmt, SM);
  }
  return;
}
