/*
	MBU 	- mailbox utility

	MBU  can create/delete/read/write mailbox devices.
	MBU  can also show and change characteristics of mailbox devices.

 COPYRIGHT NOTICE

 This software is COPYRIGHT (c) 2004, Ian Miller. ALL RIGHTS RESERVED.
 Permission is granted for not-for-profit redistribution, provided all source
 and object code remain unchanged from the original distribution, and that all
 copyright notices remain intact.

 DISCLAIMER

 This software is provided "AS IS". The author makes no representations or
 warranties with respect to the software and specifically disclaim any implied
 warranties of merchantability or fitness for any particular purpose.

*/
#pragma module MBU  "V01-004"

/* define the version to be displayed */
#define VERSION "V1.6"

#include <stdio.h>
#include <stdlib.h>
#include <descrip.h>
#include <climsgdef.h>
#include <rmsdef.h>
#include <smgmsg.h>
#include <ssdef.h>
#include <varargs.h>
#include <stsdef.h>
#include <hlpdef.h>

#define MAIN_MODULE 1

#include "mbu.h"


static void initialise(void);
static unsigned long execute(struct dsc$descriptor_s *command_dsc);

int
main(int argc, char *argv[])
{
	extern unsigned long LIB$GET_FOREIGN();
	unsigned long ccode;
	unsigned short len;
	static $DESCRIPTOR_D(line_dsc);
	static $DESCRIPTOR(prompt_dsc,"MBU> ");

	/* initialise global data	*/
	initialise();

	/* get command line		*/
   	if (argc > 1)
	{
		ccode=LIB$GET_FOREIGN(&line_dsc,&prompt_dsc,&len,0);
	}
   	else
	{
		ccode = get_input(&line_dsc,&prompt_dsc,&len);
	}
	while (1)
	{
		if ((ccode & 1) != 0 && len != 0)
		{
			/* parse and execute command	*/
			if (line_dsc.dsc$a_pointer[0] == '@')
				ccode = at_cmd(&line_dsc);
			else if (line_dsc.dsc$a_pointer[0] == '!')
				/* comment - ignore line*/
				ccode = MBU__NORMAL;
			else
				ccode = execute(&line_dsc);
			/* free command line		*/
			LIB$SFREE1_DD(&line_dsc);
		}
		else if (ccode == RMS$_EOF || ccode == SMG$_EOF)
		{
			/* end of file			*/
			ccode = SS$_NORMAL;
			if (level == 0)
			{
				/* EOF at top level	*/
				exit_cmd(FALSE);
				break;	/* exit		*/
			}
			else
				exit_cmd(FALSE);
		}
		if ((ccode & 1) == 0)
		{
			/* output error message		*/
			putmsg(ccode);
			/* unwind to top level		*/
			while (level != 0)
				exit_cmd(FALSE);
		}
		/* if invoked with command then exit	*/
		if (argc > 1 && level == 0)
			break;
		/* get next command line		*/
		ccode = get_input(&line_dsc,&prompt_dsc,&len);
	}
	
	exit_cmd(FALSE);
	return (ccode | STS$M_INHIB_MSG);
}


/*
	execute		- parse and execute command 
*/
static unsigned long
execute(struct dsc$descriptor_s *command_dsc)
{
	extern void VAXC$ESTABLISH();
	extern unsigned long LIB$SIG_TO_RET();
	unsigned long ccode;
	globalref mbucld;

	/* catch any exceptions		*/
	VAXC$ESTABLISH(LIB$SIG_TO_RET);

	/* parse command		*/
	ccode = CLI$DCL_PARSE(command_dsc,&mbucld,get_input,get_input);
	/* if ok, perform command	*/
	if ((ccode & 1) != 0)
		ccode = CLI$DISPATCH(TRUE);

	return(ccode);
}


/*
	initialise	- initialise data
*/
static void
initialise(void)
{
	int i;
   	unsigned long ccode;
	extern unsigned long smg$create_virtual_keyboard();

	/* initialse command defaults	*/
	ini_defaults();

   	/* initialise input file stack	*/
	level = 0;
	for (i=0; i < MAX_LEVEL; i++)
		input[0] = NULL;

   	/* create virtual keyboard	*/
	ccode = smg$create_virtual_keyboard(&keyboard_id,0,0,0,&RECALL_COUNT);
	if ((ccode & 1) == 0)
		exit(ccode);
}


