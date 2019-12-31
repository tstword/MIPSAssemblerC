#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenizer.h"
#include "funcwrap.h"

#define MACRO_DEFAULT_MODE      0x0
#define MACRO_DEFINITION_MODE   0x1
#define MACRO_EXPANSION_MODE    0x2

token_t lookahead;
struct tokenizer *cfg_tokenizer;
int mode;

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

void macro_label_cfg(FILE *fp, long init_offset) {
    long curr_offset;
    int ch;

    match(TOK_IDENTIFIER);

    if(lookahead == TOK_COLON) {
        curr_offset = ftell(cfg_tokenizer->fstream);
        
        fseek(cfg_tokenizer->fstream, init_offset, SEEK_SET);

        while(init_offset != curr_offset) {
            ch = fgetc(cfg_tokenizer->fstream);
            fputc(ch, fp);
            ++init_offset;
        }

        if(lookahead != TOK_MACRO) {
            while((ch = fgetc(cfg_tokenizer->fstream)) != EOF) { 
                if(ch == '\n') {
                    fputc(ch, fp);
                    ++cfg_tokenizer->lineno;
                    cfg_tokenizer->colno = 0;
                    break;
                }
            }
        }
    }
    else {
        fseek(cfg_tokenizer->fstream, init_offset, SEEK_SET);
        while((ch = fgetc(cfg_tokenizer->fstream)) != EOF) { 
            fputc(ch, fp);
            if(ch == '\n') {
                ++cfg_tokenizer->lineno;
                cfg_tokenizer->colno = 0;
                break;
            }
        }
    }
}

void macro_begin_cfg() {
    match(TOK_MACRO);

    if(lookahead == TOK_IDENTIFIER) {
        match(TOK_IDENTIFIER);
        switch(lookahead) {
            case TOK_LPAREN:
                //macro_operand_list_cfg();
                break;
            case TOK_EOL: {
                mode = MACRO_DEFINITION_MODE;
                break;
            }
            default:
                fprintf(stderr, "Unexpected token after .macro on line %ld\n", cfg_tokenizer->lineno);
                cfg_continue();
        }
    }
    else {
        fprintf(stderr, "Error: Expected identifier after .macro on line %ld, col %ld\n", cfg_tokenizer->lineno, cfg_tokenizer->colno);
        cfg_continue();     
    }
}

