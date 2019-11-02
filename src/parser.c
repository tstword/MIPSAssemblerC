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
 * Failure in parsing the tokens is indicated by the macro PARSER_STATUS_FAIL
 * Success in parsing the tokens is indicated by the macro PARSER_STATUS_OK
 * 
 * @author: Bryan Rocha
 * @version: 1.0 (8/28/2019)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

 /* * * * * * * * * * * * * * * * *
  * TO-DO: The different segment offsets and memory locations make the code confusing to read. Instead
  * have #define SEGMENT_TEXT 0x0 and #define SEGMENT_DATA 0x1 and make arrays for the offset, memory
  * location and sizes 
  * * * * * * * * * * * * * * * * */

#include "parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>

#include "instruction.h"

/* Global variable used for parsing grammar */
struct parser *cfg_parser = NULL;

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

    cfg_parser->status = PARSER_STATUS_FAIL;

    /* Check if tokenizer failed, otherwise CFG failed */
    if(cfg_parser->lookahead == TOK_INVALID) fprintf(stderr, "%s: Error: %s\n", cfg_parser->tokenizer->filename, cfg_parser->tokenizer->errmsg);
    else if(buffer != NULL) fprintf(stderr, "%s: Error: %s\n", cfg_parser->tokenizer->filename, buffer);

    /* Recover and skip to next line to retrieve extra data */
    while(cfg_parser->lookahead != TOK_EOL && cfg_parser->lookahead != TOK_NULL) {
        cfg_parser->lookahead = get_next_token(cfg_parser->tokenizer);
    }

    /* Free buffer */
    free(buffer);
}

void inc_segment_offset(offset_t offset) {
    if(cfg_parser->segment == SEGMENT_TEXT) {
        cfg_parser->segtext_offset += offset;
    }
    else if(cfg_parser->segment == SEGMENT_DATA) {
        cfg_parser->segdata_offset += offset;
    }
}

void align_segment_offset(int n) {
    /* Check bounds for sll */
    if(n < 0 || n >= 31) return;

    unsigned int divend = 1 << n;

    if(cfg_parser->segment == SEGMENT_TEXT) {
        if(cfg_parser->segtext_offset & (divend - 1)) {
            cfg_parser->segtext_offset += divend - (cfg_parser->segtext_offset & (divend - 1));
        }
    }  
    else if (cfg_parser->segment == SEGMENT_DATA) {
        if(cfg_parser->segdata_offset & (divend - 1)) {
            cfg_parser->segdata_offset += divend - (cfg_parser->segdata_offset & (divend - 1));
        }
    }
}

