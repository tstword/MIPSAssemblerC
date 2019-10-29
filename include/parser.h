/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * File: parser.h
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
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
#define PARSER_STATUS_NULL 0x0
#define PARSER_STATUS_OK   0x1
#define PARSER_STATUS_FAIL 0x2

typedef unsigned char operand_t;
typedef unsigned char pstatus_t;

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
            uint64_t integer : 32;
            uint64_t reg : 32;
        } value;
    };
    struct operand_node *next;
};

struct instruction_node {
    struct reserved_entry *mnemonic;
    struct operand_node *operand_list;
    struct instruction_node *next;
};

struct program_node {
    struct instruction_node *instruction_list;
};

/* Parser structure definition */
struct parser {
    struct tokenizer *tokenizer;            /* Address of current tokenizer */

    struct program_node *ast;               /* Root of the abstract syntax tree */
    struct linked_list *sym_list;           /* List of symbols that were referenced before definition */

    struct linked_list *src_files;          /* List of source file strings */
    struct list_node *current_file;         /* Current file */
    
    segment_t segment;                      /* Indication of current segment */

    token_t lookahead;                      /* Required for LL(1) grammar */
    pstatus_t status;                       /* Indicates the status of the parser */
    
    offset_t LC;                            /* Offset used for labels */
    
    size_t lineno;                          /* Line number of where the tokenizer last left off */
    size_t colno;                           /* Column number of where the tokenizer last left off */
};

/* Function prototypes */
struct parser *create_parser(int, const char **);
pstatus_t execute_parser(struct parser *);
void destroy_parser(struct parser **);

#endif