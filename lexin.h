#ifndef LEXIN_H_
#define LEXIN_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>

#ifndef LEXIN_INIT_CAP
#define LEXIN_INIT_CAP 16
#endif // LEXIN_INIT_CAP

#define lexin_da_alloc(list, exp_cap)           \
    do {                                        \
        if ((exp_cap) > (list)->capacity) {     \
            if ((list)->capacity == 0) {        \
                (list)->capacity = LEXIN_INIT_CAP;\
            }                                   \
            while ((exp_cap) > (list)->capacity) { \
                (list)->capacity *= 2;             \
            }                                      \
            (list)->data = (typeof((list)->data))realloc((list)->data,(list)->capacity * sizeof(*(list)->data)); \
            assert((list)->data != NULL && "Ram is not enough!"); \
        } \
    } while (0)

#define lexin_da_append(list, element)            \
    do {                                       \
        lexin_da_alloc((list), (list)->count + 1); \
        (list)->data[(list)->count++] = (element);   \
    } while (0)

#define lexin_da_append_many(list, new_el, new_el_count)          \
    do {                                                          \
        lexin_da_alloc((list), (list)->count + (new_el_count)); \
        memcpy((list)->data + (list)->count, (new_el), (new_el_count)*sizeof(*(list)->data)); \
        (list)->count += (new_el_count); \
    } while (0)

#define lexin_da_append_null(list) lexin_da_append(list,0)
#define lexin_da_append_cstr(list,str) lexin_da_append_many(list,str,strlen(str))
#define lexin_da_clear(list) \
do {memset((list)->data,0,(list)->capacity); (list)->count = 0; } while(0)
#define lexin_da_free(list) \
do { free((list)->data); (list)->data = NULL; (list)->count = 0; (list)->capacity = 0; } while(0)

#define unreachable(...) \
do { fprintf(stderr,__VA_ARGS__);exit(1);} while(0)

typedef enum {
    token_unknown = 0,
    token_str,
    token_lit,
    token_id,
    token_key,
    token_op,
} token_type_t;

typedef struct {
    token_type_t type;
    int64_t val;
} token_t;

typedef struct {
    token_t* data;
    uint32_t count;
    uint32_t capacity;
} tokens_t;

typedef struct {
    char** data;
    uint32_t count;
    uint32_t capacity;
} str_list_t;

typedef struct {
    char* ctx;
    char* ctx_end;
    uint32_t line;
    char* cursor;
    char* last_cursor;
    char* ops;
    uint32_t opc;
    char** keys;
    uint32_t keyc;
    tokens_t tokens;
    str_list_t strs;
    bool res;
} lexin_t;

bool lexin_consume_context(lexin_t* l);
bool lexin_convert_to_token(lexin_t* l);
bool lexin_is_op(lexin_t* l,char c);
char* get_token_type_str(token_t t);
bool lexin_is_keyword(lexin_t* l,char* str);
uint32_t lexin_get_index_keyword(lexin_t* l,char* str);

#define FNV_PRIME 16777619u
#define FNV_OFFSET 2166136261u

#endif // LEXIN_H_

#ifdef LEXIN_IMPLEMENTATION
#ifndef LEXIN_FIRST_IMPLEMENTATION
#define LEXIN_FIRST_IMPLEMENTATION

