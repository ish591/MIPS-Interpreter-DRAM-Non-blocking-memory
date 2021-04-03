# MIPS-Interpreter-DRAM-Non-blocking-memory
A MIPS interpreter in C++ which models DRAM by using a row buffer, and implements non-blocking memory by executing some instructions in parallel with lw/sw

### Compiling and Executing

After cloning the repository run 

```bash
 cd MIPS-Interpreter-DRAM-Non-blocking-memory
 make # To compile col216minor.cpp
 ```
 To run the executable  ```.\a.out``` :
 
 ```
 .\a.out in.txt ROW_ACCESS_DELAY COL_ACCESS_DELAY part_number
 ```
 Here , ROW_ACCESS_DELAY,COL_ACCESS_DELAY and part_number are integer parameters that must be supplied. part_number is either 1 or 2, and the other two parameters are positive integers.

### Cleaning
```
make clean #this removes the executable ./a.out from the directory
```
    
