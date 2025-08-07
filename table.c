#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define MAX_TABLE_SIZE 256
#define MAX_KEY_SIZE 50
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static unsigned int hash_index(const char *key);

typedef struct Symbol
{   const char* key;
    uint16_t val;
    struct Symbol *next;
}Symbol;

typedef struct
{
   Symbol* hash_table[MAX_TABLE_SIZE];
}SymbolTable;

SymbolTable* symbol_table_init(void)
{       
    SymbolTable *st = (SymbolTable*) malloc(sizeof(SymbolTable));

    for(int i =0; i < MAX_TABLE_SIZE; i++){
        st->hash_table[i] = NULL;
    }

    return st;
}

int symbol_table_get(SymbolTable *t, const char *key, uint16_t *targetVal)
{
    if (!t) return 0;

    unsigned int index = hash_index(key);

    Symbol* head = t->hash_table[index];
    while (head && strncmp(head->key, key, MAX_KEY_SIZE))
    {
        head = head->next;
    }

    if (head){
        *targetVal = head-> val;
        return 1;
    }
    
    return 0;
}   

void symbol_table_set(SymbolTable *t, const char *key, uint16_t val)
{   
    if (!t) return;

    unsigned int index = hash_index(key);
    Symbol *head = t->hash_table[index];

    while (head && strncmp(head->key, key, MAX_KEY_SIZE))
    {   
        head = head->next;
    }

    if (head) {
        head->val = val;
        return;
    }

    Symbol* s = (Symbol*) malloc(sizeof(Symbol));
    s->key = strdup(key);
    s->val = val;
    s->next = t->hash_table[index];
    t->hash_table[index] = s;
}

int symbol_table_delete(SymbolTable *t,const char *key, uint16_t* val)
{
    if (!t) return 0;

    unsigned int index = hash_index(key);

    Symbol* head = t->hash_table[index];
    Symbol *prev;
    while (head && strncmp(head->key, key, MAX_KEY_SIZE))
    {   
       prev = head;
       head = head->next;
    }

    uint16_t value;
    int exist = 0;

    if (head == NULL) return exist;
    
    if (prev == NULL) {
        value = head->val;
        t->hash_table[index] = head->next;
        exist = 1;
    }else {
        value = head->val;
        prev->next = head->next;
        exist = 1;
    }

    if (val) {
        *val = value;
    }


    return exist;
}


static unsigned int hash_index(const char *key)
{   
    if (!key) return 0;

    size_t key_len = strlen(key);
    size_t len =  MIN(key_len, MAX_KEY_SIZE);

    unsigned int index = 0;
    for (size_t i= 0; i < len; i++)
    {
        index += ((unsigned int) key[i]) * i;
    }

    return index;
}

void symbol_table_free(SymbolTable* t, int free_keys) {
    if (!t) return;

    for (int i = 0; i < MAX_TABLE_SIZE; i++) {
        Symbol* current = t->hash_table[i];
        while (current) {
            Symbol* next = current->next;

            if (free_keys) {
                free((void*)current->key);
            }
            free(current);

            current = next;
        }
        t->hash_table[i] = NULL;
    }

    free(t);
}
