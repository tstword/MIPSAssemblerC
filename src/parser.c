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
            if(get_symbol_table(symbol_table, id) != NULL) {
                report_cfg("Multiple definitions of label '%s' on line %ld, col %ld", id, cfg_parser->lineno, cfg_parser->colno);
            } else { 
                struct symbol_table_entry *entry = insert_symbol_table(symbol_table, id);
                entry->value.offset = cfg_parser->LC;
                entry->value.segment = cfg_parser->segment;
            }
        } else {
            report_cfg("Unrecognized mnemonic '%s' on line %ld, col %ld", id, cfg_parser->lineno, cfg_parser->colno);
        }
        free(id);
    } else {
        report_cfg(NULL);
    }
}

struct operand_node *operand_cfg() {
    struct operand_node *node = NULL;

    switch(cfg_parser->lookahead) {
        case TOK_REGISTER: {
            int value = cfg_parser->tokenizer->attrval;
            match_cfg(TOK_REGISTER);
            node = malloc(sizeof(struct operand_node));
            node->operand = OPERAND_REGISTER;
            node->value.reg = value;
            node->next = NULL;
            break;
        }
        case TOK_IDENTIFIER: {
            char *id = strdup(cfg_parser->tokenizer->lexbuf);
            match_cfg(TOK_IDENTIFIER);
            node = malloc(sizeof(struct operand_node));
            node->operand = OPERAND_LABEL;
            node->identifier = id;
            node->next = NULL;
            break;
        }
        case TOK_INTEGER: {
            int value = cfg_parser->tokenizer->attrval;
            match_cfg(TOK_INTEGER);
            node = malloc(sizeof(struct operand_node));
            node->operand = OPERAND_IMMEDIATE;
            node->value.integer = value;
            node->next = NULL;
            if(cfg_parser->lookahead == TOK_LPAREN) {
                match_cfg(TOK_LPAREN);
                int reg_value = cfg_parser->tokenizer->attrval;
                if(match_cfg(TOK_REGISTER) && match_cfg(TOK_RPAREN)) {
                    node->operand = OPERAND_ADDRESS;
                    node->value.reg = reg_value;
                }
            }
            break;
        }
		case TOK_EOL:
		case TOK_NULL:
			report_cfg("Expected operand after line %ld, col %ld", cfg_parser->lineno, cfg_parser->colno);
			break;
        default:
            report_cfg("Invalid operand '%s' on line %ld, col %ld", cfg_parser->tokenizer->lexbuf, cfg_parser->lineno, cfg_parser->colno);
    }
    
    return node;
}

struct operand_node *operand_list_cfg() {
    struct operand_node *node = NULL;

    switch(cfg_parser->lookahead) {
        case TOK_REGISTER:
        case TOK_IDENTIFIER:
        case TOK_INTEGER:
            node = operand_cfg();
            if(cfg_parser->lookahead == TOK_COMMA) {
                match_cfg(TOK_COMMA);
                if(node != NULL)
                    node->next = operand_list_cfg();
                else
                    node = operand_list_cfg();
            }
            break;
		case TOK_EOL:
		case TOK_NULL:
			report_cfg("Expected operand after line %ld, col %ld", cfg_parser->lineno, cfg_parser->colno);
			break;
        default:
            report_cfg("Invalid operand '%s' on line %ld, col %ld", cfg_parser->tokenizer->lexbuf, cfg_parser->lineno, cfg_parser->colno);
    }

    return node;
}

void verify_instruction(struct instruction_node *instr) {
    if(instr == NULL || instr->mnemonic == NULL) return;

    /* Verify operand list */
    struct operand_node *operand = instr->operand_list;
    for(int i = 0; i < 3; ++i) {
        if(((struct opcode_entry *)instr->mnemonic->attrptr)->operand[i] == OPERAND_NONE) {
            if(operand != NULL) report_cfg("Instruction '%s' takes too many operands on line %ld", instr->mnemonic->id, cfg_parser->lineno);
            break;
        } else {
            if(operand == NULL || (((struct opcode_entry *)instr->mnemonic->attrptr)->operand[i] & operand->operand) == 0) {
                report_cfg("Invalid operand combiniation for instruction '%s' on line %ld", instr->mnemonic->id, cfg_parser->lineno);
                break;
            }
        }
        operand = operand->next;
    }
}

