#include "tokenizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#include "opcode.h"

typedef enum { init_state, comma_accept, colon_accept, left_paren_accept,
               right_paren_accept, identifier_state, identifier_accept,
               integer_state, integer_accept, hex_state, zero_state,
               eof_accept, comment_state, comment_accept, negative_state,
               string_state, string_accept, eol_accept, invalid_state, period_accept } state_fsm;

struct reserved_entry reserved_table[] = {
    { "$0"      , TOK_REGISTER, .attrval = 0  },
    { "$1"      , TOK_REGISTER, .attrval = 1  },
    { "$10"     , TOK_REGISTER, .attrval = 10 },
    { "$11"     , TOK_REGISTER, .attrval = 11 },
    { "$12"     , TOK_REGISTER, .attrval = 12 },
    { "$13"     , TOK_REGISTER, .attrval = 13 },
    { "$14"     , TOK_REGISTER, .attrval = 14 },
    { "$15"     , TOK_REGISTER, .attrval = 15 },
    { "$16"     , TOK_REGISTER, .attrval = 16 },
    { "$17"     , TOK_REGISTER, .attrval = 17 },
    { "$18"     , TOK_REGISTER, .attrval = 18 },
    { "$19"     , TOK_REGISTER, .attrval = 19 },
    { "$2"      , TOK_REGISTER, .attrval = 2  },
    { "$20"     , TOK_REGISTER, .attrval = 20 },
    { "$21"     , TOK_REGISTER, .attrval = 21 },
    { "$22"     , TOK_REGISTER, .attrval = 22 },
    { "$23"     , TOK_REGISTER, .attrval = 23 },
    { "$24"     , TOK_REGISTER, .attrval = 24 },
    { "$25"     , TOK_REGISTER, .attrval = 25 },
    { "$26"     , TOK_REGISTER, .attrval = 26 },
    { "$27"     , TOK_REGISTER, .attrval = 27 },
    { "$28"     , TOK_REGISTER, .attrval = 28 },
    { "$29"     , TOK_REGISTER, .attrval = 29 },
    { "$3"      , TOK_REGISTER, .attrval = 3  },
    { "$30"     , TOK_REGISTER, .attrval = 30 },
    { "$31"     , TOK_REGISTER, .attrval = 31 },
    { "$4"      , TOK_REGISTER, .attrval = 4  },
    { "$5"      , TOK_REGISTER, .attrval = 5  },
    { "$6"      , TOK_REGISTER, .attrval = 6  },
    { "$7"      , TOK_REGISTER, .attrval = 7  },
    { "$8"      , TOK_REGISTER, .attrval = 8  },
    { "$9"      , TOK_REGISTER, .attrval = 9  },
    { "$a0"     , TOK_REGISTER, .attrval = 4  },
    { "$a1"     , TOK_REGISTER, .attrval = 5  },
    { "$a2"     , TOK_REGISTER, .attrval = 6  },
    { "$a3"     , TOK_REGISTER, .attrval = 7  },
    { "$at"     , TOK_REGISTER, .attrval = 1  },
    { "$fp"     , TOK_REGISTER, .attrval = 30 },
    { "$gp"     , TOK_REGISTER, .attrval = 28 },
    { "$k0"     , TOK_REGISTER, .attrval = 26 },
    { "$k1"     , TOK_REGISTER, .attrval = 27 },
    { "$ra"     , TOK_REGISTER, .attrval = 31 },
    { "$s0"     , TOK_REGISTER, .attrval = 16 },
    { "$s1"     , TOK_REGISTER, .attrval = 17 },
    { "$s2"     , TOK_REGISTER, .attrval = 18 },
    { "$s3"     , TOK_REGISTER, .attrval = 19 },
    { "$s4"     , TOK_REGISTER, .attrval = 20 },
    { "$s5"     , TOK_REGISTER, .attrval = 21 },
    { "$s6"     , TOK_REGISTER, .attrval = 22 },
    { "$s7"     , TOK_REGISTER, .attrval = 23 },
    { "$sp"     , TOK_REGISTER, .attrval = 29 },
    { "$t0"     , TOK_REGISTER, .attrval = 8  },
    { "$t1"     , TOK_REGISTER, .attrval = 9  },
    { "$t2"     , TOK_REGISTER, .attrval = 10 },
    { "$t3"     , TOK_REGISTER, .attrval = 11 },
    { "$t4"     , TOK_REGISTER, .attrval = 12 },
    { "$t5"     , TOK_REGISTER, .attrval = 13 },
    { "$t6"     , TOK_REGISTER, .attrval = 14 },
    { "$t7"     , TOK_REGISTER, .attrval = 15 },
    { "$t8"     , TOK_REGISTER, .attrval = 24 },
    { "$t9"     , TOK_REGISTER, .attrval = 25 },
    { "$v0"     , TOK_REGISTER, .attrval = 2  },
    { "$v1"     , TOK_REGISTER, .attrval = 3  },
    { "$zero"   , TOK_REGISTER, .attrval = 0  },
    { "add"     , TOK_MNEMONIC, opcode_table + MNEMONIC_ADD     },
    { "addi"    , TOK_MNEMONIC, opcode_table + MNEMONIC_ADDI    },
    { "addiu"   , TOK_MNEMONIC, opcode_table + MNEMONIC_ADDIU   },
    { "addu"    , TOK_MNEMONIC, opcode_table + MNEMONIC_ADDU    },
    { "and"     , TOK_MNEMONIC, opcode_table + MNEMONIC_AND     },
    { "andi"    , TOK_MNEMONIC, opcode_table + MNEMONIC_ANDI    },
    { "beq"     , TOK_MNEMONIC, opcode_table + MNEMONIC_BEQ     },
    { "beqz"    , TOK_MNEMONIC, opcode_table + MNEMONIC_BEQZ    },
    { "bge"     , TOK_MNEMONIC, opcode_table + MNEMONIC_BGE     },
    { "bgez"    , TOK_MNEMONIC, opcode_table + MNEMONIC_BGEZ    },
    { "bgezal"  , TOK_MNEMONIC, opcode_table + MNEMONIC_BGEZAL  },
    { "bgt"     , TOK_MNEMONIC, opcode_table + MNEMONIC_BGT     },
    { "bgtz"    , TOK_MNEMONIC, opcode_table + MNEMONIC_BGTZ    },
    { "ble"     , TOK_MNEMONIC, opcode_table + MNEMONIC_BLE     },
    { "blez"    , TOK_MNEMONIC, opcode_table + MNEMONIC_BLEZ    },
    { "blt"     , TOK_MNEMONIC, opcode_table + MNEMONIC_BLT     },
    { "bltz"    , TOK_MNEMONIC, opcode_table + MNEMONIC_BLTZ    },
    { "bltzal"  , TOK_MNEMONIC, opcode_table + MNEMONIC_BLTZAL  },
    { "bne"     , TOK_MNEMONIC, opcode_table + MNEMONIC_BNE     },
    { "bnez"    , TOK_MNEMONIC, opcode_table + MNEMONIC_BNEZ    },
    { "div"     , TOK_MNEMONIC, opcode_table + MNEMONIC_DIV     },
    { "divu"    , TOK_MNEMONIC, opcode_table + MNEMONIC_DIVU    },
    { "j"       , TOK_MNEMONIC, opcode_table + MNEMONIC_JMP     },
    { "jal"     , TOK_MNEMONIC, opcode_table + MNEMONIC_JAL     },
    { "jr"      , TOK_MNEMONIC, opcode_table + MNEMONIC_JR      },
    { "la"      , TOK_MNEMONIC, opcode_table + MNEMONIC_LA      },
    { "lb"      , TOK_MNEMONIC, opcode_table + MNEMONIC_LB      },
    { "lbu"     , TOK_MNEMONIC, opcode_table + MNEMONIC_LBU     },
    { "lh"      , TOK_MNEMONIC, opcode_table + MNEMONIC_LH      },
    { "lhu"     , TOK_MNEMONIC, opcode_table + MNEMONIC_LHU     },
    { "li"      , TOK_MNEMONIC, opcode_table + MNEMONIC_LI      },
    { "lui"     , TOK_MNEMONIC, opcode_table + MNEMONIC_LUI     },
    { "lw"      , TOK_MNEMONIC, opcode_table + MNEMONIC_LW      },
    { "mfhi"    , TOK_MNEMONIC, opcode_table + MNEMONIC_MFHI    },
    { "mflo"    , TOK_MNEMONIC, opcode_table + MNEMONIC_MFLO    },
    { "move"    , TOK_MNEMONIC, opcode_table + MNEMONIC_MOVE    },
    { "mult"    , TOK_MNEMONIC, opcode_table + MNEMONIC_MULT    },
    { "multu"   , TOK_MNEMONIC, opcode_table + MNEMONIC_MULTU   },
    { "nor"     , TOK_MNEMONIC, opcode_table + MNEMONIC_NOR     },
    { "not"     , TOK_MNEMONIC, opcode_table + MNEMONIC_NOT     },
    { "or"      , TOK_MNEMONIC, opcode_table + MNEMONIC_OR      },
    { "ori"     , TOK_MNEMONIC, opcode_table + MNEMONIC_ORI     },
    { "sb"      , TOK_MNEMONIC, opcode_table + MNEMONIC_SB      },
    { "sh"      , TOK_MNEMONIC, opcode_table + MNEMONIC_SH      },
    { "sll"     , TOK_MNEMONIC, opcode_table + MNEMONIC_SLL     },
    { "slt"     , TOK_MNEMONIC, opcode_table + MNEMONIC_SLT     },
    { "slti"    , TOK_MNEMONIC, opcode_table + MNEMONIC_SLTI    },
    { "sltiu"   , TOK_MNEMONIC, opcode_table + MNEMONIC_SLTIU   },
    { "sltu"    , TOK_MNEMONIC, opcode_table + MNEMONIC_SLTU    },
    { "sra"     , TOK_MNEMONIC, opcode_table + MNEMONIC_SRA     },
    { "srl"     , TOK_MNEMONIC, opcode_table + MNEMONIC_SRL     },
    { "sub"     , TOK_MNEMONIC, opcode_table + MNEMONIC_SUB     },
    { "subu"    , TOK_MNEMONIC, opcode_table + MNEMONIC_SUBU    },
    { "sw"      , TOK_MNEMONIC, opcode_table + MNEMONIC_SW      },
    { "syscall" , TOK_MNEMONIC, opcode_table + MNEMONIC_SYSCALL },
    { "xor"     , TOK_MNEMONIC, opcode_table + MNEMONIC_XOR     },
    { "xori"    , TOK_MNEMONIC, opcode_table + MNEMONIC_XORI    }
};

