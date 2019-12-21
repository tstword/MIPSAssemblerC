/**
 * @file: instruction.h
 *
 * @purpose: Contains the necessary macros used for assembling instructions.
 * The header also defines the instruction_t type, used by the assembler to create 32-bit instructions.
 *
 * @author: Bryan Rocha
 * @version: 1.0 (11/2/2019)
 **/

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

typedef uint32_t instruction_t;

#define CREATE_INSTRUCTION_R(op, rs, rt, rd, shamt, funct) (0 | (((op) & 0x3F) << 26) | (((rs) & 0x1F) << 21) | (((rt) & 0x1F) << 16) | (((rd) & 0x1F) << 11) | (((shamt) & 0x1F) << 6) | ((funct) & 0x3F))
#define CREATE_INSTRUCTION_I(op, rs, rt, imm) (0 | (((op) & 0x3F) << 26) | (((rs) & 0x1F) << 21) | (((rt) & 0x1F) << 16) | ((imm) & 0xFFFF))
#define CREATE_INSTRUCTION_J(op, address) (0 | (((op) & 0x3F) << 26) | ((address) & 0x3FFFFFF))

#endif