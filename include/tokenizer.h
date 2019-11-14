/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * File: tokenizer.h
 *
 * Purpose: Converts assembly file from source into a sequence of tokens for
 * the assembler / parser to use. 
 *
 * The tokenizer is not designed to convert the whole file into tokens and push them
 * on a stack of sorts, rather it retrieves the next token by request using the 
 * get_next_token function.
 *
 * Typical usage:
 *      struct tokenizer *tokenizer = create_tokenizer(file);
 *      token_t token;
 *      while((token = get_next_token(tokenizer)) != TOK_NULL) {
 *          if(token == TOK_INVALID) {
 *              fprintf(stderr, "Error: '%s'\n", tokenizer->errmsg);
 *          }
 *          else {
 *              // do some stuff with token
 *          }          
 *      }
 *      destroy_tokenizer(&tokenizer);
 *
 * There is a special case to consider, whenever the token TOK_INVALID is returned
 * it means that the next token couldn't be retrieved based off the contents of the
 * source file. However, the next get_next_token function call will continue from 
 * where it left off, allowing the caller to examine the rest of the file even 
 * though there are unrecognized characters / patterns.
 *
 * @author: Bryan Rocha
 * @version: 1.0 (8/28/2019)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdio.h>
#include <stdint.h>

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
#define TOK_DIRECTIVE   0x0C

/* Type definitions */
typedef unsigned int token_t;

/* Tokenizer structure */
struct tokenizer {
    char*        filename;   /* Name of the file opened */
    FILE*        fstream;    /* Pointer to file used in lexical scanner */
    char*        lexbuf;     /* Lexical buffer */
    char*        errmsg;     /* Error message buffer */
    union {                  /* Token attributes */
        int      attrval;    /* Integer */
        void*    attrptr;    /* Generic */
        char*    attrbuf;    /* Identifier */
    };
    size_t       bufpos;     /* Lexical buffer position */
    size_t       bufsize;    /* Lexical buffer physical size */
    size_t       lineno;     /* Line number */
    size_t       colno;      /* Column number */
    size_t       errsize;    /* Error buffer physical size */
};

/* Reserved keywords table */
struct reserved_entry { 
    const char* id;     /* Reserved identifier */ 
    token_t     token;  /* Matching token */
    void*   attrptr;    /* Generic Attribute */
    int     attrval;    /* Integer Attribute */
};

/* Function prototypes */
struct tokenizer* create_tokenizer(const char*);
token_t get_next_token(struct tokenizer*);
void destroy_tokenizer(struct tokenizer**);

/* Assistant function, helps for error debugging */
const char* get_token_str(token_t);

#endif