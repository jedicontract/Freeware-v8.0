/*
 * calculator.l
 * To compile: make calculator
 *  yacc -d calculator.y
 *  flex calculator.l
 *  cc -x c++ -o calculator y.tab.c lex.yy.c -ll -lm
 * NOTE: variables cannot exceed 7 letters
 */
%{
#include "y.tab.h"
#include <math.h>
#include <string.h>
extern int lineno;
extern char DECIMAL_POINT;
void count(void);
void comment(void);
int yyinput(void);

%}
%%
([0-9]+|([0-9]*(\.)[0-9]+)([eE][-+]?[0-9]+)?) {
     yylval.dval = atof(yytext);
     return NUMBER;
     }
[ \t\n{}]                     /* ignore whitespace     */
"/*"   { comment(); }
"//"   { comment(); }
">="   { count(); return GE; }
"<="   { count(); return LE; }
"=="   { count(); return EQ; }
"if"    { count(); return IF; }
"for"   { count(); return FOR; }
"while" { count(); return WHILE; }
"do"    { count(); return DO; }
"goto"  { count(); return GOTO; }
"break" { count(); return BREAK; }
"then"  { count(); return THEN; }
"case"  { count(); return CASE; }
"switch" { count(); return SWITCH; }
"return" { count(); return RETURN; }
"default"    { count(); return DEFAULT; }
"else"  { count(); return ELSE; }
"&&"    { count(); return LOGICAL_AND; }
"||"    { count(); return LOGICAL_OR; }
"+="    { count(); return PLUS_EQUALS; }
"-="    { count(); return MINUS_EQUALS; }
"*="    { count(); return TIMES_EQUALS; }
"/="    { count(); return DIVIDE_EQUALS; }
"++"    { count(); return INCREMENT; }
"--"    { count(); return DECREMENT; }
"open"  { count(); return TNI_OPEN; }
"close" { count(); return TNI_CLOSE; }
"read"  { count(); return TNI_READ; }
"print" { count(); return PRINT; }
"write" { count(); return TNI_WRITE; }
"area"  { count(); return AREA; }
"density" { count(); return DENSITY; }
"image" { count(); return IMAGE; }
"red"   { count(); return RED; }
"green" { count(); return GREEN; }
"blue"  { count(); return BLUE; }
"real"  { count(); return YYREAL; }
"imag"  { count(); return YYIMAG; }
"wave"  { count(); return YYWAVE; }
[A-Za-z][A-Za-z0-9]* {        /* return symbol pointer */
    struct symtab *sp = symlook(yytext);
    yylval.symp = sp;
    return NAME;
    }
\"[^"\n]*["\n] {              /* quoted string */
    struct symtab *sp = symlook(yytext);
    yylval.symp = sp;
    return YYSTRING; }
"$" {return 0; }              /* end of input          */
;  { lineno++; return yytext[0]; }/* end of line       */
.  return yytext[0];          /* anything else         */
%%


/* Gets rid of comments */
void comment(void)
{
        char c, c1;
loop:
        while ((c = yyinput()) != '*' && c != 0)
             ;/*   putchar(c); */

        if ((c1 = yyinput()) != '/' && c != 0)
        {
                unput(c1);
                goto loop;
        }

        if (c != 0)
            ;  /*  putchar(c1); */
}




int column = 0;

/* Figures out what column the error was at */
void count(void)
{
        int i;

        for (i = 0; yytext[i] != '\0'; i++)
                if (yytext[i] == '\n')
                        column = 0;
                else if (yytext[i] == '\t')
                        column += 8 - (column % 8);
                else
                        column++;

}


