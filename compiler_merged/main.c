#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


// for lex
#define MAXLEN 256

// Token types
typedef enum {
    UNKNOWN, END, ENDFILE, 
    INT, ID,
    ADDSUB, MULDIV,
    ASSIGN, ADDSUB_ASSIGN,
    LPAREN, RPAREN,
    AND, OR, XOR,
    INCDEC,
} TokenSet;

// Test if a token matches the current token 
extern int match(TokenSet token);

// Get the next token
extern void advance(void);

// Get the lexeme of the current token
extern char *getLexeme(void);


// for parser
// Set PRINTERR to 1 to print error message while calling error()
// Make sure you set PRINTERR to 0 before you submit your code
#define PRINTERR 0

// Call this macro to print error message and exit the program
// This will also print where you called it in your program
#define error(errorNum)                                                       \
    {                                                                         \
        if (PRINTERR)                                                         \
            fprintf(stderr, "error() called at %s:%d: ", __FILE__, __LINE__); \
        err(errorNum);                                                        \
    }

// Error types
typedef enum {
    UNDEFINED,
    MISPAREN,
    NOTID,
    NOTNUMID,
    NOTFOUND,
    RUNOUT,
    NOTLVAL,
    DIVZERO,
    SYNTAXERR
} ErrorType;

// Structure of a tree node
typedef struct _Node {
    TokenSet token_type;
    int val;
    char lexeme[MAXLEN];
    struct _Node* left;
    struct _Node* right;
} BTNode;

// Make a new node according to token type and lexeme
extern BTNode* makeNode(TokenSet tok, const char* lexe);

// Free the syntax tree
extern void freeTree(BTNode* root);

extern BTNode* factor(void);
extern BTNode* muldiv_expr(void);
extern BTNode* muldiv_expr_tail(BTNode* left);
extern BTNode* addsub_expr(void);
extern BTNode* addsub_expr_tail(BTNode* left);
extern BTNode* xor_expr(void);
extern BTNode* xor_expr_tail(BTNode* left);
extern BTNode* or_expr(void);
extern BTNode* or_expr_tail(BTNode* left);
extern BTNode* and_expr(void);
extern BTNode* and_expr_tail(BTNode* left);
extern BTNode* assign_expr(void);
extern void statement(void);

// Print error message and exit the program
extern void err(ErrorType errorNum);



// for codeGen
#define TBLSIZE 64


// Structure of the symbol table
typedef struct {
    char name[MAXLEN];
} Symbol;


// The symbol table
extern Symbol table[TBLSIZE];

// Initialize the symbol table with builtin variables
extern void initTable(void);

// Get the address of a variable
extern int get_addr(char *str, int add_var);

// Evaluate the syntax tree
extern void generate_code(BTNode *root, int use_reg);



/*============================================================================================
lex implementation
============================================================================================*/


static TokenSet getToken(void);
static TokenSet curToken = UNKNOWN;
static char lexeme[MAXLEN];

TokenSet getToken(void) {
    int i = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t')
        ;

    if (isdigit(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    } else if (c == '+' || c == '-') {
        lexeme[0] = c;
        c = fgetc(stdin);
        if (c == '=') {
            lexeme[1] = c;
            lexeme[2] = '\0';
            return ADDSUB_ASSIGN;
        }
        if (c == lexeme[0]) {
            lexeme[1] = c;
            lexeme[2] = '\0';
            return INCDEC;
        }
        ungetc(c, stdin);
        lexeme[1] = '\0';
        return ADDSUB;
    } else if (c == '*' || c == '/') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    } else if (c == '&') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return AND;
    } else if (c == '|') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return OR;
    } else if (c == '^') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return XOR;
    } else if (c == '\n') {
        lexeme[0] = '\0';
        return END;
    } else if (c == '=') {
        strcpy(lexeme, "=");
        return ASSIGN;
    } else if (c == '(') {
        strcpy(lexeme, "(");
        return LPAREN;
    } else if (c == ')') {
        strcpy(lexeme, ")");
        return RPAREN;
    } else if (isalpha(c) || c == '_') {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while ((isalnum(c) || c == '_') && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return ID;
    } else if (c == EOF) {
        return ENDFILE;
    } else {
        return UNKNOWN;
    }
}

void advance(void) {
    curToken = getToken();
}

int match(TokenSet token) {
    if (curToken == UNKNOWN)
        advance();
    return token == curToken;
}

char* getLexeme(void) {
    return lexeme;
}


/*============================================================================================
parser implementation
============================================================================================*/


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



/*============================================================================================
codeGen implementation
============================================================================================*/


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



/*============================================================================================
main
============================================================================================*/

// This package is a calculator
// It works like a Python interpretor
// Example:
// >> y = 2
// >> z = 2
// >> x = 3 * y + 4 / (2 * z)
// It will print the answer of every line
// You should turn it into an expression compiler
// And print the assembly code according to the input

// This is the grammar used in this package
// You can modify it according to the spec and the slide
// statement  :=  ENDFILE | END | expr END
// expr    	  :=  term expr_tail
// expr_tail  :=  ADDSUB term expr_tail | NiL
// term 	  :=  factor term_tail
// term_tail  :=  MULDIV factor term_tail| NiL
// factor	  :=  INT | ADDSUB INT |
//		   	      ID  | ADDSUB ID  |
//		   	      ID ASSIGN expr |
//		   	      LPAREN expr RPAREN |
//		   	      ADDSUB LPAREN expr RPAREN

int main() {
    initTable();
    while (1) {
        statement();
    }
    return 0;
}
