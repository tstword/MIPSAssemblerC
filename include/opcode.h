/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * File: opcode.h
 *
 * Purpose: Used to define the neccessary macros and structures
 * for the opcode table which maps mnemonics into their corresponding
 * instruction. 
 * 
 * There are two types of instructions that can be defined here:
 *      - DEFAULT: Core instruction, requires 4 bytes
 *      - PSUEDO:  Used for asm file, expands into one (or more) core instructions
 *
 * The global variable opcode_table contains the array of opcode mappings.
 *
 * @author: Bryan Rocha
 * @version: 1.0 (8/28/2019)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef MNEMONIC_H
#define MNEMONIC_H

#include <stdlib.h>

/* ALU core instructions */
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
#define MNEMONIC_BEQZ     0x2D
#define MNEMONIC_BGE      0x2E
#define MNEMONIC_BLE      0x2F
#define MNEMONIC_BNEZ     0x30
#define MNEMONIC_BLT      0x31
#define MNEMONIC_BGT      0x32

/* Instructions I forgot about... */
#define MNEMONIC_DIV      0x33
#define MNEMONIC_DIVU     0x34
#define MNEMONIC_MFHI     0x35
#define MNEMONIC_MFLO     0x36
#define MNEMONIC_MULT     0x37
#define MNEMONIC_MULTU    0x38

#define DIRECTIVE_INCLUDE  0x39
#define DIRECTIVE_TEXT     0x3A
#define DIRECTIVE_DATA     0x3B
#define DIRECTIVE_ASCII    0x3C
#define DIRECTIVE_ASCIIZ   0x3D
#define DIRECTIVE_BYTE     0x3E
#define DIRECTIVE_ALIGN    0x3F
#define DIRECTIVE_HALF     0x40
#define DIRECTIVE_WORD     0x41
#define DIRECTIVE_KTEXT    0x42
#define DIRECTIVE_KDATA    0x43
#define DIRECTIVE_SPACE    0x44

#define MNEMONIC_MUL       0x45
#define MNEMONIC_ABS       0x46
#define MNEMONIC_NEG       0x47
#define MNEMONIC_ROR       0x48
#define MNEMONIC_ROL       0x49
#define MNEMONIC_SGT       0x4A
#define MNEMONIC_B         0x4B
#define MNEMONIC_SNE       0x4C
#define MNEMONIC_BLEU      0x4D

/* Opcode type flags */
#define OPTYPE_DEFAULT    0x0
#define OPTYPE_PSUEDO     0x1
#define OPTYPE_DIRECTIVE  0x2

/* Opcode operands flags */
#define OPERAND_NONE       0x00
#define OPERAND_LABEL      0x01
#define OPERAND_IMMEDIATE  0x02
#define OPERAND_REGISTER   0x04
#define OPERAND_ADDRESS    0x08
#define OPERAND_STRING     0x10
#define OPERAND_REPEAT     0x20
#define OPERAND_OPTIONAL   0x40

#define MAX_OPERAND_COUNT  0x03

/* Defines the operands that the mnemonic can take */
#define OPFORMAT_NONE               { OPERAND_NONE, OPERAND_NONE, OPERAND_NONE              }
#define OPFORMAT_R_TYPE             { OPERAND_REGISTER, OPERAND_REGISTER, OPERAND_REGISTER  }
#define OPFORMAT_I_TYPE             { OPERAND_REGISTER, OPERAND_REGISTER, OPERAND_IMMEDIATE }
#define OPFORMAT_I_ADDR_TYPE        { OPERAND_REGISTER, OPERAND_ADDRESS | OPERAND_LABEL, OPERAND_NONE       }
#define OPFORMAT_I_BRANCH_TYPE      { OPERAND_REGISTER, OPERAND_REGISTER | OPERAND_IMMEDIATE , OPERAND_LABEL     }
#define OPFORMAT_J_TYPE             { OPERAND_LABEL, OPERAND_NONE, OPERAND_NONE             }
#define OPFORMAT_BRANCH_TYPE        { OPERAND_REGISTER, OPERAND_LABEL, OPERAND_NONE         }
#define OPFORMAT_REGISTER           { OPERAND_REGISTER, OPERAND_NONE, OPERAND_NONE          }
#define OPFORMAT_IMMEDIATE          { OPERAND_IMMEDIATE, OPERAND_NONE, OPERAND_NONE         }
#define OPFORMAT_REG_IMM            { OPERAND_REGISTER, OPERAND_IMMEDIATE, OPERAND_NONE     }
#define OPFORMAT_REG_REG            { OPERAND_REGISTER, OPERAND_REGISTER, OPERAND_NONE      }
#define OPFORMAT_REG_LAB            { OPERAND_REGISTER, OPERAND_LABEL, OPERAND_NONE         }
#define OPFORMAT_STR_REP            { OPERAND_STRING | OPERAND_REPEAT, OPERAND_NONE, OPERAND_NONE }
#define OPFORMAT_IMM_REP            { OPERAND_IMMEDIATE | OPERAND_REPEAT, OPERAND_NONE, OPERAND_NONE }
#define OPFORMAT_STRING             { OPERAND_STRING, OPERAND_NONE, OPERAND_NONE }
#define OPFORMAT_IMM_LAB_REP        { OPERAND_IMMEDIATE | OPERAND_LABEL | OPERAND_REPEAT, OPERAND_NONE, OPERAND_NONE }
#define OPFORMAT_R_TYPE_OPREG       { OPERAND_REGISTER, OPERAND_REGISTER, OPERAND_REGISTER | OPERAND_OPTIONAL }
#define OPFORMAT_REG_REG_RI         { OPERAND_REGISTER, OPERAND_REGISTER, OPERAND_REGISTER | OPERAND_IMMEDIATE }

/* Type definitions */
typedef unsigned int mnemonic_t;
typedef unsigned char operand_t;

/* opcode entry table structure */
struct opcode_entry {
    unsigned char opcode;           /* Instruction opcode */
    unsigned char funct;            /* Instruction funct */
    unsigned char rt;               /* Used for specific instructions like BGEZAL */
    operand_t operand[3];           /* Specifies operand format */
    unsigned char type : 2;         /* Flag used to indicate instruction type */
	unsigned char size : 7;       /* If psuedo is 1, we need the size of the instruction */
};

/* global reference to opcode table */
extern struct opcode_entry opcode_table[];
extern const size_t opcode_table_size;

#endif
