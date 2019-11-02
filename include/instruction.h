#ifndef INSTRUCTION_H
#define INSTRUCTION_H

typedef uint32_t instruction_t;

#define CREATE_INSTRUCTION_R(op, rs, rt, rd, shamt, funct) (0 | (((op) & 0x3F) << 26) | (((rs) & 0x1F) << 21) | (((rt) & 0x1F) << 16) | (((rd) & 0x1F) << 11) | (((shamt) & 0x1F) << 6) | ((funct) & 0x3F))
#define CREATE_INSTRUCTION_I(op, rs, rt, imm) (0 | (((op) & 0x3F) << 26) | (((rs) & 0x1F) << 21) | (((rt) & 0x1F) << 16) | ((imm) & 0xFFFF))

#endif