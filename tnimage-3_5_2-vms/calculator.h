/*  calculator.h - header for parser with functions */
/*  Used in calculator.y                            */

#define NSYMS 1000 /* maximum number of symbols */

struct symtab
       { char *name;
         char *string;
         void *ptr;
         double (*funcptr1)(double a);
         double (*funcptr2)(double a, double b);
         double (*funcptr3)(double a, double b, double c);
         double (*funcptr4)(double a, double b, double c, double d);
         double (*funcptrchar)(char *s);
         void (*funcptrchar2)(char *s1, char *s2);
         double value;
         int datatype;
       }symtab[NSYMS];
struct symtab *symlook(char *s);    

