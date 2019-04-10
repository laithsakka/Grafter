#!/bin/bash

# create a direcotry for the fused code and make a copy of the original code  
rm -r FUSED
mkdir FUSED
cp  ./UNFUSED/* ./FUSED/  

# run grafter on the created copy 
/home/grafter/Desktop/Grafter/build/bin/grafter  -max-merged-f=1  -max-merged-n=5 ./FUSED/main.cpp -- -I/usr/lib/llvm-3.8/bin/../lib/clang/3.8.0/include/ -std=c++11

# modify the node visits counting instrumentation to execlude when the visited node is null
perl -0777 -i.original -pe 's/\n#ifdef COUNT_VISITS \n _VISIT_COUNTER[+][+];\n #endif \n/\n#ifdef COUNT_VISITS \n if(_r) _VISIT_COUNTER++;\n #endif \n/igs' FUSED/main.cpp 

clang-format -i FUSED/main.cpp 
