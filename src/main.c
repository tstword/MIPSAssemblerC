#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "tokenizer.h"
#include "assembler.h"
#include "symtable.h"
#include "opcode.h"

struct symbol_table *symbol_table;

void terminateError() {
    /* An error occurred, terminate the program */
    fprintf(stderr, "Terminating program due to errors...\n");
    exit(-1);
}

void printToken(token_t token, struct tokenizer *tokenizer) {
    if(token != TOK_INVALID) printf("Token: ");
    switch(token) {
        case TOK_IDENTIFIER:
            printf("%-10s", "<ID>");
            break;
        case TOK_COLON:
            printf("%-10s", "<COLON>");
            break;
        case TOK_COMMA:
            printf("%-10s", "<COMMA>");
            break;
        case TOK_INTEGER:
            printf("%-10s", "<INTEGER>");
            break;
        case TOK_RPAREN:
            printf("%-10s", "<RPAREN>");
            break;
        case TOK_LPAREN:
            printf("%-10s", "<LPAREN>");
            break;
        case TOK_EOL:
            printf("%-10s", "<EOL>");
            break;
        case TOK_MNEMONIC:
            printf("%-10s", "<MNEMONIC>");
            break;
        case TOK_REGISTER:
            printf("%-10s", "<REGISTER>");
            break;
        case TOK_STRING:
            printf("%-10s", "<STRING>");
            break;
        case TOK_INVALID:
            fprintf(stderr, "An error has occurred while parsing the document...\n");
            fprintf(stderr, "Lexical Error: %s\n", tokenizer->errmsg);
            terminateError();
            break;
    }
    if(token != TOK_EOL) printf("    Buffer: \"%s\"\n", tokenizer->lexbuf);
    else printf("    Buffer: \"\\n\"\n");

    if(token == TOK_MNEMONIC) {
        struct opcode_entry *entry = (struct opcode_entry *)tokenizer->attrptr;
        printf("[ %d | %d | %d | %d ]\n", entry->opcode, entry->funct, entry->rt, entry->type);
    }
}

int main(int argc, char *argv[]) {
    if(argc == 1) {
        /* Print usage and terminate */
        fprintf(stderr, "Usage: %s [source]\n", argv[0]);
        return -1;
    }

    struct assembler *assembler = create_assembler();
    if(execute_assembler(assembler, (const char **)(argv + 1), argc - 1) == ASSEMBLER_STATUS_FAIL) 
        fprintf(stderr, "\nFailed to assemble program\n");
    destroy_assembler(&assembler);

    return 0;
}
