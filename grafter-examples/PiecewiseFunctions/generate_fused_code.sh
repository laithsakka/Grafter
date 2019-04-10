#!/bin/bash
 
# create a direcotry for the fused code and make a copy of the original code  
rm -r FUSED
mkdir FUSED
cp  ./UNFUSED/* ./FUSED/  

# run grafter on the created copy 
grafter  -max-merged-f=10 -max-merged-n=10 ./FUSED/main.cpp -- -I/usr/local/bin/../lib/clang/3.8.0/include/ -I/usr/local/include/c++/v1/ -std=c++11

clang-format -i FUSED/main.cpp 
