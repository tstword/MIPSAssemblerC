#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "tokenizer.h"
#include "symtable.h"

/* Marco definition */
#define PARSER_STATUS_NULL 0x0
#define PARSER_STATUS_OK   0x1
#define PARSER_STATUS_FAIL 0x2

typedef unsigned char pstatus_t;

/* Parser structure definition */
struct parser {
    struct tokenizer *tokenizer;        /* Pointer to tokenizer */
    
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