void write_segment_memory(void *buf, size_t size) {
    if(cfg_parser->segment == SEGMENT_TEXT) {
        if(cfg_parser->segtext_offset + size > cfg_parser->mem_segtext_size) {
            cfg_parser->mem_segtext_size += 1024;
            cfg_parser->mem_segtext = realloc(cfg_parser->mem_segtext, cfg_parser->mem_segtext_size);
            if(cfg_parser->mem_segtext == NULL) {
                fprintf(stderr, "Segment Text Memory Error: %s\n", strerror(errno));
                return;
            }
        }
        memcpy(cfg_parser->mem_segtext + cfg_parser->segtext_offset, buf, size);
        if(cfg_parser->mem_segtext_len < cfg_parser->segtext_offset + size)
            cfg_parser->mem_segtext_len = cfg_parser->segtext_offset + size;
    }
    else if(cfg_parser->segment == SEGMENT_DATA) {
        if(cfg_parser->segdata_offset + size > cfg_parser->mem_segdata_size) {
            cfg_parser->mem_segdata_size += 1024;
            cfg_parser->mem_segdata = realloc(cfg_parser->mem_segdata, cfg_parser->mem_segdata_size);
            if(cfg_parser->mem_segdata == NULL) {
                fprintf(stderr, "Segment Data Memory Error: %s\n", strerror(errno));
                return;
            }
        }
        memcpy(cfg_parser->mem_segdata + cfg_parser->segdata_offset, buf, size);
        if(cfg_parser->mem_segdata_len < cfg_parser->segdata_offset + size)
            cfg_parser->mem_segdata_len = cfg_parser->segdata_offset + size;
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
    if(cfg_parser->lookahead == token) {
        cfg_parser->lineno = cfg_parser->tokenizer->lineno;
        cfg_parser->colno = cfg_parser->tokenizer->colno;
        cfg_parser->lookahead = get_next_token(cfg_parser->tokenizer);
    } 
    else {
        report_cfg("Expected %s on line %ld, col %ld", get_token_str(token), cfg_parser->lineno, cfg_parser->colno);
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
    switch(cfg_parser->lookahead) {
        case TOK_EOL:
            match_cfg(TOK_EOL);
            break;
        case TOK_NULL:
            cfg_parser->lineno++;
            break;
        default:
            report_cfg("Unexpected %s on line %ld, col %ld", get_token_str(cfg_parser->lookahead), cfg_parser->lineno, cfg_parser->colno);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: label_cfg
 * Purpose: Attempts to match the non-terminal for label. If matched, the label
 * is inserted into the symbol table. Otherwise, an error is reported to the 
 * function report_cfg
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void label_cfg() {
    if(cfg_parser->lookahead == TOK_IDENTIFIER) {
        char *id = strdup(cfg_parser->tokenizer->lexbuf);
        match_cfg(TOK_IDENTIFIER);
        if(cfg_parser->lookahead == TOK_COLON) {
            match_cfg(TOK_COLON);
            struct symbol_table_entry *entry;
            if((entry = get_symbol_table(symbol_table, id)) != NULL) {
                if(entry->status != SYMBOL_UNDEFINED) {
                    entry->status = SYMBOL_DOUBLY;
                    report_cfg("Multiple definitions of label '%s' on line %ld, col %ld", id, cfg_parser->lineno, cfg_parser->colno);
                } 
                else {
                    if(cfg_parser->segment == SEGMENT_TEXT)
                        entry->offset = cfg_parser->segtext_offset;
                    else if(cfg_parser->segment == SEGMENT_DATA)
                        entry->offset = cfg_parser->segdata_offset;
                    entry->segment = cfg_parser->segment;
                    entry->status = SYMBOL_DEFINED;
                }
            } 
            else { 
                entry = insert_symbol_table(symbol_table, id);
                if(cfg_parser->segment == SEGMENT_TEXT)
                    entry->offset = cfg_parser->segtext_offset;
                else if(cfg_parser->segment == SEGMENT_DATA)
                    entry->offset = cfg_parser->segdata_offset;
                entry->segment = cfg_parser->segment;
                entry->status = SYMBOL_DEFINED;
            }
        } 
        else {
            report_cfg("Unrecognized mnemonic '%s' on line %ld, col %ld", id, cfg_parser->lineno, cfg_parser->colno);
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

    switch(cfg_parser->lookahead) {
        case TOK_REGISTER: {
            int value = cfg_parser->tokenizer->attrval;
            match_cfg(TOK_REGISTER);
            
            node = malloc(sizeof(struct operand_node));
            node->operand = OPERAND_REGISTER;
            node->value.reg = value;
            node->next = NULL;
            
            break;
        }
        case TOK_IDENTIFIER: {
            char *id = strdup(cfg_parser->tokenizer->lexbuf);
            match_cfg(TOK_IDENTIFIER);
            
            node = malloc(sizeof(struct operand_node));
            node->operand = OPERAND_LABEL;
            node->identifier = id;
            node->next = NULL;
            
            break;
        }
        case TOK_STRING: {
            char *id = strdup(cfg_parser->tokenizer->lexbuf);
            match_cfg(TOK_STRING);
            
            node = malloc(sizeof(struct operand_node));
            node->operand = OPERAND_STRING;
            node->identifier = id;
            node->next = NULL;
            
            break;
        }
        case TOK_INTEGER: {
            int value = cfg_parser->tokenizer->attrval;
            match_cfg(TOK_INTEGER);
            
            node = malloc(sizeof(struct operand_node));
            node->operand = OPERAND_IMMEDIATE;
            node->value.integer = value;
            node->next = NULL;
            
            if(cfg_parser->lookahead == TOK_LPAREN) {
                match_cfg(TOK_LPAREN);
                int reg_value = cfg_parser->tokenizer->attrval;
                if(match_cfg(TOK_REGISTER) && match_cfg(TOK_RPAREN)) {
                    node->operand = OPERAND_ADDRESS;
                    node->value.reg = reg_value;
                }
            }
            break;
        }
        case TOK_EOL:
        case TOK_NULL:
            report_cfg("Expected operand after line %ld, col %ld", cfg_parser->lineno, cfg_parser->colno);
            break;
        default:
            report_cfg("Invalid operand '%s' on line %ld, col %ld", cfg_parser->tokenizer->lexbuf, cfg_parser->lineno, cfg_parser->colno);
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
    struct operand_node *node = NULL;

    switch(cfg_parser->lookahead) {
        case TOK_REGISTER:
        case TOK_IDENTIFIER:
        case TOK_INTEGER:
        case TOK_STRING:
            node = operand_cfg();
            if(cfg_parser->lookahead == TOK_COMMA) {
                match_cfg(TOK_COMMA);
                if(node != NULL)
                    node->next = operand_list_cfg();
                else
                    node = operand_list_cfg();
            }
            break;
		case TOK_EOL:
		case TOK_NULL:
			report_cfg("Expected operand after line %ld, col %ld", cfg_parser->lineno, cfg_parser->colno);
			break;
        default:
            report_cfg("Invalid operand '%s' on line %ld, col %ld", cfg_parser->tokenizer->lexbuf, cfg_parser->lineno, cfg_parser->colno);
    }

    return node;
}

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
                report_cfg("Invalid operand combination for %s '%s' on line %ld", op_string, res_entry->id, cfg_parser->lineno);
                return 0;
            } 
            if(current_operand->operand & OPERAND_LABEL) {
                if(get_symbol_table(symbol_table, current_operand->identifier) == NULL)
                    insert_front(cfg_parser->ref_symlist, insert_symbol_table(symbol_table, current_operand->identifier));
            }
            
            /* Check for more operands in repeat... */
            current_operand = current_operand->next;
            while(current_operand != NULL && entry->operand[i] & current_operand->operand) {
                if(current_operand->operand & OPERAND_LABEL) {
                    if(get_symbol_table(symbol_table, current_operand->identifier) == NULL)
                        insert_front(cfg_parser->ref_symlist, insert_symbol_table(symbol_table, current_operand->identifier));
                }
                current_operand = current_operand->next;
            }
        } else {
            if(entry->operand[i] == OPERAND_NONE) {
                if(current_operand != NULL) {
                    report_cfg("Invalid operand combiniation for %s '%s' on line %ld", op_string, res_entry->id, cfg_parser->lineno);
                    return 0;
                }
                break;
            } else {
                if(current_operand == NULL || !(entry->operand[i] & current_operand->operand)) {
                    report_cfg("Invalid operand combiniation for %s '%s' on line %ld", op_string, res_entry->id, cfg_parser->lineno);
                    return 0;
                }
                if(current_operand->operand & OPERAND_LABEL) {
                    if(get_symbol_table(symbol_table, current_operand->identifier) == NULL)
                        insert_front(cfg_parser->ref_symlist, insert_symbol_table(symbol_table, current_operand->identifier));
                }
            }
            current_operand = current_operand->next;
        }
    }

    return 1;
}

void assemble_funct_instruction(struct instruction_node *instr) {
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
            write_segment_memory((void *)&instruction, 0x4);
            break; 
        }

        case 0x08: {
            struct operand_node *rs = instr->operand_list;
            instruction = CREATE_INSTRUCTION_R(0, rs->value.reg, 0, 0, 0, entry->funct);
            write_segment_memory((void *)&instruction, 0x4);
            break;
        }

        case 0x00:
        case 0x03:
        case 0x02: {
            struct operand_node *rd = instr->operand_list;
            struct operand_node *rt = instr->operand_list->next;
            struct operand_node *shamt = instr->operand_list->next->next;
            instruction = CREATE_INSTRUCTION_R(0, 0, rt->value.reg, rd->value.reg, shamt->value.integer, entry->funct);
            write_segment_memory((void *)&instruction, 0x4);
            break;
        }

        case 0x0C:
            instruction = CREATE_INSTRUCTION_R(0, 0, 0, 0, 0, entry->funct);
            write_segment_memory((void *)&instruction, 0x4);
            break;

        case 0x1A:
        case 0x1B:
        case 0x18:
        case 0x19: {
            struct operand_node *rs = instr->operand_list;
            struct operand_node *rt = instr->operand_list->next;
            instruction = CREATE_INSTRUCTION_R(0, rs->value.reg, rt->value.reg, 0, 0, entry->funct);
            write_segment_memory((void *)&instruction, 0x4);
            break;
        }
    }
}

void assemble_opcode_instruction(struct instruction_node *instr) {
    instruction_t instruction = 0;

    struct opcode_entry *entry = instr->mnemonic->attrptr;

    switch(entry->opcode) {
        case 0x08:
        case 0x09:
        case 0x0C:
        case 0x0D:
        case 0x0A:
        case 0x0B:
        case 0x0E: { /* ALU OP */
            struct operand_node *rs = instr->operand_list;
            struct operand_node *rt = instr->operand_list->next;
            struct operand_node *imm = instr->operand_list->next->next;
            instruction = CREATE_INSTRUCTION_I(entry->opcode, rs->value.reg, rt->value.reg, imm->value.integer);
            write_segment_memory((void *)&instruction, 0x4);
            break;
        }

        case 0x01:
        case 0x07:
        case 0x06: {
            struct operand_node *rs = instr->operand_list;
            struct operand_node *label = instr->operand_list->next;
            /* Check if label has been defined */
            struct symbol_table_entry *sym_entry = get_symbol_table(symbol_table, label->identifier);
            if(sym_entry == NULL) {
                insert_front(cfg_parser->ref_symlist, insert_symbol_table(symbol_table, label->identifier));
                insert_front(sym_entry->instr_list, (void *)instr);
            }
            else {
                if(sym_entry->status == SYMBOL_UNDEFINED) {
                    insert_front(sym_entry->instr_list, (void *)instr);
                }
                else {
                    offset_t branch_offset = (sym_entry->offset - (cfg_parser->segtext_offset + 4)) >> 2;
                    instruction = CREATE_INSTRUCTION_I(entry->opcode, rs->value.reg, entry->rt, branch_offset);
                    write_segment_memory((void *)&instruction, 0x4);
                }   
            }
            break;
        }

        case 0x05:
        case 0x04: {
            struct operand_node *rs = instr->operand_list;
            struct operand_node *rt = instr->operand_list->next;
            struct operand_node *label = instr->operand_list->next->next;
            /* Check if label has been defined */
            struct symbol_table_entry *sym_entry = get_symbol_table(symbol_table, label->identifier);
            if(sym_entry == NULL) {
                insert_front(cfg_parser->ref_symlist, insert_symbol_table(symbol_table, label->identifier));
                insert_front(sym_entry->instr_list, (void *)instr);
            }
            else {
                if(sym_entry->status == SYMBOL_UNDEFINED) {
                    insert_front(sym_entry->instr_list, (void *)instr);
                }
                else {
                    offset_t branch_offset = (sym_entry->offset - (cfg_parser->segtext_offset + 4)) >> 2;
                    instruction = CREATE_INSTRUCTION_I(entry->opcode, rs->value.reg, rt->value.reg, branch_offset);
                    write_segment_memory((void *)&instruction, 0x4);
                }   
            }
            break;
        }

        case 0x02:
        case 0x03: {
            struct operand_node *label = instr->operand_list;
            /* Check if label has been defined */
            struct symbol_table_entry *sym_entry = get_symbol_table(symbol_table, label->identifier);
            if(sym_entry == NULL) {
                insert_front(cfg_parser->ref_symlist, insert_symbol_table(symbol_table, label->identifier));
                insert_front(sym_entry->instr_list, (void *)instr);
            }
            else {
                if(sym_entry->status == SYMBOL_UNDEFINED) {
                    insert_front(sym_entry->instr_list, (void *)instr);
                }
                else {
                    instruction = CREATE_INSTRUCTION_J(entry->opcode, (sym_entry->offset >> 2));
                    write_segment_memory((void *)&instruction, 0x4);
                }   
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
            struct operand_node *rs = instr->operand_list;
            struct operand_node *addr = instr->operand_list->next;
            instruction = CREATE_INSTRUCTION_I(entry->opcode, rs->value.reg, addr->value.reg, addr->value.integer);
            write_segment_memory((void *)&instruction, 0x4);
            break;
        }
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: check_instruction
 * Purpose: Checks the instruction_node to see if a proper instruction was 
 * recognized based on the opcode table entry for the mnemonic. If an invalid
 * instruction is encountered, it will report the error to report_cfg.
 * @param instr -> Address of the instruction node structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void check_instruction(struct instruction_node *instr) {
    if(instr == NULL || instr->mnemonic == NULL) return;

    if(!verify_operand_list(instr->mnemonic, instr->operand_list)) return;

    /* TO-DO: Assemble (?) instruction */
    if(cfg_parser->segment == SEGMENT_DATA) {
        report_cfg("Cannot define instructions in .data segment on line %ld", cfg_parser->lineno);
        return;
    }

    struct opcode_entry *entry = instr->mnemonic->attrptr;

    if(entry->type == OPTYPE_PSUEDO) {
        //assemble_psuedo_instruction(instr);
    }
    else {
        if(entry->opcode == 0x00) {
            assemble_funct_instruction(instr);       
        }
        else {
            assemble_opcode_instruction(instr);
        }
    }

    /* Finally increment LC */
    if(((struct opcode_entry *)instr->mnemonic->attrptr)->type == OPTYPE_PSUEDO) {
		inc_segment_offset(((struct opcode_entry *)instr->mnemonic->attrptr)->size);
	} else {
		inc_segment_offset(0x4);
	}
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: check_directive
 * Purpose: Checks the directive recognized from the CFG. Ensures that the
 * operands recognized are of the correct format. If successfully recognized,
 * the function attempts to execute the directive.
 * @param directive    -> Address of the entry in the reserved table
 *        operand_list -> Address of the first operand in the operand list
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void check_directive(struct instruction_node *instr) {
    struct reserved_entry *directive = instr->mnemonic;
    struct operand_node *operand_list = instr->operand_list;

    /* Check if entry is directive */
    if(directive->token != TOK_DIRECTIVE) return;

    struct opcode_entry *entry = (struct opcode_entry *)directive->attrptr;

    if(!verify_operand_list(directive, operand_list)) return;

    /* Check if segment is text before handling directive */
    switch(entry->opcode) {
        case DIRECTIVE_ASCII:
        case DIRECTIVE_ASCIIZ:
        case DIRECTIVE_HALF:
        case DIRECTIVE_BYTE:
            if(cfg_parser->segment != SEGMENT_DATA) {
                report_cfg("Directive '%s' is not allowed in the .text segment on line %ld", directive->id, cfg_parser->lineno);
                return;
            }
            break;
    }

    switch(entry->opcode) {
        case DIRECTIVE_INCLUDE: {
            /* Create tokenizer structure */
            struct tokenizer *tokenizer = create_tokenizer(operand_list->identifier);
            if(tokenizer == NULL) {
                report_cfg("Failed to include file '%s' on line %ld : %s", operand_list->identifier, cfg_parser->lineno, strerror(errno));
            } 
            else {
                insert_front(cfg_parser->tokenizer_queue, (void *)tokenizer);
                cfg_parser->tokenizer = tokenizer;
                cfg_parser->lookahead = get_next_token(tokenizer);
            }
            break;
        }
        case DIRECTIVE_TEXT: 
            cfg_parser->segment = SEGMENT_TEXT;
            break;
        case DIRECTIVE_DATA:
            cfg_parser->segment = SEGMENT_DATA;
            break;
        case DIRECTIVE_ALIGN: {
            if(operand_list->value.integer < 0 || operand_list->value.integer >= 31) {
                report_cfg("Directive '.align n' expects n to be within the range of [0, 31] on line %ld", cfg_parser->lineno);
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
                    printf("DIRECTIVE: OPERAND FOUND\n");
                    struct symbol_table_entry *sym_entry = get_symbol_table(symbol_table, current_operand->identifier);
                    if(sym_entry == NULL) {
                        insert_front(cfg_parser->ref_symlist, insert_symbol_table(symbol_table, current_operand->identifier));
                        insert_front(sym_entry->instr_list, (void *)instr);
                    }
                    else {
                        if(sym_entry->status == SYMBOL_UNDEFINED) {
                            insert_front(sym_entry->instr_list, (void *)instr);
                        }
                        else {
                            offset_t sym_offset = sym_entry->offset;
                            write_segment_memory((void *)&sym_offset, 0x4);
                            inc_segment_offset(0x4);
                        }   
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
                inc_segment_offset(0x4);
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
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: instruction_cfg
 * Purpose: Attempts to match the non-terminal for instruction. If failed to match
 * reports error to report_cfg
 * @return Address of the instruction_node if valid, otherwise NULL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct instruction_node *instruction_cfg() {
    struct instruction_node *node = NULL;

    if(cfg_parser->lookahead == TOK_IDENTIFIER) label_cfg();

    switch(cfg_parser->lookahead) {
        case TOK_DIRECTIVE: {
            node = malloc(sizeof(struct instruction_node));
            node->mnemonic = cfg_parser->tokenizer->attrptr;
            if(cfg_parser->segment == SEGMENT_TEXT) {
                node->offset = cfg_parser->segtext_offset;
            }
            else {
                node->offset = cfg_parser->segdata_offset;
            }
            node->segment = cfg_parser->segment;
            node->next = NULL;

            match_cfg(TOK_DIRECTIVE);
            switch(cfg_parser->lookahead) {
                case TOK_IDENTIFIER:
                case TOK_INTEGER:
                case TOK_STRING:
                    node->operand_list = operand_list_cfg();
                    break;
                default:
                    node->operand_list = NULL;
            }

            end_line_cfg();

            check_directive(node);

            break;
        }
        case TOK_MNEMONIC:
            node = malloc(sizeof(struct instruction_node));
            node->mnemonic = cfg_parser->tokenizer->attrptr;
            node->offset = cfg_parser->segtext_offset;
            node->segment = cfg_parser->segment;
            node->next = NULL;
            
            match_cfg(TOK_MNEMONIC);
            switch(cfg_parser->lookahead) {
                case TOK_IDENTIFIER:
                case TOK_INTEGER:
                case TOK_REGISTER:
                    node->operand_list = operand_list_cfg();
                    break;
                default:
                    node->operand_list = NULL;
            }

            end_line_cfg();

            check_instruction(node);

            break;
        case TOK_EOL:
        case TOK_NULL:
            end_line_cfg();
            break;
        default:
            report_cfg("Unexpected %s on line %ld, col %ld", get_token_str(cfg_parser->lookahead), cfg_parser->lineno, cfg_parser->colno);
    }

    return node;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: instruction_list_cfg
 * Purpose: Attempts to match the non-terminal for instruction_list. Failure to match
 * results in reporting the error to report_cfg
 * @return Address of the first instruction_node in the instruction list, otherwise NULL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct instruction_node *instruction_list_cfg() {
    struct instruction_node *node = NULL;
    
    while(cfg_parser->lookahead == TOK_NULL) {
        /* Free current tokenizer */
        destroy_tokenizer(&cfg_parser->tokenizer);
        remove_front(cfg_parser->tokenizer_queue, LN_VSTATIC);

        /* No more files to process */
        if(cfg_parser->tokenizer_queue->front == NULL) return node;    
        
        /* Setup tokenizer */
        cfg_parser->tokenizer = cfg_parser->tokenizer_queue->front->value;

        /* Setup lookahead */
        cfg_parser->lookahead = get_next_token(cfg_parser->tokenizer);
    }
    
    if(cfg_parser->lookahead != TOK_NULL) {
        node = instruction_cfg();
        if(node != NULL)
            node->next = instruction_list_cfg();
        else
            node = instruction_list_cfg();
    }

    return node;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: program_cfg
 * Purpose: Start symbol the the LL(1) context-free grammer
 * @return Address of the allocated program_node
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct program_node *program_cfg(struct parser *parser) {
    cfg_parser = parser;
    struct program_node *node = malloc(sizeof(struct program_node));
    node->instruction_list = instruction_list_cfg();
    
    /* Verify undefined symbol table */
    for(struct list_node *head = parser->ref_symlist->front; head != NULL; head = head->next) {
        struct symbol_table_entry *sym_entry = (struct symbol_table_entry *)head->value;
        symstat_t status = sym_entry->status;
        if(status == SYMBOL_UNDEFINED) {
            /* Symbol is still undefined, program cannot be assembled */
            fprintf(stderr, "Symbol Error: Undefined symbol '%s'\n", ((struct symbol_table_entry *)head->value)->key);
            parser->status = PARSER_STATUS_FAIL;
        } else {
            struct list_node *instr_ref = sym_entry->instr_list->front;
            while(instr_ref != NULL) {
                if(((struct instruction_node *)instr_ref->value)->mnemonic->token == TOK_MNEMONIC) {
                    printf("INSTRUCTION_FOUND\n");
                    parser->segment = ((struct instruction_node *)instr_ref->value)->segment;
                    parser->segtext_offset = ((struct instruction_node *)instr_ref->value)->offset;
                    check_instruction(instr_ref->value); 
                }
                else if(((struct instruction_node *)instr_ref->value)->mnemonic->token == TOK_DIRECTIVE) {
                    printf("DIRECTIVE FOUND\n");
                    parser->segment = ((struct instruction_node *)instr_ref->value)->segment;
                    parser->segdata_offset = ((struct instruction_node *)instr_ref->value)->offset;
                    check_directive(instr_ref->value); 
                }
                instr_ref = instr_ref->next;
            }
        }
    }
    return node;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: create_parser
 * Purpose: Allocates and initializes the parser structure 
 * @return Pointer to the allocated parser structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct parser *create_parser(int file_count, const char **file_arr) {
    struct parser *parser = malloc(sizeof(struct parser));

    parser->tokenizer_queue = NULL;

    parser->src_files = create_list();
    /* Setup source files */
    for(int i = 0; i < file_count; ++i) {
        insert_rear(parser->src_files, (void *)file_arr[i]);
    }

    parser->lookahead = TOK_NULL;
    parser->ref_symlist = create_list();
    parser->status = PARSER_STATUS_NULL;
    parser->lineno = 1;
    parser->colno = 1;
    parser->segtext_offset = 0x00000000;
    parser->segdata_offset = 0x00000000;
    parser->ast = NULL;
    parser->segment = SEGMENT_TEXT;
    parser->mem_segtext = NULL;
    parser->mem_segdata = NULL;
    parser->mem_segtext_size = 0;
    parser->mem_segdata_size = 0;
    parser->mem_segdata_len = 0;
    parser->mem_segtext_len = 0;
    
    return parser;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: execute_parser
 * Purpose: Starts the parsing process. Sets up the lookahead token
 * required for the LL(1) grammer and verifies symbol table after execution. 
 * @param parser -> Address of the parser structure
 * @return PARSER_STATUS_OK if no errors, otherwise PARSER_STATUS_FAIL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
pstatus_t execute_parser(struct parser *parser) {  
    /* Setup initial tokenizer structure */
    if(parser->src_files->front == NULL) {
        fprintf(stderr, "Input: No source files to assemble\n");
        parser->status = PARSER_STATUS_FAIL;
        return parser->status;
    }

    /* Create initial tokenizer queue */
    parser->tokenizer_queue = create_list();
    struct list_node *file = parser->src_files->front;
    while(file != NULL) {
        struct tokenizer *tokenizer = create_tokenizer((const char *)file->value);
        if(tokenizer == NULL) {
            fprintf(stderr, "%s: Error: %s\n", (const char *)file->value, strerror(errno));
            parser->status = PARSER_STATUS_FAIL;
            delete_linked_list(&(parser->tokenizer_queue), LN_VSTATIC);
            return parser->status;
        }
        insert_rear(parser->tokenizer_queue, tokenizer);
        file = file->next;
    }

    /* Setup tokenizer */
    parser->tokenizer = parser->tokenizer_queue->front->value;

    /* Setup lookahead */
    parser->lookahead = get_next_token(parser->tokenizer);

    /* Allocate memory size for segment text (start with a page) */
    parser->mem_segtext_size = 1024;
    parser->mem_segtext = malloc(parser->mem_segtext_size);

    /* Allocate memory size for segment data (start with a page) */
    parser->mem_segdata_size = 1024;
    parser->mem_segdata = malloc(parser->mem_segdata_size);

    /* Start grammar recognization... */
    parser->ast = program_cfg(parser);

    if(parser->status == PARSER_STATUS_NULL) {
        parser->status = PARSER_STATUS_OK;
    }

    #ifdef DEBUG
    if(parser->status == PARSER_STATUS_OK) {
        printf("[ BEGIN SEGMENT DATA ]");
        for(int i = 0; i < parser->mem_segdata_len; i++) {
            if(i % 4 == 0) {
                printf("\n");
                printf("0x%08X  ", i);
            }
            unsigned char c = *((unsigned char *)parser->mem_segdata + i);
            if(isprint(c)) printf("'%c' ", c);
            else printf("\\%2X ", c);
        
        }
        printf("\n[ END SEGMENT DATA ]\n\n");
        printf("[ BEGIN SEGMENT TEXT ]");
        for(int i = 0; i < parser->mem_segtext_len; i++) {
            if(i % 4 == 0) {
                printf("\n");
                printf("0x%08X  ", i);
            }
            unsigned char c = *((unsigned char *)parser->mem_segtext + i);
            printf("\\%2X ", c);
        
        }
        printf("\n[ END SEGMENT TEXT ]\n\n");
    }
    #endif

    /* Free used data */
    free(parser->mem_segtext);
    free(parser->mem_segdata);
    delete_linked_list(&(parser->tokenizer_queue), LN_VSTATIC);

    return parser->status;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: destroy_symbol
 * Purpose: Deallocates all the dynamically allocated memory used for the
 * parser structure
 * @param parser -> Reference to the address of the parser structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void destroy_parser(struct parser **parser) {
    /* Destory AST */
    struct instruction_node *instr_node = (*parser)->ast ? (*parser)->ast->instruction_list : NULL;
    /* Free instructions */
    while(instr_node != NULL) {
        struct instruction_node *next_instr = instr_node->next;
        /* Free operands */
        struct operand_node *op_node = instr_node->operand_list;
        while(op_node != NULL) {
            struct operand_node *next_op = op_node->next;
            if(op_node->operand == OPERAND_LABEL) free(op_node->identifier);
            free(op_node);
            op_node = next_op;
        }
        free(instr_node);
        instr_node = next_instr;
    }

    /* Free AST */
    free((*parser)->ast);

    /* Free linked list */
    delete_linked_list(&((*parser)->src_files), LN_VSTATIC);
    delete_linked_list(&((*parser)->ref_symlist), LN_VSTATIC);

    /* Simple free the data */
    free(*parser);

    /* Set parser to NULL */
    *parser = NULL;
}
