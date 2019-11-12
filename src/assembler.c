/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * File: parser.c
 *
 * Purpose: Responsible for parsing and verifying the sequence of tokens
 * as a MIPS assembly program. The parser is implemented as a recursive-descent
 * parser, as a result the language is defined as a LL(1) grammer:
 *
 *     program          -> instruction_list
 *
 *     instruction_list -> instruction instruction_list | 
 *                         <EOF>
 *
 *     instruction      -> label <EOL> | 
 *                         label <MNEMONIC> operand_list <EOL>  | 
                           label <DIRECTIVE> operand_list <EOL> |
 *                         <EOL> ; NO-OP
 *
 *     operand_list     -> operand <COMMA> operand_list | 
 *                         operand
 * 
 *     operand          -> <REGISTER>   | 
 *                         <IDENTIFIER> | 
 *                         <INTEGER>    |
 *                         <INTEGER> <LPAREN> <REGISTER> <RPAREN>
 *                    
 *     label            -> <ID> <COLON> | epsilon
 *
 * Failure in parsing the tokens is indicated by the macro ASSEMBLER_STATUS_FAIL
 * Success in parsing the tokens is indicated by the macro ASSEMBLER_STATUS_OK
 * 
 * @author: Bryan Rocha
 * @version: 1.0 (8/28/2019)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

 /* * * * * * * * * * * * * * * * *
  * TO-DO: Right now segments are stored into buffers allocated by the heap. While this isn't an issue for small
  * programs, it can be an issue for larger programs. To support efficiency in memory management unit in the 
  * simulator, the assembler should write the bytes into a file. This will allow us to store the data and text
  * on the hard drive and load in (the page if using VM) of the required section saving on memory space. 
  *
  * Motivation (?): Why load in the entire segments into memory if they aren't being used. Just as a 20GB game would
  * load resources as they are required since all of it cannot fit into RAM, we should only load in the segments as 
  * they are requested by either PC, jump instructions, or load / store instructions.
  *
  * * * * * * * * * * * * * * * * */

#include "assembler.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>

#include "instruction.h"

/* Global variable used for parsing grammar */
struct assembler *cfg_assembler = NULL;

/* Base and limits for segments */
const offset_t segment_offset_base[MAX_SEGMENTS]  = { 
    [SEGMENT_TEXT]  = 0x00400000, [SEGMENT_DATA]  = 0x10000000, 
    [SEGMENT_KTEXT] = 0x80000000, [SEGMENT_KDATA] = 0x90000000 
};

