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
#include<ctype.h>
#ifndef YYSTYPE
#define YYSTYPE double
#endif
int yylex();
extern int yyparse();
FILE* yyin;
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


lines   :       lines expr ';' { printf("%f\n", $2); }
        |       lines ';'
        |
        ;
//TODO:完善表达式的规则
expr    :       expr ADD expr   { $$=$1+$3; }
        |       expr MINUS expr   { $$=$1-$3; }
        |       expr multiply expr   { $$=$1*$3; }
        |       expr divide expr   { $$=$1/$3; }
        |       leftp expr rightp   { $$=$2; }
        |       MINUS expr %prec UMINUS   {$$=-$2;}
        |       NUMBER  {$$=$1;}
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
        }else if(isdigit(t)){
            yylval=0; //yylval是一个全局变量，用于存储词法分析器返回的标记值
            //TODO:解析多位数字返回数字类型
            while(isdigit(t)){
                yylval=yylval*10+t-'0';
                t=getchar();
            }
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