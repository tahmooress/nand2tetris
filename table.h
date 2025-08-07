#ifndef TABLE_H
#define TABLE_H

typedef struct SymbolTable SymbolTable;

SymbolTable* symbol_table_init(void);
int symbol_table_get(SymbolTable *t, const char *key, uint16_t *targetVal);
void symbol_table_set(SymbolTable *t, const char *key, uint16_t val);
int symbol_table_delete(SymbolTable *t,const char *key, uint16_t* val);
void symbol_table_free(SymbolTable* t, int free_keys);

#endif

