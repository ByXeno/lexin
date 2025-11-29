#define LEXIN_IMPLEMENTATION
#include "lexin.h"

char* read_entrie_file
(FILE* file,uint32_t* count)
{
    int64_t size = 0,current_pos = 0;
    current_pos = ftell(file);
    if(current_pos == -1) return 0;
    if(fseek(file, 0, SEEK_END) != 0) return 0;
    size = ftell(file);
    if(size == -1) return 0;
    if(fseek(file, current_pos, SEEK_SET) != 0) return 0;
    char* buffer = calloc(size,sizeof(char));
    if(!buffer) return 0;
    *count = fread(buffer,sizeof(char),size,file);
    if(*count != size) {free(buffer);return 0;}
    return buffer;
}

int main(void)
{
    lexin_t lex = {0};
    lex.ops = "^|!#&[]%.,-<>={}+-\\/*\'\"():;";
    lex.opc = strlen(lex.ops);
    char* arr[] = {
        "return","goto","if","int64_t","char","sizeof",
        "define","include","unsigned"};
    uint32_t a;
    lex.keys = arr;
    lex.keyc = 9;
    FILE* fptr = fopen("lexin.h","r");
    if(!fptr) return 1;
    lex.ctx = read_entrie_file(fptr,&a);
    fclose(fptr);
    bool res = !lexin_consume_context(&lex);
    free(lex.ctx);      
    printf("token count: %d\n",lex.tokens.count);
    #if 0
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
    #endif
    free(lex.tokens.data);
    return res;
}
