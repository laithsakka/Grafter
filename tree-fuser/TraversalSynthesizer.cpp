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

std::map<clang::FunctionDecl *, int> TraversalSynthesizer::FunDeclToNameId =
    std::map<clang::FunctionDecl *, int>();

int TraversalSynthesizer::Count = 1;

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
    const vector<clang::CallExpr *> &ParticipatingTraversals) {
  auto NewFunctionId = createName(ParticipatingTraversals);
  return SynthesizedFunctions.count(NewFunctionId);
}

bool TraversalSynthesizer::isGenerated(
    const vector<clang::FunctionDecl *> &ParticipatingTraversals) {
  auto NewFunctionId = createName(ParticipatingTraversals);
  return SynthesizedFunctions.count(NewFunctionId);
}

std::string TraversalSynthesizer::createName(
    const std::vector<clang::CallExpr *> &ParticipatingTraversals) {

  std::vector<clang::FunctionDecl *> Temp;
  Temp.resize(ParticipatingTraversals.size());
  transform(ParticipatingTraversals.begin(), ParticipatingTraversals.end(),
            Temp.begin(), [&](clang::CallExpr *CallExpr) {
              return dyn_cast<clang::FunctionDecl>(CallExpr->getCalleeDecl())
                  ->getDefinition();
            });

  return createName(Temp);
}

std::string TraversalSynthesizer::createName(
    const std::vector<clang::FunctionDecl *> &ParticipatingTraversals) {

  std::string Output = string("_fuse_") + "_";

  for (auto *FuncDecl : ParticipatingTraversals) {
    FuncDecl = FuncDecl->getDefinition();
    if (!FunDeclToNameId.count(FuncDecl))
      FunDeclToNameId[FuncDecl] = Count++;

    Output += +"F" + std::to_string(FunDeclToNameId[FuncDecl]);
  }
  return Output;
}

int TraversalSynthesizer::getFunctionId(clang::FunctionDecl *Decl) {
  assert(FunDeclToNameId.count(Decl));

  return FunDeclToNameId[Decl];
}

void TraversalSynthesizer::setBlockSubPart(
    string &Decls, std::string &BlockPart,
    const std::vector<clang::FunctionDecl *> &ParticipatingTraversalsDecl,
    const int BlockId,
    std::unordered_map<clang::FunctionDecl *, vector<DG_Node *>> &Statements) {

  StatmentPrinter Printer;
  int TraversalIndex = -1;
  for (auto *Decl : ParticipatingTraversalsDecl) {
    TraversalIndex++;

    string NextLabel = "_label_B" + to_string(BlockId) + +"F" +
                       to_string(TraversalIndex + 1) + "_Exit";

    string BlockBody = "";
    for (DG_Node *Statement : Statements[Decl]) {
      if (Statement->getStatementInfo()->Stmt->getStmtClass() ==
          clang::Stmt::DeclStmtClass) {
        Decls += "\t" +
                 Printer.printStmt(Statement->getStatementInfo()->Stmt,
                                   ASTContext->getSourceManager(),
                                   Decl->getParamDecl(0), NextLabel,
                                   TraversalIndex) +
                 ";\n";

      } else {
        BlockBody += Printer.printStmt(
            Statement->getStatementInfo()->Stmt, ASTContext->getSourceManager(),
            Decl->getParamDecl(0), NextLabel, TraversalIndex);
      }
    }
    if (BlockBody.compare("") != 0) {
      BlockPart += "if (truncate_flags &" +
                   toBinaryString((1 << TraversalIndex)) + ") {\n";
      BlockPart += BlockBody;
      BlockPart += "}\n" + NextLabel + ":\n";
    }
  }
}

