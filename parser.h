#ifndef PARSER_H
#define PARSER_H

#define PARSE_OK 0
#define PARSE_INVALID 1
#define PARSE_EOF -1

typedef enum{
    INVALID,
    A_INSTRUCTION,
    C_INSTRUCTION,  
    L_INSTRUCTION
}instruction_type;

extern const char* instruction_type_names[];

typedef struct Parser Parser;

Parser* init_parser(const char *filename);
int advance(Parser *p);
int hasMoreLines(Parser *p);
instruction_type instructionType(Parser *p);
const char* symbol(Parser *p);
const char* dest(Parser *p);
const char* comp(Parser *p);
const char* jump(Parser *p);
size_t current_line_number(Parser *p);
void free_parser(Parser* p);
void reset(Parser* p);

#endif
