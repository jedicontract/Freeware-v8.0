/*=================================================================================  
 * calculator.y
 * Latest revision: 03-12-2005                                              
 * Copyright (C) 2005 by Thomas J. Nelson                                  
 * See xmtnimage.h for Copyright Notice               
 * 
 * Each rule must start in column 1
 * To compile: 
 *     rm y*.o l*.o
 *     make calculator
 *     yacc -d calculator.y     Bison doesn't work - make sure autoconf doesn't use it. 
 *                              The -d is essential ( creates calculator.h ).
 *     flex calculator.l        Must be run manually after changing calculator.l
 *     cc -o calculator y.tab.c lex.yy.c -ll -lm
 *=================================================================================
   After running flex, it is necessary to edit lex.yy.c:
       remove static from    static char yy_hold_char;              (line 211)
       remove static from    static char yy_n_chars;                (line 213)
       remove static from    static int yyinput YY_PROTO(( void )); (line 500)
       remove static from    static int input YY_PROTO(( void ));   (line 502)
       remove static from    static int yyinput()                   (line 1337)
       remove static from    static int input()                     (line 1339)
       At the macro starting at about line 556, i.e. from:
  
            #ifndef YY_INPUT
            #define YY_INPUT(buf,result,max_size) \
            . . .
            #endif
   
      the entire macro must be replaced with:
  
            #ifndef YY_INPUT
            #define YY_INPUT(buf,result,max_size) \
                strncpy(buf,text,max_size); \
                result = strlen(buf);
            #endif
  
      Add these 4 lines at the top of the file after line 20:

            extern char *text;
            extern char *position;
            int n; 
            int input(void);
            struct symtab* symlook(char *s);

      Also, remove any exit()'s from line 1664 (approx.) in the function:
          static void yy_fatal_error( yyconst char msg[] )
            exit( YY_EXIT_FAILURE );
   
      Line 1199, before  "  return ret_val;  "
            
            position = yytext_ptr;

   If the following messages don't appear, the compilation didn't work:
          y.tab.c: In function `int yyparse()':
          y.tab.c:774: warning: suggest parentheses around assignment used as truth value
          y.tab.c:834: warning: label `yyerrlab' defined but not used
          y.tab.c:817: warning: label `yynewerror' defined but not used
          y.tab.cc: At top level:
          y.tab.c:3: warning: `char yysccsid[36]' defined but not used
   
  
 *=================================================================================
 * For false condition, characters are thrown away until desired stop point:
 *          while(yy_hold_char != ';') yyinput();
 *          yyinput();
 *     For true condition, set if_true to 1.
 *
 * yy_n_chars = no of chars in current evaluated string          
 * yyleng = no of chars in current token                         
 * yytext = current token                                        
 * Use input() for C, yyinput() for C++                          
 *   throws away next token & prepares for more input             
 * 
 * These 3 lines would throw away the rest of the current string 
 *   which is not what we want. (The yy_buffer_state typedef struct must also
 *   be included for the following line to work.)
 *          len = yy_n_chars - strlen(yy_current_buffer->yy_ch_buf) - 1;
 *          yy_c_buf_p += len;                                          
 *          yyinput();                                                    
 *
 * yy_hold_char is current character being input.
 *
 * The input string must end with \n\255 to parse correctly.  
 *================================================================================
 */

