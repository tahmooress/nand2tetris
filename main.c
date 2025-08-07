#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "./parser.h"
#include "./code.h"

int main(int argc, char *argv[])
{
    if (argc < 2) 
    {
        fprintf(stderr, "path to the .ams file not specifiy");
        return -1;
    }

    const char *file_name = argv[1];
   
    Parser *parser = init_parser(file_name);
    if (!parser)
    {
        exit(1);
    }

    Code *c = init_code(parser, file_name);
    if (c == NULL) {
        printf("init error\n");
        free_parser(parser);
        exit(1);
    }

    int ext_code = 0;
    if (assemble(c)) {
        ext_code = 1;
    }

    free_code(c);
    exit(ext_code);
}
