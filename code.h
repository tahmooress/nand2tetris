#ifndef CODE_H
#define CODE_H
#include "./parser.h"

typedef struct Code Code;
Code* init_code(Parser *parser, const char* filename);
int assemble(Code *c);
void free_code(Code *c);

#endif
