# Artifact evaluation guide

## Getting Started Guide
The artifact is enclosed inside a virtual machine, the virtual machine contains
the source code for Grafter which is already compiled and ready to use, in
addition to the source code of the benchmarks.

This section includes the basic steps to get the VM machine running and to run
grafter. If you want to build Grafter from scratch on your own machine, you can
follow the steps in the extras.

### Steps for setting up the virtual machine
1. Install virtual box from https://www.virtualbox.org/wiki/Downloads.
2. Download and decompress grafter.tar.gz from
   https://drive.google.com/open?id=1bRKcnHLegINqUBPh6Gy0O9yHXjWFgylJ
3. From virtual box import grafter.ova (File-> import).
4. You can launch the VM using the GUI of the virtual box. When you launch
   it you might get the following errors:
```
    Could not start the machine grafter because the following physical network
    interfaces were not found: vboxnet0 (adapter 2)
    You can either change the machine's network settings or stop the machine
```
   Fix: Right click on the VM and open the VM settings, go to networks and
   change ``attached to`` from "host only adapter" to "nat".

```text
   Failed to open a session for the virtual machine grafter.
   Implementation of the USB 2.0 controller not found!
   ...etc
 ```
   Fix: Right click on the VM and open the VM settings, go to ports, choose USB
   tab and disable USB.

  **Note:** The password for the VM is admin when needed.

### Navigating the virtual machine
* All the files that you need to deal with are located under
    `/home/grafter/Desktop/Grafter` directory .
* Grafter source code: grafter is implemented as a Clang tool, thus its code
  resides in the LLVM source code directory and can be found at
  `/home/grafter/Desktop/Grafter/llvm/tools/clang/tools/grafter`.
* Grafter binary: located  at `/home/grafter/Desktop/Grafter/build/bin/grafter` .
* Benchmarks evaluated in the paper: AST traversals and RenderTree,
  located under ``/home/grafter/Desktop/Grafter/benchmarks``.

### Basic testing.
The file ``/home/grafter/Desktop/Grafter/benchmarks/BinarySearchTree/UNFUSED/\
main.cpp`` includes a simple binary search tree program that performs two
traversals (insertion and search). In this basic testing, we will show how to run
grafter on this program and generate a fused code. For more information about
the language of Grafter refer to *Writing code in Grafter*  under extras section
at end of the document.

Follow the following steps for basic testing:
```bash
     # Copy the source code into a different file.
     cd /home/grafter/Desktop/Grafter/benchmarks/BinarySearchTree
     mkdir FUSED
     cp UNFUSED/* FUSED/

     # Run Grafter
     ../../build/bin/grafter  -max-merged-f=1  -max-merged-n=5 FUSED/main.cpp\
     -- -I/usr/lib/llvm-3.8/lib/clang/3.8.0/include/ -std=c++11

     # Format the output code to make it more readable
     clang-format -i FUSED/main.cpp

     # Test code
     clang++ FUSED/main.cpp -std=c++11 -o fused
     clang++ UNFUSED/main.cpp -std=c++11 -o unfused

     # Expect the same output "10 is found"
     ./fused
     ./unfused

     # Inspect the fused code
     vim FUSED/main.cpp
```
Looking at the fused code we can see that the original calls at lines
461-463 are commented and replaced with a new call to the new fused traversal
at line 466.
In the next section the commands responsible for generating the fused code and
for compiling the programs will be executed within scripts in a way similar
to the code above.

## Step-by-Step Instructions

In our paper we demonstrated two use cases (AST traversals and Render trees).
In this section we will walk through the process of regenerating the reported
results for each of them.

**Note**: We **can not** access hardware counters from the virtual machine using
virtual box, thus we can only **perform the fusion**, and show the **speedup** and
the **reduction of node visits** on the VM.

To measure **cache performance** and **count instructions** we need to move the fused
code to an actual(physical) machine and perform the experiments on it, we
provide detailed instructions on how to do that at the end the section

**Note**: The documentation is meant to be read in order.

### Generating fused traversals and virtual machine experiments

#### AST traversals (Figure 11)

* Unfused code for AST traversals is located at
``/home/grafter/Desktop/Grafter/benchmarks/AST/UNFUSED``
* To generate Figure 11 data do the following:
```bash
   # all the commands bellow should be executed while at that directory*
   cd /home/grafter/Desktop/Grafter/benchmarks/AST

   # generate the fused code inside the folder FUSED
   ./generate_fused_code.sh

   # generate binaries (fused and unfused)
   ./generate_binaries.sh

   # single tests (takes as input the number of functions )
   ./fused 1000
   ./unfused 1000

   # Expected output; (When PAPI is not available, the case on VM)
   #### Note : Actual values might be different
   # PAPI Error starting counters
   # PAPI Error reading counters
   # L2 Cache Misses : 0
   # L3 Cache Misses  : 0
   # Instructions : 0
   # Runtime: 12861 microseconds
   # Node Visits: 242005
   ####

   # run complete test from (10, 100, 1000 ... m)
   python3 RunExperiments.py -m 10000
```
The python script *RunExperiments.py* runs the fused and the unfused binaries 10
times each and then takes the average of different measurements and generates
the normalized measurements that are reported in *Fig 11*.

