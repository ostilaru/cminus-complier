%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "syntax_tree.h"
//#include "struct _syntax_node_tree"

// external functions from lex
extern int yylex();
extern int yyparse();
extern int yyrestart();
extern FILE * yyin;

// external variables from lexical_analyzer module
extern int lines;
extern char * yytext;
extern int pos_end;
extern int pos_start;

// Global syntax tree
syntax_tree *gt;

// Error reporting
void yyerror(const char *s);

// Helper functions written for you with love
syntax_tree_node *node(const char *node_name, int children_num, ...);
%}

/* TODO: Complete this definition.
   Hint: See pass_node(), node(), and syntax_tree.h.
         Use forward declaring. */
%union {
    
    struct _syntax_tree_node  *node;
}

/* TODO: Your tokens here. */
%token <node>   ERROR       258   
%token <node>	ADD         259   
%token <node>	SUB         260   
%token <node>	MUL         261   
%token <node>	DIV         262   
%token <node>	LT          263   
%token <node>	LTE         264   
%token <node>	GT          265   
%token <node>	GTE         266   
%token <node>	EQ          267   
%token <node>	NEQ         268   
%token <node>	ASSIN       269   
%token <node>	SEMICOLON   270   
%token <node>	COMMA       271   
%token <node>	LPARENTHESE 272   
%token <node>	RPARENTHESE 273   
%token <node>	LBRACKET    274   
%token <node>	RBRACKET    275   
%token <node>	LBRACE      276   
%token <node>	RBRACE      277   
%token <node>	ELSE        278   
%token <node>	IF          279   
%token <node>	INT         280   
%token <node>	RETURN      281   
%token <node>	VOID        282   
%token <node>	WHILE       283   
%token <node>	IDENTIFIER  284   
%token <node>	INTEGER     285   
//%token <node>	ARRAY       286   
%token <node>	LETTER      287 
%token <node>	FLOATPOINT  288 
%token <node>	FLOAT       289
//%token <node>	BLANK       290
%type  <node> program

%start program

%type <node> declaration-list 
%type <node> declaration var-declaration
%type <node> type-specifier fun-declaration params param-list param
%type <node> compound-stmt local-declarations  statement-list statement
%type <node> expression-stmt selection-stmt iteration-stmt return-stmt
%type <node> expression var simple-expression relop additive-expression
%type <node> addop term mulop factor call args arg-list integer float

//%type <node> empty
/*
%right ASSIN
%left ADD SUB
%left MUL DIV
%left LPARENTHESE RPARENTHESE

%nonassoc ELSE
*/


%%
/* TODO: Your rules here. */

/* Example:
program: declaration-list {$$ = node( "program", 1, $1); gt->root = $$;}
       ;
*/

// 1.
program : declaration-list
{$$ = node("program", 1, $1); gt->root = $$;}
;

// 2.
declaration-list : declaration-list declaration
{$$ = node("declaration-list", 2, $1, $2);}
| declaration
{$$ = node("declaration-list", 1, $1);}
;

// 3.
declaration : var-declaration
{$$ = node("declaration", 1, $1);}
| fun-declaration
{$$ = node("declaration", 1, $1);}
;

// 4.
var-declaration : type-specifier IDENTIFIER SEMICOLON
{$$ = node("var-declaration", 3, $1, $2, $3);}
| type-specifier IDENTIFIER LBRACKET INTEGER RBRACKET SEMICOLON
{$$ = node("var-declaration", 6, $1, $2, $3, $4, $5, $6);}
;

// 5.
type-specifier : INT
{$$ = node("type-specifier", 1, $1);}
| FLOAT
{$$ = node("type-specifier", 1, $1);}
| VOID
{$$ = node("type-specifier", 1, $1);}
;

// 6.
fun-declaration : type-specifier IDENTIFIER LPARENTHESE params RPARENTHESE compound-stmt
{$$ = node("fun-declaration", 6, $1, $2, $3, $4, $5, $6);}
;

// 7.
params : param-list 
{$$ = node("params", 1, $1);}
| VOID
{$$ = node("params", 1, $1);}
;

// 8.
param-list : param-list COMMA param 
{$$ = node("param-list", 3, $1, $2, $3);}
| param
{$$ = node("param-list", 1, $1);}
;

// 9.
param : type-specifier IDENTIFIER
{$$ = node("param", 2, $1, $2);}
| type-specifier IDENTIFIER LBRACKET RBRACKET
{$$ = node("param", 4, $1, $2, $3, $4);}
;

// 10.
compound-stmt : LBRACE local-declarations statement-list RBRACE
{$$ = node("compound-stmt", 4, $1, $2, $3, $4);}
;

// 11.
local-declarations : local-declarations var-declaration
{$$ = node("local-declarations", 2, $1, $2);}
| 
{$$ = node("local-declarations", 0);}
;

// 12.
statement-list : statement-list statement 
{$$ = node("statement-list", 2, $1, $2);}
| 
{$$ = node("statement-list", 0);}
;

// 13.
statement : expression-stmt
{$$ = node("statement", 1, $1);}
| compound-stmt
{$$ = node("statement", 1, $1);}
| selection-stmt
{$$ = node("statement", 1, $1);}
| iteration-stmt
{$$ = node("statement", 1, $1);}
| return-stmt
{$$ = node("statement", 1, $1);}
;

