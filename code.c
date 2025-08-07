#include<stdio.h>
#include<stdlib.h>
#include <ctype.h>
#include "./parser.h"
#include"./table.h"
#include <errno.h>
#include<string.h>

#define Uint16_MAX  (1 << 15)

#define NOT_NUMBER 2
#define OVERFLOW_ERR 3
#define UNEXPECTED 4

#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7
#define R8 8
#define R9 9
#define R10 10
#define R11 11
#define R12 12
#define R13 13
#define R14 14
#define R15 15

#define SCREEN 16384
#define KBD 24576 
#define SP 0
#define LCL 1
#define ARG 2
#define THIS 3
#define THAT 4

typedef struct
{
    Parser *parser;
    SymbolTable *table;
    uint16_t next_ram_free_slot;
    FILE *output;
}Code;

struct TableEntity{
    char* mnemonic;
    char* bits; 
};

static const struct TableEntity destEntityTable[] = {
    {"",    "000"}, {"M",   "001"}, {"D",   "010"},
    {"MD",  "011"}, {"A",   "100"}, {"AM",  "101"},
    {"AD",  "110"}, {"AMD", "111"}, {NULL, NULL}
};

static const struct TableEntity computeEntityTable[] = {
    {"0", "101010"}, {"1", "111111"}, {"-1", "111010"},
    {"D", "001100"}, {"A", "110000"}, {"M", "110000"},
    {"!D", "001101"}, {"!A", "110001"}, {"!M", "110001"},
    {"-D", "001111"}, {"-A", "110011"}, {"-M", "110011"},
    {"D+1", "011111"}, {"A+1", "110111"}, {"M+1", "110111"},
    {"D-1", "001110"}, {"A-1", "110010"}, {"M-1", "110010"},
    {"D+A", "000010"}, {"D+M", "000010"}, {"D-A", "010011"},
    {"D-M", "010011"}, {"A-D", "000111"}, {"M-D", "000111"},
    {"D&A", "000000"}, {"D&M", "000000"}, {"D|A", "010101"},
    {"D|M", "010101"}, {NULL, NULL}
};

static const struct TableEntity jumpEntityTable[] = {
    {"",    "000"}, {"JGT", "001"}, {"JEQ", "010"},
    {"JGE", "011"}, {"JLT", "100"}, {"JNE", "101"},
    {"JLE", "110"}, {"JMP", "111"}, {NULL, NULL}
};

static SymbolTable* init_symbol_table_with_values(void);
static int scan(Code *code);
static int generate(Code *code);
void free_code(Code *c);
static int generate_A_instruction(SymbolTable *table, const char* symbol, char bitsBuffer[17]);
static void generate_C_instruction(const char* dest, const char* comp, const char* jump, char bitsBuffer[17]);
static int to_uint16(const char* s, uint16_t *target);
static void to_binary(uint16_t number, char *output);
static const char* lookup(const struct TableEntity table[], const char* key);
static char* change_file_extention(const char* filename);

Code* init_code(Parser *parser, const char* filename)
{   
    char* fname = change_file_extention(filename);
    if (!fname) {
        fprintf(stderr, "cant rename file");
        return NULL; 
    }

    FILE *outfile = fopen(fname, "w");
    if (!outfile) {
        fprintf(stderr, "can not open file:%s", fname);
        free(fname);
        return NULL;
    }

    free(fname);

    Code* c = malloc(sizeof(Code));
    if (!c){
       return NULL;
    }

    c->table = init_symbol_table_with_values();
    c->parser = parser;
    c->next_ram_free_slot = R15 + 1;
    c->output = outfile;

    return c;
}

int assemble(Code *c) {
    int errnum;
    
    errnum = scan(c);
    if (errnum && errnum != PARSE_EOF) {
        free_code(c);
        // delete file;
        return errnum;
    }

    errnum = generate(c);
    if (errnum && errnum != PARSE_EOF) {
        free_code(c);
        // delete file;
        return errnum;
    }

    return 0;
}

