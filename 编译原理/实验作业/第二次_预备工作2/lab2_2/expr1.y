%{
/*********************************************
YACC file
基础程序
Date:2023/9/19
forked SherryXiye
**********************************************/
#include<stdio.h>
#include <ctype.h>
#include<stdlib.h>
#ifndef YYSTYPE
#define YYSTYPE double
#endif
int yylex();
extern int yyparse();
FILE* yyin;
void yyerror(const char* s);
%}
//注意先后定义的优先级区别
//token声明了终结符号
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


lines   :       lines expr '\n' { printf("%f\n", $2); }
        |       lines '\n'
        |
        ;

expr    :       expr ADD expr   { $$=$1+$3; }// $$代表产生式左部的属性值，$n 为产生式右部第n个token的属性值
        |       expr MINUS expr   { $$=$1-$3; }
        |       expr multiply expr   { $$=$1*$3; }
        |       expr divide expr   { $$=$1/$3; }
        |       leftp expr rightp   { $$=$2; }
        |       MINUS expr %prec UMINUS   {$$=-$2;}//%prec UMINUS是一个优先级指示器，用于指定一元减号运算符的优先级
        |       NUMBER  {$$=$1;}
        ;
    


%%

// programs section

int yylex()
{
    while(1){
        int t=getchar();
       if(isdigit(t)){
            yylval=t-'0';
            return NUMBER;//yylval是一个全局变量，用于存储词法分析器返回的标记值
            
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
    yyin=stdin;
    do{
        yyparse();
    }while(!feof(yyin));
    return 0;
}
void yyerror(const char* s){
    fprintf(stderr,"Parse error: %s\n",s);
    exit(1);
}