%{
#include "config.h"
#ifndef DIGITAL
 #include<stdio.h>
 #include<string.h>
 #include<stdlib.h> 
 #include<math.h>
#endif
#include "calculator.h"
#include "xmtnimage.h"

void addfunc(char *name, double(*func)(double));
void addfunc2(char *name, double(*func)(double,double));
void addfunc3(char *name, double(*func)(double,double,double));
void addfunc4(char *name, double(*func)(double,double,double,double));
void addfuncchar(char *name, double(*func)(char *));
void addfuncchar2(char *name, void(*func)(char*, char*));
double drand(double a); 
void skip_to_end(void);
void skip_to_end_of_line(void);
double dmax(double a, double b);
double dmin(double a, double b);
void print_number(FILE *stream, double number);
void print_string(FILE *stream, char *string);

int input(void);
int yyinput(void);
char *string(struct symtab *sym);
#ifndef MIPS
extern int yylex(void);
#endif
void yyerror(char *s);
void yyrestart ( FILE *input_file );

extern Globals     g;
extern Image      *z;
extern int         ci;
extern char yy_hold_char, yy_n_chars;
extern int ii,rr,gg,bb; 
extern char *text;
extern char *yytext;
extern int column;      /* where the error is */

extern char *yy_current_buffer;
extern int yy_init;     /* whether we need to initialize */
extern int yy_start;    /* start state number */
extern int status;
extern char *yy_c_buf_p;
int if_true=0, print_count, datatype;
int rr,gg,bb,v,x,y,xstart,ystart,xend,yend,frame,bpp,ino,len;
int lineno,k,comp,invert;
char *position=0;
int fff;
FILE *fp;
void *ptr;
static char filename[FILENAMELENGTH];
static char tempstring[128];
%}

%union {  double dval;
          struct symtab *symp;
       }

/*  tokens  */ 
%token <symp> NAME
%token <dval> NUMBER
%token <symp> YYSTRING
%token IDENTIFIER
%token EQ LE GE
%token PLUS_EQUALS MINUS_EQUALS TIMES_EQUALS DIVIDE_EQUALS
%token LOGICAL_AND LOGICAL_OR INCREMENT DECREMENT
%token IMAGE RED GREEN BLUE YYREAL YYIMAG YYWAVE AREA DENSITY
%token TNI_OPEN TNI_READ TNI_WRITE PRINT TNI_CLOSE
%token IF THEN ELSE
%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO TNI_CONTINUE BREAK RETURN

/* left-associative operatore - Also assigns precedence, last %left statement 
    has the highest precedence.
*/

%left LOGICAL_AND LOGICAL_OR
%left '&' '|' '^' 
%left '<' '>' EQ GE LE
%left '-' '+'
%left '*' '/' 
%left INCREMENT DECREMENT
%nonassoc UMINUS '!' '~'
%type <dval> expression

%%

statement_list: 
        statement 
	|   identifier_list
	|   comment
        |   statement_list statement
        |   '{' statement_list '}' statement
        ;
        
statement: 
	NAME '=' expression 
                {    $1->value = $3; 
                     read_variable((char*)"temp", tempstring, $1->datatype); 
                     if($1->datatype == STRING)
                          add_string_variable((char*)$1->name, (char*)tempstring, 0);
                } ';'
	|   NAME '=' YYSTRING 
                {    add_string_variable($1->name, string($3), 0); 
                } ';'
	|   iteration_statement
	|   jump_statement
	|   PRINT '(' print_list ')' ';'
	|   TNI_WRITE '(' YYSTRING    
                {    strncpy(filename, string($3), 128);  
                     ptr = read_ptr(filename);
                     fp = (FILE*)ptr;
                } ',' write_list ')' ';' 
        |   NAME PLUS_EQUALS expression { $1->value += $3; } ';'
        |   NAME MINUS_EQUALS expression { $1->value -= $3; }';'
        |   NAME TIMES_EQUALS expression { $1->value *= $3; }';'
        |   NAME DIVIDE_EQUALS expression
                        { if ($3 == 0.0)
                              yyerror((char*)"Division by zero");
                          else
                              $1->value /= $3;
                        }  ';'
        |   NAME INCREMENT { $1->value++; } ';'
        |   NAME DECREMENT { $1->value--; } ';'
        |   expression { add_variable((char*)"temp", $1); } ';'
        |   '\n'
        |   IF '(' expression ')'
                {
                   if( $3 == 1)  if_true=1;                               
                   else        {  if_true=0;
                                  skip_to_end();
                               }
                }
        |   ELSE
                {  
                   if(if_true) skip_to_end(); 
                } 
        ;