/*
	put_output	- write message to output file
*/
static unsigned long
put_output(struct dsc$descriptor_s *msg_dsc, unsigned long severity)
{
	unsigned long result;

	if (redir)
	{
		/* output message to output file		*/
		fwrite(msg_dsc->dsc$a_pointer,msg_dsc->dsc$w_length,
			1,stdout);

		/* if not error then don't output to SYS$ERROR	*/
		if (severity == STS$K_SUCCESS || severity == STS$K_INFO)
			result = 0;	/* don't output to SYS$ERROR	*/
		else
			result = 1;	/* do output to SYS$ERROR	*/
	}
	else
		result = 1;	/* do output to SYS$ERROR	*/

	return(result);
}


/*
	putmsg		- output message
*/
void
putmsg(ccode,va_alist)
unsigned long ccode;
va_dcl
{
	unsigned long args[20], *argp;
	va_list ap;
	int nargs;
	unsigned short fac;
	extern unsigned long SYS$PUTMSG();

	fac = $VMS_STATUS_FAC_NO(ccode);
	va_start(ap);
	va_count(nargs);
	args[0] = nargs--;
	args[1] = ccode;

	if (fac == SYSTEM$_FACILITY || fac == RMS$_FACILITY)
		argp = &args[2];
	else
	{
		args[2] = nargs;
		argp = &args[3];
	}

	while (nargs-- > 0)
		*argp++ = va_arg(ap,unsigned long);

	va_end(ap);

	SYS$PUTMSG(args,put_output,0,$VMS_STATUS_SEVERITY(ccode));
}


/*
	help_cmd	- help command
*/
unsigned long
help_cmd(void)
{
	extern unsigned long LIB$PUT_OUTPUT(), LIB$GET_INPUT();
	extern unsigned long STR$CONCAT(), LBR$OUTPUT_HELP();
	static $DESCRIPTOR(prefix_dsc,"MBU  ");
	static $DESCRIPTOR(topic_dsc,"TOPIC");
	static $DESCRIPTOR(helplib_dsc,"HELPLIB");
	$DESCRIPTOR_D(param_dsc);
	$DESCRIPTOR_D(line_dsc);
	unsigned long ccode, flags;

	flags = HLP$M_PROMPT|HLP$M_PROCESS|HLP$M_GROUP|HLP$M_SYSTEM|HLP$M_HELP;
	if (level == 0)
	{	
		ccode = CLI$GET_VALUE(&topic_dsc,&param_dsc);
		ccode= STR$CONCAT(&line_dsc,&prefix_dsc,&param_dsc);
		ccode = LBR$OUTPUT_HELP(LIB$PUT_OUTPUT,0,&line_dsc,&helplib_dsc,
			&flags,LIB$GET_INPUT);
		LIB$SFREE1_DD(&line_dsc);
		LIB$SFREE1_DD(&param_dsc);
		ccode = MBU__NORMAL;
	}
	else
		ccode = MBU__INVCOM;

	return(ccode);
}
#ifdef DESC


/*
	use_cmd		- use command
*/
unsigned long
use_cmd(void)
{
	return(MBU__WISHLIST);
}
#endif


/*
	show_version_cmd	- show version command
*/
unsigned long
show_version_cmd(void)
{
	static $DESCRIPTOR(version_dsc,VERSION);
	static $DESCRIPTOR(bdate_dsc,__DATE__);
	static $DESCRIPTOR(vmsver_dsc,__VMS_VERSION);
#if __ALPHA
	static $DESCRIPTOR(arch_dsc,"Alpha");
#elif __ia64
	static $DESCRIPTOR(arch_dsc,"I64");
#elif __VAX
	static $DESCRIPTOR(arch_dsc,"VAX");
#endif

	putmsg(MBU__VERSION,&version_dsc,&bdate_dsc,&arch_dsc,&vmsver_dsc);

	return (MBU__NORMAL);
}
