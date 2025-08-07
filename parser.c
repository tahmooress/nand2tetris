#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <ctype.h>

#define PARSE_OK 0
#define PARSE_INVALID 1
#define PARSE_EOF -1

typedef enum{
    INVALID,
    A_INSTRUCTION,
    C_INSTRUCTION,  
    L_INSTRUCTION
}instruction_type;

const char* instruction_type_names[] = {
    "INVALID",
    "A_INSTRUCTION",
    "C_INSTRUCTION",
    "L_INSTRUCTION"
};

typedef struct {
    FILE *file;
    int hasNext;
    size_t line_number;
    char  line_buffer[512];
    char* line;
    instruction_type current_instruction_type;
    char* symbol;
    char* dest;
    char* comp;
    char* jump;
}Parser;

static void parse_symbol_parts(Parser *p);
static void parse_instruction_parts(Parser* p);
static int read_till_none_blank(Parser *p);
static short int is_comment(const char* s);
static char* trim(char* s);
void remove_whitespace(const char* src, char* dest);
static instruction_type parse_instruction_type(const char* instruction);

Parser* init_parser(const char *filename)
{
    const char *ext = ".asm";
    size_t ext_size = strlen(ext);
    size_t file_size = strlen(filename);
    
    if (file_size < ext_size)
    {   
        fprintf(stderr, "Err: input file should have .asm extention");
        return NULL;
    }

    if (strcmp(filename + (file_size - ext_size), ext))
    {
        fprintf(stderr, "Err: input file should have .asm extention");
        return NULL;
    }

    FILE *file = fopen(filename, "r");
    if (!file)
    {   
        perror( "Err opening file");
        return NULL;
    }

    Parser *p = malloc(sizeof(Parser));
    if (!p) return NULL;

    p->file = file;
    p->line_number = 0;
    p->hasNext = 1;
    p->symbol = NULL;
    p->dest = NULL;
    p->comp = NULL;
    p->jump = NULL;
    p->line = NULL;

    return p;
}

int hasMoreLines(Parser *p)
{
    return p->hasNext;
}

int advance(Parser *p)
{   
    int result = read_till_none_blank(p);

    if (result == PARSE_EOF) return PARSE_EOF;

    if (result == PARSE_INVALID)
    {
        fprintf(stderr, "Syntax error at line %zu: \"%s\"\n", p->line_number, p->line);
        return PARSE_INVALID;
    }

   instruction_type instyp = parse_instruction_type(p->line);
   if (instyp == INVALID)
   {    
        fprintf(stderr, "Invalid instruction at line %zu: \"%s\"\n", p->line_number, p->line);
        return PARSE_INVALID;
   }
   p->current_instruction_type = instyp;

   if (instyp == A_INSTRUCTION || instyp == L_INSTRUCTION)
   {
        parse_symbol_parts(p);
   }
    else if (instyp == C_INSTRUCTION){
        parse_instruction_parts(p);
   }

   return PARSE_OK;
}

instruction_type instructionType(Parser *p)
{   
    return p->current_instruction_type;
}


size_t current_line_number(Parser *p)
{
    return p->line_number;
}

const char *symbol(Parser *p)
{
    return p->symbol;
}

const char* dest(Parser *p)
{
    return p->dest;
}

const char* comp(Parser *p)
{
    return p->comp;
}

const char* jump(Parser *p)
{   
  return p->jump;
}

void free_parser(Parser* p) {
    if (!p) return;
    if (p->file) fclose(p->file);
    free(p);
}

void reset(Parser* p)
{   
    if (!p) return;

    if (p->file) 
    {
        fseek(p->file, 0, SEEK_SET);
    }
    
    p->line = NULL;
    p->line_number = 0;
    p->hasNext = 1;
    p->symbol = NULL;
    p->dest = NULL;
    p->comp = NULL;
    p->jump = NULL;
    p->line = NULL;
}

static instruction_type parse_instruction_type(const char* instruction)
{   
    if(!instruction || !*instruction) return INVALID;

    if (instruction[0] == '@') return A_INSTRUCTION;
    
    if (strstr(instruction, "=") || strstr(instruction, ";")) return C_INSTRUCTION;

    size_t len = strlen(instruction);
    if (len > 2 && (instruction[0] == '(') && (instruction[len - 1] == ')')) return L_INSTRUCTION;

    return INVALID;
}

static char* trim(char* s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    
    char* end;
    if ((end = strstr(s, "//")) != NULL) {
        *end = 0;
    }

    end = s + strlen(s) - 1;
    while (end > s && (isspace((unsigned char)*end) || *end == '/')) end--;

    end[1] = '\0';
    return s;
}

void remove_whitespace(const char* src, char* dest) {
    while (*src) {
        if (!isspace((unsigned char)*src)) {
            *dest++ = *src;
        }
        src++;
    }
    *dest = '\0';
}

// is_comment takes and string line and check if it
// it is a comment line means start with two contineus '//'
// returns 0 if not comment, 1 if comment and -1 if its a invalid line. 
static short int is_comment(const char* s)
{   
    // empty line is not comment line.
    if (!*s) return 0;

    if (*s++ != '/')
    {
       return 0;
    }

    return (*s && *s == '/') ? 1 : -1;
}

static int read_till_none_blank(Parser *p)
{   
    while((fgets(p->line_buffer, sizeof(p->line_buffer), p->file)))
    {   
        p->line = trim(p->line_buffer);

        short int cmt = is_comment(p->line);

        if (cmt < 0) return PARSE_INVALID;
        
        if (!cmt && *p->line)
        {
            p-> hasNext = 1;

            return PARSE_OK;
        }
    }

    p-> hasNext = 0;
    return PARSE_EOF;
}

static void parse_instruction_parts(Parser* p)
{
    p->dest = NULL;
    p->comp = NULL;
    p->jump = NULL;

    char* ptr = p->line;
    while(*ptr && (*ptr != ';') && (*ptr != '=')) ptr++;
    
    size_t len;

    if (*ptr && *ptr == '=')
    {
        *ptr = '\0';
        p->dest = trim(p->line);
        len = strlen(p->dest);
        char destbuff[len];
        remove_whitespace(p->dest, destbuff);
        p->dest = destbuff;

        p->comp = trim(++ptr);
        len = strlen(p->comp);
        char compBuff[len];
        remove_whitespace(p->comp, compBuff);
        p->comp = compBuff;
    }else if (*ptr && *ptr == ';')
    {
        *ptr = '\0';
        p->comp = trim(p->line);
        len = strlen(p->comp);
        char compBuff[len];
        remove_whitespace(p->comp, compBuff);
        p->comp = compBuff;

        p->jump = trim(++ptr);
        len = strlen(p->jump);
        char jumpBuff[len];
        remove_whitespace(p->jump, jumpBuff);
        p->jump = jumpBuff;
    }

    p->line_number++;
}

static void parse_symbol_parts(Parser *p)
{   
    p->symbol = NULL;

    if (p->current_instruction_type == A_INSTRUCTION)
    {   
        p->symbol = p->line + 1;
        p->line_number++;
    }  

    if (p->current_instruction_type == L_INSTRUCTION)
    {
        size_t len = strlen(p->line);
        p->line[len - 1] = '\0';   
        p->symbol = p->line + 1;
    }
}
