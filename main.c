#define LEXIN_IMPLEMENTATION
#include "lexin.h"

int main(void)
{
    lexin_t lex = {0};
    lex.ops = "+-/*();";
    lex.opc = strlen(lex.ops);
    char* arr[] = {"ret","goto"};
    lex.keys = arr;
    lex.keyc = 2;
    lex.ctx = "1 + 20 goto ret";
    bool res = !lexin_consume_context(&lex);
    printf("token count: %d\n",lex.tokens.count);
    uint32_t i = 0;
    for(;i < lex.tokens.count;++i){
        if(lex.tokens.data[i].type == token_op)
        {
            printf("token[%d]:%s %c\n",i,get_token_type_str(lex.tokens.data[i]),
            lex.ops[lex.tokens.data[i].val]);
        }else
        {
            printf("token[%d]:%s %d\n",i,get_token_type_str(lex.tokens.data[i]),
            lex.tokens.data[i].val);
        }
    }
    return res;
}
