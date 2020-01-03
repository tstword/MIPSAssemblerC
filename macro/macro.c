#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenizer.h"
#include "funcwrap.h"
#include "symtable.h"

#define MACRO_DEFAULT_MODE      0x0
#define MACRO_DEFINITION_MODE   0x1
#define MACRO_EXPANSION_MODE    0x2

#define MPASS_STATUS_OK         0x0
#define MPASS_STATUS_FAIL       0x1

struct macro_op_node {
    char *operand_name;
    struct macro_op_node *next;
};

struct macro_node {
    char *macro_name;
    char *macro_definition;
    size_t macro_deflen;
    size_t macro_defsize;
    struct symbol_table *symtab;
    struct macro_op_node *operand_list;
};

struct macro_node *current_macro = NULL;
token_t lookahead;
struct tokenizer *cfg_tokenizer;
int mode;
int status;

int match(token_t token) {
    if(token == lookahead) {
        lookahead = get_next_token(cfg_tokenizer);
        return 1;
    }
    lookahead = get_next_token(cfg_tokenizer);
    return 0;
}

void cfg_continue() {
    while(lookahead != TOK_EOL && lookahead != TOK_NULL) {
        lookahead = get_next_token(cfg_tokenizer);
    }
}

void macro_write_line(FILE *fp, long init_offset) {
    int ch;
    if(mode == MACRO_DEFAULT_MODE) {
        fseek(cfg_tokenizer->fstream, init_offset, SEEK_SET);
        while((ch = fgetc(cfg_tokenizer->fstream)) != EOF) {
            fputc(ch, fp);
            if(ch == '\n') {
                ++cfg_tokenizer->lineno;
                cfg_tokenizer->colno = 1;
                break;
            }
        }
    }
    else if(mode == MACRO_DEFINITION_MODE) {
        fseek(cfg_tokenizer->fstream, init_offset, SEEK_SET);
        
        lookahead = get_next_token(cfg_tokenizer);
        
        while(lookahead != TOK_NULL) {
            char *buffer = cfg_tokenizer->lexbuf;

            if(lookahead == TOK_IDENTIFIER) {
                struct symbol_table_entry *entry = get_symbol_table(current_macro->symtab, buffer);
                if(entry != NULL) {
                    char *new_label = malloc(cfg_tokenizer->bufpos + 5);

                    strcpy(new_label, buffer);
                    
                    snprintf(new_label + cfg_tokenizer->bufpos, 5, "_M%d", entry->offset);

                    if(current_macro->macro_deflen + cfg_tokenizer->bufpos + 4 >= current_macro->macro_defsize) {
                        current_macro->macro_defsize += 1024;
                        current_macro->macro_definition = realloc(current_macro->macro_definition, current_macro->macro_defsize);
                        memset((void *)(current_macro->macro_definition + current_macro->macro_deflen), 0, 1024);
                    }

                    strcpy(current_macro->macro_definition + current_macro->macro_deflen, new_label);
                    current_macro->macro_deflen += cfg_tokenizer->bufpos + 4;
                    free(new_label);

                    lookahead = get_next_token(cfg_tokenizer);

                    continue;
                }
            }
            while(*buffer != '\0') {
                if(current_macro->macro_deflen + 1 >= current_macro->macro_defsize) {
                    current_macro->macro_defsize += 1024;
                    current_macro->macro_definition = realloc(current_macro->macro_definition, current_macro->macro_defsize);
                    memset((void *)(current_macro->macro_definition + current_macro->macro_deflen), 0, 1024);
                }
                current_macro->macro_definition[current_macro->macro_deflen++] = *buffer++;
            }
            if(lookahead == TOK_EOL) break;
            
            lookahead = get_next_token(cfg_tokenizer);
        }
    }
}

