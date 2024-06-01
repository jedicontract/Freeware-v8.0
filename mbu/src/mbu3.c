/*
	MBU3	- write command
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
#pragma module MBU3 "V01-003"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <climsgdef.h>
#include <iodef.h>
#include <rmsdef.h>
#include <errno.h>
#include <dvidef.h>
#include <starlet.h>
#include <libdtdef.h>
#include <lib$routines.h>
#include <ssdef.h>
#ifdef SS$_NOREADER
#include <agndef.h>
#endif

#include "mbu.h"

/* local data		*/
static FORMAT format;
static DATA_SIZE data_size;
static unsigned long maxmsg;

/* local functions	*/
static unsigned long ReadMsg(char *input,char *buf,unsigned long *len);
static unsigned long SendMsg(unsigned short chan, char *buf, 
	unsigned long len, int wait, unsigned int timeout,int readercheck);
static unsigned long FormatLine(char *buf,unsigned long *len, char *line, 
	int linelen);

/*
	write_cmd	- execute write command
*/
unsigned long
write_cmd(void)
{
   	static $DESCRIPTOR(eof_dsc,"EOF");
	static $DESCRIPTOR(wait_dsc,"WAIT");
	static $DESCRIPTOR(name_dsc,"NAME");
	static $DESCRIPTOR(input_dsc,"INPUT");
	static $DESCRIPTOR(readercheck_dsc,"READERCHECK");
	unsigned long ccode, len;
	unsigned short chan;
	int wait, eof, readercheck;
	$DESCRIPTOR_D(value_dsc);
	$DESCRIPTOR_D(mbu_dsc);
	char input[256], *buf;
	unsigned int timeout;

	/* get command values		*/
	CLI$GET_VALUE(&name_dsc,&mbu_dsc);
	data_size = get_data_size();
	format = get_format();
	input[0] = '\0';
	ccode = CLI$PRESENT(&input_dsc);
	if ((ccode & 1) != 0)
	{
		if (def_input != NULL)
			strcpy(input,def_input);
		LIB$SFREE1_DD(&value_dsc);
		ccode = CLI$GET_VALUE(&input_dsc,&value_dsc);
		if ((ccode & 1) != 0)
		{
			strncpy(input, value_dsc.dsc$a_pointer,
				value_dsc.dsc$w_length);
			input[value_dsc.dsc$w_length] = '\0';
		}
	}

	ccode = CLI$PRESENT(&wait_dsc);
	if (ccode == CLI$_LOCPRES
	||  ccode == CLI$_PRESENT
	||  ccode == CLI$_DEFAULTED)
	{
		wait = TRUE;
	}
	else
		wait = FALSE;

	ccode = CLI$PRESENT(&readercheck_dsc);
	if (ccode == CLI$_LOCPRES
	||  ccode == CLI$_PRESENT
	||  ccode == CLI$_DEFAULTED)
	{
		readercheck = TRUE;
	}
	else
		readercheck = FALSE;

	ccode = CLI$PRESENT(&eof_dsc);
	if (ccode == CLI$_LOCPRES
	||  ccode == CLI$_PRESENT
	||  ccode == CLI$_DEFAULTED)
	{
		eof = TRUE;
	}
	else
		eof = FALSE;

	timeout = GetTimeout();

	/*  open mailbox		*/
	ccode = OpenMbx(&mbu_dsc,&chan,&maxmsg,OPEN_WRITEONLY);
	if ((ccode & 1) != 0)
	{
		if (eof)
			ccode = SendMsg(chan,NULL,0,wait,timeout,readercheck);
		else
		{
			buf = malloc(maxmsg);
			/* read message contents	*/
			ccode = ReadMsg(input,buf,&len);
			/* if ok then send message	*/
			if ((ccode & 1) != 0)
			{
				ccode = SendMsg(chan,buf,len,wait,timeout,
					readercheck);
			}
			free(buf);
		}
	}
	/* close mailbox		*/
	SYS$DASSGN(chan);

	/* free dynamic strings		*/
	LIB$SFREE1_DD(&value_dsc);
	LIB$SFREE1_DD(&mbu_dsc);

	if (ccode == MBU__MAXMSG)
	{
		putmsg(MBU__MAXMSG,maxmsg);
		ccode = MBU__NORMAL;
	}

	return(ccode);
}

unsigned long
GetTimeout(void)
{
   	static $DESCRIPTOR(timeout_dsc,"TIMEOUT");
	unsigned long ccode;
	$DESCRIPTOR_D(value_dsc);
	char tbuf[256], *p;
	unsigned long timeout;

	timeout = 0;

	if (CLI$PRESENT(&timeout_dsc) & 1)
	{
		ccode = CLI$GET_VALUE(&timeout_dsc,&value_dsc);
		if ((ccode & 1) != 0)
		{
			strncpy(tbuf, value_dsc.dsc$a_pointer,
				value_dsc.dsc$w_length);
			tbuf[value_dsc.dsc$w_length] = '\0';
			
			timeout  = strtoul(tbuf, &p, 10);
		}
	}

	return timeout;
}

