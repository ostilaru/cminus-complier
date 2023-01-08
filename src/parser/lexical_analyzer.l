%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>

#include "syntax_tree.h"
#include "syntax_analyzer.h"

//#ifndef LAB1_ONLY
//#include "syntax_analyzer.h"
//#endif



int lines;
int pos_start;
int pos_end;

int files_count;
int cmFlag;

void pass_node(char *text){
     yylval.node = new_syntax_tree_node(text);
}

/*****************声明和选项设置  end*****************/

%}

digit          [0-9]
INTEGER        {digit}+
FLOATPOINT     ({digit}+".")|({digit}*"."{digit}+)

letter         [a-zA-Z]
ID             {letter}+
/*NEWLINE        (\n)+ */
WHITESPACE     ([\t]+ | {" "}+ | [\r]+)

%x comment

%%
 /* to do for students */
 /* two cases for you, pass_node will send flex's token to bison */


\+ 	     {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return ADD;}
\-        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return SUB;}
\*        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return MUL;}
\/        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return DIV;}
\<\=      {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return LTE;}
\<        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return LT;}
\>\=      {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return GTE;}
\>        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return GT;}
\=\=      {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return EQ;}
\!\=      {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return NEQ;}
\=        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return ASSIN;}
\;        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return SEMICOLON;}
\,        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return COMMA;}
\(        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return LPARENTHESE;}
\)        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return RPARENTHESE;}
 /* *\[\]      {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return ARRAY;} */
\[        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return LBRACKET;}
\]        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return RBRACKET;}
\{        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return LBRACE;}
\}        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return RBRACE;}
else      {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return ELSE;}
if        {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return IF;}
int       {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INT;}
return    {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return RETURN;}
void      {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return VOID;}
while     {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return WHILE;}
float               {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOAT;}
 /* ([0-9]*\.[0-9]+|[0-9]+\.)    {pos_start=pos_end;pos_end=pos_start+strlen(yytext);return FLOATPOINT;} */
 
 {ID}               {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return IDENTIFIER;}
 {INTEGER}          {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INTEGER;}
 {FLOATPOINT}     {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOATPOINT;}
\t                  {pos_start = pos_end; pos_end += strlen(yytext);  }
\n                  {lines++; pos_start=1;pos_end=1; }
 /* {WHITESPACE}     {lines++; pass_node(yytext); }*/
" "       {pos_start = pos_end; pos_end += 1;}
 /* \/\*      {
               cmFlag = 0;
               pos_end += strlen(yytext);
               char c;
               do{
                    c = input();
                    pos_end++;
                    if(c == '\n'){
                         pos_end = 1;
                         lines++;
                    }
                    if(cmFlag == 0 && c == '*')   cmFlag = 1;
                    else if(cmFlag == 1 && c == '*')   cmFlag = 1;
                    else if(cmFlag == 1 && c == '/')   cmFlag = 2;
                    else cmFlag = 0;
               }while(cmFlag != 2);
               //return COMMENT;
          }     */
"/*"         BEGIN(comment);

    <comment>[^*\n]*        /* eat anything that's not a '*' */
    <comment>"*"+[^*/\n]*   /* eat up '*'s not followed by '/'s */
    <comment>\n             lines++;
    <comment>"*"+"/"        BEGIN(INITIAL); 

 /****请在此补全所有flex的模式与动作  end******/

%%

// flex部分结束，这里可以正常写c代码了

/*
int count_num_enter(char *string) {
    int ret = 0;
    int index = 0;
    while(string[index] != '\0') {
        if(string[index] == '\n')
            ret++;
        index++;
    }
    return ret;
}
*/

