#include "opcode.h"

struct opcode_entry opcode_table[] = {
    /* ALU Core OPCODES */
    { 0x00, 0x20, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_ADD */
    { 0x00, 0x21, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_ADDU */
    { 0x00, 0x24, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_AND */
    { 0x00, 0x27, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_NOR */
    { 0x00, 0x25, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_OR */
    { 0x00, 0x2A, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_SLT */
    { 0x00, 0x2B, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_SLTU */
    { 0x00, 0x22, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_SUB */
    { 0x00, 0x23, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_SUBU */
    { 0x00, 0x26, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_XOR */
    /* Shifters OPCODES */
    { 0x00, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_SLL */
    { 0x00, 0x03, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_SRA */
    { 0x00, 0x02, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_SRL */
    /* Branches OPCODES */
    { 0x04, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_BEQ */
    { 0x01, 0x00, 0x01, OPTYPE_DEFAULT }, /* MNEMONIC_BGEZ */
    { 0x01, 0x00, 0x11, OPTYPE_DEFAULT }, /* MNEMONIC_BGEZAL */
    { 0x07, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_BGTZ */
    { 0x06, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_BLEZ */
    { 0x01, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_LTZ */
    { 0x01, 0x00, 0x10, OPTYPE_DEFAULT }, /* MNEMONIC_BLTZAL */
    { 0x05, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_BNE */
    { 0x02, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_JMP */
    { 0x03, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_JAL */
    { 0x00, 0x08, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_JR */
    /* System call OPCODES */
    { 0x00, 0x00, 0x0C, OPTYPE_DEFAULT }, /* MNEMONIC_SYSCALL */
    /* Memory access OPCODES */
    { 0x20, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_LB */
    { 0x24, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_LBU */
    { 0x21, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_LH */
    { 0x25, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_LHU */
    { 0x23, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_LW */
    { 0x28, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_SB */
    { 0x29, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_SH */
    { 0x2B, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_SW */
    /* Immediate operations OPCODES */
    { 0x08, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_ADDI */
    { 0x09, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_ADDIU */
    { 0x0C, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_ANDI */
    { 0x0F, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_LUI */
    { 0x0D, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_ORI */
    { 0x0A, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_SLTI */
    { 0x0B, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_SLTIU */
    { 0x0E, 0x00, 0x00, OPTYPE_DEFAULT }, /* MNEMONIC_XORI */
    /* Psuedo instructions */
    { 0x00, 0x00, 0x00, OPTYPE_PSUEDO  }, /* MNEMONIC_MOVE */
    { 0x01, 0x00, 0x00, OPTYPE_PSUEDO  }, /* MNEMONIC_LI */
    { 0x02, 0x00, 0x00, OPTYPE_PSUEDO  }, /* MNEMONIC_LA */
    { 0x03, 0x00, 0x00, OPTYPE_PSUEDO  }  /* MNEMONIC_NOT */
};

const size_t opcode_table_size = sizeof(opcode_table) / sizeof(opcode_table[0]);