uint32_t lexin_string_hash
(char *str, uint32_t len)
{
    uint32_t hash = FNV_OFFSET;
    for (uint32_t i = 0; i < len; ++i) {
        hash ^= (uint8_t)str[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

bool lexin_is_keyword
(lexin_t* l,char* str)
{
    uint32_t i = 0;
    for(;i < l->keyc;++i){
        if(strcmp(str,l->keys[i]) == 0) return true;
    }
    return false;
}

uint32_t lexin_get_index_keyword
(lexin_t* l,char* str)
{
    uint32_t i = 0;
    for(;i < l->keyc;++i){
        if(strcmp(str,l->keys[i]) == 0) return i;
    }
    unreachable("Got invalid string at lexin_get_index_keyword");
}

char* get_token_type_str
(token_t t)
{
    switch(t.type){

        case token_str: return "Str";
        case token_lit: return "Lit";
        case token_op: return "Op";
        case token_id: return "Id";
        case token_key: return "Key";
        case token_unknown:
        default: return "Unknown";
    }
}

bool lexin_is_op
(lexin_t* l,char c)
{
    uint32_t i = 0;
    for(;i < l->opc;++i)
    {
        if(c == l->ops[i]) return true;
    }
    return false;
}

uint32_t lexin_get_index_op
(lexin_t* l,char c)
{
    uint32_t i = 0;
    for(;i < l->opc;++i)
    {
        if(c == l->ops[i]) return i;
    }
    unreachable("Got invalid char at lexin_get_index_op");
}

bool lexin_convert_to_token
(lexin_t* l)
{
    token_t tok = {0};
    char arr[l->cursor - l->last_cursor + 1];
    memcpy(arr,l->last_cursor,l->cursor - l->last_cursor);
    arr[l->cursor - l->last_cursor] = 0;
    if(isdigit(*l->last_cursor)) {
        char* end = 0;
        tok.val = strtoul(arr,&end,10);
        if(*end != '\0') {
            printf("Unknown suffix %s\n",end);
            l->last_cursor = l->cursor+1;
            return false;
        }
        if (tok.val == (uint32_t)ULONG_MAX && errno == ERANGE) {
            printf("Integer overflow \"%.*s\"\n",l->cursor - l->last_cursor,l->last_cursor);
            l->last_cursor = l->cursor+1;
            return false;
        }
        if(l->tokens.count > 1) {
            if(
            l->tokens.data[l->tokens.count-1].type == token_op &&
            l->ops[l->tokens.data[l->tokens.count-1].val] == '-' &&
            l->tokens.data[l->tokens.count-2].type == token_op)
            {
                l->tokens.count--;
                tok.val *= -1;
            }
        }
        tok.type = token_lit;
        goto end;
    }
    if(lexin_is_op(l,*l->last_cursor)) {
        tok.val = lexin_get_index_op(l,*l->last_cursor);
        tok.type = token_op;
        goto end;
    }
    if(isalpha(*l->last_cursor)) {
        if(lexin_is_keyword(l,arr)) {
            tok.type = token_key;
            tok.val = lexin_get_index_keyword(l,arr);
            goto end;
        } else {
            tok.type = token_id;
            tok.val = lexin_string_hash(arr,strlen(arr));
            goto end;
        }
    }
    printf("Unknown \"%.*s\"\n",l->cursor - l->last_cursor,l->last_cursor);
    l->last_cursor = l->cursor+1;
    return false;
end:
    lexin_da_append(&l->tokens,tok);
    l->last_cursor = l->cursor+1;
    return true;
}

bool lexin_consume_context
(lexin_t* l)
{
    if(!l->ctx) {return false;}
    if(!l->cursor)
    {l->cursor = l->ctx;}
    if(!l->last_cursor)
    {l->last_cursor = l->ctx;}
    if(!l->ctx_end)
    {l->ctx_end = strrchr(l->ctx,'\0');}
    l->res = true;
    while(l->ctx_end > l->cursor)
    {
        if(isblank(*l->cursor)) {
            if(
            !(((l->cursor - 1 == l->last_cursor)
            || (l->cursor == l->last_cursor))
            && isblank(*l->last_cursor)))
            {
                if(!lexin_convert_to_token(l)) {
                    l->res = false;
                }
                l->last_cursor--;
            }
            l->last_cursor++;
            l->cursor++;
            continue;
        }
        if(lexin_is_op(l,*l->cursor)) {
            if(l->cursor != l->last_cursor && !isblank(*l->last_cursor))
            {
                if(!lexin_convert_to_token(l))
                {l->res = false;}
            }
            token_t tok = {.type = token_op,
            .val = lexin_get_index_op(l,*l->cursor)};
            lexin_da_append(&l->tokens,tok);
            l->last_cursor = l->cursor+1;
            l->cursor++;
            continue;
        }
        l->cursor++;
    }
    if(l->last_cursor != l->cursor)
    {
        if(!lexin_convert_to_token(l))
        {l->res = false;}
    }
    return l->res;
}

#endif // LEXIN_FIRST_IMPLEMENTATION
#endif // LEXIN_IMPLEMENTATION