void free_code(Code *c) {
    if (!c) {
        return;
    }

    free_parser(c->parser);

    symbol_table_free(c->table, 0);

    if (c->output) {
        fclose(c->output);
    }
}

static SymbolTable* init_symbol_table_with_values(void)
{
    SymbolTable* st = symbol_table_init();

    symbol_table_set(st, "R0", R0);
    symbol_table_set(st, "R1", R1);
    symbol_table_set(st, "R2", R2);
    symbol_table_set(st, "R3", R3);
    symbol_table_set(st, "R4", R4);
    symbol_table_set(st, "R5", R5);
    symbol_table_set(st, "R6", R6);
    symbol_table_set(st, "R7", R7);
    symbol_table_set(st, "R8", R8);
    symbol_table_set(st, "R9", R9);
    symbol_table_set(st, "R10", R10);
    symbol_table_set(st, "R11", R11);
    symbol_table_set(st, "R12", R12);
    symbol_table_set(st, "R13", R13);
    symbol_table_set(st, "R14", R14);
    symbol_table_set(st, "R15", R15);
    symbol_table_set(st, "SCREEN", SCREEN);
    symbol_table_set(st, "KBD", KBD);
    symbol_table_set(st, "SP", SP);
    symbol_table_set(st, "LCL", LCL);
    symbol_table_set(st, "ARG", ARG);
    symbol_table_set(st, "THIS", THIS);
    symbol_table_set(st, "THAT", THAT);

    return st;
}

static int scan(Code *code)
{   
    reset(code->parser);

    int result;
    instruction_type instyp;
    const char* symbol_ptr;
    uint16_t val;
    int errnum;

    while(hasMoreLines(code->parser))
    {
        result = advance(code->parser);
        if (result != PARSE_OK) {
            return result;
        }

        instyp = instructionType(code->parser);
        if (instyp == INVALID) { 
            return -1;
        }
        
        if (instyp == A_INSTRUCTION)
        {
            symbol_ptr = symbol(code->parser);
            errnum = to_uint16(symbol_ptr, &val);
            if (errnum == OVERFLOW_ERR)
            {
                fprintf(stderr, "Err overflow on line: %zu\n", current_line_number(code->parser));
                return OVERFLOW_ERR;
            }

            // if already doesnt exist to handle reserve keyboards and L_instructions.
            if (errnum == NOT_NUMBER && !symbol_table_get(code->table, symbol_ptr, &val))
            { 
                symbol_table_set(code->table, symbol_ptr, code->next_ram_free_slot);
                code->next_ram_free_slot++;
            }
        }else if (instyp == L_INSTRUCTION)
        {   
            symbol_ptr = symbol(code->parser);
            size_t cln = current_line_number(code->parser);
            symbol_table_set(code->table, symbol_ptr, cln);
        }
    }

    return 0;
}

static int generate(Code *code)
{
    reset(code->parser);

    int result;
    instruction_type instyp;
    const char* symbol_ptr;
    const char* dest_ptr;
    const char* comp_prt;
    const char* jum_prt;
    int errnum;
    char bitsBufer[17];

    while(hasMoreLines(code->parser))
    {
        result = advance(code->parser);
        if (result != PARSE_OK)
        {
            return result;
        }

        instyp = instructionType(code->parser);
        if (instyp == A_INSTRUCTION) {
            symbol_ptr = symbol(code->parser); 
            errnum = generate_A_instruction(code->table, symbol_ptr, bitsBufer);
            if (errnum == UNEXPECTED) {
                return UNEXPECTED;
            }
            fprintf(code->output,"%s\n", bitsBufer);
        }
        else if (instyp == C_INSTRUCTION) {
            dest_ptr = dest(code->parser);
            comp_prt = comp(code->parser);
            jum_prt = jump(code->parser);

            generate_C_instruction(dest_ptr, comp_prt, jum_prt, bitsBufer);
            fprintf(code->output,"%s\n", bitsBufer);
        }
    }

    return 0;
}

