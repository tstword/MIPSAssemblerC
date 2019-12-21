/**
 * @file: parser.h
 *
 * @purpose: Responsible for parsing and verifying the sequence of tokens
 * as a MIPS assembly program. The parser is implemented as a recursive-descent
 * parser, as a result the language is defined as a LL(1) grammer:
 *
 *     program          -> instruction_list
 *
 *     instruction_list -> instruction instruction_list | 
 *                         <EOF>
 *
 *     instruction      -> label <EOL> | 
 *                         label <MNEMONIC> operand_list <EOL> | 
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
 *     label            -> <ID> <COLON>
 *
 * Failure in parsing the tokens is indicated by the macro PARSER_STATUS_FAIL
 * Success in parsing the tokens is indicated by the macro PARSER_STATUS_OK
 * 
 * @author: Bryan Rocha
 * @version: 1.0 (8/28/2019)
 **/

#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "tokenizer.h"
#include "symtable.h"
#include "opcode.h"
#include "linkedlist.h"

/* Marco definition */
#define ASSEMBLER_STATUS_NULL     0x0
#define ASSEMBLER_STATUS_OK       0x1
#define ASSEMBLER_STATUS_FAIL     0x2
#define ASSEBMLER_STATUS_CRIT     0x3

typedef unsigned char operand_t;
typedef unsigned char astatus_t;

/* Abstract syntax tree */
struct mnemonic_node {
    mnemonic_t mnemonic;
    struct opcode_entry *entry;
};

struct operand_node {
    operand_t operand;
    union {
        char *identifier;
        struct { 
            uint32_t integer;
            uint8_t neg;
            uint8_t reg;
        } value;
    };
    struct operand_node *next;
};

struct instruction_node {
    struct reserved_entry *mnemonic;
    struct operand_node *operand_list;
    offset_t offset;
    segment_t segment;
    struct instruction_node *next;
};

struct program_node {
    struct instruction_node *instruction_list;
};

// /* Parser structure definition */
// struct parser {
//     struct tokenizer       *tokenizer;                      /* Address of current tokenizer */
//     struct linked_list     *tokenizer_queue;                /* Queue of tokenizers */
    
//     struct program_node    *ast;                            /* Root of the abstract syntax tree */
    
//     struct linked_list     *ref_symlist;                    /* List of symbols that were referenced before definition */
//     struct linked_list     *src_files;                      /* List of source file strings */
    
//     unsigned char          *segment_memory[MAX_SEGMENTS];   /* Memory set aside for segment */
    
//     segment_t              segment;                         /* Indication of current segment */

//     token_t                lookahead;                       /* Required for LL(1) grammar */
//     astatus_t              status;                          /* Indicates the status of the parser */
    
//     offset_t               segment_offset[MAX_SEGMENTS];    /* Offset used for segment */
//     size_t                 seg_memory_len[MAX_SEGMENTS];    /* Length of memory for segment */
//     size_t                 seg_memory_size[MAX_SEGMENTS];   /* Size of memory for segment */
    
//     size_t                 lineno;                          /* Line number of where the tokenizer last left off */
//     size_t                 colno;                           /* Column number of where the tokenizer last left off */
// };

struct assembler {
    struct tokenizer        *tokenizer;
    struct linked_list      *tokenizer_list;

    struct symbol_table     *symbol_table;
    struct linked_list      *decl_symlist;

    void                    *segment_memory[MAX_SEGMENTS];

    token_t                 lookahead;
    astatus_t               status;
    segment_t               segment;

    offset_t                segment_offset[MAX_SEGMENTS];

    size_t                  segment_memory_offset[MAX_SEGMENTS];
    size_t                  segment_memory_size[MAX_SEGMENTS];

    size_t                  lineno;
    size_t                  colno;
};

/* Function prototypes */
struct assembler *create_assembler();
astatus_t execute_assembler(struct assembler *, const char **, size_t);
void destroy_assembler(struct assembler **);

#endif