const size_t reserved_table_size = sizeof(reserved_table) / sizeof(struct reserved_entry);

struct reserved_entry* get_reserved_table(const char *key) {
    size_t left = 0, right = reserved_table_size - 1;
    size_t mid;
    int cmp_result;
    
    while(left <= right) {
        mid = left + ((right - left) >> 1);
        cmp_result = strcmp(reserved_table[mid].id, key);
        
        if(cmp_result == 0) return reserved_table + mid;
        if(cmp_result < 0) left = mid + 1;
        if(cmp_result > 0) right = mid - 1;
    }

    return NULL;
}

int tgetc(struct tokenizer *tokenizer) {
    int ch = fgetc(tokenizer->fstream);

    /* Adjust tokenizer buffer if necessary */
    if(tokenizer->bufpos >= tokenizer->bufsize) {
        tokenizer->bufsize <<= 1;
        tokenizer->lexbuf = realloc(tokenizer->lexbuf, tokenizer->bufsize);
    }

    tokenizer->lexbuf[tokenizer->bufpos++] = ch;
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
            return identifier_state;
        case '.':
            return period_accept;
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
            report_fsm(tokenizer, "Unexpected character '%c' on line %ld, column %ld", ch, tokenizer->lineno, tokenizer->colno - 1);
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

state_fsm hex_fsm(struct tokenizer *tokenizer) {
    int ch = tgetc(tokenizer);

    switch(ch) {
        case 'A' ... 'F':
        case 'a' ... 'f':
        case '0' ... '9':
            return hex_state;
        default:
            tungetc(ch, tokenizer);
            return integer_accept;
    }

    return invalid_state;
}

state_fsm comment_fsm(struct tokenizer *tokenizer) {
    int ch = tgetc(tokenizer);

    switch(ch) {
        case EOF:
            tungetc(ch, tokenizer);
            return comment_accept;
        case '\n':
            //tokenizer->lineno++;
            //tokenizer->colno = 1;
            tungetc(ch, tokenizer);
            return comment_accept;
        default:
            return comment_state;
    }

    return invalid_state;
}

state_fsm negative_fsm(struct tokenizer *tokenizer) {
    int ch = tgetc(tokenizer);

    switch(ch) {
        case '0':
            return zero_state;
        case '1' ... '9':
            return integer_state;
        default:
            tungetc(ch, tokenizer);
            report_fsm(tokenizer, "Expected integer value to be specified on line %ld, col %ld", tokenizer->lineno, tokenizer->colno);
            return invalid_state;
    }

    return invalid_state;
}

state_fsm string_fsm(struct tokenizer *tokenizer) {
    int ch = tgetc(tokenizer);

    switch(ch) {
        case '"':
            return string_accept;
        case EOF:
        case '\n':
            tungetc(ch, tokenizer);
            report_fsm(tokenizer, "Non-terminated string, expected '\"' on line %ld, col %ld", tokenizer->lineno, tokenizer->colno);
            return invalid_state;
        default:
            return string_state;
    }

    return invalid_state;
}

token_t return_token(token_t token, struct tokenizer *tokenizer) {
    int ch;
    struct reserved_entry *entry;

    /* Set NULL terminator */
    tokenizer->lexbuf[tokenizer->bufpos] = '\0';

    /* Skip whiespace */
    while((ch = fgetc(tokenizer->fstream)) == ' ' || ch == '\t') { 
        tokenizer->colno++; 
    }
    ungetc(ch, tokenizer->fstream);

    /* Set attributes */
    switch(token) {
        case TOK_IDENTIFIER:
            /* Perform lookup on reserved keyword table */
            if((entry = get_reserved_table(tokenizer->lexbuf)) != NULL) {
                if(entry->token == TOK_MNEMONIC)
                    tokenizer->attrptr = entry;
                else if(entry->token == TOK_REGISTER)
                    tokenizer->attrval = entry->attrval;
                return entry->token;
            }
        case TOK_STRING:
            /* Set attribute to lexbuffer */
            tokenizer->attrbuf = tokenizer->lexbuf;
            return token;
        case TOK_INTEGER: {
            long long value;
            
            if(*tokenizer->lexbuf == '-') {
                tokenizer->attrval = value = strtoll(tokenizer->lexbuf + 1, NULL, 0);
                tokenizer->attrval *= -1;
                if(value > 2147483648) {
                    report_fsm(tokenizer, "Integer literal '%s' cannot be represented with 32-bits on line %ld", tokenizer->lexbuf, tokenizer->lineno);
                    return TOK_INVALID;
                }
            }
            else {
                tokenizer->attrval = value = strtoll(tokenizer->lexbuf, NULL, 0);
                if(value > 4294967295) {
                    report_fsm(tokenizer, "Integer literal '%s' cannot be represented with 32-bits on line %ld", tokenizer->lexbuf, tokenizer->lineno);
                    return TOK_INVALID;
                }
            }

            //printf("INTEGER FOUND: %d\n", tokenizer->attrval);
            return token;
        }
        default:
            /* Set attribute to NULL */
            tokenizer->attrptr = NULL;
            return token;
    }

    return token;
}

struct tokenizer *create_tokenizer(const char *file) {
    /* Create the tokenizer struct */
    struct tokenizer *tokenizer = malloc(sizeof(struct tokenizer));
    
    /* Failed to allocate space */
    if(tokenizer == NULL) { return NULL; }

    /* Open file for reading */
    tokenizer->fstream = fopen(file, "r");
    
    /* Failed to open file */
    if(tokenizer->fstream == NULL) { 
        free(tokenizer);
        return NULL; 
    }

    /* Create string buffer */
    tokenizer->lexbuf = malloc(sizeof(char) << 5);
    
    /* Failed to allocate space */
    if(tokenizer->lexbuf == NULL) { 
        fclose(tokenizer->fstream);
        free(tokenizer);
        return NULL; 
    }

    /* Set the buffer parameters */
    tokenizer->bufsize = 32;
    tokenizer->bufpos = 0;

    /* Set the file parameters */
    tokenizer->colno = 1;
    tokenizer->lineno = 1;

    /* Set the error mesage parameters */
    tokenizer->errmsg = NULL;
    tokenizer->errsize = 0;

    /* Set NULL attribute */
    tokenizer->attrptr = NULL;

    return tokenizer;
}

token_t get_next_token(struct tokenizer *tokenizer) {
    state_fsm next_state = init_state;
    
    /* Start finite state machine */
    while(1) {
        switch(next_state) {
            case init_state:
                tokenizer->bufpos = 0;
                next_state = init_fsm(tokenizer);
                break;
            case identifier_state:
                next_state = identifier_fsm(tokenizer);
                break;
            case integer_state:
                next_state = integer_fsm(tokenizer);
                break;
            case zero_state:
                next_state = zero_fsm(tokenizer);
                break;
            case hex_state:
                next_state = hex_fsm(tokenizer);
                break;
            case comment_state:
                next_state = comment_fsm(tokenizer);
                break;
            case negative_state:
                next_state = negative_fsm(tokenizer);
                break;
            case string_state:
                next_state = string_fsm(tokenizer);
                break;
            case comment_accept:
                next_state = init_state;
                break;
            case integer_accept:
                return return_token(TOK_INTEGER, tokenizer);
            case identifier_accept:
                return return_token(TOK_IDENTIFIER, tokenizer);
            case comma_accept:
                return return_token(TOK_COMMA, tokenizer);
            case colon_accept:
                return return_token(TOK_COLON, tokenizer);
            case eol_accept:
                return return_token(TOK_EOL, tokenizer);
            case left_paren_accept:
                return return_token(TOK_LPAREN, tokenizer);
            case right_paren_accept:
                return return_token(TOK_RPAREN, tokenizer);
            case string_accept:
                return return_token(TOK_STRING, tokenizer);
            case period_accept:
                return return_token(TOK_PERIOD, tokenizer);
            case eof_accept:
                return return_token(TOK_NULL, tokenizer);
            case invalid_state:
                return return_token(TOK_INVALID, tokenizer);
        }
    }

    return TOK_INVALID;
}

void destroy_tokenizer(struct tokenizer **tokenizer) {
    if(*tokenizer == NULL) return;

    /* Close reading file stream */
    fclose((*tokenizer)->fstream);

    /* Destory dynamically allocated data */
    free((*tokenizer)->lexbuf);
    free((*tokenizer)->errmsg);
    free(*tokenizer);

    /* Redirect pointer to NULL */
    *tokenizer = NULL;
}

const char *get_token_str(token_t token) {
    switch(token) {
        case TOK_IDENTIFIER:
            return "identififer";
        case TOK_COLON:
            return "':'";
        case TOK_REGISTER:
            return "register";
        case TOK_STRING:
            return "string";
        case TOK_MNEMONIC:
            return "mnemonic";
        case TOK_COMMA:
            return "','";
        case TOK_INTEGER:
            return "integer";
        case TOK_LPAREN:
            return "'('";
        case TOK_RPAREN:
            return "')'";
        case TOK_EOL:
            return "end of line";
        case TOK_PERIOD:
            return "'.'";
    }
    
    return NULL;
}