const offset_t segment_offset_limit[MAX_SEGMENTS] = { 
    [SEGMENT_TEXT]  = 0x0FFFFFFF, [SEGMENT_DATA]  = 0x1000FFFF,
    [SEGMENT_KTEXT] = 0x8FFFFFFF, [SEGMENT_KDATA] = 0xFFFEFFFF
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: report_cfg
 * Purpose: Reports an error in the context-free grammar to stderr
 * @param fmt -> Format string
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void report_cfg(const char *fmt, ...) {
    /* Routine to allow formatted printing similar to printf */
    va_list vargs;
    size_t bufsize = 0;
    char *buffer = NULL;

    if(fmt != NULL) {
        va_start(vargs, fmt);
        bufsize = vsnprintf(NULL, 0, fmt, vargs) + 1;
        va_end(vargs);

        buffer = malloc(bufsize * sizeof(char));

        va_start(vargs, fmt);
        vsnprintf(buffer, bufsize, fmt, vargs);
        va_end(vargs);
    }
    /* End routine */

    cfg_assembler->status = ASSEMBLER_STATUS_FAIL;

    /* Check if tokenizer failed, otherwise CFG failed */
    if(cfg_assembler->lookahead == TOK_INVALID) fprintf(stderr, "%s: Error: %s\n", cfg_assembler->tokenizer->filename, cfg_assembler->tokenizer->errmsg);
    else if(buffer != NULL) fprintf(stderr, "%s: Error: %s\n", cfg_assembler->tokenizer->filename, buffer);

    /* Recover and skip to next line to retrieve extra data */
    while(cfg_assembler->lookahead != TOK_EOL && cfg_assembler->lookahead != TOK_NULL) {
        cfg_assembler->lookahead = get_next_token(cfg_assembler->tokenizer);
    }

    /* Free buffer */
    free(buffer);
}

void inc_segment_offset(offset_t offset) {
    offset_t next_offset = cfg_assembler->segment_offset[cfg_assembler->segment] + offset;
    if(next_offset > segment_offset_limit[cfg_assembler->segment]) {
        fprintf(stderr, "Memory Error: Segment '%s' exceeded limit. Base: 0x%08X, Offset: 0x%08X, Limit: 0x%08X\n", 
                segment_string[cfg_assembler->segment], segment_offset_base[cfg_assembler->segment], 
                next_offset, segment_offset_limit[cfg_assembler->segment]);
        cfg_assembler->status = ASSEMBLER_STATUS_FAIL;
    }
    cfg_assembler->segment_offset[cfg_assembler->segment] += offset;
}

void align_segment_offset(uint32_t n) {
    /* Check bounds for sll */
    if(n >= 31) return;

    uint32_t dividend = 1 << n;
    uint32_t remainder = cfg_assembler->segment_offset[cfg_assembler->segment] & (dividend - 1);

    if(remainder != 0) {
        cfg_assembler->segment_offset[cfg_assembler->segment] += dividend - remainder;
    }
}

void write_segment_memory(void *buf, size_t size) {
    segment_t segment = cfg_assembler->segment;
    offset_t buf_offset = cfg_assembler->segment_offset[segment] - segment_offset_base[segment];
    offset_t next_offset = buf_offset + size;
    if(next_offset > cfg_assembler->segment_memory_size[segment]) {
        size_t mem_offset = cfg_assembler->segment_memory_size[segment];
        cfg_assembler->segment_memory_size[segment] += 1024;
        cfg_assembler->segment_memory[segment] = realloc(cfg_assembler->segment_memory[segment], cfg_assembler->segment_memory_size[segment]);
        memset(cfg_assembler->segment_memory[segment] + mem_offset, 0, 1024);
        if(cfg_assembler->segment_memory[segment] == NULL) {
            fprintf(stderr, "Failed to allocate memory for segment: %s\n", strerror(errno));
            cfg_assembler->status = ASSEMBLER_STATUS_FAIL;
            return;
        }
    }
    memcpy(cfg_assembler->segment_memory[segment] + buf_offset, buf, size);
    if(next_offset > cfg_assembler->segment_memory_offset[segment]) {
        cfg_assembler->segment_memory_offset[segment] = next_offset;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: match_cfg
 * Purpose: Checks if the most recent token returned by the tokenizer matches
 * the token passed to the function. On a non-match, it reports an error and fails
 * the parser
 * @param token -> Token to match
 * @return 1 if token matches, 0 otherwise
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int match_cfg(token_t token) {
    if(cfg_assembler->lookahead == token) {
        cfg_assembler->lineno = cfg_assembler->tokenizer->lineno;
        cfg_assembler->colno = cfg_assembler->tokenizer->colno;
        cfg_assembler->lookahead = get_next_token(cfg_assembler->tokenizer);
    } 
    else {
        report_cfg("Expected %s on line %ld, col %ld", get_token_str(token), cfg_assembler->lineno, cfg_assembler->colno);
        return 0;
    }
    return 1;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: end_line_cfg
 * Purpose: Checks and matches for the end of line token, if fails reports
 * specific error to the report_cfg function
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void end_line_cfg() {
    switch(cfg_assembler->lookahead) {
        case TOK_EOL:
            match_cfg(TOK_EOL);
            break;
        case TOK_NULL:
            cfg_assembler->lineno++;
            break;
        default:
            report_cfg("Unexpected %s on line %ld, col %ld", get_token_str(cfg_assembler->lookahead), cfg_assembler->lineno, cfg_assembler->colno);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: label_cfg
 * Purpose: Attempts to match the non-terminal for label. If matched, the label
 * is inserted into the symbol table. Otherwise, an error is reported to the 
 * function report_cfg
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void label_cfg() {
    if(cfg_assembler->lookahead == TOK_IDENTIFIER) {
        char *id = strdup(cfg_assembler->tokenizer->lexbuf);
        match_cfg(TOK_IDENTIFIER);
        if(cfg_assembler->lookahead == TOK_COLON) {
            match_cfg(TOK_COLON);
            struct symbol_table_entry *entry;
            if((entry = get_symbol_table(cfg_assembler->symbol_table, id)) != NULL) {
                if(entry->status != SYMBOL_UNDEFINED) {
                    entry->status = SYMBOL_DOUBLY;
                    report_cfg("Multiple definitions of label '%s' on line %ld, col %ld", id, cfg_assembler->lineno, cfg_assembler->colno);
                } 
                else {
                    entry->offset = cfg_assembler->segment_offset[cfg_assembler->segment];
                    entry->segment = cfg_assembler->segment;
                    entry->status = SYMBOL_DEFINED;
                }
            } 
            else { 
                entry = insert_symbol_table(cfg_assembler->symbol_table, id);
                entry->offset = cfg_assembler->segment_offset[cfg_assembler->segment];
                entry->segment = cfg_assembler->segment;
                entry->status = SYMBOL_DEFINED;
            }
        } 
        else {
            report_cfg("Unrecognized mnemonic '%s' on line %ld, col %ld", id, cfg_assembler->lineno, cfg_assembler->colno);
        }
        free(id);
    } else {
        report_cfg(NULL);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: operand_cfg
 * Purpose: Attempts to match the non-terminal for operand. Failure to match
 * results in reporting the error to report_cfg.
 * @return Address of the operand_node based on operand matched, otherwise NULL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct operand_node *operand_cfg() {
    struct operand_node *node = NULL;

    switch(cfg_assembler->lookahead) {
        case TOK_REGISTER: {
            int value = cfg_assembler->tokenizer->attrval;
            match_cfg(TOK_REGISTER);
            
            node = malloc(sizeof(struct operand_node));
            node->operand = OPERAND_REGISTER;
            node->identifier = NULL;
            node->value.reg = value;
            node->next = NULL;
            
            break;
        }
        case TOK_IDENTIFIER: {
            char *id = strdup(cfg_assembler->tokenizer->lexbuf);
            match_cfg(TOK_IDENTIFIER);
            
            node = malloc(sizeof(struct operand_node));
            node->operand = OPERAND_LABEL;
            node->identifier = id;
            node->next = NULL;
            
            break;
        }
        case TOK_STRING: {
            char *id = strdup(cfg_assembler->tokenizer->lexbuf);
            match_cfg(TOK_STRING);
            
            node = malloc(sizeof(struct operand_node));
            node->operand = OPERAND_STRING;
            node->identifier = id;
            node->next = NULL;
            
            break;
        }
        case TOK_INTEGER: {
            int value = cfg_assembler->tokenizer->attrval;
            match_cfg(TOK_INTEGER);
            
            node = malloc(sizeof(struct operand_node));
            node->operand = OPERAND_IMMEDIATE;
            node->identifier = NULL;
            node->value.integer = value;
            node->next = NULL;
            
            if(cfg_assembler->lookahead == TOK_LPAREN) {
                match_cfg(TOK_LPAREN);
                int reg_value = cfg_assembler->tokenizer->attrval;
                if(match_cfg(TOK_REGISTER) && match_cfg(TOK_RPAREN)) {
                    node->operand = OPERAND_ADDRESS;
                    node->value.reg = reg_value;
                }
            }
            break;
        }
        case TOK_LPAREN: {
            match_cfg(TOK_LPAREN);
            int reg_value = cfg_assembler->tokenizer->attrval;
            if(match_cfg(TOK_REGISTER) && match_cfg(TOK_RPAREN)) {
                node = malloc(sizeof(struct operand_node));
                node->operand = OPERAND_ADDRESS;
                node->value.reg = reg_value;
                node->value.integer = 0;
                node->next = NULL;
            }
            break;
        }
        case TOK_EOL:
        case TOK_NULL:
            report_cfg("Expected operand after line %ld, col %ld", cfg_assembler->lineno, cfg_assembler->colno);
            break;
        default:
            report_cfg("Invalid operand '%s' on line %ld, col %ld", cfg_assembler->tokenizer->lexbuf, cfg_assembler->lineno, cfg_assembler->colno);
    }
    
    return node;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: operand_list_cfg
 * Purpose: Attempts to match the non-terminal for operand_list. Failure to match
 * results in reporting the error to report_cfg.
 * @return Address of the first operand_node in the operand list, otherwise NULL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct operand_node *operand_list_cfg() {
    struct operand_node *root = NULL;
    struct operand_node *current_operand = NULL;

    while(1) {
        switch(cfg_assembler->lookahead) {
            case TOK_REGISTER:
            case TOK_IDENTIFIER:
            case TOK_INTEGER:
            case TOK_STRING:
            case TOK_LPAREN:
                if(current_operand == NULL) {
                    current_operand = operand_cfg();
                    if(root == NULL) root = current_operand;
                }
                else {
                    current_operand->next = operand_cfg();
                    current_operand = current_operand->next;
                }
                if(cfg_assembler->lookahead == TOK_COMMA) {
                    match_cfg(TOK_COMMA);
                    continue;
                }
                else if (cfg_assembler->lookahead == TOK_REGISTER || cfg_assembler->lookahead == TOK_IDENTIFIER 
                            || cfg_assembler->lookahead == TOK_STRING || cfg_assembler->lookahead == TOK_LPAREN
                            || cfg_assembler->lookahead == TOK_INTEGER) continue;
                return root;
            case TOK_EOL:
            case TOK_NULL:
                report_cfg("Expected operand after line %ld, col %ld", cfg_assembler->lineno, cfg_assembler->colno);
                return root;
            default:
                report_cfg("Invalid operand '%s' on line %ld, col %ld", cfg_assembler->tokenizer->lexbuf, cfg_assembler->lineno, cfg_assembler->colno);
                return root;
        }
    }

    return root;
}

int is_symbol_defined(const char *symbol) {
    struct symbol_table_entry *sym_entry = get_symbol_table(cfg_assembler->symbol_table, symbol);
    if(sym_entry == NULL) {
        insert_symbol_table(cfg_assembler->symbol_table, symbol);
        return 0;
    }
    return sym_entry->status != SYMBOL_UNDEFINED;
}
    //  if(sym_entry == NULL) {
    //             insert_front(cfg_assembler->decl_symlist, insert_symbol_table(cfg_assembler->symbol_table, label->identifier));
    //             insert_front(sym_entry->instr_list, (void *)instr);
    //             inc_segment_offset(0x8);
    //             return 0;
    //         }
    //         else {
    //             if(sym_entry->status == SYMBOL_UNDEFINED) {
    //                 insert_front(sym_entry->instr_list, (void *)instr);
    //                 inc_segment_offset(0x8);
    //                 return 0;
    //             }
    //             else {
    //                 instruction = CREATE_INSTRUCTION_I(0x0F, 0, rd->value.reg, (sym_entry->offset >> 16));
    //                 write_segment_memory((void *)&instruction, 0x4);
    //                 inc_segment_offset(0x4);
    //                 instruction = CREATE_INSTRUCTION_I(0x0D, 0, rd->value.reg, sym_entry->offset);
    //                 write_segment_memory((void *)&instruction, 0x4);
    //                 inc_segment_offset(0x4);
    //             }   
    //         }

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: verify_operand_list
 * Purpose: Given an entry in the reserved table, it checks the operand list
 * to see if it matches the corresponding operand format.
 * @param res_entry    -> Address of the entry in the reserved table
 *        operand_list -> Address of the start of the operand list
 * @return 1 if operand list matches operand format, otherwise 0
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int verify_operand_list(struct reserved_entry *res_entry, struct operand_node *operand_list) {
    /* Check if reserved_entry is valid */
    if(res_entry->token != TOK_MNEMONIC && res_entry->token != TOK_DIRECTIVE) return 0;

    /* Verify operand list */
    struct operand_node *current_operand = operand_list;
    struct opcode_entry *entry = (struct opcode_entry *)res_entry->attrptr;

    const char *op_string = res_entry->token == TOK_DIRECTIVE ? "directive" : "mnemonic";

    for(int i = 0; i < MAX_OPERAND_COUNT; ++i) {
        if(entry->operand[i] & OPERAND_REPEAT) {
            if(current_operand == NULL || !(entry->operand[i] & current_operand->operand)) {
                report_cfg("Invalid operand combination for %s '%s' on line %ld", op_string, res_entry->id, cfg_assembler->lineno);
                return 0;
            } 
            if(current_operand->operand & OPERAND_LABEL) {
                if(get_symbol_table(cfg_assembler->symbol_table, current_operand->identifier) == NULL)
                    insert_front(cfg_assembler->decl_symlist, insert_symbol_table(cfg_assembler->symbol_table, current_operand->identifier));
            }
            
            /* Check for more operands in repeat... */
            current_operand = current_operand->next;
            while(current_operand != NULL && entry->operand[i] & current_operand->operand) {
                if(current_operand->operand & OPERAND_LABEL) {
                    if(get_symbol_table(cfg_assembler->symbol_table, current_operand->identifier) == NULL)
                        insert_front(cfg_assembler->decl_symlist, insert_symbol_table(cfg_assembler->symbol_table, current_operand->identifier));
                }
                current_operand = current_operand->next;
            }
        } else {
            if(entry->operand[i] == OPERAND_NONE) {
                if(current_operand != NULL) {
                    report_cfg("Invalid operand combiniation for %s '%s' on line %ld", op_string, res_entry->id, cfg_assembler->lineno);
                    return 0;
                }
                break;
            } else {
                if(current_operand == NULL || !(entry->operand[i] & current_operand->operand)) {
                    report_cfg("Invalid operand combiniation for %s '%s' on line %ld", op_string, res_entry->id, cfg_assembler->lineno);
                    return 0;
                }
                if(current_operand->operand & OPERAND_LABEL) {
                    if(get_symbol_table(cfg_assembler->symbol_table, current_operand->identifier) == NULL)
                        insert_front(cfg_assembler->decl_symlist, insert_symbol_table(cfg_assembler->symbol_table, current_operand->identifier));
                }
            }
            current_operand = current_operand->next;
        }
    }

    return 1;
}

int assemble_psuedo_instruction(struct instruction_node *instr) {
    instruction_t instruction[MAX_INSTRUCTIONS];

    struct opcode_entry *entry = instr->mnemonic->attrptr;
    uint32_t instr_size = entry->size;

    int assemble_status = 1; /* Assume instruction is assembled */

    switch(entry->opcode) {
        case 0x00: {
            struct operand_node *rd = instr->operand_list;
            struct operand_node *rs = instr->operand_list->next;
            instruction[0] = CREATE_INSTRUCTION_R(0, 0, rs->value.reg, rd->value.reg, 0, 0x21);
            break;
        }
        case 0x01: {
            struct operand_node *rd = instr->operand_list;
            struct operand_node *imm = instr->operand_list->next;
            uint32_t immediate = imm->value.integer;
            if(((immediate >> 16) & 0xFFFF) != 0xFFFF && ((immediate >> 16) & 0xFFFF) != 0x0000) {
                /* 32-bit sign extended or unsigned value */
                instruction[0] = CREATE_INSTRUCTION_I(0x0F, 0, 1, (immediate >> 16));
                instruction[1] = CREATE_INSTRUCTION_I(0x0D, 1, rd->value.reg, immediate);
            }
            else {
                instruction[0] = CREATE_INSTRUCTION_I(0x09, 0, rd->value.reg, immediate);
                instr_size = 4;
            }
            break;
        }
        case 0x02: {
            struct operand_node *rd = instr->operand_list;
            struct operand_node *label = instr->operand_list->next;
            /* Check if label has been defined */
            struct symbol_table_entry *sym_entry = get_symbol_table(cfg_assembler->symbol_table, label->identifier);
            if(sym_entry->status == SYMBOL_UNDEFINED) {
                insert_front(sym_entry->instr_list, (void *)instr);
                assemble_status = 0;
            }
            else {
                instruction[0] = CREATE_INSTRUCTION_I(0x0F, 0, 1, (sym_entry->offset >> 16));
                instruction[1] = CREATE_INSTRUCTION_I(0x0D, 1, rd->value.reg, sym_entry->offset);
            }   
            break;
        }
        case 0x03: {
            struct operand_node *rd = instr->operand_list;
            struct operand_node *rs = instr->operand_list->next;
            instruction[0] = CREATE_INSTRUCTION_R(0, rs->value.reg, 0, rd->value.reg, 0, 0x27);
            break; 
        }
        case 0x04: {
            struct operand_node *rs = instr->operand_list;
            struct operand_node *label = instr->operand_list->next;
            /* Check if label has been defined */
            struct symbol_table_entry *sym_entry = get_symbol_table(cfg_assembler->symbol_table, label->identifier);
            if(sym_entry->status == SYMBOL_UNDEFINED) {
                insert_front(sym_entry->instr_list, (void *)instr);
                assemble_status = 0;
            }
            else {
                offset_t branch_offset = (sym_entry->offset - (cfg_assembler->segment_offset[cfg_assembler->segment] + 4)) >> 2;
                instruction[0] = CREATE_INSTRUCTION_I(0x04, rs->value.reg, 0, branch_offset);
            }   
            break;
        }
        case 0x05: {
            struct operand_node *rs = instr->operand_list;
            struct operand_node *rt = instr->operand_list->next;
            struct operand_node *label = instr->operand_list->next->next;
            /* Check if label has been defined */
            struct symbol_table_entry *sym_entry = get_symbol_table(cfg_assembler->symbol_table, label->identifier);
            if(sym_entry->status == SYMBOL_UNDEFINED) {
                insert_front(sym_entry->instr_list, (void *)instr);
                assemble_status = 0;
            }
            else {
                instruction[0] = CREATE_INSTRUCTION_R(0, rs->value.reg, rt->value.reg, 1, 0, 0x2A);
                offset_t branch_offset = (sym_entry->offset - (cfg_assembler->segment_offset[cfg_assembler->segment] + 8)) >> 2;
                instruction[1] = CREATE_INSTRUCTION_I(0x04, 1, 0, branch_offset); 
            }
            break;
        }
        case 0x06: {
            struct operand_node *rs = instr->operand_list;
            struct operand_node *rt = instr->operand_list->next;
            struct operand_node *label = instr->operand_list->next->next;
            /* Check if label has been defined */
            struct symbol_table_entry *sym_entry = get_symbol_table(cfg_assembler->symbol_table, label->identifier);
            if(sym_entry->status == SYMBOL_UNDEFINED) {
                insert_front(sym_entry->instr_list, (void *)instr);
                assemble_status = 0;
            }
            else {
                instruction[0] = CREATE_INSTRUCTION_R(0, rt->value.reg, rs->value.reg, 1, 0, 0x2A);
                offset_t branch_offset = (sym_entry->offset - (cfg_assembler->segment_offset[cfg_assembler->segment] + 8)) >> 2;
                instruction[1] = CREATE_INSTRUCTION_I(0x04, 1, 0, branch_offset);
            }
            break;
        }
        case 0x07: {
            struct operand_node *rs = instr->operand_list;
            struct operand_node *label = instr->operand_list->next;
            /* Check if label has been defined */
            struct symbol_table_entry *sym_entry = get_symbol_table(cfg_assembler->symbol_table, label->identifier);
            if(sym_entry->status == SYMBOL_UNDEFINED) {
                insert_front(sym_entry->instr_list, (void *)instr);
                assemble_status = 0;
            }
            else {
                offset_t branch_offset = (sym_entry->offset - (cfg_assembler->segment_offset[cfg_assembler->segment] + 4)) >> 2;
                instruction[0] = CREATE_INSTRUCTION_I(0x05, rs->value.reg, 0, branch_offset);  
            }
            break;
        }
        case 0x08: {
            struct operand_node *rs = instr->operand_list;
            struct operand_node *rt = instr->operand_list->next;
            struct operand_node *label = instr->operand_list->next->next;
            /* Check if label has been defined */
            struct symbol_table_entry *sym_entry = get_symbol_table(cfg_assembler->symbol_table, label->identifier);
            if(sym_entry->status == SYMBOL_UNDEFINED) {
                insert_front(sym_entry->instr_list, (void *)instr);
                assemble_status = 0;
            }
            else {
                instruction[0] = CREATE_INSTRUCTION_R(0, rs->value.reg, rt->value.reg, 1, 0, 0x2A);
                offset_t branch_offset = (sym_entry->offset - (cfg_assembler->segment_offset[cfg_assembler->segment] + 8)) >> 2;
                instruction[1] = CREATE_INSTRUCTION_I(0x05, 1, 0, branch_offset);
            }
            break;
        }
        case 0x09: {
            struct operand_node *rs = instr->operand_list;
            struct operand_node *rt = instr->operand_list->next;
            struct operand_node *label = instr->operand_list->next->next;
            /* Check if label has been defined */
            struct symbol_table_entry *sym_entry = get_symbol_table(cfg_assembler->symbol_table, label->identifier);
            if(sym_entry->status == SYMBOL_UNDEFINED) {
                insert_front(sym_entry->instr_list, (void *)instr);
                assemble_status = 0;
            }
            else {
                instruction[0] = CREATE_INSTRUCTION_R(0, rt->value.reg, rs->value.reg, 1, 0, 0x2A);
                offset_t branch_offset = (sym_entry->offset - (cfg_assembler->segment_offset[cfg_assembler->segment] + 8)) >> 2;
                instruction[1] = CREATE_INSTRUCTION_I(0x05, 1, 0, branch_offset); 
            }
            break;
        }
    }

    if(assemble_status) write_segment_memory((void *)instruction, instr_size);
    inc_segment_offset(instr_size);

    return assemble_status;
}

int assemble_funct_instruction(struct instruction_node *instr) {
    instruction_t instruction = 0;

    struct opcode_entry *entry = instr->mnemonic->attrptr;

    switch(entry->funct) {
        case 0x21:
        case 0x24:
        case 0x27:
        case 0x25:
        case 0x2A:
        case 0x2B:
        case 0x22:
        case 0x23:
        case 0x26:
        case 0x20: {
            struct operand_node *rd = instr->operand_list;
            struct operand_node *rs = instr->operand_list->next;
            struct operand_node *rt = instr->operand_list->next->next;
            instruction = CREATE_INSTRUCTION_R(0, rs->value.reg, rt->value.reg, rd->value.reg, 0, entry->funct);
            break; 
        }

        case 0x08: {
            struct operand_node *rs = instr->operand_list;
            instruction = CREATE_INSTRUCTION_R(0, rs->value.reg, 0, 0, 0, entry->funct);
            break;
        }

        case 0x10:
        case 0x12: {
            struct operand_node *rd = instr->operand_list;
            instruction = CREATE_INSTRUCTION_R(0, 0, 0, rd->value.reg, 0, entry->funct);
            break;
        }

        case 0x00:
        case 0x03:
        case 0x02: {
            struct operand_node *rd = instr->operand_list;
            struct operand_node *rt = instr->operand_list->next;
            struct operand_node *shamt = instr->operand_list->next->next;
            instruction = CREATE_INSTRUCTION_R(0, 0, rt->value.reg, rd->value.reg, shamt->value.integer, entry->funct);
            break;
        }

        case 0x0C:
            instruction = CREATE_INSTRUCTION_R(0, 0, 0, 0, 0, entry->funct);
            break;

        case 0x1A:
        case 0x1B:
        case 0x18:
        case 0x19: {
            struct operand_node *rs = instr->operand_list;
            struct operand_node *rt = instr->operand_list->next;
            instruction = CREATE_INSTRUCTION_R(0, rs->value.reg, rt->value.reg, 0, 0, entry->funct);
            break;
        }
    }

    /* No branch instructions here, instructions will always be assembled */
    write_segment_memory((void *)&instruction, 0x4);
    inc_segment_offset(0x4);

    return 1;
}

int assemble_opcode_instruction(struct instruction_node *instr) {
    instruction_t instruction = 0;

    struct opcode_entry *entry = instr->mnemonic->attrptr;
    int assemble_status = 1; /* Assume instruction is assembled */

    switch(entry->opcode) {
        case 0x08:
        case 0x09:
        case 0x0C:
        case 0x0D:
        case 0x0A:
        case 0x0B:
        case 0x0E: { /* ALU OP */
            struct operand_node *rt = instr->operand_list;
            struct operand_node *rs = instr->operand_list->next;
            struct operand_node *imm = instr->operand_list->next->next;
            instruction = CREATE_INSTRUCTION_I(entry->opcode, rs->value.reg, rt->value.reg, imm->value.integer);
            break;
        }

        case 0x0F: {
            struct operand_node *rd = instr->operand_list;
            struct operand_node *imm = instr->operand_list->next;
            uint32_t immediate = imm->value.integer;
            instruction = CREATE_INSTRUCTION_I(entry->opcode, 0, rd->value.reg, immediate);
            break;
        }

        case 0x01:
        case 0x07:
        case 0x06: {
            struct operand_node *rs = instr->operand_list;
            struct operand_node *label = instr->operand_list->next;
            /* Check if label has been defined */
            struct symbol_table_entry *sym_entry = get_symbol_table(cfg_assembler->symbol_table, label->identifier);
            if(sym_entry->status == SYMBOL_UNDEFINED) {
                insert_front(sym_entry->instr_list, (void *)instr);
                assemble_status = 0;
            }
            else {
                offset_t branch_offset = (sym_entry->offset - (cfg_assembler->segment_offset[cfg_assembler->segment] + 4)) >> 2;
                instruction = CREATE_INSTRUCTION_I(entry->opcode, rs->value.reg, entry->rt, branch_offset);
            }   
            break;
        }

        case 0x05:
        case 0x04: {
            struct operand_node *rs = instr->operand_list;
            struct operand_node *rt = instr->operand_list->next;
            struct operand_node *label = instr->operand_list->next->next;
            /* Check if label has been defined */
            struct symbol_table_entry *sym_entry = get_symbol_table(cfg_assembler->symbol_table, label->identifier);
            if(sym_entry->status == SYMBOL_UNDEFINED) {
                insert_front(sym_entry->instr_list, (void *)instr);
                assemble_status = 0;
            }
            else {
                offset_t branch_offset = (sym_entry->offset - (cfg_assembler->segment_offset[cfg_assembler->segment] + 4)) >> 2;
                instruction = CREATE_INSTRUCTION_I(entry->opcode, rs->value.reg, rt->value.reg, branch_offset);
            }
            break;
        }

        case 0x02:
        case 0x03: {
            struct operand_node *label = instr->operand_list;
            /* Check if label has been defined */
            struct symbol_table_entry *sym_entry = get_symbol_table(cfg_assembler->symbol_table, label->identifier);
            if(sym_entry->status == SYMBOL_UNDEFINED) {
                insert_front(sym_entry->instr_list, (void *)instr);
                assemble_status = 0;
            }
            else {
                instruction = CREATE_INSTRUCTION_J(entry->opcode, (sym_entry->offset >> 2));   
            }
            break;
        }
        

        case 0x20:
        case 0x24:
        case 0x21:
        case 0x25:
        case 0x23: 
        case 0x28: 
        case 0x29:
        case 0x2B: {
            struct operand_node *rt = instr->operand_list;
            struct operand_node *addr = instr->operand_list->next;
            instruction = CREATE_INSTRUCTION_I(entry->opcode, addr->value.reg, rt->value.reg, addr->value.integer);
            break;
        }
    }

    if(assemble_status) write_segment_memory((void *)&instruction, 0x4);
    inc_segment_offset(0x4);

    return assemble_status;
}

void destroy_instruction(struct instruction_node *instr) {
    /* Free operands */
    struct operand_node *op_node = instr->operand_list;
    while(op_node != NULL) {
        struct operand_node *next_op = op_node->next;
        if(op_node->operand == OPERAND_LABEL || op_node->operand == OPERAND_STRING) free(op_node->identifier);
        free(op_node);
        op_node = next_op;
    }
    free(instr);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: assemble_instruction
 * Purpose: Checks the instruction_node to see if a proper instruction was 
 * recognized based on the opcode table entry for the mnemonic. If an invalid
 * instruction is encountered, it will report the error to report_cfg.
 * @param instr -> Address of the instruction node structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int assemble_instruction(struct instruction_node *instr) {
    if(instr == NULL || instr->mnemonic == NULL) return 0;

    if(!verify_operand_list(instr->mnemonic, instr->operand_list)) {
        destroy_instruction(instr);
        return 0;
    }

    int assemble_status;

    /* TO-DO: Assemble (?) instruction */
    if(cfg_assembler->segment == SEGMENT_DATA) {
        fprintf(stderr, "Cannot define instructions in .data segment on line %ld\n", cfg_assembler->lineno);
        cfg_assembler->status = ASSEMBLER_STATUS_FAIL;
        destroy_instruction(instr);
        return 0;
    }

    struct opcode_entry *entry = instr->mnemonic->attrptr;

    if(entry->type == OPTYPE_PSUEDO) {
        assemble_status = assemble_psuedo_instruction(instr);
    }
    else if(entry->opcode == 0x00) {
        assemble_status = assemble_funct_instruction(instr);     
    }
    else {
        assemble_status = assemble_opcode_instruction(instr); 
    }

    return assemble_status;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: check_directive
 * Purpose: Checks the directive recognized from the CFG. Ensures that the
 * operands recognized are of the correct format. If successfully recognized,
 * the function attempts to execute the directive.
 * @param directive    -> Address of the entry in the reserved table
 *        operand_list -> Address of the first operand in the operand list
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int check_directive(struct instruction_node *instr) {
    struct reserved_entry *directive = instr->mnemonic;
    struct operand_node *operand_list = instr->operand_list;

    /* Check if entry is directive */
    if(directive->token != TOK_DIRECTIVE) return 0;

    struct opcode_entry *entry = (struct opcode_entry *)directive->attrptr;

    int assemble_status = 1; /* Assume assembled directive */

    if(!verify_operand_list(directive, operand_list)) {
        destroy_instruction(instr);
        return 0;
    }

    /* Check if segment is text before handling directive */
    switch(entry->opcode) {
        case DIRECTIVE_ASCII:
        case DIRECTIVE_ASCIIZ:
        case DIRECTIVE_HALF:
        case DIRECTIVE_BYTE:
            if(cfg_assembler->segment != SEGMENT_DATA) {
                fprintf(stderr, "Directive '%s' is not allowed in the .text segment on line %ld\n", directive->id, cfg_assembler->lineno);
                cfg_assembler->status = ASSEMBLER_STATUS_FAIL;
                destroy_instruction(instr);
                return 0;
            }
            break;
    }

    switch(entry->opcode) {
        case DIRECTIVE_INCLUDE: {
            /* Create tokenizer structure */
            struct tokenizer *tokenizer = create_tokenizer(operand_list->identifier);
            if(tokenizer == NULL) {
                fprintf(stderr, "Failed to include file '%s' on line %ld : %s\n", operand_list->identifier, cfg_assembler->lineno, strerror(errno));
                cfg_assembler->status = ASSEMBLER_STATUS_FAIL;
                destroy_instruction(instr);
                assemble_status = 0;
            } 
            else {
                insert_front(cfg_assembler->tokenizer_list, (void *)tokenizer);
                cfg_assembler->tokenizer = tokenizer;
                cfg_assembler->lookahead = get_next_token(tokenizer);
            }
            break;
        }
        case DIRECTIVE_TEXT: 
            cfg_assembler->segment = SEGMENT_TEXT;
            break;
        case DIRECTIVE_DATA:
            cfg_assembler->segment = SEGMENT_DATA;
            break;
        case DIRECTIVE_KTEXT:
            cfg_assembler->segment = SEGMENT_KTEXT;
            break;
        case DIRECTIVE_KDATA:
            cfg_assembler->segment = SEGMENT_KDATA;
            break;
        case DIRECTIVE_ALIGN: {
            if(operand_list->value.integer < 0 || operand_list->value.integer >= 31) {
                fprintf(stderr, "Directive '.align n' expects n to be within the range of [0, 31] on line %ld\n", cfg_assembler->lineno);
                cfg_assembler->status = ASSEMBLER_STATUS_FAIL;
                destroy_instruction(instr);
                assemble_status = 0;
            }
            else if(operand_list->value.integer == 0) {
                /* TO-DO: Disable automatic alignment of .half, .word, directives until next .data segment */
            }
            else {
                align_segment_offset(operand_list->value.integer);
            }
            break;
        }
        case DIRECTIVE_WORD: {
            struct operand_node *current_operand = operand_list;
            while(current_operand != NULL) {
                if(current_operand->operand & OPERAND_LABEL) {
                    /* Check if label has been defined */
                    struct symbol_table_entry *sym_entry = get_symbol_table(cfg_assembler->symbol_table, current_operand->identifier);
                    if(sym_entry->status == SYMBOL_UNDEFINED) {
                        insert_front(sym_entry->instr_list, (void *)instr);
                        assemble_status = 0;
                    }
                    else {
                        offset_t sym_offset = sym_entry->offset;
                        write_segment_memory((void *)&sym_offset, 0x4);
                        inc_segment_offset(0x4);   
                    }
                }
                else {
                    int value = current_operand->value.integer;
                    write_segment_memory((void *)&value, 0x4);
                    inc_segment_offset(0x4);
                }
                current_operand = current_operand->next;
            }
            break;
        }
        case DIRECTIVE_HALF: {
            struct operand_node *current_operand = operand_list;
            while(current_operand != NULL) {
                int value = current_operand->value.integer;
                write_segment_memory((void *)&value, 0x2);
                inc_segment_offset(0x2);
                current_operand = current_operand->next;
            }
            break;
        }
        case DIRECTIVE_BYTE: {
            struct operand_node *current_operand = operand_list;
            while(current_operand != NULL) {
                int value = current_operand->value.integer;
                write_segment_memory((void *)&value, 0x1);
                inc_segment_offset(0x1);
                current_operand = current_operand->next;
            }
            break;
        }
        case DIRECTIVE_ASCII: {
            offset_t length = strlen(operand_list->identifier);
            write_segment_memory((void *)operand_list->identifier, length);
            inc_segment_offset(length);
            break;
        }
        case DIRECTIVE_ASCIIZ: {
            offset_t length = strlen(operand_list->identifier) + 1;
            write_segment_memory((void *)operand_list->identifier, length);
            inc_segment_offset(length);
            break;
        }
    }

    return assemble_status;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: instruction_cfg
 * Purpose: Attempts to match the non-terminal for instruction. If failed to match
 * reports error to report_cfg
 * @return Address of the instruction_node if valid, otherwise NULL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct instruction_node *instruction_cfg() {
    struct instruction_node *node = NULL;

    if(cfg_assembler->lookahead == TOK_IDENTIFIER) label_cfg();

    switch(cfg_assembler->lookahead) {
        case TOK_DIRECTIVE: {
            node = malloc(sizeof(struct instruction_node));
            node->mnemonic = cfg_assembler->tokenizer->attrptr;
            node->offset = cfg_assembler->segment_offset[cfg_assembler->segment];
            node->segment = cfg_assembler->segment;
            node->next = NULL;

            match_cfg(TOK_DIRECTIVE);
            switch(cfg_assembler->lookahead) {
                case TOK_IDENTIFIER:
                case TOK_INTEGER:
                case TOK_STRING:
                    node->operand_list = operand_list_cfg();
                    break;
                default:
                    node->operand_list = NULL;
            }

            if(check_directive(node))
                destroy_instruction(node);

            end_line_cfg();

            break;
        }
        case TOK_MNEMONIC:
            node = malloc(sizeof(struct instruction_node));
            node->mnemonic = cfg_assembler->tokenizer->attrptr;
            node->offset = cfg_assembler->segment_offset[cfg_assembler->segment];
            node->segment = cfg_assembler->segment;
            node->next = NULL;
            
            match_cfg(TOK_MNEMONIC);
            switch(cfg_assembler->lookahead) {
                case TOK_IDENTIFIER:
                case TOK_INTEGER:
                case TOK_REGISTER:
                    node->operand_list = operand_list_cfg();
                    break;
                default:
                    node->operand_list = NULL;
            }

            if(assemble_instruction(node))
                destroy_instruction(node);

            end_line_cfg();

            break;
        case TOK_EOL:
        case TOK_NULL:
            end_line_cfg();
            break;
        default:
            report_cfg("Unexpected %s on line %ld, col %ld", get_token_str(cfg_assembler->lookahead), cfg_assembler->lineno, cfg_assembler->colno);
    }

    return node;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: instruction_list_cfg
 * Purpose: Attempts to match the non-terminal for instruction_list. Failure to match
 * results in reporting the error to report_cfg
 * @return Address of the first instruction_node in the instruction list, otherwise NULL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void instruction_list_cfg() {  
    while(1) {  
        while(cfg_assembler->lookahead == TOK_NULL) {
            /* Free current tokenizer */
            destroy_tokenizer(&cfg_assembler->tokenizer);
            remove_front(cfg_assembler->tokenizer_list, LN_VSTATIC);

            /* No more files to process */
            if(cfg_assembler->tokenizer_list->front == NULL) return;  
            
            /* Setup tokenizer */
            cfg_assembler->tokenizer = cfg_assembler->tokenizer_list->front->value;

            /* Setup lookahead */
            cfg_assembler->lookahead = get_next_token(cfg_assembler->tokenizer);
        }
    
        if(cfg_assembler->lookahead != TOK_NULL) {
            instruction_cfg();
        }
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: program_cfg
 * Purpose: Start symbol the the LL(1) context-free grammer
 * @return Address of the allocated program_node
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void program_cfg(struct assembler *assembler) {
    cfg_assembler = assembler;

    instruction_list_cfg();
    
    /* Verify undefined symbol table */
    for(struct list_node *head = assembler->decl_symlist->front; head != NULL; head = head->next) {
        struct symbol_table_entry *sym_entry = (struct symbol_table_entry *)head->value;
        symstat_t status = sym_entry->status;
        if(status == SYMBOL_UNDEFINED) {
            /* Symbol is still undefined, program cannot be assembled */
            fprintf(stderr, "Symbol Error: Undefined symbol '%s'\n", ((struct symbol_table_entry *)head->value)->key);
            struct list_node *instr_ref = sym_entry->instr_list->front;
            while(instr_ref != NULL) {
                destroy_instruction(instr_ref->value);
                instr_ref = instr_ref->next;
            }
            assembler->status = ASSEMBLER_STATUS_FAIL;
        } else {
            struct list_node *instr_ref = sym_entry->instr_list->front;
            while(instr_ref != NULL) {
                if(((struct instruction_node *)instr_ref->value)->mnemonic->token == TOK_MNEMONIC) {
                    assembler->segment = ((struct instruction_node *)instr_ref->value)->segment;
                    assembler->segment_offset[assembler->segment] = ((struct instruction_node *)instr_ref->value)->offset;
                    assemble_instruction(instr_ref->value); 
                    destroy_instruction(instr_ref->value);
                }
                else if(((struct instruction_node *)instr_ref->value)->mnemonic->token == TOK_DIRECTIVE) {
                    assembler->segment = ((struct instruction_node *)instr_ref->value)->segment;
                    assembler->segment_offset[assembler->segment] = ((struct instruction_node *)instr_ref->value)->offset;
                    check_directive(instr_ref->value); 
                    destroy_instruction(instr_ref->value);
                }
                instr_ref = instr_ref->next;
            }
        }
    }
}

void destroy_tokenizer_list(struct assembler *assembler) {
    struct list_node *node = assembler->tokenizer_list->front;
    while(node != NULL) {
        destroy_tokenizer((struct tokenizer **)&node->value);
        node = node->next;
    }
    delete_linked_list(&(assembler->tokenizer_list), LN_VSTATIC);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: create_parser
 * Purpose: Allocates and initializes the parser structure 
 * @return Pointer to the allocated parser structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct assembler *create_assembler() {
    struct assembler *assembler = malloc(sizeof(struct assembler));

    assembler->tokenizer = NULL;
    assembler->tokenizer_list = NULL;

    assembler->lookahead = TOK_NULL;
    assembler->decl_symlist = NULL;
    assembler->status = ASSEMBLER_STATUS_NULL;
    assembler->lineno = 1;
    assembler->colno = 1;

    assembler->segment = SEGMENT_TEXT;
    
    for(segment_t segment = 0; segment < MAX_SEGMENTS; ++segment) {
        assembler->segment_offset[segment] = segment_offset_base[segment];
        assembler->segment_memory[segment] = NULL;
        assembler->segment_memory_offset[segment] = 0;
        assembler->segment_memory_size[segment] = 0;
    }
    
    return assembler;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: execute_parser
 * Purpose: Starts the parsing process. Sets up the lookahead token
 * required for the LL(1) grammer and verifies symbol table after execution. 
 * @param assembler -> Address of the assembler structure
 * @return ASSEMBLER_STATUS_OK if no errors, otherwise ASSEMBLER_STATUS_FAIL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
astatus_t execute_assembler(struct assembler *assembler, const char **files, size_t size) {
    /* Setup Tokenizer List */
    assembler->tokenizer_list = create_list();
    for(size_t i = 0; i < size; ++i) {
        struct tokenizer *tokenizer = create_tokenizer(files[i]);
        if(tokenizer == NULL) {
            fprintf(stderr, "%s: Error: %s\n", files[i], strerror(errno));
            assembler->status = ASSEMBLER_STATUS_FAIL;
            destroy_tokenizer_list(assembler);
            return assembler->status;
        }
        insert_rear(assembler->tokenizer_list, (void *)tokenizer);
    }

    /* Setup initial tokenizer structure */
    if(assembler->tokenizer_list->front == NULL) {
        fprintf(stderr, "Input: No source files to assemble\n");
        assembler->status = ASSEMBLER_STATUS_FAIL;
        destroy_tokenizer_list(assembler);
        return assembler->status;
    }

    /* Setup tokenizer */
    assembler->tokenizer = (struct tokenizer *)assembler->tokenizer_list->front->value;

    /* Setup lookahead */
    assembler->lookahead = get_next_token(assembler->tokenizer);

    /* Setup Symbol Table */
    assembler->symbol_table = create_symbol_table();

    /* Setup declared symbol list */
    assembler->decl_symlist = create_list();

    /* Default segment is SEGMENT_TEXT */
    assembler->segment = SEGMENT_TEXT;

    /* Setup segment / memory offsets */
    for(segment_t segment = 0; segment < MAX_SEGMENTS; ++segment) {
        assembler->segment_offset[segment] = segment_offset_base[segment];
        assembler->segment_memory_offset[segment] = 0;
    }

    /* Default is ASSEMBLER_STATUS_OK */
    assembler->status = ASSEMBLER_STATUS_OK;

    /* Start grammar recognization... */
    program_cfg(assembler);

    #ifdef DEBUG
    if(assembler->status == ASSEMBLER_STATUS_OK) {
        for(segment_t segment = 0; segment < MAX_SEGMENTS; ++segment) {
            if(assembler->segment_memory_offset[segment] == 0) continue;
            // printf("[ * Memory Segment %-4s * ]", segment_string[segment]);
            for(int i = 0; i < assembler->segment_memory_offset[segment]; i+=4) {
                // if((i & (0x3)) == 0) printf("\n0x%08X  ", segment_offset_base[segment] + i);
                printf("%08x\n", *((instruction_t *)(assembler->segment_memory[segment] + i)));
                // unsigned char c = *((unsigned char *)assembler->segment_memory[segment] + i);
                // printf("\\%02X ", c);
            }
            //printf("\n\n");
        }
    }
    //print_symbol_table(assembler->symbol_table);
    #endif

    /* Destory tokenizer list */
    destroy_tokenizer_list(assembler);

    /* Destroy symbol table */
    destroy_symbol_table(&assembler->symbol_table);

    /* Destroy declared symbol list */
    delete_linked_list(&assembler->decl_symlist, LN_VSTATIC);

    return assembler->status;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: destroy_symbol
 * Purpose: Deallocates all the dynamically allocated memory used for the
 * assembler structure
 * @param assembler -> Reference to the address of the assembler structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void destroy_assembler(struct assembler **assembler) {
    /* Free all segment memory */
    for(segment_t segment = 0; segment < MAX_SEGMENTS; ++segment) {
        free((*assembler)->segment_memory[segment]);
    }

    /* Simple free the data */
    free(*assembler);

    /* Set assembler to NULL */
    *assembler = NULL;
}
