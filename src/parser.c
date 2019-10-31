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
 *                         label <MNEMONIC> operand_list <EOL>  | 
                           label <DIRECTIVE> operand_list <EOL> |
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
 *     label            -> <ID> <COLON> | epsilon
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
    if(cfg_parser->lookahead == TOK_INVALID) fprintf(stderr, "%s: Error: %s\n", cfg_parser->tokenizer->filename, cfg_parser->tokenizer->errmsg);
    else if(buffer != NULL) fprintf(stderr, "%s: Error: %s\n", cfg_parser->tokenizer->filename, buffer);

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
    } 
    else {
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
                } 
                else {
                    entry->offset = cfg_parser->LC;
                    entry->segment = cfg_parser->segment;
                    entry->status = SYMBOL_DEFINED;
                }
            } 
            else { 
                entry = insert_symbol_table(symbol_table, id);
                entry->offset = cfg_parser->LC;
                entry->segment = cfg_parser->segment;
                entry->status = SYMBOL_DEFINED;
            }
        } 
        else {
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
        case TOK_STRING: {
            char *id = strdup(cfg_parser->tokenizer->lexbuf);
            match_cfg(TOK_STRING);
            
            node = malloc(sizeof(struct operand_node));
            node->operand = OPERAND_STRING;
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
        case TOK_STRING:
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
 * Function: verify_operand_list
 * Purpose: Given an entry in the reserved table, it checks the operand list
 * to see if it matches the corresponding operand format.
 * @param res_entry    -> Address of the entry in the reserved table
 *        operand_list -> Address of the start of the operand list
 * @return 1 if operand list matches operand format, otherwise 0
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int verify_operand_list(struct reserved_entry *res_entry, struct operand_node *operand_list) {
    /* Check if reserved_entry is valid */
    if(res_entry->token != TOK_MNEMONIC && res_entry->token != TOK_DIRECTIVE) return 0;

    /* Verify operand list */
    struct operand_node *current_operand = operand_list;
    struct opcode_entry *entry = (struct opcode_entry *)res_entry->attrptr;

    const char *op_string = res_entry->token == TOK_DIRECTIVE ? "directive" : "mnemonic";

    for(int i = 0; i < MAX_OPERAND_COUNT; ++i) {
        if(entry->operand[i] & OPERAND_REPEAT) {
            if(current_operand == NULL || !(entry->operand[i] & current_operand->operand)) {
                report_cfg("Invalid operand combination for %s '%s' on line %ld", op_string, res_entry->id, cfg_parser->lineno - 1);
                return 0;
            } 
            if(current_operand->operand & OPERAND_LABEL) {
                insert_front(cfg_parser->sym_list, insert_symbol_table(symbol_table, current_operand->identifier));
            }
            
            /* Check for more operands in repeat... */
            current_operand = current_operand->next;
            while(current_operand != NULL && entry->operand[i] & current_operand->operand) {
                if(current_operand->operand & OPERAND_LABEL) {
                    insert_front(cfg_parser->sym_list, insert_symbol_table(symbol_table, current_operand->identifier));
                }
                current_operand = current_operand->next;
            }
        } else {
            if(entry->operand[i] == OPERAND_NONE) {
                if(current_operand != NULL) {
                    report_cfg("Invalid operand combiniation for %s '%s' on line %ld", op_string, res_entry->id, cfg_parser->lineno - 1);
                    return 0;
                }
                break;
            } else {
                if(current_operand == NULL || !(entry->operand[i] & current_operand->operand)) {
                    report_cfg("Invalid operand combiniation for %s '%s' on line %ld", op_string, res_entry->id, cfg_parser->lineno - 1);
                    return 0;
                }
                if(current_operand->operand & OPERAND_LABEL) {
                    insert_front(cfg_parser->sym_list, insert_symbol_table(symbol_table, current_operand->identifier));
                }
            }
            current_operand = current_operand->next;
        }
    }

    return 1;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: check_instruction
 * Purpose: Checks the instruction_node to see if a proper instruction was 
 * recognized based on the opcode table entry for the mnemonic. If an invalid
 * instruction is encountered, it will report the error to report_cfg.
 * @param instr -> Address of the instruction node structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void check_instruction(struct instruction_node *instr) {
    if(instr == NULL || instr->mnemonic == NULL) return;

    if(!verify_operand_list(instr->mnemonic, instr->operand_list)) return;

    /* TO-DO: Assemble (?) instruction */
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: check_directive
 * Purpose: Checks the directive recognized from the CFG. Ensures that the
 * operands recognized are of the correct format. If successfully recognized,
 * the function attempts to execute the directive.
 * @param directive    -> Address of the entry in the reserved table
 *        operand_list -> Address of the first operand in the operand list
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void check_directive(struct reserved_entry *directive, struct operand_node *operand_list) {
    /* Check if entry is directive */
    if(directive->token != TOK_DIRECTIVE) return;

    struct opcode_entry *entry = (struct opcode_entry *)directive->attrptr;

    if(!verify_operand_list(directive, operand_list)) return;

    switch(entry->opcode) {
        case DIRECTIVE_INCLUDE: {
            /* Create tokenizer structure */
            struct tokenizer *tokenizer = create_tokenizer(operand_list->identifier);
            if(tokenizer == NULL) {
                report_cfg("Failed to include file '%s' on line %ld : %s", operand_list->identifier, cfg_parser->lineno - 1, strerror(errno));
            } 
            else {
                insert_front(cfg_parser->tokenizer_queue, (void *)tokenizer);
                cfg_parser->tokenizer = tokenizer;
                cfg_parser->lookahead = get_next_token(tokenizer);
            }
            break;
        }
        case DIRECTIVE_TEXT: 
            cfg_parser->segment = SEGMENT_TEXT;
            break;
        case DIRECTIVE_DATA:
            cfg_parser->segment = SEGMENT_DATA;
            break;
        case DIRECTIVE_ALIGN: {
            if(operand_list->value.integer < 0 || operand_list->value.integer >= 31) {
                report_cfg("Directive '.align n' expects n to be within the range of [0, 31] on line %ld", cfg_parser->lineno - 1);
            }
            else if(operand_list->value.integer == 0) {
                /* TO-DO: Disable automatic alignment of .half, .word, directives until next .data segment */
            }
            else {
                unsigned int multiple = 1 << operand_list->value.integer;
                unsigned int remainder = cfg_parser->LC & (multiple - 1);
                if(remainder) cfg_parser->LC = cfg_parser->LC + multiple - remainder; 
            }
            break;
        }
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

    if(cfg_parser->lookahead == TOK_IDENTIFIER) label_cfg();

    switch(cfg_parser->lookahead) {
        case TOK_DIRECTIVE: {
            struct reserved_entry *entry = cfg_parser->tokenizer->attrptr;
            struct operand_node *op_list = NULL;

            match_cfg(TOK_DIRECTIVE);
            switch(cfg_parser->lookahead) {
                case TOK_IDENTIFIER:
                case TOK_INTEGER:
                case TOK_STRING:
                    op_list = operand_list_cfg();
                    break;
                default:
                    op_list = NULL;
            }
            end_line_cfg();

            check_directive(entry, op_list);

            /* Delete operand list */
            struct operand_node *op_node = op_list;
            while(op_node != NULL) {
                struct operand_node *next_op = op_node->next;
                if(op_node->operand == OPERAND_LABEL || op_node->operand == OPERAND_STRING) free(op_node->identifier);
                free(op_node);
                op_node = next_op;
            }
            break;
        }
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

            check_instruction(node);

            if(((struct opcode_entry *)node->mnemonic->attrptr)->type == OPTYPE_PSUEDO) {
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
    
    while(cfg_parser->lookahead == TOK_NULL) {
        /* Free current tokenizer */
        destroy_tokenizer(&cfg_parser->tokenizer);
        remove_front(cfg_parser->tokenizer_queue, LN_VSTATIC);

        /* No more files to process */
        if(cfg_parser->tokenizer_queue->front == NULL) return node;    
        
        /* Setup tokenizer */
        cfg_parser->tokenizer = cfg_parser->tokenizer_queue->front->value;

        /* Setup lookahead */
        cfg_parser->lookahead = get_next_token(cfg_parser->tokenizer);
    }
    
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

    parser->tokenizer_queue = NULL;

    parser->src_files = create_list();
    /* Setup source files */
    for(int i = 0; i < file_count; ++i) {
        insert_rear(parser->src_files, (void *)file_arr[i]);
    }

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
    if(parser->src_files->front == NULL) {
        fprintf(stderr, "Input: No source files to assemble\n");
        parser->status = PARSER_STATUS_FAIL;
        return parser->status;
    }

    /* Create initial tokenizer queue */
    parser->tokenizer_queue = create_list();
    struct list_node *file = parser->src_files->front;
    while(file != NULL) {
        struct tokenizer *tokenizer = create_tokenizer((const char *)file->value);
        if(tokenizer == NULL) {
            fprintf(stderr, "%s: Error: %s\n", (const char *)file->value, strerror(errno));
            parser->status = PARSER_STATUS_FAIL;
            delete_linked_list(&(parser->tokenizer_queue), LN_VSTATIC);
            return parser->status;
        }
        insert_rear(parser->tokenizer_queue, tokenizer);
        file = file->next;
    }

    /* Setup tokenizer */
    parser->tokenizer = parser->tokenizer_queue->front->value;

    /* Setup lookahead */
    parser->lookahead = get_next_token(parser->tokenizer);

    /* Start grammar recognization... */
    parser->ast = program_cfg(parser);
    
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

    delete_linked_list(&(parser->tokenizer_queue), LN_VSTATIC);

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
