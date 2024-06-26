#ifndef __PARSER__
#define __PARSER__

#include "lex.h"

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

#endif  // __PARSER__
