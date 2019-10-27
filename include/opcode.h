#ifndef MNEMONIC_H
#define MNEMONIC_H

#include <stdlib.h>

/* Type definitions */
typedef unsigned int mnemonic_t;

/* Mnemonic name macros */
/* ALU */
#define MNEMONIC_ADD      0x00
#define MNEMONIC_ADDU     0x01
#define MNEMONIC_AND      0x02
#define MNEMONIC_NOR      0x03
#define MNEMONIC_OR       0x04
#define MNEMONIC_SLT      0x05
#define MNEMONIC_SLTU     0x06
#define MNEMONIC_SUB      0x07
#define MNEMONIC_SUBU     0x08
#define MNEMONIC_XOR      0x09

/* Shifters */
#define MNEMONIC_SLL      0x0A
#define MNEMONIC_SRA      0x0B
#define MNEMONIC_SRL      0x0C

/* Branches */
#define MNEMONIC_BEQ      0x0D
#define MNEMONIC_BGEZ     0x0E
#define MNEMONIC_BGEZAL   0x0F
#define MNEMONIC_BGTZ     0x10
#define MNEMONIC_BLEZ     0x11
#define MNEMONIC_BLTZ     0x12
#define MNEMONIC_BLTZAL   0x13
#define MNEMONIC_BNE      0x14
#define MNEMONIC_JMP      0x15
#define MNEMONIC_JAL      0x16
#define MNEMONIC_JR       0x17

/* System Call */
#define MNEMONIC_SYSCALL  0x18

/* Memory Access */
#define MNEMONIC_LB       0x19
#define MNEMONIC_LBU      0x1A
#define MNEMONIC_LH       0x1B
#define MNEMONIC_LHU      0x1C
#define MNEMONIC_LW       0x1D
#define MNEMONIC_SB       0x1E
#define MNEMONIC_SH       0x1F
#define MNEMONIC_SW       0x20

/* Immediate Operations */
#define MNEMONIC_ADDI     0x21
#define MNEMONIC_ADDIU    0x22
#define MNEMONIC_ANDI     0x23
#define MNEMONIC_LUI      0x24
#define MNEMONIC_ORI      0x25
#define MNEMONIC_SLTI     0x26
#define MNEMONIC_SLTIU    0x27
#define MNEMONIC_XORI     0x28

/* Psuedo Instructions */
#define MNEMONIC_MOVE     0x29
#define MNEMONIC_LI       0x2A
#define MNEMONIC_LA       0x2B
#define MNEMONIC_NOT      0x2C

/* opcode type flags */
#define OPTYPE_DEFAULT    0x0
#define OPTYPE_PSUEDO     0x1

/* opcode entry table structure */
struct opcode_entry {
    unsigned char opcode;           /* Instruction opcode */
    unsigned char funct;            /* Instruction funct */
    unsigned char rt;               /* Used for specific instructions like BGEZAL */
    unsigned char psuedo;           /* Flag used to indicate psuedo instructions */
};

/* global reference to opcode table */
extern struct opcode_entry opcode_table[];
extern const size_t opcode_table_size;

#endif