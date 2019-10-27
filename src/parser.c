#include "parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

/* Global variable used for parsing grammar */
struct parser *cfg_parser = NULL;

void report_cfg(const char *fmt, ...) {
    va_list vargs;
    size_t bufsize = 0;
    char *buffer = NULL;

    if(fmt != NULL) {
        va_start(vargs, fmt);
        bufsize = vsnprintf(NULL, 0, fmt, vargs) + 1;
        va_end(vargs);

        buffer = malloc(bufsize * sizeof(char));

        va_start(vargs, fmt);
        vsnprintf(buffer, bufsize, fmt, vargs);
        va_end(vargs);
    }

    cfg_parser->status = PARSER_STATUS_FAIL;

    if(cfg_parser->lookahead == TOK_INVALID) fprintf(stderr, "Lexical Error: %s\n", cfg_parser->tokenizer->errmsg);
    else if(buffer != NULL) fprintf(stderr, "Syntax Error: %s\n", buffer);

    /* Recover and skip to next line to retrieve extra data */
    while(cfg_parser->lookahead != TOK_EOL && cfg_parser->lookahead != TOK_NULL) {
        cfg_parser->lookahead = get_next_token(cfg_parser->tokenizer);
    }

    /* Free buffer */
    free(buffer);
}

int match_cfg(token_t token) {
    if(cfg_parser->lookahead == token) {
        cfg_parser->lineno = cfg_parser->tokenizer->lineno;
        cfg_parser->colno = cfg_parser->tokenizer->colno;
        cfg_parser->lookahead = get_next_token(cfg_parser->tokenizer);
    } else {
        report_cfg("Expected %s on line %ld, col %ld", get_token_str(token), cfg_parser->lineno, cfg_parser->colno);
        return 0;
    }
    return 1;
}

void end_line_cfg() {
    switch(cfg_parser->lookahead) {
        case TOK_EOL:
            match_cfg(TOK_EOL);
            break;
        case TOK_NULL:
            break;
        default:
            report_cfg("Unexpected %s on line %ld, col %ld", get_token_str(cfg_parser->lookahead), cfg_parser->lineno, cfg_parser->colno);
    }
}

void label_cfg() {
    if(cfg_parser->lookahead == TOK_IDENTIFIER) {
        char *id = strdup(cfg_parser->tokenizer->lexbuf);
        match_cfg(TOK_IDENTIFIER);
        if(cfg_parser->lookahead == TOK_COLON) {
            match_cfg(TOK_COLON);
            if(get_symbol_table(symbol_table, id) != NULL)
                report_cfg("Multiple definitions of label '%s' on line %ld, col %ld", id, cfg_parser->lineno, cfg_parser->colno);
            else 
                insert_symbol_table(symbol_table, id)->value.offset = cfg_parser->LC;
        } else {
            report_cfg("Unrecognized mnemonic '%s' on line %ld, col %ld", id, cfg_parser->lineno, cfg_parser->colno);
        }
        free(id);
    } else {
        report_cfg(NULL);
    }
}

void operand_cfg() {
    switch(cfg_parser->lookahead) {
        case TOK_REGISTER:
            match_cfg(TOK_REGISTER);
            break;
        case TOK_IDENTIFIER:
            match_cfg(TOK_IDENTIFIER);
            break;
        case TOK_INTEGER:
            match_cfg(TOK_INTEGER);
            if(cfg_parser->lookahead == TOK_LPAREN) {
                if(!(match_cfg(TOK_LPAREN) && 
                     match_cfg(TOK_REGISTER) && 
                     match_cfg(TOK_RPAREN))) break;
            }
            break;
		case TOK_EOL:
		case TOK_NULL:
			report_cfg("Expected operand after line %ld, col %ld", cfg_parser->lineno, cfg_parser->colno);
			break;
        default:
            report_cfg("Invalid operand '%s' on line %ld, col %ld", cfg_parser->tokenizer->lexbuf, cfg_parser->lineno, cfg_parser->colno);
    }
}

void operand_list_cfg() {
    switch(cfg_parser->lookahead) {
        case TOK_REGISTER:
        case TOK_IDENTIFIER:
        case TOK_INTEGER:
            operand_cfg();
            if(cfg_parser->lookahead == TOK_COMMA) {
                match_cfg(TOK_COMMA);
                operand_list_cfg();
            }
            break;
		case TOK_EOL:
		case TOK_NULL:
			report_cfg("Expected operand after line %ld, col %ld", cfg_parser->lineno, cfg_parser->colno);
			break;
        default:
            report_cfg("Invalid operand '%s' on line %ld, col %ld", cfg_parser->tokenizer->lexbuf, cfg_parser->lineno, cfg_parser->colno);
    }
}

void instruction_cfg() {
    switch(cfg_parser->lookahead) {
        case TOK_IDENTIFIER:
            label_cfg();
            if(cfg_parser->lookahead == TOK_MNEMONIC) {
                match_cfg(TOK_MNEMONIC);
                operand_list_cfg();
                cfg_parser->LC += 0x8;
            }
            end_line_cfg();
            break;
        case TOK_MNEMONIC:
            match_cfg(TOK_MNEMONIC);
            switch(cfg_parser->lookahead) {
                case TOK_IDENTIFIER:
                case TOK_INTEGER:
                case TOK_REGISTER:
                    operand_list_cfg();
            }
            end_line_cfg();
            cfg_parser->LC += 0x8;
            break;
        case TOK_EOL:
        case TOK_NULL:
            end_line_cfg();
            break;
        default:
            report_cfg("Unexpected %s on line %ld, col %ld", get_token_str(cfg_parser->lookahead), cfg_parser->lineno, cfg_parser->colno);
    }
}

void instruction_list_cfg() {
    if(cfg_parser->lookahead != TOK_NULL) {
        instruction_cfg();
        instruction_list_cfg();
    }
}

void program_cfg(struct parser *parser) {
    cfg_parser = parser;
    instruction_list_cfg();
}

struct parser *create_parser(struct tokenizer *tk) {
    struct parser *parser = malloc(sizeof(struct parser));

    parser->tokenizer = tk;
    parser->lookahead = TOK_NULL;
    parser->status = PARSER_STATUS_NULL;
    parser->lineno = 1;
    parser->colno = 1;
    parser->LC = 0x00400000;
    
    return parser;
}

pstatus_t execute_parser(struct parser *parser) {
    /* Get lookahead token... */
    parser->lookahead = get_next_token(parser->tokenizer);
    
    /* Start grammar recognization... */
    program_cfg(parser);

    if(parser->status == PARSER_STATUS_NULL) {
        parser->status = PARSER_STATUS_OK;
    }

    return parser->status;
}

void destroy_parser(struct parser **parser) {
    /* Simple free the data */
    free(*parser);

    /* Set parser to NULL */
    *parser = NULL;
}
