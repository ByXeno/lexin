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

// TODO convert val to an union which holds
// lit op id key
// This is fine for right now but we can
// need in the near future
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
    bool res;
    uint32_t line;
    uint32_t col;
    char* cursor;
    char* last_cursor;
    FILE* err_out;
    char* file_name;
    char* ops;
    uint32_t opc;
    char** keys;
    uint32_t keyc;
    tokens_t tokens;
    str_list_t strs;
    char* sl_com;
    char* ml_com_start;
    char* ml_com_end;
    bool str_mode;
} lexin_t;

bool lexin_consume_context(lexin_t* l);
bool lexin_convert_to_token(lexin_t* l);
bool lexin_is_op(lexin_t* l,char c);
char* get_token_type_str(token_t t);
bool lexin_is_keyword(lexin_t* l,char* str);
uint32_t lexin_get_index_keyword(lexin_t* l,char* str);
void print_token(lexin_t* l,token_t t,uint32_t i);

#define FNV_OFFSET 0xcbf29ce484222325ULL
#define FNV_PRIME 0x100000001b3ULL

#endif // LEXIN_H_

#ifdef LEXIN_IMPLEMENTATION
#ifndef LEXIN_FIRST_IMPLEMENTATION
#define LEXIN_FIRST_IMPLEMENTATION

uint64_t lexin_string_hash
(char *str, uint32_t len)
{
    uint64_t hash = FNV_OFFSET;
    const uint64_t prime = FNV_PRIME;
    for (uint32_t i = 0; i < len; ++i) {
        hash ^= (uint8_t)str[i];
        hash *= prime;
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

void print_token
(lexin_t* l,token_t t,uint32_t i)
{
    if(t.type == token_op)
    {
        printf("token[%d]:%s %c\n",i,get_token_type_str(t),
        l->ops[t.val]);
    }else if(t.type == token_str)
    {
        printf("token[%d]:%s %s\n",i,get_token_type_str(t),
        l->strs.data[t.val]);
    }else if(t.type == token_key)
    {
        printf("token[%d]:%s %s\n",i,get_token_type_str(t),
        l->keys[t.val]);
    }else
    {printf("token[%d]:%s %d\n",i,get_token_type_str(t),t.val);}

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

#define lexer_printf(l,fmt,...) \
do { fprintf((l)->err_out,"%s:%d:%d:"fmt,(l)->file_name,(l)->line,(l)->col,__VA_ARGS__);} while(0)

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
        if (tok.val == (uint32_t)ULONG_MAX && errno == ERANGE) {
            lexer_printf(l,"Integer overflow \"%.*s\"\n",l->cursor - l->last_cursor,l->last_cursor);
            l->last_cursor = l->cursor+1;
            return false;
        }
        if(*end != '\0') {
            if(*end == 'x') {
                tok.val = strtoul(end,0,16);
            } else if(*end == 'b') {
                tok.val = strtoul(end,0,2);
            } else if(*end == 'o') {
                tok.val = strtoul(end,0,8);
            } else {
                lexer_printf(l,"Unknown suffix %s\n",end);
                l->last_cursor = l->cursor+1;
                return false;
            }
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
    if(isalpha(*l->last_cursor) || *l->last_cursor == '_') {
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
    if(l->cursor == l->last_cursor) return true;
    lexer_printf(l,"Unknown \"%.*s\"\n",l->cursor - l->last_cursor,l->last_cursor);
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
    if(!l->err_out)
    {l->err_out = stderr;}
    l->col = 1;l->line = 1;
    l->str_mode = false;
    uint32_t sl_len = strlen(l->sl_com);
    char buf[sl_len + 1];
    buf[sl_len] = 0;
    while(l->ctx_end > l->cursor)
    {
        // Add more checking
        if(l->str_mode && *l->cursor == '"' &&
        *(l->cursor-1) != '\\' &&
        *(l->cursor-1) != '\'') {
            token_t tok = {.val = l->strs.count,.type = token_str};
            char* str = strndup(l->last_cursor,l->cursor - l->last_cursor);
            lexin_da_append(&l->strs,str);
            lexin_da_append(&l->tokens,tok);
            l->last_cursor = l->cursor + 1;
            l->cursor++;
            l->str_mode = false;
            continue;
        }
        if(!l->str_mode && *l->cursor == '"' &&
        *(l->cursor-1) != '\\' &&
        *(l->cursor-1) != '\'') {
            if(!lexin_convert_to_token(l)){l->res = false;}
            l->last_cursor = l->cursor + 1;
            l->str_mode = true;
        }
        if(l->str_mode)
        {
            l->col++;
            l->cursor++;
            continue;
        }
        if(l->ctx_end - l->cursor > sl_len)
        {memcpy(buf,l->cursor,sl_len);}
        else {memset(buf,0,sl_len);}
        bool is_sl_com = (strcmp(buf,l->sl_com) == 0);
        // This includes '//' in the strings so we need first check
        // the comment is in the string or not and for that we should
        // add string system
        if(
        isblank(*l->cursor) || *l->cursor == '\n' || is_sl_com) {
            if(
            !(((l->cursor - 1 == l->last_cursor)
            || (l->cursor == l->last_cursor))
            && isblank(*l->last_cursor)))
            {
                if(!lexin_convert_to_token(l)){l->res = false;}
                l->last_cursor--;
            }
            if(is_sl_com) {
                char* end = strchr(l->cursor,'\n');
                if(!end) {break;}
                l->cursor = end+1;
                l->line++;
                l->col = 1;
                l->last_cursor = end+1;
            }
            if(*l->cursor == '\n')
            {
                l->line++;
                l->col = 1;
                l->last_cursor = l->cursor+1;
            }
            else{l->last_cursor++;}
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
        l->col++;
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