comment:
        '#'     { skip_to_end_of_line(); }
        ;

iteration_statement: 
        /* These are handled in xmtnimage19.cc */
	WHILE '(' statement ')' statement 
	| DO statement WHILE '(' expression ')' ';'
        | FOR '(' statement statement ')' statement
        | FOR '(' statement statement expression ')' statement
	;

jump_statement: 
	GOTO identifier ';'
	| TNI_CONTINUE ';'
	| BREAK ';'
	| RETURN ';'
	| RETURN expression ';'
	;

identifier_list: 
	identifier
	| identifier_list ',' identifier
	;


print_list: 
	print_list ',' expression { print_number(stdout, $3); }
	| print_list ',' YYSTRING { print_string(stdout, string($3)); }
	| expression { print_number(stdout, $1); }
	| YYSTRING { print_string(stdout, string($1)); }
	;

write_list: 
	write_list ',' expression { print_number(fp, $3); }
	| write_list ',' YYSTRING { print_string(fp, string($3)); }
	| expression { print_number(fp, $1);  }
	| YYSTRING {  print_string(fp, string($1)); }
	;

identifier: 
	IDENTIFIER
	;

expression: 
        expression '+' expression { $$ = $1 + $3; }
        |   expression '-' expression { $$ = $1 - $3; }
        |   expression '*' expression { $$ = $1 * $3; }
        |   expression '/' expression 
                        { if ($3 == 0.0)
                              yyerror((char*)"Division by zero");
                          else
                              $$ = $1 / $3; 
                        }
        |   '-' expression %prec UMINUS { $$ = -$2; }
        |   '!' expression %prec '!' { $$ = !$2; }
        |   '~' expression %prec '~' { $$ = (double)(~(int)$2); }
        |   '(' expression ')'          { $$ = $2; }
        |   expression '<' expression { $$ = $1 < $3; }
        |   expression '>' expression { $$ = $1 > $3; }
        |   expression LE expression { $$ = $1 <= $3; }
        |   expression GE expression { $$ = $1 >= $3; }
        |   expression EQ expression { $$ = $1 == $3; }
        |   expression LOGICAL_AND expression { $$ = $1 && $3; }
        |   expression LOGICAL_OR expression { $$ = $1 || $3; }
        |   expression '&' expression { $$ = (double)((int)$1 & (int)$3); }
        |   expression '|' expression { $$ = (double)((int)$1 | (int)$3); }
        |   expression '^' expression { $$ = (double)((int)$1 ^ (int)$3); }
        |   NUMBER                   
        |   NAME {  $$ = $1->value; }
        |   NAME '(' expression ')' 
                 {  
                    if($1->funcptr1)          /* func using a regular variable */
                        $$ = ($1->funcptr1)($3);
                    else
                    {   sprintf(tempstring, "%s is not a 1-argument function\n", $1->name);
                        yyerror(tempstring);
                        $$ = 0.0;
                    }
                 }
        |   NAME '(' expression ',' expression ')' 
                 {  
                    if($1->funcptr2)
                        $$ = ($1->funcptr2)($3,$5);
                    else
                    {   sprintf(tempstring, "%s is not a 2-argument function\n", $1->name);
                        yyerror(tempstring);
                        $$ = 0.0;
                    }
                 }
        |   NAME '(' expression ',' expression ',' expression ')' 
                 {  
                    if($1->funcptr3)
                        $$ = ($1->funcptr3)($3,$5,$7);
                    else
                    {   sprintf(tempstring, "%s is not a 3-argument function\n", $1->name);
                        yyerror(tempstring);
                        $$ = 0.0;
                    }
                 }
        |   NAME '(' expression ',' expression ',' expression ',' expression  ')' 
                 {  
                    if($1->funcptr4)
                        $$ = ($1->funcptr4)($3,$5,$7,$9);
                    else
                    {   sprintf(tempstring, "%s is not a 4-argument function\n", $1->name);
                        yyerror(tempstring);
                        $$ = 0.0;
                    }
                 }
        |   NAME '(' YYSTRING ')' 
                 {  
                    if($1->funcptrchar)
                    {   $$ = ($1->funcptrchar)(string($3));
                        status = GOTNEW;       /* Break out of pixel loop */
                    }else if($1->funcptrchar2) /* func using a string variable  */
                    {   ($1->funcptrchar2) (string($3), tempstring); 
                        add_string_variable((char*)"temp", tempstring, 0);
                        $1->datatype = STRING;
                        status = GOTNEW;       /* Break out of pixel loop */
                    }else
                    {   sprintf(tempstring, "%s: error executing function\n", $1->name);
                        yyerror(tempstring);
                        $$ = 0.0;
                    }
                 }
        |   DENSITY '[' expression ']' '['expression ']' '['expression ']' '['expression ']'
                {    ino    = (int) $3;
                     frame  = (int) $6;
                     x      = (int) $9;
                     y      = (int) $12;
                     comp   = (int)read_variable((char*)"COMPENSATE", NULL, datatype);
                     invert = (int)read_variable((char*)"INVERT", NULL, datatype);
                     if(between(ino, 0, g.image_count) &&
                        between(frame, 0, z[ino].frames-1) &&
                        between(x, 0, z[ino].xsize-1) &&
                        between(y, 0, z[ino].ysize-1))
                     { 
                        $$ = pixeldensity_image(x, y, ino, frame, comp, invert); 
                     }else yyerror((char*)"Array out of bounds");
                }
        |   IMAGE '[' expression ']' '['expression ']' '['expression ']' '['expression ']'
                {    ino   = (int) $3;
                     frame = (int) $6;
                     x     = (int) $9;
                     y     = (int) $12;
                     if(between(ino, 0, g.image_count) &&
                        between(frame, 0, z[ino].frames-1) &&
                        between(x, 0, z[ino].xsize-1) &&
                        between(y, 0, z[ino].ysize-1))
                     {  bpp   = z[ino].bpp;
                        $$ = (double)(pixelat(z[ino].image[frame][y] + x*g.off[bpp], bpp)); 
                     }else yyerror((char*)"Array out of bounds");
                }
        |   RED '[' expression ']' '['expression ']' '['expression ']' '['expression ']'
                {    ino   = (int) $3;
                     frame = (int) $6;
                     x     = (int) $9;
                     y     = (int) $12;
                     if(between(ino, 0, g.image_count) &&
                        between(frame, 0, z[ino].frames-1) &&
                        between(x, 0, z[ino].xsize-1) &&
                        between(y, 0, z[ino].ysize-1))
                     {  bpp   = z[ino].bpp;
                        v = pixelat(z[ino].image[frame][y] + x*g.off[bpp], bpp); 
                        valuetoRGB(v,rr,gg,bb,bpp);
                        $$ = (double)rr;
                     }else yyerror((char*)"Array out of bounds");
                }
        |   GREEN '[' expression ']' '['expression ']' '['expression ']' '['expression ']'
                {    ino   = (int) $3;
                     frame = (int) $6;
                     x     = (int) $9;
                     y     = (int) $12;
                     if(between(ino, 0, g.image_count) &&
                        between(frame, 0, z[ino].frames-1) &&
                        between(x, 0, z[ino].xsize-1) &&
                        between(y, 0, z[ino].ysize-1))
                     {  bpp   = z[ino].bpp;
                        v = pixelat(z[ino].image[frame][y] + x*g.off[bpp], bpp); 
                        valuetoRGB(v,rr,gg,bb,bpp);
                        $$ = (double)gg;
                     }else yyerror((char*)"Array out of bounds");
                }
        |   BLUE '[' expression ']' '['expression ']' '['expression ']' '['expression ']'
                {    ino   = (int) $3;
                     frame = (int) $6;
                     x     = (int) $9;
                     y     = (int) $12;
                     if(between(ino, 0, g.image_count) &&
                        between(frame, 0, z[ino].frames-1) &&
                        between(x, 0, z[ino].xsize-1) &&
                        between(y, 0, z[ino].ysize-1))
                     {  bpp   = z[ino].bpp;
                        v = pixelat(z[ino].image[frame][y] + x*g.off[bpp], bpp); 
                        valuetoRGB(v,rr,gg,bb,bpp);
                        $$ = (double)bb;
                     }else yyerror((char*)"Array out of bounds");
                }
        |   YYREAL '[' expression ']' '['expression ']' '['expression ']'
                {    ino   = (int) $3;
                     x     = (int) $6;
                     y     = (int) $9;
                     if(between(ino, 0, g.image_count-1) &&
                        between(x, 0, z[ino].xsize-1) &&
                        between(y, 0, z[ino].ysize-1) &&
                        z[ino].floatexists)
                          $$ = z[ino].fft[y][x].real();
                     else yyerror((char*)"Array out of bounds");
                }
        |   YYIMAG '[' expression ']' '['expression ']' '['expression ']'
                {    ino   = (int) $3;
                     x     = (int) $6;
                     y     = (int) $9;
                     if(between(ino, 0, g.image_count-1) &&
                        between(x, 0, z[ino].xsize-1) &&
                        between(y, 0, z[ino].ysize-1) &&
                        z[ino].floatexists)
                          $$ = z[ino].fft[y][x].imag();
                     else yyerror((char*)"Array out of bounds");
                }
        |   YYWAVE '[' expression ']' '['expression ']' '['expression ']'
                {    ino   = (int) $3;
                     x     = (int) $6;
                     y     = (int) $9;
                     if(between(ino, 0, g.image_count-1) &&
                        between(x, 0, z[ino].xsize-1) &&
                        between(y, 0, z[ino].ysize-1) &&
                        z[ino].waveletexists)
                          $$ = z[ino].wavelet[y][x];
                     else yyerror((char*)"Array out of bounds");
                }
        ;