The output table is printed on the screen and stored in *output.csv*, it should
look like:
```
Size,Normalized L2 Cache Misses,Normalized L3 Cache Misses, Normalized Instructions, Normalized Runtime, Normalized Node Visits
10,-1,-1,-1,0.9,0.76
100,-1,-1,-1,0.91,0.76
1000,-1,-1,-1,0.52,0.76
10000,-1,-1,-1,0.52,0.76
```
Note : Actual values might be different.

It will consider trees up to size m, where m is the binary input; number of
functions for AST, and number of pages for RenderTree.
As mentioned earlier, this will only include normalized runtime and normalized
node visits when executed on the VM.

#### Treefuser-RenderTree (Figure 9.b)
In the paper we ran render tree benchmark on a mobile device, in this evaluation
we will do the tests on the machine to simplify the task.

* Source code is in ``/home/grafter/Desktop/Grafter/benchmarks/RenderTree/Treefuser/UNFUSED``
* To generate Figure 9.b data do the following:
```bash
   cd /home/grafter/Desktop/Grafter/benchmarks/RenderTree/Treefuser

   # generate the fused code inside the folder FUSED
   ./generate_fused_code.sh

   # generate binaries (fused and unfused
   ./generate_binaries.sh

   # single tests (takes as input the number of pages   
   ./fused 1000
   ./unfused 1000
       
   # run complete test from (10, 100, 1000 ... m)
   python3 RunExperiments.py -m 10000
```
output will be in *output.csv* just like AST.

#### Grafter (Figure 9.a)
* Source code  code is in ``/home/grafter/Desktop/Grafter/benchmarks/RenderTree/Grafter/UNFUSED``
* To generate Figure 9.a data do the following:
```bash
   cd /home/grafter/Desktop/Grafter/benchmarks/RenderTree/Grafter

   # generate the fused code inside the folder FUSED
   ./generate_fused_code.sh

   # generate binaries (fused and unfused)
   ./generate_binaries.sh

   # single tests (takes as input the number of pages)
   ./fused 1000
   ./unfused 1000

   # run complete test from (10, 100, 1000 ... m)
   python3 RunExperiments.py -m 10000
```
output will be in *output.csv* just like other benchmarks.

### Real machine experiments

To evaluate **Cache misses** and **Instruction count** we need to access hardware
counters for PAPI library to work. For that we need to do the experiments
on a physical machine.

#### Requirements
* The machine you are running on should have intel processor.
* The OS should be linux.
* We want PAPI to be installed on the machine, can be done as the following:
   http://icl.cs.utk.edu/papi/software/index.html
 ```bash
    mkdir tmp && cd tmp
    git clone https://bitbucket.org/icl/papi.git
    cd papi
    git pull https://bitbucket.org/icl/papi.git
    cd src
    ./configure
    make -j 20
    sudo make install
```
* Make sure hardware counters access is enabled by doing the following:
   (this need to be done if the machine is restarted)
```bash
   sudo -i
   echo 0 > /proc/sys/kernel/perf_event_paranoid
   exit
```
* Install python3 if not installed

#### Steps
1. Generate the fused code **on the VM** as mentioned earlier by executing 
``./generate_fused_code.sh`` in each of the three benchmarks directories. 
If you already did the steps before this should be already done.

2. Copy the folder `` /home/grafter/Desktop/Grafter/benchmarks`` into the
physical machine at some directory $DIR.

3. Execute ``python3 RunExperiments.py -m MAX-SIZE`` in each of the benchmarks
directories.
   * $DIR/benchmarks/AST to generate Figure 11 data.
   * $DIR/benchmarks/RenderTree/Grafter to generate Figure 9.b data.
   * $DIR/benchmarks/RenderTree/Treefuser to generate Figure 9.a data.