void TraversalSynthesizer::setCallPart(
    std::string &CallPartText,
    const std::vector<clang::CallExpr *> &ParticipatingCallExpr,
    const std::vector<clang::FunctionDecl *> &ParticipatingTraversalsDecl,
    DG_Node *CallNode, FusedTraversalWritebackInfo *WriteBackInfo) {

  CallPartText = "";
  StatmentPrinter Printer;

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

  string CallConditionText =
      "if ( (truncate_flags & " + toBinaryString(ConditionBitMask) + ") )";

  if (NextCallNodes.size() == 1) {
    // call the original traversals

    int CalledTraversalId = NextCallNodes[0]->getTraversalId();
    string NextCallName =
        ParticipatingTraversalsDecl[CalledTraversalId]->getNameAsString();
    CallPartText += CallConditionText + "{\n\t";

    auto FirstArgument =
        dyn_cast<clang::CallExpr>(CallNode->getStatementInfo()->Stmt)
            ->getArg(0);

    auto *RootDecl = CallNode->getStatementInfo()
                         ->getEnclosingFunction()
                         ->getFunctionDecl()
                         ->getParamDecl(0);
    CallPartText +=
        NextCallName + "(" +
        Printer.printStmt(FirstArgument, ASTContext->getSourceManager(),
                          RootDecl, "", -1);

    auto *CallExpr =
        dyn_cast<clang::CallExpr>(CallNode[0].getStatementInfo()->Stmt);

    for (int ArgIndex = 1; ArgIndex < CallExpr->getNumArgs(); ArgIndex++) {
      CallPartText +=
          ", " + Printer.printStmt(CallExpr->getArg(ArgIndex),
                                   ASTContext->getSourceManager(), RootDecl,
                                   "not used", CalledTraversalId /*not used*/);
    }
    CallPartText += ");";
    CallPartText += "\n}";
    return;
  }

  std::vector<clang::CallExpr *> NextCallCallExpression;
  for (auto *Node : NextCallNodes)
    NextCallCallExpression.push_back(
        dyn_cast<clang::CallExpr>(Node->getStatementInfo()->Stmt));

  std::string NextCallName = createName(NextCallCallExpression);

  if (!isGenerated(NextCallCallExpression))
    Transformer->performFusion(NextCallCallExpression, false, nullptr);

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

  string NextCallParamsText;
  for (auto *CallNode : NextCallNodes) {
    auto *CallExpr =
        dyn_cast<clang::CallExpr>(CallNode->getStatementInfo()->Stmt);
    for (int ArgIdx = 1; ArgIdx < CallExpr->getNumArgs(); ArgIdx++) {

      NextCallParamsText +=
          ", " + Printer.printStmt(CallExpr->getArg(ArgIdx),
                                   ASTContext->getSourceManager(),
                                   RootDecl /* not used*/, "not-used",
                                   CallNode->getTraversalId() /*not used*/);
    }
  }

  NextCallParamsText += ",truncate_flags & " + toBinaryString(ConditionBitMask);
  CallPartText += NextCallParamsText;
  CallPartText += ");";
  CallPartText += "\n}";
  return;
}

clang::QualType
getTraversedType(const std::vector<clang::CallExpr *> &ParticipatingCalls) {
  return ParticipatingCalls[0]->getArg(0)->getType();
}

void TraversalSynthesizer::generateWriteBackInfo(
    const std::vector<clang::CallExpr *> &ParticipatingCalls,
    const std::vector<DG_Node *> &ToplogicalOrder) {

  StatmentPrinter Printer;

  // generate the name of the new function
  string idName = createName(ParticipatingCalls);

  // check if already generated and assert on It
  assert(!isGenerated(ParticipatingCalls));

  // create writeback info
  FusedTraversalWritebackInfo *WriteBackInfo =
      new FusedTraversalWritebackInfo();

  SynthesizedFunctions[idName] = WriteBackInfo;

  // what is this lol!
  WriteBackInfo->ParticipatingCalls = ParticipatingCalls;
  WriteBackInfo->FunctionName = idName;

  // create forward declaration
  WriteBackInfo->ForwardDeclaration = "void " + idName + "(";

  // what is this doing lol(its adding the type of the traversed node)
  // this should be the bottom of the latic
  WriteBackInfo->ForwardDeclaration +=
      string(getTraversedType(ParticipatingCalls).getAsString()) + " _r";

  vector<clang::FunctionDecl *> TraversalsDeclarationsList;
  TraversalsDeclarationsList.resize(ParticipatingCalls.size());
  transform(ParticipatingCalls.begin(), ParticipatingCalls.end(),
            TraversalsDeclarationsList.begin(), [](clang::CallExpr *CallExpr) {
              return dyn_cast<clang::FunctionDecl>(CallExpr->getCalleeDecl())
                  ->getDefinition();
            });

  // append the arguments of each method and rename locals  by adding _fx_ only
  // participating traversals
  int Idx = -1;
  for (auto *Decl : TraversalsDeclarationsList) {
    bool First = true;
    Idx++;
    for (auto *Param : Decl->parameters()) {
      if (First) {
        First = false;
        continue;
      }

      WriteBackInfo->ForwardDeclaration +=
          "," + string(Param->getType().getAsString()) + " _f" +
          to_string(Idx) + "_" + Param->getDeclName().getAsString();
    }
  }

  WriteBackInfo->ForwardDeclaration += ",unsigned int truncate_flags)";

  // Stores all local declarations in here
  string Decls;

  unordered_map<FunctionDecl *, vector<DG_Node *>> StamentsOderedByTId;

  int CurBlockId = 0;

  for (auto *DG_Node : ToplogicalOrder) {

    if (DG_Node->getStatementInfo()->isCallStmt()) {
      CurBlockId++;
      // the block part
      WriteBackInfo->Body += "//block " + to_string(CurBlockId) + "\n";

      string blockSubPart = "";
      setBlockSubPart(Decls, blockSubPart, TraversalsDeclarationsList,
                      CurBlockId, StamentsOderedByTId);
      WriteBackInfo->Body += blockSubPart;

      //  call part
      string CallPartText = "";
      this->setCallPart(CallPartText, ParticipatingCalls,
                        TraversalsDeclarationsList, DG_Node, WriteBackInfo);
      // callect call expression (only for participating traversals)
      WriteBackInfo->Body += CallPartText;

      StamentsOderedByTId.clear();
    } else {

      StamentsOderedByTId[TraversalsDeclarationsList[DG_Node->getTraversalId()]]
          .push_back(DG_Node);
    }
  }
  CurBlockId++;
  WriteBackInfo->Body += "//block " + to_string(CurBlockId) + "\n";

  string blockSubPart = "";
  this->setBlockSubPart(Decls, blockSubPart, TraversalsDeclarationsList,
                        CurBlockId, StamentsOderedByTId);

  std::string CallPartText = "return ;\n";
  // callect call expression (only for participating traversals)
  WriteBackInfo->Body += CallPartText;
  WriteBackInfo->Body = Decls + WriteBackInfo->Body;

  string fullFun = "//****** this fused method is generated by PLCL\n " +
                   WriteBackInfo->ForwardDeclaration + "{\n" +
                   "//first level declarations\n" + "\n//Body\n" +
                   WriteBackInfo->Body + "\n}\n\n";
}