struct instruction_node *instruction_cfg() {
    struct instruction_node *node = NULL;

    switch(cfg_parser->lookahead) {
        case TOK_IDENTIFIER:
            label_cfg();
            if(cfg_parser->lookahead == TOK_MNEMONIC) {
                node = malloc(sizeof(struct instruction_node));
                node->mnemonic = cfg_parser->tokenizer->attrptr;
                node->next = NULL;
                match_cfg(TOK_MNEMONIC);
                switch(cfg_parser->lookahead) {
                    case TOK_IDENTIFIER:
                    case TOK_INTEGER:
                    case TOK_REGISTER:
                        node->operand_list = operand_list_cfg();
                        break;
                    default:
                        node->operand_list = NULL;
                }
				if(((struct opcode_entry *)node->mnemonic->attrptr)->psuedo) {
                	cfg_parser->LC += ((struct opcode_entry *)node->mnemonic->attrptr)->size;
				} else {
					cfg_parser->LC += 0x4;
				}
            }
            end_line_cfg();
            break;
        case TOK_MNEMONIC:
            node = malloc(sizeof(struct instruction_node));
            node->mnemonic = cfg_parser->tokenizer->attrptr;
            node->next = NULL;
            match_cfg(TOK_MNEMONIC);
            switch(cfg_parser->lookahead) {
                case TOK_IDENTIFIER:
                case TOK_INTEGER:
                case TOK_REGISTER:
                    node->operand_list = operand_list_cfg();
                    break;
                default:
                    node->operand_list = NULL;
            }
            end_line_cfg();
            if(((struct opcode_entry *)node->mnemonic->attrptr)->psuedo) {
				cfg_parser->LC += ((struct opcode_entry *)node->mnemonic->attrptr)->size;
			} else {
				cfg_parser->LC += 0x4;
			}
            break;
        case TOK_EOL:
        case TOK_NULL:
            end_line_cfg();
            break;
        default:
            report_cfg("Unexpected %s on line %ld, col %ld", get_token_str(cfg_parser->lookahead), cfg_parser->lineno, cfg_parser->colno);
    }

    verify_instruction(node);

    return node;
}

struct instruction_node *instruction_list_cfg() {
    struct instruction_node *node = NULL;
    if(cfg_parser->lookahead != TOK_NULL) {
        node = instruction_cfg();
        if(node != NULL)
            node->next = instruction_list_cfg();
        else
            node = instruction_list_cfg();
    }
    return node;
}

struct program_node *program_cfg(struct parser *parser) {
    cfg_parser = parser;
    struct program_node *node = malloc(sizeof(struct program_node));
    node->instruction_list = instruction_list_cfg();
    return node;
}

struct parser *create_parser(struct tokenizer *tk) {
    struct parser *parser = malloc(sizeof(struct parser));

    parser->tokenizer = tk;
    parser->lookahead = TOK_NULL;
    parser->status = PARSER_STATUS_NULL;
    parser->lineno = 1;
    parser->colno = 1;
    parser->LC = 0x00000000;
    parser->ast = NULL;
    parser->segment = SEGMENT_TEXT;
    
    return parser;
}

pstatus_t execute_parser(struct parser *parser) {
    /* Get lookahead token... */
    parser->lookahead = get_next_token(parser->tokenizer);
    
    /* Start grammar recognization... */
    parser->ast = program_cfg(parser);
    //program_cfg(parser);

    if(parser->status == PARSER_STATUS_NULL) {
        parser->status = PARSER_STATUS_OK;
    }

    return parser->status;
}

void destroy_parser(struct parser **parser) {
    /* Destory AST */
    struct instruction_node *instr_node = (*parser)->ast ? (*parser)->ast->instruction_list : NULL;
    /* Free instructions */
    while(instr_node != NULL) {
        struct instruction_node *next_instr = instr_node->next;
        /* Free operands */
        struct operand_node *op_node = instr_node->operand_list;
        while(op_node != NULL) {
            struct operand_node *next_op = op_node->next;
            if(op_node->operand == OPERAND_LABEL) free(op_node->identifier);
            free(op_node);
            op_node = next_op;
        }
        free(instr_node);
        instr_node = next_instr;
    }

    /* Free AST */
    free((*parser)->ast);

    /* Simple free the data */
    free(*parser);

    /* Set parser to NULL */
    *parser = NULL;
}
