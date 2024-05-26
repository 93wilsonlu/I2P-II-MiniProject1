#ifndef __CODEGEN__
#define __CODEGEN__

#include "parser.h"

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

#endif // __CODEGEN__
