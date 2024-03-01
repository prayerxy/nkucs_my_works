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
#define YYSTYPE char*
#endif
int yylex();
extern int yyparse();
FILE* yyin;
char num[100]="";
void yyerror(const char* s);
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
%left ADD MINUS
%left multiply divide
%right UMINUS   
%nonassoc leftp rightp //无结合性


%%


lines   :       lines expr ';' { printf("%s\n", $2); }
        |       lines ';'
        |
        ;
//TODO:完善表达式的规则
expr    :       expr ADD expr   { $$=(char*)malloc(100);strcpy($$,$1); strcat($$,$3); strcat($$,"+ "); }
        |       expr MINUS expr   { $$=(char*)malloc(100);strcpy($$,$1); strcat($$,$3); strcat($$,"- ");}
        |       expr multiply expr   { $$=(char*)malloc(100);strcpy($$,$1); strcat($$,$3); strcat($$,"* ");}
        |       expr divide expr   { $$=(char*)malloc(100);strcpy($$,$1); strcat($$,$3); strcat($$,"/ ");}
        |       leftp expr rightp   {$$ = (char*)malloc(100); strcpy($$,$2);}
        |       MINUS expr %prec UMINUS   {$$ = (char*)malloc(100);strcpy($$,$2);strcat($$,"- ");}
        |       NUMBER  {$$ = (char*)malloc(100); strcpy($$,$1); strcat($$," ");}
        ;

%%

// programs section

int yylex()
{
    int t;
    while(1){
        t=getchar();
        if(t==' '||t=='\t'||t=='\n'){
            //忽略空格、制表符、回车等空白符
        }else if(t>='0' && t<='9'){
            int tmp=0;
            //TODO:解析多位数字返回数字类型
            while(t>='0' && t<='9'){
                num[tmp]=t;
                t=getchar();
                tmp++;
            }
            num[tmp]=0;//多一个空格符
            yylval=num;
            ungetc(t,stdin);
            return NUMBER;

        }else if(t=='+'){
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
    }while(!feof(yyin));
    return 0;
}
void yyerror(const char* s){
    fprintf(stderr,"Parse error: %s\n",s);
    exit(1);
}