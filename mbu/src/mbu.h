/* MBU utility global definitions COPYRIGHT NOTICE

 This software is COPYRIGHT © 2004, Ian Miller. ALL RIGHTS RESERVED.
 Permission is granted for not-for-profit redistribution, provided all source
 and object code remain unchanged from the original distribution, and that all
 copyright notices remain intact.

 DISCLAIMER

 This software is provided "AS IS". The author makes no representations or
 warranties with respect to the software and specifically disclaim any implied
 warranties of merchantability or fitness for any particular purpose.

*/
#include <stdio.h>
#include <descrip.h>

/*
	definitions
*/

/* event flag numbers - each should be unique and not 0 */
#define GET_EFN	5	/* sensemode qio 	*/
#define WRITE_EFN 4	/* write qio		*/
#define TIMER_EFN 3	/* timer 		*/
#define READ_EFN 2	/* read qio		*/
#define SET_EFN	1	/* set qio		*/

/* mailbox open modes	*/
#define OPEN_READONLY	1
#define OPEN_WRITEONLY	2

/* command file max nesting level including terminal	*/
#define MAX_LEVEL 5

/* display width limits		*/
#define MIN_WIDTH 20
#define MAX_WIDTH 200

/* command line recall count	*/
#define RECALL_COUNT 20

#define $DESCRIPTOR_D(name) \
struct dsc$descriptor_s name = { 0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0 }

/*
	typedefs	
*/
typedef enum {HEX=0,DECIMAL=1,TEXT=2} FORMAT;
typedef enum {BYTE,WORD,LONG} DATA_SIZE;
typedef struct {
	unsigned short buflen;
	unsigned short item;
	void *bufadr;
	unsigned short *retlenadr;
} ITEM;
/* record of interesting information from an IRP	*/
typedef struct
{
	unsigned long pid;	/* internal pid of requestor 	*/
	unsigned long func;	/* I/O function code		*/
	unsigned long bcnt;	/* byte count			*/
	unsigned char efn;	/* event flag number		*/
	unsigned char mode;	/* mode KESU			*/
	unsigned char pad[2];	/* padding to longword boundry	*/
	unsigned long ast;	/* AST address			*/
	unsigned long astprm;	/* AST parameter		*/
} IRPI;

typedef struct
{
	unsigned long pid;	/* internal pid of requestor 	*/
	unsigned long ast;	/* AST address			*/
	unsigned long astprm;	/* AST parameter		*/
	unsigned char rmode;	/* mode KESU 			*/
	unsigned char pad;	
	unsigned short chan;	/* channel			*/
} ASTI;

/*
	global data
*/
#ifndef MAIN_MODULE
#define GLOBAL extern
#else
#define GLOBAL /**/
#endif
/* defaults		*/
GLOBAL FORMAT def_format;
GLOBAL DATA_SIZE def_data_size;
#ifdef DESC
GLOBAL char *def_description;
#endif
GLOBAL unsigned long def_maxmsg;
GLOBAL unsigned long def_bufquo;
GLOBAL unsigned long def_length;
GLOBAL unsigned long def_count;
GLOBAL unsigned long def_width;
GLOBAL unsigned long def_promsk;
GLOBAL char *def_output;
GLOBAL char *def_input;
/* indirect command files	*/
GLOBAL FILE *input[MAX_LEVEL];
GLOBAL unsigned short level;
/* output redirected flag	*/
GLOBAL unsigned short redir;
/* virtual keyboard id		*/
GLOBAL unsigned long keyboard_id;
/* verify flag - display lines of indirect command files */
GLOBAL int verify_flag;

/*
	global functions
*/
extern void ini_defaults(void);
extern void putmsg();
extern unsigned long exit_cmd(unsigned long flag);
extern unsigned long get_input(struct dsc$descriptor_s *result,
		 struct dsc$descriptor_s *prompt,
		 unsigned short *length);
extern FORMAT get_format(void);
extern DATA_SIZE get_data_size(void);
extern unsigned long get_width(void);
extern unsigned long OpenMbx(struct dsc$descriptor_s *name,unsigned short *chan,
	unsigned long *maxmsg, int openflag);
extern char *display_prot(unsigned long promsk);
extern unsigned long at_cmd(struct dsc$descriptor_s *);
extern unsigned long parse_prot(void *str,unsigned long len,
	unsigned long curprot, unsigned long *newprotaddr);
unsigned long GetTimeout(void);
unsigned long StartTimer(unsigned short chan, unsigned int len);
unsigned long CancelTimer(unsigned short chan);

#if __VAX
#define CLI$DCL_PARSE cli$dcl_parse
#define CLI$DISPATCH cli$dispatch
#define CLI$GET_VALUE cli$get_value
#define CLI$PRESENT cli$present
#define LIB$SFREE1_DD lib$sfree1_dd
#define SYS$CANTIM sys$cantim
#define SYS$SETIMR sys$setimr
#define SYS$QIOW sys$qiow
#define SYS$ASSIGN sys$assign
#define SYS$DASSGN sys$dassgn
#define LIB$CVT_TO_INTERNAL_TIME lib$cvt_to_internal_time
#define SYS$CANCEL sys$cancel
#define LIB$GETDVI lib$getdvi
#endif
extern unsigned long CLI$DCL_PARSE(), CLI$DISPATCH(), CLI$GET_VALUE(), 
	CLI$PRESENT();
extern unsigned int LIB$SFREE1_DD();

/* routines in mbu8.mar */
extern unsigned long getmbx(unsigned short chan, unsigned long *iniquo, 
	unsigned long *bufquo,struct dsc$descriptor_s *lnm,
	unsigned short *lnmlen,struct dsc$descriptor_s *lnt,
	unsigned short *lntlen, unsigned long *rrefc, unsigned long *wrefc,
	IRPI *rdrq, unsigned long *rdrqlen,
	IRPI *wrwq, unsigned long *wrwqlen,
	IRPI *rdwq, unsigned long *rdwqlen,
	ASTI *rstq, unsigned long *raqlen,
	ASTI *wstq, unsigned long *waqlen,
	ASTI *nstq, unsigned long *naqlen);
extern unsigned long setmbx(unsigned short chan, unsigned long new_iniquo,
	unsigned short new_max_msg_size);
extern unsigned long getmsg(unsigned short chan, unsigned char *buf,
	unsigned long bufsiz, unsigned long *msgcnt);
extern unsigned long cvtpid(unsigned long pid);
extern unsigned long lockcode(void);

/* other routines	*/
unsigned long getmsgcnt(unsigned short chan, unsigned long *msgcnt, 
	unsigned long *bytcnt);
void get_process_name(unsigned long pid, char *name, int namelen);
void display_io_func(unsigned long func, char *buf);

/*
	condition codes
*/
globalvalue MBU__NORMAL;
globalvalue MBU__VERSION;
globalvalue MBU__DEFAULTS;
globalvalue MBU__MBUCHAR;
globalvalue MBU__NOMSG;
globalvalue MBU__WARNING;
globalvalue MBU__WISHLIST;
globalvalue MBU__INVVAL;
globalvalue MBU__ERROR;
globalvalue MBU__INVCMD;
globalvalue MBU__TOODEEP;
globalvalue MBU__INVCOM;
globalvalue MBU__BADFORMAT;
globalvalue MBU__BADSIZE;
globalvalue MBU__BADFP;
globalvalue MBU__MAXMSG;
globalvalue MBU__INVINP;
globalvalue MBU__SYNTAX;
globalvalue MBU__FATAL;
globalvalue MBU__BUG;
