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

#include <set>
#include <stdio.h>
#include <unordered_map>
#include <vector>

struct FusedTraversalWritebackInfo;
class StatmentPrinter;

class TraversalSynthesizer {
private:
  /// A counter that tracks the number of synthesized traversals
  static int FunctionCounter;

  /// A unique integer assignd to the synthesized traversal
  int FunctionId;

  /// Declaration of the function where the traversal is called
  const clang::FunctionDecl *EnclosingFunctionDecl;

  /// ASTContext
  ASTContext *ASTContext;

  /// Clang source code rewriter for the associated AST
  clang::Rewriter &Rewriter;

  /// Toplogical order for all the statementa that need to be synthesized
  std::vector<DG_Node *> &StatmentsTopologicalOrder;

  /// List of the original function calls of the fused traversals
  const vector<clang::CallExpr *> &TraversalsCallExpressionsList;

  /// List of the declarations of the original traversals before fusion
  std::vector<clang::FunctionDecl *> TraversalsDeclarationsList;

  /// Creates a funciton name for a sub-traversal that traverse the
  /// participating traversals
  std::string createName(std::vector<bool> &ParticipatingTraversals);

  /// Maps synthesized sub traversals to their WriteBackInfo
  std::unordered_map<string, FusedTraversalWritebackInfo *>
      SynthesizedFunctions;

  /// Return the first participating traversal
  int getFirstParticipatingTraversal(std::vector<bool> &ParticipatingTraversals);

  /// Count the number of participating traversals
  unsigned getNumberOfParticipatingTraversals(
      std::vector<bool> &ParticipatingTraversals);

  void generateWriteBackInfo(std::vector<bool> &ParticipatingTraversals,
                             std::unordered_map<int, const clang::CallExpr *>
                                 ParticipatingTraversalsAndCalls);

  void setBlockSubPart(string &Decls, std::string &BlockPart,
                       std::vector<bool> &ParticipatingTraversals, int BlockId,
                       std::vector<vector<DG_Node *>> Statements);

  void setCallPart(std::string &CallPart,
                   std::vector<bool> &ParticipatingTraversals,
                   DG_Node *CallNode,
                   FusedTraversalWritebackInfo *WriteBackInfo);

  /// Return true if a subtraversal with the given participating traversal is
  /// already synthesized
  bool isGenerated(vector<bool> &ParticipatingTraversals);

public:
  /// Generates the code of the new traversal
  void writeFusedVersion();

  TraversalSynthesizer(
      const vector<clang::CallExpr *> &TraversalsCallExpressions,
      const clang::FunctionDecl *EnclosingFunDeclaration,
      clang::ASTContext *ASTContext, clang::Rewriter &Rewriter_,
      vector<DG_Node *> &StatmentsTopologicalOrder);

};

class StatmentPrinter {
private:
  /// A string where the output of the printer instance is stored in
  std::string Output;

  /// The declaration of the root node of the traversed tree
  clang::ValueDecl *RootNodeDecl;

  /// A string that represents the exit label (returns will jump to that)
  std::string NextLabel;

  /// The function id of the synthesized traversal
  int FunctionId;

  /// Variable used to track the depth of nested expressions
  int NestedExpressionDepth = 0;

  /// Inner call that performs actual text generation
  void print_handleStmt(const clang::Stmt *Stmt, SourceManager &SM);

public:
  /// Return a new string for the given statment that is used in the new
  /// synthesized traversal
  std::string printStmt(const clang::Stmt *Stmt, SourceManager &SM,
                        clang::ValueDecl *RootDecl, string NextLabel,
                        int FunctionId) {
    this->Output = "";
    this->RootNodeDecl = RootDecl;
    this->NextLabel = NextLabel;
    this->FunctionId = FunctionId;
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
  std::unordered_map<int, const clang::CallExpr *>
      ParticipatingTraversalsAndCalls;
};

#endif