%%


void initialize_evaluator(void)
{   
   /* 1-parameter functions - new functions can be added here */
   addfunc((char*)"abs",  fabs); 
   addfunc((char*)"acos", acos); 
   addfunc((char*)"asin", asin); 
   addfunc((char*)"atan", atan); 
   addfunc((char*)"cos",  cos); 
   addfunc((char*)"cosh", cosh); 
   addfunc((char*)"exp",  exp);
   addfunc((char*)"log",  log);
   addfunc((char*)"log10",log10);
   addfunc((char*)"rand", drand);
   addfunc((char*)"sin",  sin);
   addfunc((char*)"sinh", sinh);
   addfunc((char*)"sqrt", sqrt);
   addfunc((char*)"tan",  tan);
   addfunc((char*)"tanh", tanh);
   addfunc((char*)"erf",  erf);
   addfunc((char*)"spotsize", spot_size);
   addfunc((char*)"spotsignal", spot_signal);
   addfunc((char*)"spotx", spot_x); 
   addfunc((char*)"spoty", spot_y); 
   addfunc((char*)"pixels", pixels); 

#ifdef HAVE_ASINH
   addfunc((char*)"asinh",asinh); 
#endif
#ifdef HAVE_ACOSH
   addfunc((char*)"acosh",acosh); 
#endif
#ifdef HAVE_ATANH
   addfunc((char*)"atanh",atanh); 
#endif
#ifdef HAVE_CBRT
   addfunc((char*)"cbrt", cbrt); 
#endif

   /* 2-parameter functions - new functions can be added here */
   addfunc2((char*)"pow", pow); 
   addfunc2((char*)"min", dmin); 
   addfunc2((char*)"max", dmax); 

   /* 3-parameter functions - new functions can be added here */
   addfunc3((char*)"between", fbetween); 
   addfunc3((char*)"cdensity", cdensity); 

   /* 4-parameter functions - new functions can be added here */
   addfunc4((char*)"rdensity", rdensity); 

   /* functions with 1 string - new functions can be added here */
   addfuncchar((char*)"input", macro_double_input);

   addfuncchar2((char*)"getstring", macro_string_input);

   /* Functions that take a string as argument */
   /* To use this, change the YYSTRING entry from:
        |   YYSTRING  
                 {   len = strlen($1->name)-2;
                     strncpy(tempstring,$1->name+1,len);
                     tempstring[len]=0;
                     $$ = double_image_number(tempstring);
                 }
      to:
        |   NAME '(' YYSTRING ')' 
                 {
                    if($1-> char)
                    {   len = strlen($3->name)-2;
                        strncpy(tempstring,$3->name+1,len);
                        tempstring[len]=0;
                        $$ = ($1->funcptrchar)(tempstring);
                    }else
                    {   sprintf(tempstring, "Argument to %s is not a string\n", $1->name);
                        yyerror(tempstring);
                        $$ = 0.0;
                    }                    
     Can't have both because it causes a shift/reduce conflict.
   */
   /* addfuncchar((char*)"ino", double_image_number); */

   lineno = 0;
   yy_n_chars = 0;
}