void macro_cfg(FILE *fp) {
    long init_offset = 0;

    while(lookahead != TOK_NULL) {
        switch(lookahead) {
            case TOK_MACRO:
                macro_begin_cfg();
                break;
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
}

// if(lookahead == TOK_MACRO) {
            
//                 macro_begin_cfg();
//             }
//             init_offset = ftell(cfg_tokenizer->fstream);
//         }
//         else if(lookahead == TOK_IDENTIFIER) {
//             macro_label_cfg(tempfp, init_offset);
//             init_offset = ftell(cfg_tokenizer->fstream);
//         }
//         else if(lookahead == TOK_MACRO_END) {
//             if(mode == MACRO_DEFINITION_MODE) {
//                 mode = MACRO_DEFAULT_MODE;
//             }
//             else {
//                 fprintf(stderr, "Unexpected .end_macro without definition of .macro on line %ld\n", cfg_tokenizer->lineno);
//                 cfg_continue();
//             }
//             init_offset = ftell(cfg_tokenizer->fstream);
//         }
//         else {
//             if(mode == MACRO_DEFAULT_MODE) {
//                 fseek(tokenizer->fstream, init_offset, SEEK_SET);
//                 while((ch = fgetc(tokenizer->fstream)) != EOF) {
//                     fputc(ch, tempfp);
//                     if(ch == '\n') {
//                         ++cfg_tokenizer->lineno;
//                         cfg_tokenizer->colno = 0;
//                         break;
//                     }
//                 }
//                 init_offset = ftell(cfg_tokenizer->fstream);
//             } 
//             else {
//                 cfg_continue();
//                 init_offset = ftell(cfg_tokenizer->fstream);
//             }
//         }
//         lookahead = get_next_token(cfg_tokenizer);
//     }

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

    macro_cfg();

    return tempfile;
}

char *macro_test(const char *program) {
    struct tokenizer *tokenizer = create_tokenizer(program);
    if(tokenizer == NULL) {
        fprintf(stderr, "Failed to create tokenizer\n");
        exit(EXIT_FAILURE);
    }

    FILE *fp = tokenizer->fstream;
    int ch;

    const char *basename = strrchr(program, '/');
    if(basename == NULL) {
        basename = program;
    }
    else {
        ++basename;
    }

    char *tempfile = "file.i";
    if(tempfile == NULL) {
        fprintf(stderr, "Failed to get temporary file name\n");
        exit(EXIT_FAILURE);
    }

    FILE *tempfp = fopen_wrap(tempfile, "w");
    if(tempfp == NULL) {
        fprintf(stderr, "Failed to create temporary file: %s\n", tempfile);
        exit(EXIT_FAILURE);
    }

    /* Task 1: Ignore any lines with directives */
    // long file_offset = 0;
    // token_t token = TOK_INVALID;
    // ch = 0;

    // while(ch != EOF && token != TOK_NULL) {
    //     file_offset = ftell(fp);

    //     if((token = get_next_token(tokenizer)) != TOK_DIRECTIVE) {
    //         fseek(fp, file_offset, SEEK_SET);
    //         while((ch = fgetc(fp)) != EOF) {
    //             fputc(ch, tempfp);
    //             if(ch == '\n') break;
    //         }
    //     } 
    //     else {
    //         while(token != TOK_EOL && token != TOK_NULL) {
    //             token = get_next_token(tokenizer);
    //         }
    //     }
    // }
    /* End Task */

    /* Task 2: Ignore any lines with directives, but keep the labels */
    // long file_offset = 0;
    // token_t token;
    // ch = 0;

    // cfg_tokenizer = tokenizer;
    // lookahead = get_next_token(tokenizer);
    
    // do {
    //     if(lookahead == TOK_IDENTIFIER) {
    //         match(TOK_IDENTIFIER);
    //         long curr_offset = ftell(fp);
    //         if(match(TOK_COLON)) {
    //             fseek(fp, file_offset, SEEK_SET);
    //             while(file_offset != curr_offset) {
    //                 ch = fgetc(fp);
    //                 fputc(ch, tempfp);
    //                 ++file_offset;
    //             }
    //             while((ch = fgetc(fp)) != EOF) { 
    //                 if(ch == '\n') {
    //                     fputc(ch, tempfp);
    //                     break;
    //                 }
    //             }
    //         } 
    //         else {
    //             fseek(fp, file_offset, SEEK_SET);
    //             while((ch = fgetc(fp)) != EOF) {
    //                 fputc(ch, tempfp);
    //                 if(ch == '\n') break;
    //             }
    //         }
    //     }
    //     else if(lookahead == TOK_DIRECTIVE) {
    //         while(lookahead != TOK_EOL && lookahead != TOK_NULL) { 
    //             lookahead = get_next_token(tokenizer);
    //         }
    //     }
    //     else {
    //         fseek(fp, file_offset, SEEK_SET);
    //         while((ch = fgetc(fp)) != EOF) {
    //             fputc(ch, tempfp);
    //             if(ch == '\n') break;
    //         }
    //     }
    //     file_offset = ftell(fp);
    //     lookahead = get_next_token(tokenizer);
    // } while(ch != EOF && lookahead != TOK_NULL);

    /* End task */

    /* Task 3: Recognize .macro and .end_macro blocks. Instead of storing data into MDT. Syntax for MACROS:
    * macro -> macro_begin instruction_list macro_end |
    *          label macro_begin instruction_list macro_end
    *
    * macro_begin   -> <begin_macro> <identifier> <lparen> macro_oplist <rparen> <EOL> |
    *                  <begin_macro> <identifier> <lparen> <rparen> <EOL> |
    *                  <begin_macro> <identifier> <EOL>
    *
    * macro_oplist -> macro_operand <comma> macro_oplist |
    *                 macro_operand
    *
    * macro_operand -> <percent> <identifer>
    *
    * macro_end -> <end_macro> <EOL>
    **/

    long file_offset = 0, curr_offset;
    ch = 0;

    cfg_tokenizer = tokenizer;
    lookahead = get_next_token(tokenizer);

    do {
    if(lookahead == TOK_IDENTIFIER) {
        match(TOK_IDENTIFIER);
        
        curr_offset = ftell(fp);
        
        if(lookahead == TOK_COLON) {
            fseek(fp, file_offset, SEEK_SET);
            
            while(file_offset != curr_offset) {
                ch = fgetc(fp);
                fputc(ch, tempfp);
                ++file_offset;
            }
        }
        else {
            fseek(fp, file_offset, SEEK_SET);

            while((ch = fgetc(fp)) != EOF) {
                fputc(ch, tempfp);
                if(ch == '\n') break;
            }
        }
    }
    else if(lookahead == TOK_MACRO) {
        match(TOK_MACRO);

        if(lookahead == TOK_IDENTIFIER) {
            match(TOK_IDENTIFIER);
            if(lookahead == TOK_LPAREN) {

            }
            else if(lookahead == TOK_EOL) {
                while(lookahead != TOK_MACRO_END) {
                    if(lookahead == TOK_NULL) {
                        fprintf(stderr, "Error: No .end_macro defined\n");
                        break;
                    }
                    lookahead = get_next_token(tokenizer);
                }

                if(lookahead == TOK_MACRO_END) match(TOK_MACRO_END);

                if(lookahead != TOK_EOL) {
                    fprintf(stderr, "Error: Expected end of line after .end_macro\n");
                    break;
                }
            }
            else {
                fprintf(stderr, "Unexpected token after identifier on .macro\n");
                break;
            }
        }
        else {
            fprintf(stderr, "Error: Expected identifier after .macro\n");
            break;
        }
    }
    else {
        fseek(fp, file_offset, SEEK_SET);
        while((ch = fgetc(fp)) != EOF) {
            fputc(ch, tempfp);
            if(ch == '\n') break;
        }
    }

    file_offset = ftell(fp);
    lookahead = get_next_token(tokenizer);
    
    } while(ch != EOF && lookahead != TOK_NULL);

    /* End task */

    destroy_tokenizer(&tokenizer);
    fclose(tempfp);

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