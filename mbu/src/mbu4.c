/*
	MBU4	- read and view commands
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
#pragma module MBU4 "V01-005"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <climsgdef.h>
#include <syidef.h>
#include <iodef.h>
#include <errno.h>
#include <ssdef.h>

#ifndef SYI$K_ARCH_VAX 
#define SYI$K_ARCH_VAX 1
#endif

#include "mbu.h"

/* local data		*/
static FORMAT format;
static DATA_SIZE data_size;
static unsigned long maxmsg;

/* mailbox message read I/O status block */
static struct
{
	unsigned short IOstatus;
	unsigned short msglen;
	unsigned long pid;
} read_iosb;

/* mailbox message buffer header	*/
/* for pre VMS V5.5	*/
typedef struct
{
	void *msg_l_flink;		/* forward link		*/
	void *msg_l_blink;		/* backward link	*/
	unsigned short msg_w_size;	/* size of buffer	*/
	unsigned char msg_b_type;	/* data type of buffer	*/
	unsigned char msg_b_func;	/* I/O function code	*/
	unsigned short msg_w_msgsiz;	/* message size		*/
	void *msg_l_irpadr;		/* address of IRP	*/
	unsigned long msg_l_pid;	/* PID of writer	*/
} MSGV54;

/* for VMS V5.5 	*/
typedef struct 
{
	void *msg_l_flink;		/* forward link		*/
	void *msg_l_blink;		/* backward link	*/
	unsigned short msg_w_size;	/* size of buffer	*/
	unsigned char msg_b_type;	/* data type of buffer	*/
	unsigned char msg_b_func;	/* I/O function code	*/
	void *msg_l_irpadr;		/* address of IRP	*/
	unsigned long msg_l_pid;	/* PID of writer	*/
	void *msg_l_nordrwaitq_fl;	/*?q to wait for readers*/
	void *msg_l_nordrwaitq_bl;	/*?to deassign channel	*/
	unsigned char *msg_l_datastart;	/*?ptr to start of msg	*/
	unsigned short msg_w_msgsiz;	/* message size		*/
	unsigned short msg_w_quota;	/*?buffer quota charged */
} MSGV55;

/* for Alpha VMS V7.0 and later	*/
typedef struct
{
	void *msg_l_flink;		/* forward link		*/
	void *msg_l_blink;		/* backward link	*/
	unsigned short msg_w_size;	/* size of buffer	*/
	unsigned char msg_b_type;	/* data type of buffer	*/
	unsigned char msg_b_func;	/* I/O function code	*/
	void *msg_l_irpadr;		/* address of IRP	*/
	unsigned long msg_q_usrbufadr[2];/* user buffer address	*/
	void *msg_l_nordrwaitq_fl;	/* q to wait for readers*/
	void *msg_l_nordrwaitq_bl;	/* to deassign channel	*/
	unsigned long msg_l_pid;	/* PID of writer	*/
	unsigned char *msg_l_datastart;	/* ptr to start of msg	*/
	unsigned short msg_w_msgsiz;	/* message size		*/
	unsigned short msg_w_quota;	/* buffer quota charged */
	unsigned long msg_l_kpid;	/* kernal thread PID	*/
} MSGV70;

/* local functions	*/
static unsigned long ReadMsg(unsigned short chan, char *buf, int wait,
	unsigned int timeout,int writercheck);
static unsigned long DisplayMsg(char *buf, unsigned long maxlen, 
	unsigned long width, unsigned long pid, unsigned long len,
	unsigned char func);

