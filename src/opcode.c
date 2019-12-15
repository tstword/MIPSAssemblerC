/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * File: opcode.c
 * Purpose: Defines the opcode table which maps mnemonics to their
 * operand format, instruction type, and opcode / funct.
 * 
 * Each entry in the opcode table corresponds to their MNEMONIC macro
 * value to allow O(1) access time. 
 *
 * Should you decide to add any more instructions make sure the index entry
 * is correctly mapped to their MNEMONIC macro value.
 *
 * @author: Bryan Rocha
 * @version: 1.0 (2/3/2019)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "opcode.h"

/* Opcode table definition */
struct opcode_entry opcode_table[] = {
    /* ALU Core OPCODES */
    { 0x00, 0x20, 0x00, OPFORMAT_R_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_ADD */
    { 0x00, 0x21, 0x00, OPFORMAT_R_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_ADDU */
    { 0x00, 0x24, 0x00, OPFORMAT_R_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_AND */
    { 0x00, 0x27, 0x00, OPFORMAT_R_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_NOR */
    { 0x00, 0x25, 0x00, OPFORMAT_R_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_OR */
    { 0x00, 0x2A, 0x00, OPFORMAT_R_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_SLT */
    { 0x00, 0x2B, 0x00, OPFORMAT_R_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_SLTU */
    { 0x00, 0x22, 0x00, OPFORMAT_R_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_SUB */
    { 0x00, 0x23, 0x00, OPFORMAT_R_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_SUBU */
    { 0x00, 0x26, 0x00, OPFORMAT_R_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_XOR */
    /* Shifter OPCODES */
    { 0x00, 0x00, 0x00, OPFORMAT_I_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_SLL */
    { 0x00, 0x03, 0x00, OPFORMAT_I_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_SRA */
    { 0x00, 0x02, 0x00, OPFORMAT_I_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_SRL */
    /* Branch OPCODES */
    { 0x04, 0x00, 0x00, OPFORMAT_I_BRANCH_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_BEQ */
    { 0x01, 0x00, 0x01, OPFORMAT_BRANCH_TYPE  , OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_BGEZ */
    { 0x01, 0x00, 0x11, OPFORMAT_BRANCH_TYPE  , OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_BGEZAL */
    { 0x07, 0x00, 0x00, OPFORMAT_BRANCH_TYPE  , OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_BGTZ */
    { 0x06, 0x00, 0x00, OPFORMAT_BRANCH_TYPE  , OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_BLEZ */
    { 0x01, 0x00, 0x00, OPFORMAT_BRANCH_TYPE  , OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_BLTZ */
    { 0x01, 0x00, 0x10, OPFORMAT_BRANCH_TYPE  , OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_BLTZAL */
    { 0x05, 0x00, 0x00, OPFORMAT_I_BRANCH_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_BNE */
    { 0x02, 0x00, 0x00, OPFORMAT_J_TYPE       , OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_JMP */
    { 0x03, 0x00, 0x00, OPFORMAT_J_TYPE       , OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_JAL */
    { 0x00, 0x08, 0x00, OPFORMAT_REGISTER     , OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_JR */
    /* System call OPCODES */
    { 0x00, 0x0C, 0x00, OPFORMAT_NONE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_SYSCALL */
    /* Memory access OPCODES */
    { 0x20, 0x00, 0x00, OPFORMAT_I_ADDR_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_LB */
    { 0x24, 0x00, 0x00, OPFORMAT_I_ADDR_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_LBU */
    { 0x21, 0x00, 0x00, OPFORMAT_I_ADDR_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_LH */
    { 0x25, 0x00, 0x00, OPFORMAT_I_ADDR_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_LHU */
    { 0x23, 0x00, 0x00, OPFORMAT_I_ADDR_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_LW */
    { 0x28, 0x00, 0x00, OPFORMAT_I_ADDR_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_SB */
    { 0x29, 0x00, 0x00, OPFORMAT_I_ADDR_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_SH */
    { 0x2B, 0x00, 0x00, OPFORMAT_I_ADDR_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_SW */
    /* Immediate operations OPCODES */
    { 0x08, 0x00, 0x00, OPFORMAT_I_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_ADDI */
    { 0x09, 0x00, 0x00, OPFORMAT_I_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_ADDIU */
    { 0x0C, 0x00, 0x00, OPFORMAT_I_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_ANDI */
    { 0x0F, 0x00, 0x00, OPFORMAT_REG_IMM, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_LUI */
    { 0x0D, 0x00, 0x00, OPFORMAT_I_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_ORI */
    { 0x0A, 0x00, 0x00, OPFORMAT_I_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_SLTI */
    { 0x0B, 0x00, 0x00, OPFORMAT_I_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_SLTIU */
    { 0x0E, 0x00, 0x00, OPFORMAT_I_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_XORI */
    /* Psuedo instructions */
    { MNEMONIC_MOVE, 0x00, 0x00, OPFORMAT_REG_REG, OPTYPE_PSUEDO, 0x4 }, /* MNEMONIC_MOVE */
    { MNEMONIC_LI  , 0x00, 0x00, OPFORMAT_REG_IMM, OPTYPE_PSUEDO, 0x8 }, /* MNEMONIC_LI */
    { MNEMONIC_LA  , 0x00, 0x00, OPFORMAT_REG_LAB, OPTYPE_PSUEDO, 0x8 }, /* MNEMONIC_LA */
    { MNEMONIC_NOT , 0x00, 0x00, OPFORMAT_REG_REG, OPTYPE_PSUEDO, 0x4 }, /* MNEMONIC_NOT */
    { MNEMONIC_BEQZ, 0x00, 0x00, OPFORMAT_BRANCH_TYPE, OPTYPE_PSUEDO, 0x4 }, /* MNEMONIC_BEQZ */
    { MNEMONIC_BGE , 0x00, 0x00, OPFORMAT_I_BRANCH_TYPE, OPTYPE_PSUEDO, 0x8 }, /* MNEMONIC_BGE */
    { MNEMONIC_BLE , 0x00, 0x00, OPFORMAT_I_BRANCH_TYPE, OPTYPE_PSUEDO, 0x8 }, /* MNEMONIC_BLE */
    { MNEMONIC_BNEZ, 0x00, 0x00, OPFORMAT_BRANCH_TYPE, OPTYPE_PSUEDO, 0x4 }, /* MNEMONIC_BNEZ */
    { MNEMONIC_BLT, 0x00, 0x00, OPFORMAT_I_BRANCH_TYPE, OPTYPE_PSUEDO, 0x8 },  /* MNEMONIC_BLT */
    { MNEMONIC_BGT, 0x00, 0x00, OPFORMAT_I_BRANCH_TYPE, OPTYPE_PSUEDO, 0x8 },  /* MNEMONIC_BGT */
    /* Division / Multiplication instructions */
    { 0x00, 0x1A, 0x00, OPFORMAT_R_TYPE_OPREG, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_DIV */
    { 0x00, 0x1B, 0x00, OPFORMAT_R_TYPE_OPREG, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_DIVU */
    { 0x00, 0x10, 0x00, OPFORMAT_REGISTER, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_MFHI */
    { 0x00, 0x12, 0x00, OPFORMAT_REGISTER, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_MFLO */
    { 0x00, 0x18, 0x00, OPFORMAT_REG_REG, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_MULT */
    { 0x00, 0x19, 0x00, OPFORMAT_REG_REG, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_MULTU */
    /* Directive instructions */
    { DIRECTIVE_INCLUDE, 0x00, 0x00, OPFORMAT_STRING, OPTYPE_DIRECTIVE, 0x0 }, /* DIRECTIVE_INCLUDE */ 
    { DIRECTIVE_TEXT, 0x00, 0x00, OPFORMAT_NONE, OPTYPE_DIRECTIVE, 0x0 }, /* DIRECTIVE_TEXT */ 
    { DIRECTIVE_DATA, 0x00, 0x00, OPFORMAT_NONE, OPTYPE_DIRECTIVE, 0x0 }, /* DIRECTIVE_DATA */
    { DIRECTIVE_ASCII, 0x00, 0x00, OPFORMAT_STRING, OPTYPE_DIRECTIVE, 0x0 }, /* DIRECTIVE_ASCII */
    { DIRECTIVE_ASCIIZ, 0x00, 0x00, OPFORMAT_STRING, OPTYPE_DIRECTIVE, 0x0 }, /* DIRECTIVE_ASCIIZ */
    { DIRECTIVE_BYTE, 0x00, 0x00, OPFORMAT_IMM_REP, OPTYPE_DIRECTIVE, 0x0 }, /* DIRECTIVE_BYTE */
    { DIRECTIVE_ALIGN, 0x00, 0x00, OPFORMAT_IMMEDIATE, OPTYPE_DIRECTIVE, 0x0 }, /* DIRECTIVE_ALIGN */   
    { DIRECTIVE_HALF, 0x00, 0x00, OPFORMAT_IMM_REP, OPTYPE_DIRECTIVE, 0x0 }, /* DIRECTIVE_HALF */
    { DIRECTIVE_WORD, 0x00, 0x00, OPFORMAT_IMM_LAB_REP, OPTYPE_DIRECTIVE, 0x0 }, /* DIRECTIVE_WORD */
    { DIRECTIVE_KTEXT, 0x00, 0x00, OPFORMAT_NONE, OPTYPE_DIRECTIVE, 0x0 }, /* DIRECTIVE_KTEXT */
    { DIRECTIVE_KDATA, 0x00, 0x00, OPFORMAT_NONE, OPTYPE_DIRECTIVE, 0x0 },  /* DIRECTIVE_KDATA */
    { DIRECTIVE_SPACE, 0x00, 0x00, OPFORMAT_IMMEDIATE, OPTYPE_DIRECTIVE, 0x0 }, /* DIRECTIVE_SPACE */ 
    /* Additional psuedo instructions */
    { 0x1C, 0x02, 0x00, OPFORMAT_R_TYPE, OPTYPE_DEFAULT, 0x0 }, /* MNEMONIC_MUL */
    { MNEMONIC_ABS , 0x00, 0x00, OPFORMAT_REG_REG, OPTYPE_PSUEDO, 0xC }, /* MNEMONIC_ABS */
    { MNEMONIC_NEG , 0x00, 0x00, OPFORMAT_REG_REG, OPTYPE_PSUEDO, 0x4 }, /* MNEMONIC_NEG */
    { MNEMONIC_ROR , 0x00, 0x00, OPFORMAT_I_TYPE, OPTYPE_PSUEDO, 0xC }, /* MNEMONIC_ROR */
    { MNEMONIC_ROL , 0x00, 0x00, OPFORMAT_I_TYPE, OPTYPE_PSUEDO, 0xC }, /* MNEMONIC_ROL */
    { MNEMONIC_SGT , 0x00, 0x00, OPFORMAT_R_TYPE, OPTYPE_PSUEDO, 0x4 }, /* MNEMONIC_SGT */
    { MNEMONIC_B   , 0x00, 0x00, OPFORMAT_J_TYPE, OPTYPE_PSUEDO, 0x4 }, /* MNEMONIC_B */
    { MNEMONIC_SNE , 0x00, 0x00, OPFORMAT_R_TYPE, OPTYPE_PSUEDO, 0x8 }, /* MNEMONIC_SNE */
    { MNEMONIC_BLEU, 0x00, 0x00, OPFORMAT_I_BRANCH_TYPE, OPTYPE_PSUEDO, 0x8 }, /* MNEMONIC_BLEU */
    { MNEMONIC_BGEU, 0x00, 0x00, OPFORMAT_I_BRANCH_TYPE, OPTYPE_PSUEDO, 0x8 }, /* MNEMONIC_BGEU */
    { MNEMONIC_BLTU, 0x00, 0x00, OPFORMAT_I_BRANCH_TYPE, OPTYPE_PSUEDO, 0x8 }, /* MNEMONIC_BLTU */
    { MNEMONIC_BGTU, 0x00, 0x00, OPFORMAT_I_BRANCH_TYPE, OPTYPE_PSUEDO, 0x8 }, /* MNEMONIC_BGTU */
};

const size_t opcode_table_size = sizeof(opcode_table) / sizeof(opcode_table[0]);
