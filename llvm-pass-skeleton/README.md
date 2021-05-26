# llvm-pass-skeleton

Original: [Here](https://github.com/sampsyo/llvm-pass-skeleton.git)
A completely useless LLVM pass.  
Uses Legacy pass manager to Print function names.  
No modification to IR.

Build:

    $ cd llvm-pass-skeleton
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ cd ..

RUN:

    $ clang hello.c -emit-llvm -c  
    $ opt -enable-new-pm=0 -disable-output -load build/skeleton/libSkeletonPass.so -skeleton -time-passes hello.bc  
I saw a function called main!  
===  -------------------------------------------------------------------------===  
                      ... Pass execution timing report ...  
===  -------------------------------------------------------------------------===  
  Total Execution Time: 0.0001 seconds (0.0001 wall clock)  

   ---User Time---   --User+System--   ---Wall Time---  --- Name ---  
   0.0000 ( 59.7%)   0.0000 ( 59.7%)   0.0000 ( 59.4%)  Skeleton World Pass  
   0.0000 ( 40.3%)   0.0000 ( 40.3%)   0.0000 ( 40.6%)  Module Verifier  
   0.0001 (100.0%)   0.0001 (100.0%)   0.0001 (100.0%)  Total  

===  -------------------------------------------------------------------------===  
                                LLVM IR Parsing  
===  -------------------------------------------------------------------------===  
  Total Execution Time: 0.0002 seconds (0.0002 wall clock)  

   ---User Time---   --System Time--   --User+System--   ---Wall Time---  --- Name ---  
   0.0001 (100.0%)   0.0001 (100.0%)   0.0002 (100.0%)   0.0002 (100.0%)  Parse IR  
   0.0001 (100.0%)   0.0001 (100.0%)   0.0002 (100.0%)   0.0002 (100.0%)  Total  

