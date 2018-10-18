# Treefuser2
TreeFuser is a tool that perform traversals fusion for recursive tree traversals written in subset of the c++ language, for more information about the tool check the paper(https://dl.acm.org/citation.cfm?id=3133900).

TreeFuser2 is an ongoing extention for TreeFuser that is aiming to mainly add the following support:
1. Node Creation.
2. Node Deletion.
3. Aliasing Statements.
4. Support Hentrogenous Types through Inheritance and Virtual functions.
5. Support mutual recursion.
6. Change the access representation to an automatas using OpenFST and depndence checks to intersections.


# INSTALLATION

Treefuser uses Clang libraries and by design it is built as one of Clang tools. The build process is not much different from a regular Clang build. The following instructions assumes that you are running under Linux (works for mac)

### Prerequisite 
(1) Make sure that you have OpenFST library installed in the system. http://www.openfst.org/twiki/bin/view/FST/WebHome 

### Now you are ready

Start with cloning LLVM and Clang repos:

> git clone https://github.com/llvm-mirror/llvm llvm

> cd llvm/tools

> git clone https://github.com/llvm-mirror/clang clang

> cd clang/tools

Download the folder tree-fuser from the repo and put inside llvm/tools/clang/tools
> git clone -b treefuser2 https://github.com/laithsakka/TreeFuser/ tmp

> mv tmp/tree-fuser ./

> rm -rf ./tmp

> cd ..

#### Add the following line to llvm/tools/clang/tools/CMakeLists.txt \"add_clang_subdirectory(tree-fuser)\" 
#### Update llvm/tools/clang/tools/tree-fuser/CMakeLists.txt by adding the path to OpenFST library to target_link_libraries by defualt it will try to link  /usr/local/lib/libfst.13.dylib 

Proceed to a normal LLVM build using a compiler with C++11 support (for GCC use version 4.9 or later):

> cd ../../../../

> mkdir build

> cd build

> cmake -G Ninja ../llvm

> ninja tree-fuser

tree-fuser will be available under ./bin/ Add this directory to your path to ensure the rest of the commands in this tutorial work.

.

# Usage
This part considers that the user is using TreeFuser2.
you can run tree-fuser simply as tree-fuser file.cpp --

TreeFuser will look for sequqnce of traversals that traverse the same structure and try to fuse them, It will update the files, make sure to operate on a copy of the files or create a backup.

## Language 
TreeFuser operates on code written in subset of C++ language, the code can coexists with other general C++ code without problems.
The restrictions are for the functions that are annotated to be considered for fusion by TreeFuser.

There are four main annotations that are used in TreeFuser langauage:

1. tree_structure : A class annotation that indentifies tree structures:
2. tree_child: A class member annotation that idenitifes recursiveFields: 
3. tree_traversals: Indentify tree traversals
4. abstract_access.

An example of a code written for treefuser is bellow:

```c++
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#define _Bool bool
#define __tree_structure__ __attribute__((annotate("tf_tree")))
#define __tree_child__ __attribute__((annotate("tf_child")))
#define __tree_traversal__ __attribute__((annotate("tf_fuse")))
enum NodeType { VAL_NODE, NULL_NODE };

class __tree_structure__ Node {
public:
  bool Found = false;
  NodeType Type;
  __tree_traversal__ virtual void search(int Key, bool ValidCall) {
    Found = false;
  }
  __tree_traversal__ virtual void insert(int Key, bool ValidCall) {}
};

class __tree_structure__ NullNode : public Node {};

class __tree_structure__ ValueNode : public Node {
public:
  int Value;
  __tree_child__ Node *Left, *Right;

  __tree_traversal__ void search(int Key, bool ValidCall) override {

    if (ValidCall != true)
      return;
    Found = false;

    if (Key == Value) {
      Found = true;
      return;
    }
    Left->search(Key, Key < Value);
    Right->search(Key, Key >= Value);
    Found = Left->Found || Right->Found;
  }
  };
```

### Tree strucutre
Any c++ class can be annotated as a tree strucutre, recursive fields should be annotated as well,
all a tree structure that are going to be used in the tree traversals should be public this is something that 
we are planning to work at.

Hetrogenious types are supported thorugh inheritance, all classes that are derived from a tree should
have the tree structure annotation as well.

### Traversals
There are two ways traversals can be written :

1. Global Functions: Those traversals should have the traversed node as the first paranmeter( a pointer to the traversed node).
2. Member Functions: Those traversals are class members and implicity traverse the node refered to by the (this), 
Those traversals can be virtual functions, and they she be used along with inhertince to handle type based traversals.
In the example above searchin a null node does not do anything so the base function is called, while searching a ValueNode performs actual search.

#### Traversals Langauge Restrictions:
Those restriction are enforced by tree-fuser and violated, tree-fuser should indicate the violation. 
Yet more testing is needed for robustness.

1. Return types should be void. (except for absract accesses).
2. Traversing Calls cant be conditioned. (except for absract accesses).
3. No for loops.

What is allowd
1. Mutual recursions.
2. If Satement.
3. Assigment.
4. Node deletion delete <path to tree node>
5. Node creation <path to tree node> = new ().
6. Aliasing statement : TreeNodeType * const X = <path to tree node>
7. BinaryExpressions (>, <, ==, &&, || ..etc)
8. NULL Expression.
9. Calls to other traversals.
10. Calls to abstarct access fuctions**.
..etc try some other things! 
                            


