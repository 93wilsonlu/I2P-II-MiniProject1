#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codeGen.h"

BTNode* makeNode(TokenSet tok, const char* lexe) {
    BTNode* node = (BTNode*)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->token_type = tok;
    node->val = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void optimize(BTNode* node) {
    if (node->left && node->right && node->left->token_type == INT &&
        node->right->token_type == INT) {
        int a = node->left->val, b = node->right->val;
        switch (node->lexeme[0]) {
            case '+':
                node->val = a + b;
                break;
            case '-':
                node->val = a - b;
                break;
            case '*':
                node->val = a * b;
                break;
            case '/':
                if (node->right->val == 0) {
                    error(DIVZERO);
                }
                node->val = a / b;
                break;

            case '&':
                node->val = a & b;
                break;
            case '|':
                node->val = a | b;
                break;
            case '^':
                node->val = a ^ b;
                break;
            default:
                break;
        }
        freeTree(node->left);
        freeTree(node->right);
        node->left = node->right = 0;
        node->token_type = INT;
    }
}

void freeTree(BTNode* root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

// factor := INT | ID |
//           INCDEC ID |
//		   	 LPAREN assign_expr RPAREN
BTNode* factor(void) {
    BTNode* retp = NULL;

    if (match(INT)) {
        retp = makeNode(INT, getLexeme());
        retp->val = atoi(getLexeme());
        advance();
    } else if (match(ID)) {
        retp = makeNode(ID, getLexeme());
        advance();
    } else if (match(INCDEC)) {
        retp = makeNode(INCDEC, getLexeme());
        advance();
        if (match(ID)) {
            retp->left = makeNode(ID, getLexeme());
            advance();
        } else {
            error(NOTID);
        }
    } else if (match(LPAREN)) {
        advance();
        retp = assign_expr();
        if (match(RPAREN))
            advance();
        else
            error(MISPAREN);
    } else {
        error(SYNTAXERR);
    }
    return retp;
}

// unary_expr := ADDSUB unary_expr | factor
BTNode* unary_expr(void) {
    BTNode* node = NULL;
    if (match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        if (node->lexeme[0] == '+') {
            return unary_expr();
        }
        node->left = makeNode(INT, "0");
        node->left->val = 0;
        node->right = unary_expr();
        optimize(node);
        return node;
    } else {
        return factor();
    }
}

// muldiv_expr := unary_expr muldiv_expr_tail
BTNode* muldiv_expr(void) {
    BTNode* node = unary_expr();
    return muldiv_expr_tail(node);
}

// muldiv_expr_tail := MULDIV unary_expr muldiv_expr_tail | NiL
BTNode* muldiv_expr_tail(BTNode* left) {
    BTNode* node = NULL;

    if (match(MULDIV)) {
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        node->right = unary_expr();
        optimize(node);
        return muldiv_expr_tail(node);
    } else {
        return left;
    }
}

// addsub_expr := muldiv_expr addsub_expr_tail
BTNode* addsub_expr(void) {
    BTNode* node = muldiv_expr();
    return addsub_expr_tail(node);
}

// addsub_expr_tail := ADDSUB muldiv_expr addsub_expr_tail | NiL
BTNode* addsub_expr_tail(BTNode* left) {
    BTNode* node = NULL;

    if (match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        node->right = muldiv_expr();
        optimize(node);
        return addsub_expr_tail(node);
    } else {
        return left;
    }
}

// and_expr := addsub_expr and_expr_tail
BTNode* and_expr(void) {
    BTNode* node = addsub_expr();
    return and_expr_tail(node);
}

// and_expr_tail := AND addsub_expr and_expr_tail | NiL
BTNode* and_expr_tail(BTNode* left) {
    BTNode* node = NULL;

    if (match(AND)) {
        node = makeNode(AND, getLexeme());
        advance();
        node->left = left;
        node->right = addsub_expr();
        optimize(node);
        return and_expr_tail(node);
    } else {
        return left;
    }
}

// xor_expr := and_expr xor_expr_tail
BTNode* xor_expr(void) {
    BTNode* node = and_expr();
    return xor_expr_tail(node);
}

// xor_expr_tail := XOR and_expr xor_expr_tail | NiL
BTNode* xor_expr_tail(BTNode* left) {
    BTNode* node = NULL;

    if (match(XOR)) {
        node = makeNode(XOR, getLexeme());
        advance();
        node->left = left;
        node->right = and_expr();
        optimize(node);
        return xor_expr_tail(node);
    } else {
        return left;
    }
}

// or_expr := xor_expr or_expr_tail
BTNode* or_expr(void) {
    BTNode* node = xor_expr();
    return or_expr_tail(node);
}

// or_expr_tail := OR xor_expr or_expr_tail | NiL
BTNode* or_expr_tail(BTNode* left) {
    BTNode* node = NULL;

    if (match(OR)) {
        node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
        node->right = xor_expr();
        optimize(node);
        return or_expr_tail(node);
    } else {
        return left;
    }
}

// assign_expr := ID ASSIGN assign_expr | ID ADDSUB_ASSIGN assign_expr | or_expr
BTNode* assign_expr(void) {
    BTNode *retp = NULL, *left = or_expr();
    if (left->token_type == ID) {
        if (match(ASSIGN)) {
            retp = makeNode(ASSIGN, getLexeme());
            retp->left = left;
            advance();
            retp->right = assign_expr();
            return retp;
        } else if (match(ADDSUB_ASSIGN)) {
            retp = makeNode(ADDSUB_ASSIGN, getLexeme());
            retp->left = left;
            advance();
            retp->right = assign_expr();
            return retp;
        }
    }
    return left;
}

// statement := ENDFILE | END | assign_expr END
void statement(void) {
    BTNode* retp = NULL;

    if (match(ENDFILE)) {
        puts("MOV r0 [0]");
        puts("MOV r1 [4]");
        puts("MOV r2 [8]");
        puts("EXIT 0");
        exit(0);
    } else if (match(END)) {
        advance();
    } else {
        retp = assign_expr();
        if (match(END)) {
            generate_code(retp, 0);
            freeTree(retp);
            advance();
        } else {
            error(SYNTAXERR);
        }
    }
}

void err(ErrorType errorNum) {
    if (PRINTERR) {
        fprintf(stderr, "error: ");
        switch (errorNum) {
            case MISPAREN:
                fprintf(stderr, "mismatched parenthesis\n");
                break;
            case NOTID:
                fprintf(stderr, "number or identifier expected\n");
                break;
            case NOTNUMID:
                fprintf(stderr, "number or identifier expected\n");
                break;
            case NOTFOUND:
                fprintf(stderr, "variable not defined\n");
                break;
            case RUNOUT:
                fprintf(stderr, "out of memory\n");
                break;
            case NOTLVAL:
                fprintf(stderr, "lvalue required as an operand\n");
                break;
            case DIVZERO:
                fprintf(stderr, "divide by constant zero\n");
                break;
            case SYNTAXERR:
                fprintf(stderr, "syntax error\n");
                break;
            default:
                fprintf(stderr, "undefined error\n");
                break;
        }
    }
    printf("EXIT 1");
    exit(0);
}