/*
	OpenMbx		- open mailbox
*/
unsigned long
OpenMbx(struct dsc$descriptor_s *name, unsigned short *chan,
	unsigned long *maxmsg,int openflag)
{
	unsigned long ccode;
	unsigned long flags = 0;

#ifdef AGN$M_READONLY
	if (openflag == OPEN_READONLY)
		flags = AGN$M_READONLY;
#endif
#ifdef AGN$M_WRITEONLY
	if (openflag == OPEN_WRITEONLY)
		flags = AGN$M_WRITEONLY;
#endif

	/* assign channel	*/
	ccode = SYS$ASSIGN(name,chan,0,0,flags);

	/* if ok, get maxmsg	*/
	if ((ccode & 1) != 0)
		ccode = LIB$GETDVI(&DVI$_DEVBUFSIZ,chan,0,maxmsg,0,0);

	return(ccode);
}

/*
	ReadMsg		- read message
*/
static unsigned long 
ReadMsg(char *input,char *buf,unsigned long *len)
{
	static $DESCRIPTOR(prompt_dsc,"_Message: ");
	$DESCRIPTOR_D(line_dsc);
	unsigned long ccode;
	char line[133];
	FILE *fp;

	*len = 0;

	if (*input == '\0')
	{
		/* read from SYS$INPUT	*/
		while (*len <= maxmsg)
		{
			ccode = get_input(&line_dsc,&prompt_dsc,0);
			ccode = FormatLine(buf,len,
						line_dsc.dsc$a_pointer,
						line_dsc.dsc$w_length);
			if ((ccode & 1) == 0)
				break;
		}
		LIB$SFREE1_DD(&line_dsc);
	}
	else
	{
		/* read from input file	*/
		if ((fp=fopen(input,"r")) != NULL)
		{
			while (*len <= maxmsg)	/* while not reach max size*/
			{
				/* read a line	*/
				line[0] = '\0';
				fgets(line,sizeof(line),fp);

				/* parse line contents depending on format */
				ccode = FormatLine(buf,len,line,strlen(line));
				if ((ccode & 1) == 0)
					break;	/* all done */
			}
			fclose(fp);
		}
		else
			ccode = vaxc$errno;
	}

	if (ccode == RMS$_EOF)
		ccode = MBU__NORMAL;

	return(ccode);
}

/*
	FormatLine	- format line of mesage contents
*/
static 
unsigned long 
FormatLine(char *buf,unsigned long *len, char *line, int linelen)
{
	static char *forms[] = {"%x","%d","%s"};
	static char *delim[] = {" \t\n"," \t\n","\n"};
	unsigned long ccode, inc, value;
	char *token, *out;

	if (linelen == 0)
		return(RMS$_EOF);

	ccode = MBU__NORMAL;
       	out = buf + *len;
	if (format == TEXT)
	{
		if ((*len + linelen) < maxmsg)
		{
			memcpy(out,line,linelen);
			*len += linelen;
                        out[linelen] = '\0';
		}
		else
		{
			ccode = MBU__MAXMSG;
		}
	}
	else
	{
        	token=strtok(line,delim[format]);
		while (token != NULL)
		{
			if (sscanf(token,forms[format],&value) == 1)
			{
				switch (data_size)
				{
				case BYTE:
					*out = (unsigned char)value;
					inc = sizeof(char);
					break;
				case WORD:
					*out = (short)value;
					inc = sizeof(short);
					break;
				case LONG:
					*out = (long)value;
					inc = sizeof(long);
					break;
				}
			}
			else
			{
				ccode = MBU__INVINP;
				break;
			}
			if ((*len + inc) > maxmsg)
			{
				ccode = MBU__MAXMSG;
				break;
			}
			*len += inc;
			out += inc;
			token=strtok(NULL,delim[format]);
		}
	}
	return(ccode);
}

/*
	SendMsg		- Send message
*/
static unsigned long 
SendMsg(unsigned short chan, char *buf, unsigned long len, int wait,
	unsigned int timeout, int readercheck)
{
	unsigned long ccode;
	unsigned short iosb[4], func;

	if (buf == NULL)
		func = IO$_WRITEOF|IO$M_NORSWAIT;
	else
		func = IO$_WRITEVBLK|IO$M_NORSWAIT;

	if (!wait)
		func |= IO$M_NOW;

#ifdef IO$M_READERCHECK
	if (readercheck)
		func |= IO$M_READERCHECK;
#endif

	if (timeout != 0)
		StartTimer(chan,timeout);

	ccode = SYS$QIOW(WRITE_EFN,chan,func,iosb,0,0,buf,len,0,0,0,0);
	if ((ccode & 1) != 0)
		ccode = iosb[0];

	/* if abort assume it was a timeout	*/
	if (ccode == SS$_ABORT)
		ccode = SS$_TIMEOUT;	
	CancelTimer(chan);

	return(ccode);
}

/*
	TimerAst - when qio timer expires abort the I/O
*/
static unsigned long 
TimerAst(unsigned long chan)
{
	return SYS$CANCEL(chan);
}

/*
	StartTimer - start a timer to abort an I/O

	chan	- channel number that I/O is or will be using
	len	- number of seconds

*/
unsigned long
StartTimer(unsigned short chan, unsigned int len)
{
	unsigned long ccode, func;
	unsigned short qtime[4];

	func = LIB$K_DELTA_SECONDS;
	ccode = LIB$CVT_TO_INTERNAL_TIME(&func,&len,qtime);
	
	if (ccode & 1)
	{
		ccode = SYS$SETIMR(TIMER_EFN,qtime,TimerAst,chan,0);
	}

	return ccode;
}

/*
	CancelTimer	- cancel the timer 
*/
unsigned long
CancelTimer(unsigned short chan)
{
	return SYS$CANTIM(chan,0);
}