// 14.
expression-stmt : expression SEMICOLON
{$$ = node("expression-stmt", 2, $1, $2);}
| SEMICOLON
{$$ = node("expression-stmt", 1, $1);}
;

// 15.
selection-stmt : IF LPARENTHESE expression RPARENTHESE statement
{$$ = node("selection-stmt", 5, $1, $2, $3, $4, $5);}
| IF LPARENTHESE expression RPARENTHESE statement ELSE statement
{$$ = node("selection-stmt", 7, $1, $2, $3, $4, $5, $6, $7);}
;

// 16.
iteration-stmt : WHILE LPARENTHESE expression RPARENTHESE statement
{$$ = node("iteration-stmt", 5, $1, $2, $3, $4, $5);}
;

// 17.
return-stmt : RETURN SEMICOLON
{$$ = node("return-stmt", 2, $1, $2);}
| RETURN expression SEMICOLON
{$$ = node("return-stmt", 3, $1, $2, $3);}
;

// 18.
expression : var ASSIN expression
{$$ = node("expression", 3, $1, $2, $3);}
| simple-expression
{$$ = node("expression", 1, $1);}
;

// 19.
var : IDENTIFIER
{$$ = node("var", 1, $1);}
| IDENTIFIER LBRACKET expression RBRACKET
{$$ = node("var", 4, $1, $2, $3, $4);}
;

// 20.
simple-expression : additive-expression relop additive-expression 
{$$ = node("simple-expression", 3, $1, $2, $3);}
| additive-expression
{$$ = node("simple-expression", 1, $1);}
;

// 21.
relop : LTE
{$$ = node("relop", 1, $1);}
| LT
{$$ = node("relop", 1, $1);}
| GT
{$$ = node("relop", 1, $1);}
| GTE
{$$ = node("relop", 1, $1);}
| EQ
{$$ = node("relop", 1, $1);}
| NEQ
{$$ = node("relop", 1, $1);}
;

// 22.
additive-expression :additive-expression addop term 
{$$ = node("additive-expression", 3, $1, $2, $3);}
| term
{$$ = node("additive-expression", 1, $1);}
;

// 23.
addop : ADD
{$$ = node("addop", 1, $1);}
| SUB
{$$ = node("addop", 1, $1);}
;

// 24.
term : term mulop factor
{$$ = node("term", 3, $1, $2, $3);}
| factor
{$$ = node("term", 1, $1);}
;

// 25.
mulop : MUL
{$$ = node("mulop", 1, $1);}
| DIV
{$$ = node("mulop", 1, $1);}
;

// 26.
factor : LPARENTHESE expression RPARENTHESE
{$$ = node("factor", 3, $1, $2, $3);}
| var
{$$ = node("factor", 1, $1);}
| call 
{$$ = node("factor", 1, $1);}
| integer
{$$ = node("factor", 1, $1);}
| float
{$$ = node("factor", 1, $1);}
;

// 27.
integer : INTEGER
{$$ = node("integer", 1, $1);}
;

// 28.
float : FLOATPOINT
{$$ = node("float", 1, $1);}
;

//29.
call : IDENTIFIER LPARENTHESE args RPARENTHESE
{$$ = node("call", 4, $1, $2, $3, $4);}
;

// 30.
args : arg-list
{$$ = node("args", 1, $1);}
| 
{$$ = node("args", 0);}
;

// 31.
arg-list : arg-list COMMA expression
{$$ = node("arg-list", 3, $1, $2, $3);}
| expression
{$$ = node("arg-list", 1, $1);}
;

%%

/// The error reporting function.
void yyerror(const char * s)
{
    // TO STUDENTS: This is just an example.
    // You can customize it as you like.
    fprintf(stderr, "error at line %d column %d: %s\n", lines, pos_start, s);
}

/// Parse input from file `input_path`, and prints the parsing results
/// to stdout.  If input_path is NULL, read from stdin.
///
/// This function initializes essential states before running yyparse().
syntax_tree *parse(const char *input_path)
{
    if (input_path != NULL) {
        if (!(yyin = fopen(input_path, "r"))) {
            fprintf(stderr, "[ERR] Open input file %s failed.\n", input_path);
            exit(1);
        }
    } else {
        yyin = stdin;
    }

    lines = pos_start = pos_end = 1;
    gt = new_syntax_tree();
    yyrestart(yyin);
    yyparse();
    return gt;
}

/// A helper function to quickly construct a tree node.
///
/// e.g. $$ = node("program", 1, $1);
syntax_tree_node *node(const char *name, int children_num, ...)
{
    //fprintf(stderr, "hahaha %s woc\n", name);
    syntax_tree_node *p = new_syntax_tree_node(name);
    syntax_tree_node *child;
    if (children_num == 0) {
        child = new_syntax_tree_node("epsilon");
        syntax_tree_add_child(p, child);
    } else {
        va_list ap;
        va_start(ap, children_num);
        for (int i = 0; i < children_num; ++i) {
            child = va_arg(ap, syntax_tree_node *);
            syntax_tree_add_child(p, child);
            //fprintf(stderr, "hahaha %s woc\n", child->name);
        }
        va_end(ap);
    }
    return p;
}