void macro_label_cfg(FILE *fp, long init_offset) {
    static int macro_unique = 0;

    int ch;
    long curr_offset;
    char *identifier = strdup_wrap(cfg_tokenizer->lexbuf);
    size_t identifier_len = cfg_tokenizer->bufpos;

    match(TOK_IDENTIFIER);

    if(lookahead == TOK_COLON) {
        if(mode == MACRO_DEFAULT_MODE) {
            free(identifier);
            curr_offset = ftell(cfg_tokenizer->fstream);
            fseek(cfg_tokenizer->fstream, init_offset, SEEK_SET);

            while(init_offset != curr_offset) {
                ch = fgetc(cfg_tokenizer->fstream);
                fputc(ch, fp);
                ++init_offset;
            }
        }
        else if(mode == MACRO_DEFINITION_MODE) {
            struct symbol_table_entry *entry = get_symbol_table(current_macro->symtab, identifier);
            
            if(entry == NULL) {
                entry = insert_symbol_table(current_macro->symtab, identifier);
                entry->offset = macro_unique++;
            }

            char *new_label = malloc(identifier_len + 5);

            strcpy(new_label, identifier);
            
            snprintf(new_label + identifier_len, 5, "_M%d:", entry->offset);

            if(current_macro->macro_deflen + identifier_len + 4 >= current_macro->macro_defsize) {
                current_macro->macro_defsize += 1024;
                current_macro->macro_definition = realloc(current_macro->macro_definition, current_macro->macro_defsize);
                memset((void *)(current_macro->macro_definition + current_macro->macro_deflen), 0, 1024);
            }

            strcpy(current_macro->macro_definition + current_macro->macro_deflen, new_label);

            current_macro->macro_deflen += identifier_len + 4;

            free(identifier);
            free(new_label);
        }
    }
    else {
        free(identifier);
        macro_write_line(fp, init_offset);
    }
}

struct macro_op_node *macro_operand_list_cfg() {
    struct macro_op_node *root = NULL; 
    struct macro_op_node *current_operand = NULL;

    match(TOK_LPAREN);
    
    while(1) {
        switch(lookahead) {
            case TOK_IDENTIFIER: {
                if(*cfg_tokenizer->attrbuf != '%' && *cfg_tokenizer->attrbuf != '$') {
                    fprintf(stderr, "Error: Invalid operand '%s' for .macro on line %ld\n", cfg_tokenizer->lexbuf, cfg_tokenizer->lineno);
                    cfg_continue();
                    status = MPASS_STATUS_FAIL;
                    return root;
                }

                struct macro_op_node *operand = malloc(sizeof(struct macro_op_node));
                
                if(operand == NULL) {
                    fprintf(stderr, "Critical Error: Failed to allocate memory for macro operand node\n");
                    exit(EXIT_FAILURE);
                }
                
                operand->operand_name = strdup_wrap(cfg_tokenizer->attrbuf);
                operand->next = NULL;

                if(current_operand == NULL) {
                    current_operand = operand;
                    if(root == NULL) root = operand;
                }
                else {
                    current_operand->next = operand;
                    current_operand = operand;
                }

                match(TOK_IDENTIFIER);
                break;
            }
            case TOK_RPAREN: {
                size_t lineno = cfg_tokenizer->lineno;
                size_t colno = cfg_tokenizer->colno;

                match(TOK_RPAREN);

                if(lookahead != TOK_EOL) {
                    fprintf(stderr, "Error: Unexpected token %s on line %ld, col %ld\n", get_token_str(lookahead), lineno, colno);
                }
                else {
                    mode = MACRO_DEFINITION_MODE;
                }
                return root;
            }
            case TOK_COMMA: {
                size_t lineno = cfg_tokenizer->lineno;
                size_t colno = cfg_tokenizer->colno;
                match(TOK_COMMA);
                if(lookahead != TOK_IDENTIFIER) {
                    fprintf(stderr, "Error: Expected .macro operand on line %ld, col %ld\n", lineno, colno);
                    status = MPASS_STATUS_FAIL;
                    cfg_continue();
                    return root;
                }
                break;
            }
            default:
                fprintf(stderr, "Error: Unexpected %s on line %ld\n", get_token_str(lookahead), cfg_tokenizer->lineno);
                status = MPASS_STATUS_FAIL;
                cfg_continue();
                return root;
        }
    }

    return root;
}

struct macro_node *macro_begin_cfg() {
    struct macro_node *macro = NULL;

    match(TOK_MACRO);