void yyerror(char *s)
{  
   formula_error(s, text); 
   yyrestart(NULL);
}

void add_variable(char *name, double value)
{
   struct symtab *sp = symlook(name);
   sp->ptr = NULL;
   sp->funcptr1 = NULL;
   sp->funcptr2 = NULL;
   sp->funcptr3 = NULL;
   sp->funcptr4 = NULL;
   sp->funcptrchar = NULL;
   sp->funcptrchar2 = NULL;
   sp->value = value;
   sp->datatype = FLOAT;
}

/* Include a value so a number can be assoc. with string */
void add_string_variable(char *name, char *string, double value)
{
   if(string==NULL) return;
   struct symtab *sp = symlook(name);
   sp->ptr = NULL;
   sp->funcptr1 = NULL;
   sp->funcptr2 = NULL;
   sp->funcptr3 = NULL;
   sp->funcptr4 = NULL;
   sp->funcptrchar = NULL;
   sp->funcptrchar2 = NULL;
   sp->value = value;
   if(sp->string!=NULL) free(sp->string);
   sp->string = strdup(string);
   sp->datatype = STRING;
}

void add_pointer_variable(char *name, void *ptr)
{
   if(ptr==NULL) return;
   struct symtab *sp = symlook(name);
   sp->ptr = ptr;
   sp->funcptr1 = NULL;
   sp->funcptr2 = NULL;
   sp->funcptr3 = NULL;
   sp->funcptr4 = NULL;
   sp->funcptrchar = NULL;
   sp->funcptrchar2 = NULL;
   sp->value = 0;
   sp->datatype = POINTER;
}


