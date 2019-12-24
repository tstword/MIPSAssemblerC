<img src="https://fontmeme.com/temporary/ba4c1b6030ee3486f05b156afcecfb95.png">

# MIPS Assembler C

> A MIPS assembler written in C that supports the core arithmetic instructions, assembly directives, and psuedo instructions.

[![License](http://img.shields.io/:license-mit-blue.svg?style=flat-square)](http://badges.mit-license.org)

---

## Installation

### Prerequisites

- C / C++ Compiler (Developed and tested using GCC 5.4, Clang / Clang++, and MSVC)
- GNU Make Utility (Linux / MacOS)
- Visual Studio (Windows)

### Clone

- Clone this repo to your local machine using `https://github.com/tstword/MIPSAssemblerC`

### Setup

#### Ubuntu
```shell
$ sudo apt-get install gcc make
```

#### MacOS
```shell
$ xcode-select --install
```

#### Windows
- Since there is no convenient way to get a C compiler on Windows, we will default to using a C++ compiler.
- Fear not, the assembler was written to be compatible with Microsoft's MSVC compiler shipped with Visual Studio. 

<a href="https://visualstudio.microsoft.com/">Download Visual Studio</a>.

## Compilation

### Linux / Mac OS
- Change into the directory containing the source code
```shell
$ cd MIPSAssembler
```
- Compile using the Make Utility
```shell
$ make
```
### Windows
1. Open Visual Studio and create an Empty Project (C++)
2. Import the source files into the 'Source Files' folder in the project
3. Change project properties to add the include path containing the header files
4. Build project for target platform (x86/x64)

---

## Usage

### Provided main program
- Typical usage
```shell
$ bin/assembler program.asm -o program.obj
```
- Dump text segment (binary format)
```shell
$ bin/assembler -a program.asm -t text.dump
```
- Dump data segment (binary format)
```shell
$ bin/assembler -a program.asm -d text.dump
```
- Usage statement
```
$ bin/assembler -h
Usage: bin/assembler [-a] [-h] [-t output] [-d output] [-o output] file...
A MIPS assembler written in C

The following options may be used:
  -a                   Only assembles program, does not create object code file
                       * Note: This does not disable segment dumps
  -d <output>          Stores data segment in <output>
  -h                   Displays this message
  -t <output>          Stores text segment in <output>
  -o <output>          Stores object code in <output>
                       * Note: If this option is not specified, <output> defaults to a.obj

Refer to the repository at <https://github.com/tstword/MIPSAssemblerC>
```

### Using source code
Should you decide to use the assembler in your own main program, here is an example
```C
#include <stdio.h>
#include <stdlib.h>
#include "assembler.h"

int main(int argc, char *argv[]) {
    const char *program_files[] = {"program1.asm", "program2.asm"};
    
    struct assembler *assembler = create_assembler();
    
    /* Execute and check to see if the assembler failed to parse the files */
    if(execute_assembler(assembler, program_files, 2) != ASSEMBLER_STATUS_OK) {
        fprintf(stderr, "Failed to assembler program\n");
        return EXIT_FAILURE;
    }
    
    /**
     * The binary data for the segments are referenced by: assembler->segment_memory[SEGMENT]
     * The number of bytes stored is indicated by: assembler->segment_memory_offset[SEGMENT]
     * where SEGMENT equals: SEGMENT_TEXT, SEGMENT_DATA, SEGMENT_KTEXT, or SEGMENT_KDATA 
     *
     * Example: Dumping the data segment 
     **/
     
     printf("Dumping data segment...");
     
     for(int i = 0; i < assembler->segment_memory_offset[SEGMENT_DATA]; i++) {
        /* If offset is a multiple of 4, print address */
        if((i & (0x3)) == 0) printf("\n0x%08X  ", i);
        
        /* Print the byte located at the offset */
        printf("\\%02X ", *((unsigned char *)assembler->segment_memory[SEGMENT_DATA] + i));
     }
     
     printf("\nFinished!\n");
     
     /* Free up the memory used by the assembler */
     destroy_assembler(&assembler);
     
     return EXIT_SUCCESS;
}
```
    
---
## Features

### Custom object files
If the program has been successfully assembled, the assembler will produce a custom object file using the structures defined in the file mipsfhdr.h:
```C
struct MIPS_file_header {
    uint8_t m_magic[4];
    uint8_t m_endianness;
    uint8_t m_version;
    uint8_t m_shnum;
    uint8_t m_padding[1];
};

struct MIPS_sect_header {
    uint8_t sh_segment;
    uint8_t sh_padding[3];
    uint32_t sh_offset;
    uint32_t sh_size;
};
```

Each object file created by the assembler contains a file header that is 8 bytes long

Field        | Meaning       | Value
------------ | ------------- | ------------
m_magic | Magic number | "MIPS"
m_endianness | Indicates endianness of system | 0x01 (little-endian)<br />0x02 (big-endian)
m_version | Version the object file was assembled in | 0x01
m_shnum | The number of section headers in the file | Situational
m_padding | Unused data for padding | 0x00

Following the file header is the section header of the first segment. Each section header is 12 bytes long

Field        | Meaning       | Value
------------ | ------------- | ------------
sh_section | The ID of the segment | 0x00 (.text)<br />0x01 (.data)<br />0x02 (.ktext)<br />0x03 (.kdata)
sh_padding | Unused data for padding | 0x00
sh_offset | The file offset in bytes where the header starts | Situational
sh_size | The number of bytes in the segment | Situational

Following the section header are the bytes of the segment itself indicated by sh_section. The next section header (if it exists) can be located using sh_size.

### Support for the core arithmetic instruction set

The assembler supports the instruction set listed here: <a href="http://www.mrc.uidaho.edu/mrc/people/jff/digital/MIPSir.html">http://www.mrc.uidaho.edu/mrc/people/jff/digital/MIPSir.html</a>

### Support for assembly directives

Directive    | Description       
------------ | -------------
.ascii "\<string\>", ... | Creates non-null terminated strings in the .data segment
.asciiz "\<string\>", ... | Creates null terminated strings in the .data segment
.align \<n\> | Aligns the current segment offset to 2^\<n\>*
.byte \<byte\>, ... | Creates bytes in the .data segment
.data | Changes the segment to DATA
.half \<half\>, ... | Creates half-words in the .data segment
.include "\<file\>" | Opens the file with the name \<file\> and assembles the file
.kdata | Changes the segment to KDATA
.ktext | Changes the segment to KTEXT
.space \<n\> | Creates \<n\> bytes of unitialized space (value defaults to 0)
.text | Changes the segment to TEXT
.word \<word\>, ... | Creates words in the .text or .data segment

### Support for psuedo instructions

Psuedo Instruction | Description
------------------ | -----------
abs $d, $s | Stores the absolute value of $s into $d
addi $t, $s, imm32 | Addition with 32-bit immediate
addiu $t, $s, imm32 | Unsigned addition with 32-bit immediate
andi $t, $s, imm32 | Logical and with 32-bit immediate
b label | Branch to label
beq $s, imm, label | Branch on equal with 16-bit immediate
beqz $s, label | Branch on equal to zero
bge $s, $t, label | Branch on greater than or equal to
bge $s, imm, label | Branch on greater than or equal to with 16-bit immediate
bgeu $s, $t, label | Branch on greater than or equal to unsigned
bgeu $s, imm, label | Branch on greater than or equal to unsigned with 16-bit immediate
bgt $s, $t, label | Branch on greater than
bgt $s, imm, label | Branch on greater than with 16-bit immediate
bgtu $s, $t, label | Branch on greater than unsigned
bgtu $s, imm, label | Branch on greater than unsigned with 16-bit immediate
ble $s, $t, label | Branch on less than or equal
ble $s, imm, label | Branch on less than or equal with 16-bit immediate
bleu $s, $t, label | Branch on less than or equal to unsigned
bleu $s, imm, label | Branch on less than or equal to unsigned with 16-bit immediate
blt $s, $t, label | Branch on less than
blt $s, imm, label | Branch on less than with 16-bit immediate
bltu $s, $t, label | Branch on less than unsigned
bltu $s, imm, label | Branch on less than unsigned with 16-bit immediate
bne $t, imm, label | Branch on not equal with 16-bit immediate
bnez $t, label | Branch on not equal to zero
la $t, label | Loads the address of the label into register $t
lb $t, label | Load byte stored at the label
lbu $t, label | Load unsigned byte stored at the label
lh $t, label | Load half-word stored at the label
lhu $t, label | Load half-word unsigned stored at the label
li $t, imm32 | Loads 32-bit immediate into register $t
lw $t, label | Load word stored at the label
move $t, $s | Moves contents of register $s to register $t
neg $d, $s | Stores the negated value of $s into $d
not $d, $s | Logical not
ori $t, $s, imm32 | Logical or with 32-bit immediate
rol $t, $s, imm | Rotates the contents of $s to the left by the amount specified by imm
ror $t, $s, imm | Rotates the contents of $s to the right by the amount specified by imm
sb $t, label | Store byte at the label
sgt $d, $s, $t | Set greater than
sh $t, label | Store half-word at the label
slti $t, $s, imm32 | Set less than with 32-bit immediate 
sltiu $t, $s, imm32 | Set less than unsigned with 32-bit immediate
sne $d, $s, $t | Set not equal
sw $t, label | Store word at the label
xori $t, $s, imm32 | Logical xor with 32-bit immediate

---

## License

[![License](http://img.shields.io/:license-mit-blue.svg?style=flat-square)](http://badges.mit-license.org)

- **[MIT license](http://opensource.org/licenses/mit-license.php)**
- Copyright 2019 © Bryan Rocha
