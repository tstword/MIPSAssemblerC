#include "tokenizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

typedef enum { init_state, comma_accept, colon_accept, left_paren_accept,
               right_paren_accept, identifier_state, identifier_accept,
               integer_state, integer_accept, hex_state, zero_state,
               eof_accept, comment_state, comment_accept, negative_state,
               string_state, string_accept, eol_accept, invalid_state } state_fsm;

struct reserved_entry reserved_table[] = {
    { "add",     TOK_MNEMONIC, 0 },
    { "sub",     TOK_MNEMONIC, 0 },
    { "addi",    TOK_MNEMONIC, 0 },
    { "syscall", TOK_MNEMONIC, 0 }
};

struct reserved_entry* get_reserved_table(const char *key) {
    
}

int tgetc(struct tokenizer *tokenizer) {
    int ch = fgetc(tokenizer->fstream);

    /* Adjust tokenizer buffer if necessary */
    if(tokenizer->bufpos >= tokenizer->bufsize) {
        tokenizer->bufsize <<= 1;
        tokenizer->attrbuf = realloc(tokenizer->attrbuf, tokenizer->bufsize);
    }

    tokenizer->attrbuf[tokenizer->bufpos++] = ch;
    tokenizer->colno++;

    return ch;
}

void tungetc(int ch, struct tokenizer *tokenizer) {
    ungetc(ch, tokenizer->fstream);
    tokenizer->colno--;
    tokenizer->bufpos--;
}

int tpeekc(struct tokenizer *tokenizer) {
    int ch = fgetc(tokenizer->fstream);
    ungetc(ch, tokenizer->fstream);
    return ch;
}

void report_fsm(struct tokenizer *tokenizer, const char *fmt, ...) {
    va_list vargs;
    size_t bufsize = 0;

    va_start(vargs, fmt);
    bufsize = vsnprintf(NULL, 0, fmt, vargs) + 1;
    va_end(vargs);

    if(bufsize > tokenizer->errsize) {
        tokenizer->errsize = bufsize;
        tokenizer->errmsg = realloc(tokenizer->errmsg, bufsize);
    }

    va_start(vargs, fmt);
    vsnprintf(tokenizer->errmsg, bufsize, fmt, vargs);
    va_end(vargs);
}

state_fsm init_fsm(struct tokenizer *tokenizer) {
    int ch = tgetc(tokenizer);

    switch(ch) {
        case ':':
            return colon_accept;
        case ',':
            return comma_accept;
        case '(':
            return left_paren_accept;
        case ')':
            return right_paren_accept;
        case '\n':
            tokenizer->lineno++;
            tokenizer->colno = 1;
            return eol_accept;
        case 'A' ... 'Z':
        case 'a' ... 'z':
        case '$':
        case '_':
        case '.':
            return identifier_state;
        case '"':
            return string_state;
        case '#':
            return comment_state;
        case '-':
            return negative_state;
        case '1' ... '9':
            return integer_state;
        case '0':
            return zero_state;
        case ' ':
        case '\t':
            /* Skip whitespace */
            tokenizer->bufpos--;
            return init_state;
        case EOF:
            return eof_accept;
        default:
            reportFSM(tokenizer, "Unexpected character '%c' on line %ld, column %ld", ch, tokenizer->lineno, tokenizer->colno - 1);
            return invalid_state;
    }
}

state_fsm identifier_fsm(struct tokenizer *tokenizer) {
    int ch = tgetc(tokenizer);

    switch(ch) {
        case 'A' ... 'Z':
        case 'a' ... 'z':
        case '_':
        case '0' ... '9':
            return identifier_state;
        
        default:
            tungetc(ch, tokenizer);
            return identifier_accept;
    }

    return invalid_state;
}

state_fsm integer_fsm(struct tokenizer *tokenizer) {
    int ch = tgetc(tokenizer);

    switch(ch) {
        case '0' ... '9':
            return integer_state;

        default:
            tungetc(ch, tokenizer);
            return integer_accept;
    }

    return invalid_state;
}

state_fsm zero_fsm(struct tokenizer *tokenizer) {
    int ch = tgetc(tokenizer);

    switch(ch) {
        case 'x':
        case 'X':
            // Check if next character is digit
            if(isxdigit(tpeekc(tokenizer))) return hex_state;
            tungetc(ch, tokenizer);
            return integer_accept;

        case '0' ... '9':
            return integer_state;

        default:
            tungetc(ch, tokenizer);
            return integer_accept;
    }

    return invalid_state;
}
