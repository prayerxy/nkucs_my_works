%{
/*********************************************
将所有的词法分析功能均放在 yylex 函数内实现，为 +、-、*、\、(、 ) 每个运算符及整数分别定义一个单词类别，在 yylex 内实现代码，能
识别这些单词，并将单词类别返回给词法分析程序。
实现功能更强的词法分析程序，可识别并忽略空格、制表符、回车等
空白符，能识别多位十进制整数。
YACC file
**********************************************/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#ifndef YYSTYPE
typedef struct {
    double num;
    char* str;
} s;
#define YYSTYPE s
#endif
int yylex();
extern int yyparse();
FILE* yyin;

void yyerror(const char* s);
struct SymbolEntry {
    char name[100];
    double value;
};
 char IDs[100]; //储存标识符的字符串
struct SymbolEntry symbol_table[100];
int symbol_count = 0;
void insert_symbol(const char* identifier, double value);
double lookup_symbol(const char* identifier);

%}
//定义区
//TODO:给每个符号定义一个单词类别


%token ADD 
%token MINUS
%token multiply 
%token divide
%token leftp 
%token rightp
%token NUMBER
%token ID
%token equal   //赋值符号

%type expr
%type lines
%type expr2

%right equal
%left ADD MINUS
%left multiply divide
%right UMINUS 
  
%nonassoc leftp rightp //无结合性


%%


lines   :       lines expr ';' { printf("%f\n", $2.num); }
        |       lines expr2 ';'{ printf("%s\n", $2.str); }
        |       lines ';'
        |
        ;
//TODO:完善表达式的规则
expr    :       expr ADD expr   { $$.num=$1.num+$3.num; }
        |       expr MINUS expr   { $$.num=$1.num-$3.num; }
        |       expr multiply expr   { $$.num=$1.num*$3.num; }
        |       expr divide expr   { $$.num=$1.num/$3.num; }
        |       leftp expr rightp   { $$.num=$2.num; }
        |       MINUS expr %prec UMINUS   {$$.num=-$2.num;}
        |       ID      {$$.num=lookup_symbol($1.str);$$.str=(char*)malloc(100);strcpy($$.str,$1.str);}
        |       NUMBER  {$$.num=$1.num;}
        ;

expr2   :       ID equal expr   {$$.num=$3.num;$$.str=(char*)malloc(100);strcpy($$.str,$1.str);strcat($$.str,"被赋值");insert_symbol($1.str,$$.num);}
        ;

%%

// programs section
//插入符号表
void insert_symbol(const char* identifier, double value) {
    //更新值
     for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, identifier) == 0) {
            symbol_table[i].value = value;
            return;
        }
    }
    if (symbol_count < 100) {
        strcpy(symbol_table[symbol_count].name, identifier);
        symbol_table[symbol_count].value = value;
        symbol_count++;
    } else {
        fprintf(stderr, "Symbol table is full. Cannot insert more symbols.\n");
    }
}

// 查询符号表中的值
double lookup_symbol(const char* identifier) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, identifier) == 0) {
            return symbol_table[i].value;
        }
    }
    return 0.0;
    //fprintf(stderr, "Undefined variable: %s\n", identifier);
    //exit(1);
}


int yylex()
{
    int t;
    while(1){
        t=getchar();
        if(t==' '||t=='\t'||t=='\n'){
            //忽略空格、制表符、回车等空白符
        }else if(isdigit(t)){
            yylval.num=0;
            yylval.str="";
            //TODO:解析多位数字返回数字类型
            while(isdigit(t)){
                yylval.num=yylval.num*10+t-'0';
                t=getchar();
            }
            ungetc(t,stdin);
            return NUMBER;

        } //标识符
        else if(t=='_'||(t>='a'&&t<='z')||(t>='A'&&t<='Z')){
           
            int tmp=0;
            yylval.str=(char*)malloc(100);
            while(t=='_'||(t>='a'&&t<='z')||(t>='A'&&t<='Z')||(t>='0' && t<='9')){
                IDs[tmp]=t;
                t=getchar();
                tmp++;
            }
            IDs[tmp]=0;//多一个空格符
            strcpy(yylval.str,IDs);
        
            yylval.num=lookup_symbol(IDs);
            ungetc(t,stdin);
            return ID;

        }
        else if(t=='+'){
            return ADD;
        }else if(t=='-'){
            return MINUS;
        }//TODO:识别其他符号
        else if(t=='*'){
            return multiply;
        }
        else if(t=='/'){
            return divide;
        }
        else if(t=='('){
            return leftp;
        }
        else if(t==')'){
            return rightp;
        }
        else if(t=='='){
            return equal;
        }
        else{
            return t;
        }
    }
}

int main(void)
{
    yyin = stdin;
    do{
        yyparse();
        for(int i=0;i<100;i++){
            IDs[i]=0;
        }
    }while(!feof(yyin));
    return 0;
}
void yyerror(const char* s){
    fprintf(stderr,"Parse error: %s\n",s);
    exit(1);
}