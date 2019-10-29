/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * File: parser.c
 *
 * Purpose: Responsible for parsing and verifying the sequence of tokens
 * as a MIPS assembly program. The parser is implemented as a recursive-descent
 * parser, as a result the language is defined as a LL(1) grammer:
 *
 *     program          -> instruction_list
 *
 *     instruction_list -> instruction instruction_list | 
 *                         <EOF>
 *
 *     instruction      -> label <EOL> | 
 *                         label <MNEMONIC> operand_list <EOL> | 
 *                         <EOL> ; NO-OP
 *
 *     operand_list     -> operand <COMMA> operand_list | 
 *                         operand
 * 
 *     operand          -> <REGISTER>   | 
 *                         <IDENTIFIER> | 
 *                         <INTEGER>    |
 *                         <INTEGER> <LPAREN> <REGISTER> <RPAREN>
 *                    
 *     label            -> <ID> <COLON>
 *
 * Failure in parsing the tokens is indicated by the macro PARSER_STATUS_FAIL
 * Success in parsing the tokens is indicated by the macro PARSER_STATUS_OK
 * 
 * @author: Bryan Rocha
 * @version: 1.0 (8/28/2019)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

/* Global variable used for parsing grammar */
struct parser *cfg_parser = NULL;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: report_cfg
 * Purpose: Reports an error in the context-free grammar to stderr
 * @param fmt -> Format string
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void report_cfg(const char *fmt, ...) {
    /* Routine to allow formatted printing similar to printf */
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
    /* End routine */

    cfg_parser->status = PARSER_STATUS_FAIL;

    /* Check if tokenizer failed, otherwise CFG failed */
    if(cfg_parser->lookahead == TOK_INVALID) fprintf(stderr, "%s: Error: %s\n", (const char*)cfg_parser->current_file->value, cfg_parser->tokenizer->errmsg);
    else if(buffer != NULL) fprintf(stderr, "%s: Error: %s\n", (const char*)cfg_parser->current_file->value, buffer);

    /* Recover and skip to next line to retrieve extra data */
    while(cfg_parser->lookahead != TOK_EOL && cfg_parser->lookahead != TOK_NULL) {
        cfg_parser->lookahead = get_next_token(cfg_parser->tokenizer);
    }

    /* Free buffer */
    free(buffer);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: match_cfg
 * Purpose: Checks if the most recent token returned by the tokenizer matches
 * the token passed to the function. On a non-match, it reports an error and fails
 * the parser
 * @param token -> Token to match
 * @return 1 if token matches, 0 otherwise
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: end_line_cfg
 * Purpose: Checks and matches for the end of line token, if fails reports
 * specific error to the report_cfg function
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: label_cfg
 * Purpose: Attempts to match the non-terminal for label. If matched, the label
 * is inserted into the symbol table. Otherwise, an error is reported to the 
 * function report_cfg
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void label_cfg() {
    if(cfg_parser->lookahead == TOK_IDENTIFIER) {
        char *id = strdup(cfg_parser->tokenizer->lexbuf);
        match_cfg(TOK_IDENTIFIER);
        if(cfg_parser->lookahead == TOK_COLON) {
            match_cfg(TOK_COLON);
            struct symbol_table_entry *entry;
            if((entry = get_symbol_table(symbol_table, id)) != NULL) {
                if(entry->status != SYMBOL_UNDEFINED) {
                    entry->status = SYMBOL_DOUBLY;
                    report_cfg("Multiple definitions of label '%s' on line %ld, col %ld", id, cfg_parser->lineno, cfg_parser->colno);
                } else {
                    entry->offset = cfg_parser->LC;
                    entry->segment = cfg_parser->segment;
                    entry->status = SYMBOL_DEFINED;
                }
            } else { 
                entry = insert_symbol_table(symbol_table, id);
                entry->offset = cfg_parser->LC;
                entry->segment = cfg_parser->segment;
                entry->status = SYMBOL_DEFINED;
            }
        } else {
            report_cfg("Unrecognized mnemonic '%s' on line %ld, col %ld", id, cfg_parser->lineno, cfg_parser->colno);
        }
        free(id);
    } else {
        report_cfg(NULL);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: operand_cfg
 * Purpose: Attempts to match the non-terminal for operand. Failure to match
 * results in reporting the error to report_cfg.
 * @return Address of the operand_node based on operand matched, otherwise NULL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: operand_list_cfg
 * Purpose: Attempts to match the non-terminal for operand_list. Failure to match
 * results in reporting the error to report_cfg.
 * @return Address of the first operand_node in the operand list, otherwise NULL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: verify_instruction
 * Purpose: Checks the instruction_node to see if a proper instruction was 
 * recognized based on the opcode table entry for the mnemonic. If an invalid
 * instruction is encountered, it will report the error to report_cfg.
 * @param instr -> Address of the instruction node structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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
            if((((struct opcode_entry *)instr->mnemonic->attrptr)->operand[i] & OPERAND_LABEL)) {
                if(get_symbol_table(symbol_table, operand->identifier) == NULL) {
                    insert_front(cfg_parser->sym_list, insert_symbol_table(symbol_table, operand->identifier));
                }
            }
        }
        operand = operand->next;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: instruction_cfg
 * Purpose: Attempts to match the non-terminal for instruction. If failed to match
 * reports error to report_cfg
 * @return Address of the instruction_node if valid, otherwise NULL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: instruction_list_cfg
 * Purpose: Attempts to match the non-terminal for instruction_list. Failure to match
 * results in reporting the error to report_cfg
 * @return Address of the first instruction_node in the instruction list, otherwise NULL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: program_cfg
 * Purpose: Start symbol the the LL(1) context-free grammer
 * @return Address of the allocated program_node
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct program_node *program_cfg(struct parser *parser) {
    cfg_parser = parser;
    struct program_node *node = malloc(sizeof(struct program_node));
    node->instruction_list = instruction_list_cfg();
    return node;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: create_parser
 * Purpose: Allocates and initializes the parser structure 
 * @return Pointer to the allocated parser structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct parser *create_parser(int file_count, const char **file_arr) {
    struct parser *parser = malloc(sizeof(struct parser));

    parser->tokenizer = NULL;
    parser->src_files = create_list();

    /* Setup source files */
    for(int i = 0; i < file_count; ++i) {
        insert_rear(parser->src_files, (void *)file_arr[i]);
    }

    parser->current_file = parser->src_files->front;
    parser->lookahead = TOK_NULL;
    parser->sym_list = create_list();
    parser->status = PARSER_STATUS_NULL;
    parser->lineno = 1;
    parser->colno = 1;
    parser->LC = 0x00000000;
    parser->ast = NULL;
    parser->segment = SEGMENT_TEXT;
    
    return parser;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: execute_parser
 * Purpose: Starts the parsing process. Sets up the lookahead token
 * required for the LL(1) grammer and verifies symbol table after execution. 
 * @param parser -> Address of the parser structure
 * @return PARSER_STATUS_OK if no errors, otherwise PARSER_STATUS_FAIL
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
pstatus_t execute_parser(struct parser *parser) {  
    /* Setup initial tokenizer structure */
    if(parser->current_file == NULL) {
        fprintf(stderr, "Input: No source files to assemble\n");
        parser->status = PARSER_STATUS_FAIL;
        return parser->status;
    }
    
    /* Go through entire source files */
    while(parser->current_file != NULL) {
        /* Setup tokenizer */
        parser->tokenizer = create_tokenizer((const char *)parser->current_file->value);
        
        if(parser->tokenizer == NULL) {
            fprintf(stderr, "%s: File does not exist or cannot be opened\n", (const char *)parser->current_file->value);
            cfg_parser->status = PARSER_STATUS_FAIL;
            return cfg_parser->status;
        }

        /* Setup lookahead */
        parser->lookahead = get_next_token(parser->tokenizer);

        /* Start grammar recognization... */
        parser->ast = program_cfg(parser);

        /* Destory AST */
        struct instruction_node *instr_node = parser->ast ? parser->ast->instruction_list : NULL;
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
        free(parser->ast);

        parser->ast = NULL;

        /* Delete tokenizer structure */
        destroy_tokenizer(&(parser->tokenizer));

        /* Get next file in the file list */
        parser->current_file = parser->current_file->next;
    }
    
    /* Verify undefined symbol table */
    for(struct list_node *head = parser->sym_list->front; head != NULL; head = head->next) {
        symstat_t status = ((struct symbol_table_entry *)head->value)->status;
        if(status == SYMBOL_UNDEFINED) {
            /* Symbol is still undefined, program cannot be assembled */
            fprintf(stderr, "Symbol Error: Undefined symbol '%s'\n", ((struct symbol_table_entry *)head->value)->key);
            parser->status = PARSER_STATUS_FAIL;
        }
    }

    if(parser->status == PARSER_STATUS_NULL) {
        parser->status = PARSER_STATUS_OK;
    }

    return parser->status;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: destroy_symbol
 * Purpose: Deallocates all the dynamically allocated memory used for the
 * parser structure
 * @param parser -> Reference to the address of the parser structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

    /* Free linked list */
    delete_linked_list(&((*parser)->src_files), LN_VSTATIC);
    delete_linked_list(&((*parser)->sym_list), LN_VSTATIC);

    /* Simple free the data */
    free(*parser);

    /* Set parser to NULL */
    *parser = NULL;
}
