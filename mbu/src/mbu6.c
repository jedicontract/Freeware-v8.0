/*
	MBU6	- indirect file commands
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
#pragma module MBU6 "V01-003"

#include <rmsdef.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "mbu.h"

/*
	at_cmd		- execute @ command
*/
unsigned long
at_cmd(struct dsc$descriptor_s *command_dsc)
{
	unsigned long ccode;
	char filename[256];

	if (level+1 < MAX_LEVEL)
	{
		strncpy(filename,command_dsc->dsc$a_pointer+1,
			command_dsc->dsc$w_length-1);
		filename[command_dsc->dsc$w_length-1] = '\0';
		if (strchr(filename,'.') == NULL)
			strcat(filename,".COM");
	
		if ((input[level+1] = fopen(filename,"r")) != NULL)
		{
			level++;
			ccode = MBU__NORMAL;
		}
		else
			ccode = vaxc$errno;
	}
	else
		ccode = MBU__TOODEEP;
	return(ccode);
}

/*
	wait_cmd	_ execute wait command
*/
unsigned long
wait_cmd(void)
{
	extern unsigned long SYS$BINTIM(), SYS$SETIMR(), SYS$WAITFR();
	static $DESCRIPTOR(time_dsc,"TIME");
	unsigned long ccode;
	unsigned short time[4];
	$DESCRIPTOR_D(timbuf_dsc);

	if (level > 0)
	{
		CLI$GET_VALUE(&time_dsc,&timbuf_dsc,0);
		ccode = SYS$BINTIM(&timbuf_dsc,time);
		if ((ccode & 1) != 0)
		{
			ccode = SYS$SETIMR(1,time,0,0,0);
			if ((ccode & 1) != 0)
				SYS$WAITFR(1);
		}
	}
	else
		ccode = MBU__INVCMD;
		
	return(ccode);
}

/*
	rewind_cmd	_ execute rewind command
*/
unsigned long
rewind_cmd(void)
{
	unsigned long ccode;

	if (level > 0)
	{
		if (input[level] != NULL)
		{
			if (rewind(input[level]) == 0)
				ccode = MBU__NORMAL;
			else
				ccode = vaxc$errno;
		}
		else
			ccode = MBU__BADFP;
	}
	else
		ccode = MBU__INVCMD;
		
	return(ccode);
}

/*
	exit_cmd	- exit command
*/
unsigned long 
exit_cmd(unsigned long flag)
{
	/* if at top level then exit program		*/
	if (level == 0)
	{
		if (flag == TRUE)
			exit(1);
	}
	else
	{
		if (input[level] != NULL)
			fclose(input[level]);
		level--;
	}
	return(MBU__NORMAL);
}

/*
	get_input	- read a line from current command input
*/
unsigned long
get_input(result,prompt,length)
struct dsc$descriptor_s *result;
struct dsc$descriptor_s *prompt;
unsigned short *length;
{
	unsigned long ccode, len;
	char buf[1025];
	extern unsigned long SMG$READ_COMPOSED_LINE(), STR$COPY_R();

	if (level == 0)
   	{
		ccode = SMG$READ_COMPOSED_LINE(&keyboard_id,0,result,prompt,
				length);
	}
	else
	{
		buf[0] = '\0';
		if (fgets(buf,sizeof(buf) - 1,input[level]) != NULL)
		{
			len = strlen(buf) - 1; 	/* length without NL 	*/
			buf[len] = '\0';	/* remove NL 		*/
			STR$COPY_R(result,&len,buf);
			if (length != NULL)
				*length = len;
			if (verify_flag)
				puts(buf);
			ccode = MBU__NORMAL;
		}
		else
			ccode = RMS$_EOF;
	}
	return(ccode);
}

/*
	set_output_cmd	- redirect output
*/
unsigned long 
set_output_cmd(void)
{
	static $DESCRIPTOR(file_dsc,"FILE");
	$DESCRIPTOR_D(value_dsc);
	unsigned short new_redir;
	unsigned long ccode;
	char filename[256];

	/* get filename		*/
	ccode = CLI$GET_VALUE(&file_dsc,&value_dsc);
	if ((ccode & 1) != 0)
	{
		/* filename supplied - redirect output to it	*/
		strncpy(filename,value_dsc.dsc$a_pointer,
			value_dsc.dsc$w_length);
		filename[value_dsc.dsc$w_length] = '\0';
		/* default extention is .LOG			*/
		if (strchr(filename,'.') == NULL)
			strcat(filename,".LOG");
		new_redir = TRUE;
	}
	else
	{
		/* no filename - output back to SYS$OUTPUT	*/
		strcpy(filename,"SYS$OUTPUT");
		new_redir = FALSE;
	}

	/* open new output file	*/
	if (freopen(filename,"w",stdout) != NULL)
	{
		ccode = MBU__NORMAL;
		redir = new_redir;	/* update output redirected flag */
	}
	else
		ccode = vaxc$errno;

	return(ccode);
}