void addfunc(char *name, double(*func)(double))
{
   struct symtab *sp = symlook(name);
   sp->ptr = NULL;
   sp->funcptr1 = func;
   sp->funcptr2 = NULL;
   sp->funcptr3 = NULL;
   sp->funcptr4 = NULL;
   sp->funcptrchar = NULL;
   sp->funcptrchar2 = NULL;
}

void addfunc2(char *name, double(*func)(double, double))
{
   struct symtab *sp = symlook(name);
   sp->ptr = NULL;
   sp->funcptr1 = NULL;
   sp->funcptr2 = func;
   sp->funcptr3 = NULL;
   sp->funcptr4 = NULL;
   sp->funcptrchar = NULL;
   sp->funcptrchar2 = NULL;
}

void addfunc3(char *name, double(*func)(double, double, double))
{
   struct symtab *sp = symlook(name);
   sp->ptr = NULL;
   sp->funcptr1 = NULL;
   sp->funcptr2 = NULL;
   sp->funcptr3 = func;
   sp->funcptr4 = NULL;
   sp->funcptrchar = NULL;
   sp->funcptrchar2 = NULL;
}

void addfunc4(char *name, double(*func)(double, double, double, double))
{
   struct symtab *sp = symlook(name);
   sp->ptr = NULL;
   sp->funcptr1 = NULL;
   sp->funcptr2 = NULL;
   sp->funcptr3 = NULL;
   sp->funcptr4 = func;
   sp->funcptrchar = NULL;
   sp->funcptrchar2 = NULL;
}