/*
	read_cmd	- execute read command
*/
unsigned long 
read_cmd(void)
{
	static $DESCRIPTOR(length_dsc,"LENGTH");
	static $DESCRIPTOR(count_dsc,"COUNT");
	static $DESCRIPTOR(wait_dsc,"WAIT");
	static $DESCRIPTOR(writercheck_dsc,"WRITERCHECK");
	static $DESCRIPTOR(name_dsc,"NAME");
	static $DESCRIPTOR(output_dsc,"OUTPUT");
	unsigned long ccode, maxlen, count, width;
	unsigned short chan;
	int wait, writercheck;
	$DESCRIPTOR_D(value_dsc);
	$DESCRIPTOR_D(mbu_dsc);
	char output[256], *buf;
	extern unsigned long SYS$DASSGN();
	unsigned int timeout;

	/* get command values		*/
	CLI$GET_VALUE(&name_dsc,&mbu_dsc);
	data_size = get_data_size();
	format = get_format();
	width = get_width();
	count =  def_count;
	LIB$SFREE1_DD(&value_dsc);
	ccode = CLI$GET_VALUE(&count_dsc,&value_dsc);
	if ((ccode & 1) != 0)
		count = atoi(value_dsc.dsc$a_pointer);
	maxlen = def_length;
	LIB$SFREE1_DD(&value_dsc);
	ccode = CLI$GET_VALUE(&length_dsc,&value_dsc);
	if ((ccode & 1) != 0)
		maxlen = atoi(length_dsc.dsc$a_pointer);
	output[0] = '\0';
	ccode = CLI$PRESENT(&output_dsc);
	if ((ccode & 1) != 0)
	{
		if (def_output != NULL)
			strcpy(output,def_output);
		LIB$SFREE1_DD(&value_dsc);
		ccode = CLI$GET_VALUE(&output_dsc,&value_dsc);
		if ((ccode & 1) != 0)
		{
			strncpy(output, value_dsc.dsc$a_pointer,
				value_dsc.dsc$w_length);
			output[value_dsc.dsc$w_length] = '\0';
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

	ccode = CLI$PRESENT(&writercheck_dsc);
	if (ccode == CLI$_LOCPRES
	||  ccode == CLI$_PRESENT
	||  ccode == CLI$_DEFAULTED)
	{
		writercheck = TRUE;
	}
	else
		writercheck = FALSE;

	timeout = GetTimeout();

	/*  open mailbox		*/
	ccode = OpenMbx(&mbu_dsc,&chan,&maxmsg,OPEN_READONLY);
	if ((ccode & 1) != 0)
	{
		buf = malloc(maxmsg);
		if (maxlen > maxmsg)
			maxlen = maxmsg;
		while (1)
		{
			/* read message 	*/
			ccode = ReadMsg(chan,buf,wait,timeout,writercheck);
			if ((ccode & 1) != 0)
			{
				DisplayMsg(buf,maxlen,width,read_iosb.pid,
					read_iosb.msglen,0);
			}
			else
				break;
			if (count != 0)
				if (--count == 0)
					break;
			wait = FALSE; /* only wait for first msg */
		}
		free(buf);
	}
	/* close mailbox		*/
	SYS$DASSGN(chan);

	/* free dynamic strings		*/
	LIB$SFREE1_DD(&value_dsc);
	LIB$SFREE1_DD(&mbu_dsc);


	return(ccode);
}


/*
	view_cmd	- execute view command
*/
unsigned long 
view_cmd(void)
{
	unsigned long ccode, msgcnt, bytcnt, syi_item_code, isavax,vms_ver,pid;
	unsigned short chan, size, msgsize;
	static $DESCRIPTOR(length_dsc,"LENGTH");
	static $DESCRIPTOR(count_dsc,"COUNT");
	static $DESCRIPTOR(wait_dsc,"WAIT");
	static $DESCRIPTOR(name_dsc,"NAME");
	static $DESCRIPTOR(output_dsc,"OUTPUT");
	unsigned long maxlen, count, width, bufsiz, msgcnt2=0;
	char output[256], *mp;
	unsigned char *buf=NULL, *p;
	$DESCRIPTOR_D(value_dsc);
	$DESCRIPTOR_D(mbu_dsc);
	extern unsigned long SYS$DASSGN(), LIB$GETSYI(), vms$gl_license_version;
	MSGV70 *mptr70;
	MSGV55 *mptr55;
	MSGV54 *mptr54;
	unsigned char func = 0;

	/* get command values		*/
	CLI$GET_VALUE(&name_dsc,&mbu_dsc);	/* mailbox name 	*/
	data_size = get_data_size();		/* data size		*/
	format = get_format();			/* display format	*/
	width = get_width();	/* display width			*/
	count =  def_count;	/* number of messages to display	*/
	LIB$SFREE1_DD(&value_dsc);
	ccode = CLI$GET_VALUE(&count_dsc,&value_dsc);
	if (ccode & 1)
		count = atoi(value_dsc.dsc$a_pointer);
	maxlen = def_length;	/* number of bytes in in message to display */
	LIB$SFREE1_DD(&value_dsc);
	ccode = CLI$GET_VALUE(&length_dsc,&value_dsc);
	if ((ccode & 1) != 0)
		maxlen = atoi(length_dsc.dsc$a_pointer);
	/* where to output the messages	*/
	output[0] = '\0';
	ccode = CLI$PRESENT(&output_dsc);
	if (ccode & 1) 
	{
		if (def_output != NULL)
			strcpy(output,def_output);
		LIB$SFREE1_DD(&value_dsc);
		ccode = CLI$GET_VALUE(&output_dsc,&value_dsc);
		if (ccode & 1)
		{
			strncpy(output, value_dsc.dsc$a_pointer,
				value_dsc.dsc$w_length);
			output[value_dsc.dsc$w_length] = '\0';
			LIB$SFREE1_DD(&value_dsc);
		}
	}
	/*  open mailbox		*/
	ccode = OpenMbx(&mbu_dsc,&chan,&maxmsg,OPEN_READONLY);
	if (ccode & 1) 
	{	
		/* allocate buffer for mailbox message blocks */
		bufsiz = 16000;	/* random large number for now*/
		buf = malloc(bufsiz);
		if (maxlen > maxmsg)
			maxlen = maxmsg;

		/* get vms version (to determine message format) */
		vms_ver = vms$gl_license_version;

		/* determine if we on a VAX			*/
		syi_item_code = SYI$_ARCH_TYPE;
		ccode = LIB$GETSYI(&syi_item_code,&isavax,NULL,sizeof(isavax));
		if (ccode & 1)
			isavax = isavax == SYI$K_ARCH_VAX;
	}

	if ((buf != NULL) && (ccode & 1))
	{
		ccode = lockcode();	/* lock priv code in WS */
		if (ccode & 1)
		{
			/* get the message buffers from the mailbox */
			ccode = getmsg(chan,buf,bufsiz,&msgcnt2);
		}
	}
	/* determine how many messages we are going to display */
	if (count == 0)
		count = msgcnt2;
	else if (count > msgcnt2)
		count = msgcnt2;
	if ((ccode & 1) && (count == 0))
		ccode = MBU__NOMSG;	/* no messages to view */
	/* display the messages	*/
	for (p=buf;count > 0;--count)
	{
#ifndef __VAX
		if (vms_ver < 0x70000)
		{
			mptr55 = (MSGV55 *)p;
			mp = (char *)p + sizeof(MSGV55);
			func = mptr55->msg_b_func;
			if (mptr55->msg_b_func != 0)
				pid = cvtpid(mptr55->msg_l_pid);
			else
				pid = 0; /* message not written by a process */
			msgsize = mptr55->msg_w_msgsiz;
			size = mptr55->msg_w_size;
		}
		else
		{
			mptr70 = (MSGV70 *)p;
			mp = (char *)p + sizeof(MSGV70);
			func = mptr70->msg_b_func;
			if (mptr70->msg_b_func != 0)
				pid = cvtpid(mptr70->msg_l_pid);
			else
				pid = 0; /* message not written by a process */
			msgsize = mptr70->msg_w_msgsiz;
			size = mptr70->msg_w_size;
		}
#endif
#ifdef __VAX
		if (vms_ver <= 0x50040)
		{
			mptr54 = (MSGV54 *)p;
			mp = (char *)p + sizeof(MSGV54);
			func = mptr54->msg_b_func;
			if (mptr54->msg_b_func != 0)
				pid = cvtpid(mptr54->msg_l_pid);
			else
				pid = 0; /* message not written by a process */
			msgsize = mptr54->msg_w_msgsiz;
			size = mptr54->msg_w_size;
		}
		else
		{
			mptr55 = (MSGV55 *)p;
			mp = (char *)p + sizeof(MSGV55);
			func = mptr55->msg_b_func;
			if (mptr55->msg_b_func != 0)
				pid = cvtpid(mptr55->msg_l_pid);
			else
				pid = 0; /* message not written by a process */
			msgsize = mptr55->msg_w_msgsiz;
			size = mptr55->msg_w_size;
		}
#endif
		DisplayMsg(mp,maxlen,width,pid,msgsize,func);
		p += size;
	}
	/* close mailbox		*/
	SYS$DASSGN(chan);
	
	/* free dynamic strings		*/
	LIB$SFREE1_DD(&value_dsc);
	LIB$SFREE1_DD(&mbu_dsc);

	/* free message buffer		*/
	if (buf != NULL)
		free(buf);

	return(ccode);
}


/*
	ReadMsg		- read message from mailbox
*/
static unsigned long 
ReadMsg(unsigned short chan,char *buf, int wait,unsigned int timeout,
	int writercheck)
{
	unsigned long ccode;
	unsigned short func;
	extern unsigned long SYS$QIOW();

	if (wait)
		func = IO$_READVBLK;
	else
		func = IO$_READVBLK|IO$M_NOW;
#ifdef IO$M_WRITERCHECK
	if (writercheck)
		func |= IO$M_WRITERCHECK;
#endif

	if (timeout != 0)
		StartTimer(chan,timeout);

	ccode = SYS$QIOW(READ_EFN,chan,func,&read_iosb,0,0,buf,maxmsg,0,0,0,0);
	if ((ccode & 1) != 0)
		ccode = read_iosb.IOstatus;

	/* if abort assume it was a timeout	*/
	if (ccode == SS$_ABORT)
		ccode = SS$_TIMEOUT;	

	CancelTimer(chan);

	return(ccode);
}


/*
	DisplayMsg	- display message in required format

	buf is ptr to message
	maxlen is max number of bytes to output or 0 if no limit
	width is the display width
	pid is the process if the process that wrote the message
	len is the length of the message
	func is the I/O function code
*/
static unsigned long 
DisplayMsg(char *buf, unsigned long maxlen, unsigned long width,
	unsigned long pid, unsigned long len, unsigned char func)
{
	unsigned long ccode=1, value;
	int outlen, outwidth, inc, dec,i;
	char *outbuf, outbuf2[50], outbuf3[50], *outptr;

	get_process_name(pid,outbuf2,sizeof(outbuf2)-1);
	if (func != 0)
	{
		display_io_func(func, outbuf3);
		printf("Message from %08x %s, length %u, I/O function code %s\n",
			pid,outbuf2,len,outbuf3);
	}
	else
	{
		printf("Message from %08x %s, length %u\n",pid,outbuf2,len);
	}

	/* allocate a buffer for formatted output */
	outbuf = malloc(width+1);

	/* set outlen to be the number of bytes of the message to output */
	if (maxlen != 0 && len > maxlen)
		outlen = maxlen;
	else
		outlen = len;

	outwidth=width;	/* number of characters to output	*/
	outptr=outbuf;	/* display buffer ptr			*/

	while (outlen > 0)	/* while bytes left to display	*/
	{
		if (format == TEXT)
		{

			outptr = outbuf; /* reset to start of display buffer*/
			if (outlen > width) /* if No. bytes left too wide */
                        	inc = width;	/* do this many */
			else
				inc = outlen; 
			/* copy bytes from message to output buffer 	*/
			/* replacing non-printable characters by .	*/
			for (i=0; i < inc; i++)
			{
				if (isprint(*buf))
					*outptr++ = *buf;
				else
					*outptr++ = '.';
				buf++;
				outlen--;
			}
			*outptr = '\0'; /* terminate display string */
			puts(outbuf);	/* output display string    */
			outwidth = width;
		}
		else
		{
   			value = 0;
			switch(data_size)
			{
			case BYTE:
				value = *(unsigned char *)buf;
				dec = sizeof(char);
				break;
			case WORD:
				value = *(short *)buf;
				dec = sizeof(short);
				break;
			case LONG:
				value = *(long *)buf;
				dec = sizeof(long);
				break;
			}
   			if (format == HEX)
   				sprintf(outbuf2,"%x ",(unsigned)value);
   			else
				sprintf(outbuf2,"%d ",value);
			inc = strlen(outbuf2);
			if ((outwidth - inc) <= 0)
			{
				puts(outbuf);
				outptr = outbuf;
				outwidth = width;
			}
			strcpy(outptr,outbuf2);
			outwidth -= inc; /* output inc characters	*/
			outptr += inc;
			*outptr = '\0';
			outlen -= dec;   /* processed dec bytes		*/
			buf += dec;
		}
	}
	if (outwidth != width)
		puts(outbuf);	/* display remaining part of buffer */
			
	
	free(outbuf);
	return(ccode);
}
