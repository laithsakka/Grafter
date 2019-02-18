//===--- FiniteStateMachine.h ---------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This file includes functions that act as an interface for  openFST an open
// source library for automatas
// (http://www.openfst.org/twiki/bin/view/FST/WebHome), creating automatas that
// represent accesses should be done through this class.
//===----------------------------------------------------------------------===//

#ifndef TREE_FUSER_FINITE_STATE_MACHINE
#define TREE_FUSER_FINITE_STATE_MACHINE

#include <LLVMDependencies.h>
#include <fst/fstlib.h>
#include <unordered_map>

typedef fst::StdVectorFst FSM;

class FSMUtility {

private:
  /// A counter that tracks the number unique symbols(labels) and used to assign
  /// labels to new symbols
  static int Counter;

  /// Maps symbols to their label
  static std::unordered_map<clang::ValueDecl *, int> SymbolToLabel;

  /// Maps labels to their symbols
  static std::unordered_map<int, clang::ValueDecl *> LabelToSymbol;

  /// Maps strict access symbols to their label
  static std::unordered_map<int, int> SymbolToLabel_Abst;

  /// Maps strict access labels to their symbols
  static std::unordered_map<int, int> LabelToSymbol_Abst;

  static FSM *AnyClosureAutomata;

public:
  /// Add a transition symbol to the language and give it a label
  static void addSymbol(clang::ValueDecl *ValueDecl);

  static void addSymbol(int AbstractAccessId);

  /// Add a transition between two states on a specific transition symbol
  static void addTransition(FSM &Automata, int Src, int Dest,
                            clang::ValueDecl *);

  static void addTransitionOnAbstractAccess(FSM &Automata, int Src,
                                                 int Dest, int Access);
  /// Add epsilon (optional) transition between two states
  static void addEpsTransition(FSM &Automata, int Src, int Dest);

  /// Add a transition between two states on each symbol in the language
  static void addAnyTransition(FSM &Automata, int Src, int Dest);

  /// Add a transition on label 1 which is reserved for the current traversed
  /// node
  static void addTraversedNodeTransition(FSM &Automata, int Src, int Dest);

  /// Check if two automata intersect
  static bool hasNonEmptyIntersection(const FSM &Automata1,
                                      const FSM &Automata2);

  /// Return an automata that matches a transition on any possible access
  static const FSM &getAnyClosureAutomata();

  /// Return a copy of the automata with the root transition removed
  static FSM *CopyRootRemoved(const FSM &In);

  /// Print the automata into a visual form
  static void print(const FSM &Automata, std::string FileName = "tmp",
                    bool Simplify = false);

  /// Check if the automata does not accept any word
  static bool isEmpty(const FSM &Automata);
};

#endif