void TraversalSynthesizer::WriteUpdates(
    const std::vector<clang::CallExpr *> CallsExpressions,
    const clang::FunctionDecl *EnclosingFunctionDecl) {

  for (auto *CallExpr : CallsExpressions)
    Rewriter.InsertText(CallExpr->getExprLoc(), "//");

  // add forward declarations
  for (unordered_map<string, FusedTraversalWritebackInfo *>::iterator It =
           SynthesizedFunctions.begin();
       It != SynthesizedFunctions.end(); It++) {
    Rewriter.InsertText(EnclosingFunctionDecl->getLocStart(),
                        (It->second->ForwardDeclaration + "\n{\n" +
                         It->second->Body + "\n};\n"));
  }

  StatmentPrinter Printer;
  // 2-build the new function call and add It.

  string NewCall = createName(CallsExpressions) + "(" +
                   Printer.stmtTostr(CallsExpressions[0]->getArg(0),
                                     ASTContext->getSourceManager());

  // 3-append arguments of all methods in the same order and build defualt
  // params

  for (auto *CallExpr : CallsExpressions) {

    for (int ArgIdx = 1; ArgIdx < CallExpr->getNumArgs(); ArgIdx++) {
      NewCall += "," + Printer.stmtTostr(CallExpr->getArg(ArgIdx),
                                         ASTContext->getSourceManager());
    }
  }

  // add initial truncate flags
  unsigned int x = 0;
  for (int i = 0; i < CallsExpressions.size(); i++)
    x |= (1 << i);

  NewCall += "," + toBinaryString(x) + ");";
  Rewriter.InsertTextAfter(
      Lexer::findLocationAfterToken(
          CallsExpressions[CallsExpressions.size() - 1]->getLocEnd(),
          tok::TokenKind::semi, ASTContext->getSourceManager(),
          ASTContext->getLangOpts(), true),
      "\n\t//added by fuse transformer \n\t" + NewCall + "\n");
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
  case Stmt::CallExprClass: {
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
      Output += "_r";
    else if (!DeclRefExpr->getDecl()->isDefinedOutsideFunctionOrMethod())
      Output += "_f" + to_string(this->TraversalIndex) + "_" +
                DeclRefExpr->getDecl()->getNameAsString();
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
  case Stmt::ReturnStmtClass:
    Output += "\t truncate_flags&=" +
              toBinaryString(~(unsigned int)(1 << (TraversalIndex))) +
              "; goto " + NextLabel + " ;\n";

    break;
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
  default:
    Output += stmtTostr(Stmt, SM);
  }
  return;
}
