#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "tokenizer.h"
#include "symtable.h"
#include "opcode.h"

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
    struct tokenizer *tokenizer;        /* Pointer to tokenizer */

    struct program_node *ast;           /* Root of the abstract syntax tree */
    
    segment_t segment;                  /* Indication of current segment */

    token_t lookahead;                  /* Required for LL(1) grammar */
    pstatus_t status;                   /* Indicates the status of the parser */
    
    offset_t LC;                        /* Offset used for labels */
    
    size_t lineno;                      /* Line number of where the tokenizer last left off */
    size_t colno;                       /* Column number of where the tokenizer last left off */
};

/* Function prototypes */
struct parser *create_parser(struct tokenizer *);
pstatus_t execute_parser(struct parser *);
void destroy_parser(struct parser **);

#endif