static int generate_A_instruction(SymbolTable *table,const char* symbol, char bitsBuffer[17])
{   
    for (int i = 0; i < 17; i++) {
        bitsBuffer[i] = '0';
    }

    uint16_t val;
    int perr = to_uint16(symbol, &val);
    if (perr == OVERFLOW_ERR)
    {
        return UNEXPECTED;
    }

    if (perr == NOT_NUMBER)
    {
        if (!symbol_table_get(table, symbol, &val)){
            return UNEXPECTED;
        }
    }

    to_binary(val, bitsBuffer);
    
    return 0;
}

static void generate_C_instruction(const char* dest, const char* comp, const char* jump, char bitsBuffer[17])
{   
    for (int i = 0; i < 17; i++) {
        bitsBuffer[i] = '0';
    }


    bitsBuffer[16] = 0;
    bitsBuffer[0] = bitsBuffer[1] = bitsBuffer[2] = '1';

    if (strstr(comp, "M")) {
        bitsBuffer[3] = '1';
    }

    if (dest) {
        const char* destBits  = lookup(destEntityTable, dest);
        if (!destBits) {
            fprintf(stderr, "invalid expression: %s\n", dest);
            exit(1);
        }

        for (int i = 0, j = 10; j < 13; i++, j++) {
            bitsBuffer[j] = destBits[i];
        }
    }

    if (comp) {
        const char* compBits  = lookup(computeEntityTable, comp);
        if (!compBits) {
            fprintf(stderr, "invalid expression: %s\n", comp);
            exit(1);
        }

        for (int i = 0, j = 4; j < 10; i++, j++) {
            bitsBuffer[j] = compBits[i];
        }
    }

    if (jump) {
        const char* jumpBits  = lookup(jumpEntityTable, jump);
        if (!jumpBits) {
            fprintf(stderr, "invalid expression: %s\n", jump);
            exit(1);
        }

        for (int i = 0, j = 13; j < 16; i++, j++) {
            bitsBuffer[j] = jumpBits[i];
        }
    }
}

static int to_uint16(const char* s, uint16_t *target) {
    if (!s || *s == '\0') return 0;

    errno = 0;
    char* endptr;

    long val = strtol(s, &endptr, 10);

    if (errno == ERANGE) return OVERFLOW_ERR; 
    if (endptr == s) return NOT_NUMBER; 
    if (*endptr != '\0') return NOT_NUMBER;
    if (val > Uint16_MAX)
    {
        return OVERFLOW_ERR;
    }

    *target = (uint16_t)val;

    return 0;
}

static void to_binary(uint16_t number, char *output)
{
    for (int i = 15; i >= 0; i--)
    {
        output[15 - i] = (number & (1 << i)) ? '1' : '0';
    }

    output[16] = 0;
}

static const char* lookup(const struct TableEntity table[], const char* key) {
    for (int i = 0; table[i].mnemonic != NULL; i++) {
        if (strcmp(table[i].mnemonic, key) == 0 ) {
            return table[i].bits;
        }
    }
    
    return NULL;
}

static char* change_file_extention(const char* filename) {
    const char *asm_ext = ".asm";
    size_t asm_ext_size = strlen(asm_ext);
    const char *ext = ".hack";
    size_t ext_size = strlen(ext);
    size_t file_size = strlen(filename);

    char* fname = (char*) malloc(file_size - asm_ext_size + ext_size + 1);
    strlcpy(fname, filename, file_size - asm_ext_size + 1);

    for (size_t i = file_size - asm_ext_size, j = 0; i <  file_size + ext_size; i++, j++) {
        fname[i] = ext[j];
    }

    fname[file_size - asm_ext_size + ext_size] = 0;

    return fname;
}

