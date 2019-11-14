/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * File: tokenizer.c
 * Purpose: Converts assembly file from source into a sequence of tokens for
 * the assembler / parser to use. 
 *
 * Defines the neccessary functions for the tokenizer to work. The tokenizer 
 * uses a reserved keyword table to recognize special keywords like the mnemonics or 
 * registers. If you decide to add new reserved keywords ensure that the array
 * is sorted by keyword, otherwise the get_reserved_table function may fail. 
 *
 * The tokenizer uses a finite state machine along with the reserved keyword table
 * to recognize tokens.
 *
 * @author: Bryan Rocha
 * @version: 1.0 (8/28/2019)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "tokenizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#include "opcode.h"

/* Finite state machine states */
typedef enum { init_state, comma_accept, colon_accept, left_paren_accept,
               right_paren_accept, identifier_state, identifier_accept,
               integer_state, integer_accept, hex_state, zero_state,
               character_state, character_accept, eof_accept, comment_state, 
               comment_accept, negative_state, string_state, string_accept, 
               eol_accept, invalid_state } state_fsm;

/* Reserved keyword table */
struct reserved_entry reserved_table[] = {
    { "$0"      , TOK_REGISTER, NULL,  0  },
    { "$1"      , TOK_REGISTER, NULL,  1  },
    { "$10"     , TOK_REGISTER, NULL,  10 },
    { "$11"     , TOK_REGISTER, NULL,  11 },
    { "$12"     , TOK_REGISTER, NULL,  12 },
    { "$13"     , TOK_REGISTER, NULL,  13 },
    { "$14"     , TOK_REGISTER, NULL,  14 },
    { "$15"     , TOK_REGISTER, NULL,  15 },
    { "$16"     , TOK_REGISTER, NULL,  16 },
    { "$17"     , TOK_REGISTER, NULL,  17 },
    { "$18"     , TOK_REGISTER, NULL,  18 },
    { "$19"     , TOK_REGISTER, NULL,  19 },
    { "$2"      , TOK_REGISTER, NULL,  2  },
    { "$20"     , TOK_REGISTER, NULL,  20 },
    { "$21"     , TOK_REGISTER, NULL,  21 },
    { "$22"     , TOK_REGISTER, NULL,  22 },
    { "$23"     , TOK_REGISTER, NULL,  23 },
    { "$24"     , TOK_REGISTER, NULL,  24 },
    { "$25"     , TOK_REGISTER, NULL,  25 },
    { "$26"     , TOK_REGISTER, NULL,  26 },
    { "$27"     , TOK_REGISTER, NULL,  27 },
    { "$28"     , TOK_REGISTER, NULL,  28 },
    { "$29"     , TOK_REGISTER, NULL,  29 },
    { "$3"      , TOK_REGISTER, NULL,  3  },
    { "$30"     , TOK_REGISTER, NULL,  30 },
    { "$31"     , TOK_REGISTER, NULL,  31 },
    { "$4"      , TOK_REGISTER, NULL,  4  },
    { "$5"      , TOK_REGISTER, NULL,  5  },
    { "$6"      , TOK_REGISTER, NULL,  6  },
    { "$7"      , TOK_REGISTER, NULL,  7  },
    { "$8"      , TOK_REGISTER, NULL,  8  },
    { "$9"      , TOK_REGISTER, NULL,  9  },
    { "$a0"     , TOK_REGISTER, NULL,  4  },
    { "$a1"     , TOK_REGISTER, NULL,  5  },
    { "$a2"     , TOK_REGISTER, NULL,  6  },
    { "$a3"     , TOK_REGISTER, NULL,  7  },
    { "$at"     , TOK_REGISTER, NULL,  1  },
    { "$fp"     , TOK_REGISTER, NULL,  30 },
    { "$gp"     , TOK_REGISTER, NULL,  28 },
    { "$k0"     , TOK_REGISTER, NULL,  26 },
    { "$k1"     , TOK_REGISTER, NULL,  27 },
    { "$ra"     , TOK_REGISTER, NULL,  31 },
    { "$s0"     , TOK_REGISTER, NULL,  16 },
    { "$s1"     , TOK_REGISTER, NULL,  17 },
    { "$s2"     , TOK_REGISTER, NULL,  18 },
    { "$s3"     , TOK_REGISTER, NULL,  19 },
    { "$s4"     , TOK_REGISTER, NULL,  20 },
    { "$s5"     , TOK_REGISTER, NULL,  21 },
    { "$s6"     , TOK_REGISTER, NULL,  22 },
    { "$s7"     , TOK_REGISTER, NULL,  23 },
    { "$sp"     , TOK_REGISTER, NULL,  29 },
    { "$t0"     , TOK_REGISTER, NULL,  8  },
    { "$t1"     , TOK_REGISTER, NULL,  9  },
    { "$t2"     , TOK_REGISTER, NULL,  10 },
    { "$t3"     , TOK_REGISTER, NULL,  11 },
    { "$t4"     , TOK_REGISTER, NULL,  12 },
    { "$t5"     , TOK_REGISTER, NULL,  13 },
    { "$t6"     , TOK_REGISTER, NULL,  14 },
    { "$t7"     , TOK_REGISTER, NULL,  15 },
    { "$t8"     , TOK_REGISTER, NULL,  24 },
    { "$t9"     , TOK_REGISTER, NULL,  25 },
    { "$v0"     , TOK_REGISTER, NULL,  2  },
    { "$v1"     , TOK_REGISTER, NULL,  3  },
    { "$zero"   , TOK_REGISTER, NULL,  0  },
    { ".align"  , TOK_DIRECTIVE, opcode_table + DIRECTIVE_ALIGN   },
    { ".ascii"  , TOK_DIRECTIVE, opcode_table + DIRECTIVE_ASCII   },
    { ".asciiz" , TOK_DIRECTIVE, opcode_table + DIRECTIVE_ASCIIZ  },
    { ".byte"   , TOK_DIRECTIVE, opcode_table + DIRECTIVE_BYTE    },
    { ".data"   , TOK_DIRECTIVE, opcode_table + DIRECTIVE_DATA    },
    { ".half"   , TOK_DIRECTIVE, opcode_table + DIRECTIVE_HALF    },
    { ".include", TOK_DIRECTIVE, opcode_table + DIRECTIVE_INCLUDE },
    { ".kdata"  , TOK_DIRECTIVE, opcode_table + DIRECTIVE_KDATA   },
    { ".ktext"  , TOK_DIRECTIVE, opcode_table + DIRECTIVE_KTEXT   },
    { ".text"   , TOK_DIRECTIVE, opcode_table + DIRECTIVE_TEXT    },
    { ".word"   , TOK_DIRECTIVE, opcode_table + DIRECTIVE_WORD    },
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: get_reserved_table
 * Purpose: Retrieves entry in reserved table based on key
 * @param key -> Keyword string to check
 * @return Returns the corresponding entry in the table if found, otherwise NULL
 * @comments: Executes in O(log(n)) time
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct reserved_entry* get_reserved_table(const char *key) {
    ssize_t left = 0, right = reserved_table_size - 1;
    ssize_t mid;
    int cmp_result;
    
    while(left <= right) {
        mid = left + ((right - left) / 2);
        cmp_result = strcmp(reserved_table[mid].id, key);
        
        if(cmp_result == 0) return reserved_table + mid;
        if(cmp_result < 0) left = mid + 1;
        if(cmp_result > 0) right = mid - 1;
    }

    return NULL;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: tgetc
 * Purpose: Retrieves next character in file stream and stores into lexical buffer
 * @param tokenizer -> Pointer to the tokenizer structure
 * @return The next character in the file stream
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int tgetc(struct tokenizer *tokenizer) {
    int ch = fgetc(tokenizer->fstream);

    /* Adjust tokenizer buffer if necessary */
    if(tokenizer->bufpos >= tokenizer->bufsize) {
        tokenizer->bufsize <<= 1;
        tokenizer->lexbuf = (char *)realloc(tokenizer->lexbuf, tokenizer->bufsize);
    }

    tokenizer->lexbuf[tokenizer->bufpos++] = ch;
    tokenizer->colno++;

    return ch;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: tungetc
 * Purpose: Puts back character in file stream and removes from lexical buffer
 * @param tokenizer -> Pointer to the tokenizer structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void tungetc(int ch, struct tokenizer *tokenizer) {
    ungetc(ch, tokenizer->fstream);
    tokenizer->colno--;
    tokenizer->bufpos--;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: tpeekc
 * Purpose: Retrieves next character in file stream without consuming it
 * @param tokenizer -> Pointer to the tokenizer structure
 * @return The next character in the file stream
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int tpeekc(struct tokenizer *tokenizer) {
    int ch = fgetc(tokenizer->fstream);
    ungetc(ch, tokenizer->fstream);
    return ch;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: report_fsm
 * Purpose: Reports an error in the finite state machine and stores it 
 * into tokenizer->errmsg
 * @param tokenizer -> Pointer to the tokenizer structure
 *        fmt       -> Format string
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void report_fsm(struct tokenizer *tokenizer, const char *fmt, ...) {
    va_list vargs;
    size_t bufsize = 0;

    va_start(vargs, fmt);
    bufsize = vsnprintf(NULL, 0, fmt, vargs) + 1;
    va_end(vargs);

    if(bufsize > tokenizer->errsize) {
        tokenizer->errsize = bufsize;
        tokenizer->errmsg = (char *)realloc(tokenizer->errmsg, bufsize);
    }

    va_start(vargs, fmt);
    vsnprintf(tokenizer->errmsg, bufsize, fmt, vargs);
    va_end(vargs);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: init_fsm
 * Purpose: Initial finite state machine used to transition into other states
 * @param tokenizer -> Pointer to the tokenizer structure
 * @return The next FSM state to go to
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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
            /* Ignore " character */
            tokenizer->bufpos--;
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
            tungetc(ch, tokenizer);
            return eof_accept;
        default:
            if(isprint(ch))
                report_fsm(tokenizer, "Unexpected character '%c' on line %ld, column %ld", ch, tokenizer->lineno, tokenizer->colno - 1);
            else 
                report_fsm(tokenizer, "Unexpected character 0x%02X on line %ld, column %ld", ch, tokenizer->lineno, tokenizer->colno - 1);
            return invalid_state;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: identifier_fsm
 * Purpose: Used to recognize identifiers [A-Za-z0-9_]*
 * @param tokenizer -> Pointer to the tokenizer structure
 * @return The next FSM state to go to
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: integer_fsm
 * Purpose: Used to recognize integers [1-9][0-9]*
 * @param tokenizer -> Pointer to the tokenizer structure
 * @return The next FSM state to go to
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: zero_fsm
 * Purpose: Used to determine if integer is a hex number or decimal number
 * @param tokenizer -> Pointer to the tokenizer structure
 * @return The next FSM state to go to
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: hex_fsm
 * Purpose: Used to recognize hex numbers [0x | 0X][0-9A-Fa-f]*
 * @param tokenizer -> Pointer to the tokenizer structure
 * @return The next FSM state to go to
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: comment_fsm
 * Purpose: Used to recognize comments [#][^\n]
 * NOTE: When accepted, get_next_token will ignore it and move to the next token
 * @param tokenizer -> Pointer to the tokenizer structure
 * @return The next FSM state to go to
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
state_fsm comment_fsm(struct tokenizer *tokenizer) {
    int ch = tgetc(tokenizer);

    switch(ch) {
        case EOF:
            tungetc(ch, tokenizer);
            return comment_accept;
        case '\n':
            tungetc(ch, tokenizer);
            return comment_accept;
        default:
            return comment_state;
    }

    return invalid_state;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: negative_fsm
 * Purpose: Used to determine if negative integer is hex or decimal
 * @param tokenizer -> Pointer to the tokenizer structure
 * @return The next FSM state to go to
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: string_fsm
 * Purpose: Used to recognize strings
 * @param tokenizer -> Pointer to the tokenizer structure
 * @return The next FSM state to go to
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
state_fsm string_fsm(struct tokenizer *tokenizer) {
    int ch = tgetc(tokenizer);

    switch(ch) {
        case '"':
            /* Ignore " character */
            tokenizer->bufpos--;
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

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: return_token
 * Purpose: Serves as a final step before returning the token. If the token is
 * an identifier it will attempt to look up the identifier in the reserved
 * keyword table and return the appropriate token. Additionally, this sets the
 * attribute in the tokenizer based on the token.
 * @param token     -> Token recognized from finite state machine
 *        tokenizer -> Pointer to the tokenizer structure
 * @return The actual token to return to the function get_next_token
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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
                if(entry->token == TOK_MNEMONIC || entry->token == TOK_DIRECTIVE)
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
            return token;
        }
        default:
            /* Set attribute to NULL */
            tokenizer->attrptr = NULL;
            return token;
    }

    return token;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: create_tokenizer
 * Purpose: Allocates and initializes the tokenizer structure 
 * @param file -> Name of the file to read from
 * @return Pointer to the allocated tokenizer structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct tokenizer *create_tokenizer(const char *file) {
    /* Create the tokenizer struct */
    struct tokenizer *tokenizer = (struct tokenizer *)malloc(sizeof(struct tokenizer));
    
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
    tokenizer->lexbuf = (char *)malloc(32);
    
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

    /* Store filename */
    tokenizer->filename = strdup(file);

    return tokenizer;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: get_next_token
 * Purpose: Retrieves the next token from the file stream.
 * Special cases: TOK_INVALID -> Unrecognizable pattern / character 
 *                               Error message found in tokenizer->errmsg
 *                TOK_NULL    -> Indicates EOF
 * @param tokenizer -> Pointer to the tokenizer structure
 * @return The next token in the file stream
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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
            case eof_accept:
                return return_token(TOK_NULL, tokenizer);
            case invalid_state:
                return return_token(TOK_INVALID, tokenizer);
        }
    }

    return TOK_INVALID;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: destroy_tokenizer
 * Purpose: Deallocates the tokenizer structure and sets it to NULL
 * @param tokenizer -> Reference to the pointer to the tokenizer structure
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void destroy_tokenizer(struct tokenizer **tokenizer) {
    if(*tokenizer == NULL) return;

    /* Close reading file stream */
    fclose((*tokenizer)->fstream);
    free((*tokenizer)->filename);

    /* Destory dynamically allocated data */
    free((*tokenizer)->lexbuf);
    free((*tokenizer)->errmsg);
    free(*tokenizer);

    /* Redirect pointer to NULL */
    *tokenizer = NULL;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function: get_token_str
 * Purpose: Returns the string associated with the token
 * @param token -> Token to convert
 * @return String corresponding to the token
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
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
        case TOK_DIRECTIVE:
            return "directive";
    }
    
    return NULL;
}