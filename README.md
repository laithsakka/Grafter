# Treefuser
TreeFuser is a tool that perform traversals fusion for recursive tree traversals written in subset of the c++ language, for more information about the tool check the paper(https://dl.acm.org/citation.cfm?id=3133900).

# INSTALLATION
Treefuser uses Clang libraries and by design it is built as one of Clang tools. The build process is not much different from a regular Clang build. The following instructions assumes that you are running under Linux.

Start with cloning LLVM and Clang repos:

> git clone https://github.com/llvm-mirror/llvm llvm

> cd llvm/tools

> git clone https://github.com/llvm-mirror/clang clang

> cd clang/tools

Download the folder tree-fuser from the repo and put inside llvm/tools/clang/tools
> git clone https://github.com/laithsakka/TreeFuser/ tmp

> mv tmp/tree-fuser ./

> rm -rf ./tmp

> cd ..

 Add the following line to llvm/tools/clang/tools/CMakeLists.txt 
 "add_clang_subdirectory(tree-fuser)"

Proceed to a normal LLVM build using a compiler with C++11 support (for GCC use version 4.9 or later):

> cd ../../../../

> mkdir build

> cd build

> cmake -G Ninja ../llvm

> ninja or ninja tree-fuser

tree-fuser will be available under ./bin/ Add this directory to your path to ensure the rest of the commands in this tutorial work.


## TreeFuser2
Insrtructions for building TreeFuser2 is same as posted above, except that you need to make sure that you have OpenFST library installed in the system. http://www.openfst.org/twiki/bin/view/FST/WebHome 
# Usage


