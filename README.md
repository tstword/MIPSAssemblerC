# C MIPS Assembler

A MIPS assembler written in C. 
I began work on this project during my free time as a grader for ICS 51 at the University of California, Irvine and eventually grew attached.
What was the motive, you may ask: The MARS simulator is wonderful IDE however, it takes forever to load into memory and execute. Normally this isn't an issue but because I had to run scripts on over 150+ submissions and each submission took about ~3 minutes to execute all ~40 test cases I sought out to create a MIPS assembler in C for faster results.

## Getting Started

Install git and clone the project into a directory

```
git clone https://github.com/tstword/NewMIPSAssembler MIPSAssembler
```

## Prerequisites

C / C++ Compiler (Developed and Tested using GCC 5.4 and Clang)
GNU Make Utility (Linux / MacOS)
Visual Studio (Windows)

# Ubuntu
```
sudo apt-get install gcc make
```

# MacOS
```
xcode-select --install
```

# Windows
Since GCC isn't supported on Windows, we will default to using a C++ compiler.
The MSVC compiler is currently not supported (for language constructs reasons) however the project can be compiled with Clang and Visual Studio.
For information on setting up Clang on Windows, see: https://llvm.org/docs/GettingStartedVS.html

## Compilation

# Linux / Mac OS
Change into the directory containing the source code
```
cd MIPSAssembler
```
Compile using the Make Utility
```
make
```

# Windows
1. Open Visual Studio and create an Empty Project (C++)
2. Import the source files into the 'Source Files' folder in the project
3. Change project properties to add the include path containing the header files
4. Change the platform toolset to LLVM (clang)
5. Build project for target platform (x86/x64)