    if(lookahead == TOK_IDENTIFIER) {
        macro = malloc(sizeof(struct macro_node));

        if(macro == NULL) {
            fprintf(stderr, "Critical Error: Failed to allocate memory for macro entry\n");
            exit(EXIT_FAILURE);
        }

        macro->macro_name = strdup_wrap(cfg_tokenizer->attrbuf);
        macro->macro_definition = NULL;
        macro->macro_defsize = 0;
        macro->macro_deflen = 0;
        macro->symtab = create_symbol_table();
        macro->operand_list = NULL;

        match(TOK_IDENTIFIER);

        switch(lookahead) {
            case TOK_LPAREN:
                macro->operand_list = macro_operand_list_cfg();
                break;
            case TOK_EOL: {
                mode = MACRO_DEFINITION_MODE;
                break;
            }
            default:
                fprintf(stderr, "Error: Unexpected token after .macro identifier on line %ld\n", cfg_tokenizer->lineno);
                cfg_continue();
                mode = MACRO_DEFAULT_MODE;
                status = MPASS_STATUS_FAIL;
                
                free(macro->macro_name);
                free(macro);
                
                macro = NULL;
        }
    }
    else {
        fprintf(stderr, "Error: Expected identifier after .macro on line %ld, col %ld\n", cfg_tokenizer->lineno, cfg_tokenizer->colno);
        cfg_continue();
        mode = MACRO_DEFAULT_MODE;
        status = MPASS_STATUS_FAIL;
    }

    return macro;
}

void macro_end_cfg() {
    match(TOK_MACRO_END);

    if(mode != MACRO_DEFINITION_MODE) {
        fprintf(stderr, "Error: .end_macro declared without .macro on line %ld\n", cfg_tokenizer->lineno);
        cfg_continue();
        status = MPASS_STATUS_FAIL;
    }
    else {
        printf("        > definition: ");
        char *buffer = current_macro->macro_definition;
        while(*buffer != '\0') {
            printf("%c", *buffer);
            if(*buffer++ == '\n') {
                printf("                      ");
            }
        }
        printf("\n");
    }

    if(lookahead != TOK_EOL) {
        fprintf(stderr, "Error: Unexpected token after .end_macro on line %ld, col %ld\n", cfg_tokenizer->lineno, cfg_tokenizer->colno);
        cfg_continue();
        status = MPASS_STATUS_FAIL;
    }

    mode = MACRO_DEFAULT_MODE;
}

void macro_cfg(FILE *fp) {
    long init_offset = 0;

    while(lookahead != TOK_NULL) {
        switch(lookahead) {
            case TOK_MACRO: {
                struct macro_node *macro = macro_begin_cfg();
                if(macro != NULL) {
                    printf("> .macro %s\n", macro->macro_name);
                    struct macro_op_node *current_operand = macro->operand_list;
                    int i = 0;
                    while(current_operand != NULL) {
                        printf("    > arg%d: %s\n", i++, current_operand->operand_name);
                        current_operand = current_operand->next;
                    }
                }
                current_macro = macro;
                break;
            }
            case TOK_IDENTIFIER:
                macro_label_cfg(fp, init_offset);
                break;
            case TOK_MACRO_END:
                macro_end_cfg();
                break;
            default:
                macro_write_line(fp, init_offset);
                break;
        }
        init_offset = ftell(cfg_tokenizer->fstream);
        lookahead = get_next_token(cfg_tokenizer);
    }

    if(mode == MACRO_DEFINITION_MODE) {
        fprintf(stderr, "Error: .macro definition started but didn't recieve a .end_macro\n");
        status = MPASS_STATUS_FAIL;
    }
}

char *assembler_macro_pass(const char *program) {
    struct tokenizer *tokenizer = create_tokenizer(program);
    char *tempfile = "test.i";
    FILE *tempfp;

    if(tokenizer == NULL) {
        fprintf(stderr, "Failed to create tokenizer\n");
        exit(EXIT_FAILURE);
    }
    
    if(tempfile == NULL) {
        fprintf(stderr, "Failed to get temporary file\n");
        return NULL;
    }
    
    if((tempfp = fopen_wrap(tempfile, "w")) == NULL) {
        fprintf(stderr, "Failed to open temporary file: %s\n", tempfile);
        return NULL;
    }

    cfg_tokenizer = tokenizer;
    lookahead = get_next_token(tokenizer);
    mode = MACRO_DEFAULT_MODE;
    status = MPASS_STATUS_OK;

    macro_cfg(tempfp);

    if(status != MPASS_STATUS_OK) {
        fprintf(stderr, "\n%s: Failed to assemble program...\n", program);
        exit(EXIT_FAILURE);
    }

    return tempfile;
}

int main(int argc, char *argv[]) {
	if(argc != 2) {
		fprintf(stderr, "Usage: %s [file]\n", argv[0]);
		return EXIT_FAILURE;
	}
    printf("Copied file '%s' to '%s' successfully\n", argv[1], assembler_macro_pass(argv[1]));
    return EXIT_SUCCESS;
}