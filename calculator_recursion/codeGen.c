#include "codeGen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sbcount = 0;
Symbol table[TBLSIZE];

void initTable(void) {
    strcpy(table[0].name, "x");
    strcpy(table[1].name, "y");
    strcpy(table[2].name, "z");
    sbcount = 3;
}

int get_addr(char* str, int add_var) {
    for (int i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            return i << 2;
        }
    }
    if (!add_var) {
        error(NOTFOUND);
    }
    strcpy(table[sbcount++].name, str);
    return (sbcount - 1) << 2;
}

void generate_code(BTNode* root, int use_reg) {
    if (root != NULL) {
        switch (root->token_type) {
            case ID:
                printf("MOV r%d [%d]\n", use_reg, get_addr(root->lexeme, 0));
                break;
            case INT:
                printf("MOV r%d %d\n", use_reg, root->val);
                break;
            case ASSIGN:
                if (root->left->token_type != ID) {
                    error(NOTID);
                }
                generate_code(root->right, use_reg);
                printf("MOV [%d] r%d\n", get_addr(root->left->lexeme, 1),
                       use_reg);
                break;
            case ADDSUB_ASSIGN:
                if (root->left->token_type != ID) {
                    error(NOTID);
                }
                generate_code(root->right, (use_reg + 1) % 8);
                generate_code(root->left, use_reg);
                if (root->lexeme[0] == '+') {
                    printf("ADD r%d r%d\n", use_reg, (use_reg + 1) % 8);
                } else {
                    printf("SUB r%d r%d\n", use_reg, (use_reg + 1) % 8);
                }
                printf("MOV [%d] r%d\n", get_addr(root->left->lexeme, 0),
                       use_reg);
                break;
            case INCDEC:
                if (root->left->token_type != ID) {
                    error(NOTID);
                }
                generate_code(root->left, use_reg);
                printf("MOV r%d, 1\n", (use_reg + 1) % 8);
                if (root->lexeme[0] == '+') {
                    printf("ADD r%d r%d\n", use_reg, (use_reg + 1) % 8);
                } else {
                    printf("SUB r%d r%d\n", use_reg, (use_reg + 1) % 8);
                }
                printf("MOV [%d] r%d\n", get_addr(root->left->lexeme, 0),
                       use_reg);
                break;
            case AND:
            case OR:
            case XOR:
            case ADDSUB:
            case MULDIV:
                generate_code(root->left, use_reg);
                generate_code(root->right, (use_reg + 1) % 8);
                if (root->lexeme[0] == '+') {
                    printf("ADD r%d r%d\n", use_reg, (use_reg + 1) % 8);
                } else if (root->lexeme[0] == '-') {
                    printf("SUB r%d r%d\n", use_reg, (use_reg + 1) % 8);
                } else if (root->lexeme[0] == '*') {
                    printf("MUL r%d r%d\n", use_reg, (use_reg + 1) % 8);
                } else if (root->lexeme[0] == '/') {
                    printf("DIV r%d r%d\n", use_reg, (use_reg + 1) % 8);
                } else if (root->lexeme[0] == '&') {
                    printf("AND r%d r%d\n", use_reg, (use_reg + 1) % 8);
                } else if (root->lexeme[0] == '|') {
                    printf("OR r%d r%d\n", use_reg, (use_reg + 1) % 8);
                } else if (root->lexeme[0] == '^') {
                    printf("XOR r%d r%d\n", use_reg, (use_reg + 1) % 8);
                }
                break;
            default:
                break;
        }
    }
}
