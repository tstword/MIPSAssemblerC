#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdio.h>

/* Type definitions */
typedef unsigned int token_t;

/* Token name macros */
#define TOK_NULL        0x00    
#define TOK_COLON       0x01
#define TOK_COMMA       0x02
#define TOK_IDENTIFIER  0x03
#define TOK_INTEGER     0x04
#define TOK_LPAREN      0x05
#define TOK_RPAREN      0x06
#define TOK_EOL         0x07
#define TOK_MNEMONIC    0x08
#define TOK_REGISTER    0x09
#define TOK_STRING      0x0A
#define TOK_INVALID     0x0B

/* Tokenizer structure */
struct tokenizer {
    FILE*       fstream;    /* Pointer to file used in lexical scanner */
    char*       lexbuf;     /* Lexical buffer */
    char*       errmsg;     /* Error message buffer */
    union {                 /* Token attributes */
        int     attrval;    /* Integer */
        void*   attrptr;    /* Generic */
        char*   attrbuf;    /* Identifier */
    };
    size_t      bufpos;     /* Lexical buffer position */
    size_t      bufsize;    /* Lexical buffer physical size */
    size_t      lineno;     /* Line number */
    size_t      colno;      /* Column number */
    size_t      errsize;    /* Error buffer physical size */
};

/* Reserved keywords table */
struct reserved_entry { 
    const char* id;         /* Reserved identifier */ 
    token_t     token;      /* Matching token */
    union {
        void*   attrptr;    /* Generic Attribute */
        int     attrval;    /* Integer Attribute */
    };
};

/* Function prototypes */
struct tokenizer* create_tokenizer(char*);
token_t get_next_token(struct tokenizer*);
void destroy_tokenizer(struct tokenizer**);

/* Assistant function, helps for error debugging */
const char* get_token_str(token_t);

#endif