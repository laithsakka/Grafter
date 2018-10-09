//===--- TraversalSynthesizer.h -------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#ifndef TREE_FUSER_CODE_WRITER_H
#define TREE_FUSER_CODE_WRITER_H

#include "AccessPath.h"
#include "DependenceGraph.h"
#include "FunctionAnalyzer.h"
#include "FunctionsFinder.h"
#include "LLVMDependencies.h"
#include <FuseTransformation.h>
#include <set>
#include <stdio.h>
#include <unordered_map>
#include <vector>

struct FusedTraversalWritebackInfo;
class StatmentPrinter;
class FusionTransformer;

class TraversalSynthesizer {
private:
  static std::map<clang::FunctionDecl *, int> FunDeclToNameId;
  static int Count;

  /// A counter that tracks the number of synthesized traversals
  // int FunctionCounter;

  FusionTransformer *Transformer;
  /// Maps synthesized traversals to their WriteBackInfo
  std::unordered_map<string, FusedTraversalWritebackInfo *>
      SynthesizedFunctions;

  /// ASTContext
  ASTContext *ASTContext;

  /// Clang source code rewriter for the associated AST
  clang::Rewriter &Rewriter;

  int getFunctionId(clang::FunctionDecl *) ;

  /// Return the first participating traversal
  int getFirstParticipatingTraversal(
      const std::vector<bool> &ParticipatingTraversals) const;

  /// Count the number of participating traversals
  unsigned getNumberOfParticipatingTraversals(
      const std::vector<bool> &ParticipatingTraversals) const;

  void setBlockSubPart(
      string &Decls, std::string &BlockPart,
      const std::vector<clang::FunctionDecl *> &ParticipatingTraversals,
      int BlockId,
      std::unordered_map<clang::FunctionDecl *, vector<DG_Node *>> &Statements);

  ///
  void setCallPart(
      std::string &CallPartText,
      const std::vector<clang::CallExpr *> &ParticipatingCallExpr,
      const std::vector<clang::FunctionDecl *> &ParticipatingTraversalsDecl,
      DG_Node *CallNode, FusedTraversalWritebackInfo *WriteBackInfo);

  /// Return true if a subtraversal with the given participating traversal
  /// is already synthesized
  bool
  isGenerated(const vector<clang::FunctionDecl *> &ParticipatingTraversals);

public:
  /// Creates a funciton name for a sub-traversal that traverse the
  /// participating traversals
  std::string
  createName(const std::vector<clang::CallExpr *> &ParticipatingTraversals);

  std::string
  createName(const std::vector<clang::FunctionDecl *> &ParticipatingTraversal);
  /// Return true if a subtraversal with the given participating traversal is
  /// already synthesized
  bool isGenerated(const vector<clang::CallExpr *> &ParticipatingTraversals);

  /// Generates the code of the new traversal
  void WriteUpdates(const std::vector<clang::CallExpr *> CallsExpressions,
                    const clang::FunctionDecl *EnclosingFunctionDecl);

  void generateWriteBackInfo(
      const std::vector<clang::CallExpr *> &ParticipatingTraversals,
      const std::vector<DG_Node *> &ToplogicalOrder);

  TraversalSynthesizer(clang::ASTContext *ASTContext,
                       clang::Rewriter &Rewriter_,
                       FusionTransformer *Transformer_)
      : Rewriter(Rewriter_), ASTContext(ASTContext), Transformer(Transformer_) {
  }
};

class StatmentPrinter {
private:
  /// A string where the output of the printer instance is stored in
  std::string Output;

  /// The declaration of the root node of the traversed tree
  clang::ValueDecl *RootNodeDecl;

  /// A string that represents the exit label (returns will jump to that)
  std::string NextLabel;

  int TraversalIndex;

  /// Variable used to track the depth of nested expressions
  int NestedExpressionDepth = 0;

  /// Inner call that performs actual text generation
  void print_handleStmt(const clang::Stmt *Stmt, SourceManager &SM);

public:
  /// Return a new string for the given statment that is used in the new
  /// synthesized traversal
  std::string printStmt(const clang::Stmt *Stmt, SourceManager &SM,
                        clang::ValueDecl *RootDecl, string NextLabel,
                        int TraversalIndex) {
    this->Output = "";
    this->RootNodeDecl = RootDecl;
    this->NextLabel = NextLabel;
    this->TraversalIndex = TraversalIndex;
    this->NestedExpressionDepth = 0;
    print_handleStmt(Stmt, SM);
    return Output;
  }

  /// Return the string representation for clang statement without modifying it
  std::string stmtTostr(const clang::Stmt *Stmt, const SourceManager &SM) {
    string Output = Lexer::getSourceText(
        CharSourceRange::getTokenRange(Stmt->getSourceRange()), SM,
        LangOptions(), 0);
    if (Output.at(Output.size() - 1) == ',')
      return Lexer::getSourceText(
          CharSourceRange::getCharRange(Stmt->getSourceRange()), SM,
          LangOptions(), 0);
    return Output;
  }
};

struct FusedTraversalWritebackInfo {
public:
  std::string Body;
  std::string ForwardDeclaration;
  std::string FunctionName;
  std::vector<clang::CallExpr *> ParticipatingCalls;
};

#endif