void addfuncchar(char *name, double(*func)(char *))
{
   struct symtab *sp = symlook(name);
   sp->ptr = NULL;
   sp->funcptr1 = NULL;
   sp->funcptr2 = NULL;
   sp->funcptr3 = NULL;
   sp->funcptr4 = NULL;
   sp->funcptrchar = func;
   sp->funcptrchar2 = NULL;
}

void addfuncchar2(char *name, void(*func)(char*, char*))
{
   struct symtab *sp = symlook(name);
   sp->ptr = NULL;
   sp->funcptr1 = NULL;
   sp->funcptr2 = NULL;
   sp->funcptr3 = NULL;
   sp->funcptr4 = NULL;
   sp->funcptrchar = NULL;
   sp->funcptrchar2 = func;
}



double read_variable(char *name, char *answer, int &datatype)
{
   struct symtab *sp = symlook(name);
   if(sp->datatype==STRING && answer != NULL)
   {   if(sp->string) strcpy(answer, sp->string); else answer[0]=0;
   }else
   datatype = sp->datatype;
   return sp->value;
}


void *read_ptr(char *name)
{
   struct symtab *sp = symlook(name);
   return sp->ptr;
}


struct symtab* symlook(char *s)
{
   struct symtab *sp;
   for(sp=symtab; sp< &symtab[NSYMS]; sp++)
   {  
       if(sp->name && !strcmp(sp->name, s)) return sp;  /* is it already here */
       if(!sp->name)                                    /* is it free         */
       {   sp->name = strdup(s); 
           sp->string = NULL;
           sp->datatype = NONE;
           return sp; 
       }
   }
   /* Make sure they don't know what to do */
   yyerror((char*)"Too many symbols, ask a wizard to enlarge me");
   return NULL;
}

void skip_to_end(void)
{
   while(strchr("\n ",yy_hold_char)!=NULL) yyinput(); 
   int got_left_brace = yy_hold_char=='{';
   if(got_left_brace)
   {   while(yy_hold_char != '}')
       {    yyinput();
            if(yy_hold_char==0) break;
       }
   }else
   {   while(strchr(";}\n",yy_hold_char)==NULL) yyinput(); 
   }
   yyinput();
   while(yy_hold_char=='\n') yyinput(); 
}

void skip_to_end_of_line(void)
{
   while(yy_hold_char != '\n' && yy_hold_char) yyinput(); 
   yyinput();
}

double dmax(double a, double b){ if(a>b) return a; return b; }
double dmin(double a, double b){ if(a<b) return a; return b; }
double drand(double a){ a=a; return rand()/(double)RAND_MAX; }

char *string(struct symtab *sym)
{
   static char sss[FILENAMELENGTH];
   len = strlen(sym->name)-2;
   strncpy(sss, sym->name+1, len);
   sss[len]=0;
   return sss;
}

void print_number(FILE *stream, double number) 
{
   fprintf(stream, "%g",number); 
   fflush(stream);
}

void print_string(FILE *stream, char *string)
{
   int j,k;
   char *s;
   int len = strlen(string);
   j=0;
   s = (char*) malloc(16384);
   for(k=0; k<len; k++) 
   {   if(k<len-1 && string[k]=='\\') switch(string[++k])
       {   case '0': s[j++] = '\0'; break;
           case 'a': s[j++] = '\a'; break;
           case 'b': s[j++] = '\b'; break;
           case 't': s[j++] = '\t'; break;
           case 'n': s[j++] = '\n'; break;
           case 'v': s[j++] = '\v'; break;
           case 'f': s[j++] = '\f'; break;
           case 'r': s[j++] = '\r'; break;
           case '\\': s[j++] = '\\'; break;
       }else s[j++] = string[k];
   }
   s[j] = 0;
   fprintf(stream, "%s",s);
   fflush(stream);
   free(s);
}