The results are going to be stored in  *output.csv* in each of the directories
as mentioned earlier, and should look like:
```
Size,Normalized L2 Cache Misses,Normalized L3 Cache Misses, Normalized Instructions, Normalized Runtime, Normalized Node Visits
10,1.01,2.23,1.12,1.0,0.76
100,0.25,2.09,1.13,0.63,0.76
1000,0.25,0.45,1.13,0.62,0.76
10000,0.24,0.24,1.13,0.48,0.76
```
Note : Actual values might be different.
# Extras
## Building grafter from scratch.
Follow the following steps to build grafter on your machine (linux )
1. Install cmake and ninja.
```bash
   sudo apt-get update -y
   sudo apt-get install -y wget git build-essential cmake ninja-build python
```
2. Install openfst.(http://www.openfst.org)
```bash
   mkdir tmp
   cd /tmp
   wget -nv http://www.openfst.org/twiki/pub/FST/FstDownload/openfst-1.6.9.tar.gz
   tar xf openfst-1.6.9.tar.gz
   cd openfst-1.6.9 && ./configure &&  make
   sudo make install
```
3. Get llvm source code using commit hash 97d7bcd5c024ee6aec4eecbc723bb6d4f4c3dc3d
```bash
   wget -nv https://github.com/llvm-mirror/llvm/archive/97d7bcd5c024ee6aec4eecbc723bb6d4f4c3dc3d.tar.gz
   tar xf 97d7bcd5c024ee6aec4eecbc723bb6d4f4c3dc3d.tar.gz
   mv llvm-97d7bcd5c024ee6aec4eecbc723bb6d4f4c3dc3d  $LLVM_ROOT
```
4. Get clang soure code using hash commit 6093fea79d46ed6f9846e7f069317ae996149c69
     and place it in $LLVM_ROOT/tools/clang
```bash
   wget --progress=dot:giga https://github.com/llvm-mirror/clang/archive/6093fea79d46ed6f9846e7f069317ae996149c69.tar.gz
   tar xf 6093fea79d46ed6f9846e7f069317ae996149c69.tar.gz
   mv clang-6093fea79d46ed6f9846e7f069317ae996149c69  $LLVM_ROOT/tools/clang
```
5. Get grafter source code and place it  $LLVM_ROOT/tools/clang/tools/grafter
```bash
   wget http://github.com/laithsakka/TreeFuser/archive/pldi2019.tar.gz
   tar xf pldi2019.tar.gz
   mv TreeFuser-pldi2019/grafter $LLVM_ROOT/tools/clang/tools/grafter
```
6. Add the following line to $LLVM_ROOT/tools/clang/tools/CMakeLists.txt
    ``add_clang_subdirectory(grafter)``
7. Make a build directory $BUILD_DIR outside $LLVM_ROOT
8. Build grafter
```bash
   cd $BUILD_DIR
   cmake -G Ninja  $LLVM_ROOT
   ninja grafter
```
9. Check the binary in $BUILD_DIR/bin/grafter.


## Writing code in Grafter.
* Grafter operates on code written in a subset of C++ language, the code can
coexist with other general C++ code without problems.

* The language of grafter need to be obeyed in the traversals in order for them
  to be considered for fusion.

* There are four main annotations that are used in Grafter language:
  1. tree_structure : A class annotation that identifies tree structures:
  2. tree_child: A class member annotation that identifies recursive Fields:
  3. tree_traversals: Identify tree traversals
  4. abstract access.

  ```
     #define __tree_structure__ __attribute__((annotate("tf_tree")))
     #define __tree_child__ __attribute__((annotate("tf_child")))
     #define __tree_traversal__ __attribute__((annotate("tf_fuse")))
  ```

* A good start point is to look at the simple example
  ``/home/grafter/Desktop/Grafter/benchmarks/BinarySearchTree/UNFUSED/main.cpp``
   and read the language section in the paper in addition to looking at the
   AST and the RenderTree/Grafter examples.

* Make sure to have all your code in one compilation unit; for separation you
  can write separate traversals in separate header files.(see benchmarks)

### Tree structure
* Any c++ class can be annotated as a tree structure, tree children should be
annotated as well, all tree structure ' s members that are going to be used in the
tree traversals should be **public** .

* Heterogeneous types are supported through inheritance, all classes that are
derived from a tree structure should have the tree structure annotation as well.


#### Traversals Language Restrictions:
* This is a small set of can and can not do things in Grafter tree traversals,
 yet **not complete**.

  1. Return types should be void.
  2. Traversing Calls cant be conditioned.
  3. No explicit for loops.
  4. Do not use pointers for data (only for tree nodes).
* What is allowed?
  1. Mutual recursions.
  2. If Statement.
  3. Assignment.
  4. Node deletion delete path-to-tree-node
  5. Node creation path-to-tree-node = new ().
  6. Aliasing statement: TreeNodeType * const X = path-to-tree-node
  7. BinaryExpressions (>, <, ==, &&, || ..etc)
  8. NULL Expression.
  9. Calls to other traversals.
  10. Calls to abstract access functions**.
