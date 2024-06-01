/*
	MBU5	- set/show mailbox

 COPYRIGHT NOTICE

 This software is COPYRIGHT (c) 2004, Ian Miller. ALL RIGHTS RESERVED.
 Permission is granted for not-for-profit redistribution, provided all source
 and object code remain unchanged from the original distribution, and that all
 copyright notices remain intact.

 DISCLAIMER

 This software is provided "AS IS". The author makes no representations or
 warranties with respect to the software and specifically disclaim any implied
 warranties of merchantability or fitness for any particular purpose.

 History
 =======
 07 Dec 1994    V01-003 I.Miller        Added wildcard name support.
    Jun 2004	V01-004 I.Miller	Port to alpha and other enhancements.
 01 Jul 2004	V01-005	I.Miller	Display I/O queues
 26 Feb 2005	V01-006	I.Miller	Display I/O function code
 19 Aug 2005	V01-007	I.Miller	Changes due to differences in mailbox
					full device name in VMS V8.2
					Silently skip devices that are not
					mailboxes instead of complaining.
*/
#pragma module MBU5 "V01-007"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <climsgdef.h>
#include <dvidef.h>
#include <jpidef.h>
#include <dcdef.h>
#include <ssdef.h>
#include <iodef.h>
#include <lnmdef.h>
#ifdef __VAX
#define UCB$M_PRMMBX 1
#else
#include <ucbdef.h>
#endif
#include <ssdef.h>
#if SS$_NOREADER
#include <agndef.h>
#endif
#include "mbu.h"

/* local data	*/
static const $DESCRIPTOR(name_dsc,"NAME");

#define	DVS$_DEVCLASS	1
#define	DVS$_DEVTYPE	2

static unsigned long trnlog(struct dsc$descriptor_s *name_dsc);
static void display_irpq(IRPI *q, int qlen);
static void display_astq(ASTI *q, int qlen);
static unsigned long set_prot(unsigned short chan, unsigned long promsk);

/*
	show_mailbox_cmd	- execute show mailbox command
*/
unsigned long
show_mailbox_cmd(void)
{
	extern unsigned long SYS$DEVICE_SCAN(), SYS$ASSIGN(), SYS$DASSGN(), 
		SYS$GETDVIW(),SYS$IDTOASC(), SYS$QIOW();
	static unsigned long bufsiz,devclass,msgcnt,ownuic,ownpid,promsk,refcnt,opcnt;
	static unsigned long devtype, msgcnt2, devsts, rrefc, wrefc;
	static char devnam[256];
	static unsigned short devnamlen, len;
	$DESCRIPTOR(devname_dsc,devnam);
	static char lnm[256];
	static unsigned short lnmlen;
	$DESCRIPTOR(lnm_dsc,lnm);
	static char lnt[256];
	static unsigned short lntlen;
	$DESCRIPTOR(lnt_dsc,lnt);
	static $DESCRIPTOR(tempqual_dsc,"TEMPORARY");
	int temp_only = FALSE;
	static $DESCRIPTOR(permqual_dsc,"PERMANENT");
	int perm_only = FALSE;
	static $DESCRIPTOR(inusequal_dsc,"INUSE");
	int inuse_qual = 0;
	static $DESCRIPTOR(emptyqual_dsc,"EMPTY");
	int empty_qual = 0;
	static $DESCRIPTOR(permmbx_dsc,"permanent");
	static $DESCRIPTOR(tempmbx_dsc,"temporary");
	static IRPI rdrq[5];	/* mailbox reader IRP queue info */
	static unsigned long rdrqlen;
	static IRPI wrwq[5];	/* mailbox writer wait IRP queue info */
	static unsigned long wrwqlen;
	static IRPI rdwq[5];	/* mailbox reader wait IRP queue info */
	static unsigned long rdwqlen;
	static ASTI rstq[5];	/* mailbox read attn AST queue info */
	static unsigned long raqlen;
	static ASTI wstq[5];	/* mailbox write attn AST queue info */
	static unsigned long waqlen;
	static ASTI nstq[5];	/* mailbox room notify AST queue info */
	static unsigned long naqlen;
	int i;
	static ITEM dvi_list[] =
	{
		{sizeof(devclass),DVI$_DEVCLASS,&devclass,0},
		{sizeof(devtype),DVI$_DEVTYPE,&devtype,0},
		{sizeof(bufsiz),DVI$_DEVBUFSIZ,&bufsiz,0},
		{sizeof(bufsiz),DVI$_DEVSTS,&devsts,0},
		{sizeof(msgcnt),DVI$_DEVDEPEND,&msgcnt,0},
		{sizeof(ownuic),DVI$_OWNUIC,&ownuic,0},
		{sizeof(ownpid),DVI$_PID,&ownpid,0},
		{sizeof(promsk),DVI$_VPROT,&promsk,0},
		{sizeof(refcnt),DVI$_REFCNT,&refcnt,0},
		{sizeof(devnam),DVI$_DEVNAM,devnam,&devnamlen},
	   	{sizeof(opcnt),DVI$_OPCNT,&opcnt,0},
		{0,0,0,0}
	};
   	static char *fmt ="\n\
    Device name        %15.15s   Operations completed %17u\n\
    Creator process    %15.15s   Owner uic %28s\n\
    Creator process id    %08x       Dev prot  %28.28s\n\
    Reference count    %15u   Max msg size %25u\n\
    Buffer quota       %15u   Buffer quota remaining %15u\n";
	static char *fmt2="\
    Message count      %15u\n";
	static char *fmt3="\
    Message count      %15u   Number of bytes in mailbox %11u\n";
	static char *fmt4="\
    Mailbox logical name %s in table %s\n";
	static char *fmt5="\
    Mailbox reader count %13u   Writer count             %13u\n";

	unsigned short iosb[4], chan;
	unsigned long ccode, group, member, iniquo, bufquo, bytcnt;
	int bytcnt_valid = FALSE;
	static const $DESCRIPTOR(prefix_dsc,"_*");
	static const $DESCRIPTOR(postfix_dsc,":");
	$DESCRIPTOR_D(value_dsc);
	$DESCRIPTOR_D(param_dsc);
	static char uicbuf[80],ownname[17];
	static $DESCRIPTOR(uicbuf_dsc,&uicbuf[1]);
	char dname[65], *dp;
	$DESCRIPTOR(dname_dsc,dname);
	unsigned short dname_len,context[4];
	struct { 
	        short buflen, itmcod;
		void *bufadr, *retadr;
	} dsitemlist[2] = 
	{
		{sizeof(devclass),DVS$_DEVCLASS,
		&devclass,0},
		{0,0,0,0}
	};
	int dcount;
	extern unsigned long STR$CONCAT(), STR$COPY_DX(), STR$APPEND();

	/* get mailbox name	*/
	CLI$GET_VALUE(&name_dsc,&param_dsc);

	/* get qualifiers	*/
	ccode = CLI$PRESENT(&tempqual_dsc);
	if (ccode == CLI$_PRESENT || ccode == CLI$_LOCPRES 
	||  ccode == CLI$_DEFAULTED)
		temp_only = TRUE;
	ccode = CLI$PRESENT(&permqual_dsc);
	if (ccode == CLI$_PRESENT || ccode == CLI$_LOCPRES 
	||  ccode == CLI$_DEFAULTED)
		perm_only = TRUE;
	ccode = CLI$PRESENT(&inusequal_dsc);
	if (ccode == CLI$_NEGATED || ccode == CLI$_LOCNEG)
		inuse_qual = -1;	/* only not inuse mbx 	*/
	else if (ccode == CLI$_PRESENT || ccode == CLI$_LOCPRES ||  ccode == CLI$_DEFAULTED)
		inuse_qual = 1;		/* only inuse mbx	*/		
	else
		inuse_qual = 0;		/* inuse or not		*/
	ccode = CLI$PRESENT(&emptyqual_dsc);
	if (ccode == CLI$_NEGATED || ccode == CLI$_LOCNEG)
		empty_qual = -1;	/* only not empty mbx 	*/
	else if (ccode == CLI$_PRESENT || ccode == CLI$_LOCPRES ||  ccode == CLI$_DEFAULTED)
		empty_qual = 1;		/* only empty mbx	*/		
	else
		empty_qual = 0;		/* empty or not		*/

	/* check to see if the supplied name is a logical name		*/
	ccode = trnlog(&param_dsc);

        /* setup for sys$device_scan    				*/
	/* which will compare the name supplied against the full 	*/
	/* devicename of mailboxes and return those that match		*/
	/* before VMS V8.2 the full name is  _nodename$MBAnnn:		*/
	/* after V8.2 the full name is _MBAnnn:				*/

	/* if the supplied name does not start with _ 		*/
	/* then add _* prefix					*/
	if (*param_dsc.dsc$a_pointer != '_')
	{
		STR$CONCAT(&value_dsc,&prefix_dsc,&param_dsc);
	}
	else
	{
		STR$COPY_DX(&value_dsc,&param_dsc);
	}
	/* if the supplied value does not end with a : or a *	*/
	/* then append :					*/
	if ((value_dsc.dsc$a_pointer[value_dsc.dsc$w_length-1] != ':')
	&&  (value_dsc.dsc$a_pointer[value_dsc.dsc$w_length-1] != '*'))
		STR$APPEND(&value_dsc,&postfix_dsc);

        context[0] = context[1] = context[2] = context[3] = 0;
        devclass = DC$_MAILBOX;
	dcount = 0;
        while (1)
        {
                dname_dsc.dsc$w_length = sizeof(dname)-1;
                ccode = SYS$DEVICE_SCAN(&dname_dsc,&dname_len,&value_dsc,
                        dsitemlist,context);
                if (!(ccode & 1))
		{
			if (dcount == 0)
				ccode = SS$_NOSUCHDEV;
			else
				ccode = MBU__NORMAL;
                        break;
		}
                dname_dsc.dsc$w_length = dname_len;
		dcount++;

        	/* assign channel	*/
        	ccode = SYS$ASSIGN(&dname_dsc,&chan,0,0,0);	
        	if ((ccode & 1) == 0)
        		break;
        	/* get mailbox data	*/
        	ccode = SYS$GETDVIW(0,chan,0,&dvi_list,iosb,0,0,0);
                if (ccode & 1)
                        ccode = iosb[0];
        	if (ccode & 1)
        	{
			if ((devtype != DT$_MBX)
			&&  (devtype != DT$_SHRMBX)
			&&  (devtype != 0))	/* first few MB have this */
			{
				/* could be null device or pipe device*/
		        	SYS$DASSGN(chan);
				continue;	/* skip this one */
			}
			if (perm_only && !(devsts & UCB$M_PRMMBX))
			{
				/* only want permanent mbx */
		        	SYS$DASSGN(chan);
				continue;	/* skip this one */
			}
			if (temp_only && (devsts & UCB$M_PRMMBX))
			{
				/* only want temp mbx */
		        	SYS$DASSGN(chan);
				continue;	/* skip this one */
			}
			if ((empty_qual > 0) && (msgcnt != 0))
			{
				/* only want empty mbx */
		        	SYS$DASSGN(chan);
				continue;	/* skip this one */
			}
			if ((empty_qual < 0) && (msgcnt == 0))
			{
				/* only want non-empty mbx */
		        	SYS$DASSGN(chan);
				continue;	/* skip this one */
			}

			/* fiddle refcnt's to exclude us */
       			refcnt--;	
			if ((inuse_qual > 0) && (refcnt == 0))
			{
				/* only want inuse mbx */
		        	SYS$DASSGN(chan);
				continue;	/* skip this one */
			}
			if ((inuse_qual < 0) && (refcnt != 0))
			{
				/* only want not in use mbx */
		        	SYS$DASSGN(chan);
				continue;	/* skip this one */
			}

       			/* format data		*/
       			devnam[devnamlen] = '\0';
			devname_dsc.dsc$a_pointer = devnam;
			devname_dsc.dsc$w_length = devnamlen;
      			if (ownpid != 0)
       			{
				get_process_name(ownpid,ownname,
					sizeof(ownname)-1);
       			}
       			else
       			{
       				ownname[0] = '\0';
       			}
       			uicbuf[0] = '[';
       			uicbuf_dsc.dsc$w_length = sizeof(uicbuf)-1;
       			ccode = SYS$IDTOASC(ownuic,&len,&uicbuf_dsc,0,0,0);
       			if ((ccode & 1) != 0)
       			{
       				uicbuf[len+1] = ']';
       				uicbuf[len+2] = '\0';
       			}
       			else
       			{
       				group = ownuic / 65535;
       				member = ownuic % 65535 - group;
       				sprintf(uicbuf,"[%o,%o]",group,member);
       			}
       			uicbuf_dsc.dsc$w_length = strlen(uicbuf);

      			/* try to get total bytes used	*/
			if (getmsgcnt(chan,&msgcnt2,&bytcnt) & 1)
       				bytcnt_valid = TRUE;

			ccode = lockcode();	/* lock priv code in WS */
			if (ccode & 1)
			{
				rdrqlen = sizeof(rdrq);
				wrwqlen = sizeof(wrwq);
				rdwqlen = sizeof(rdwq);
				raqlen = sizeof(rstq);
				waqlen = sizeof(wstq);
				naqlen = sizeof(nstq);
	       			ccode = getmbx(chan,&iniquo,&bufquo,
					&lnm_dsc,&lnmlen,&lnt_dsc,&lntlen,
					&rrefc,&wrefc,rdrq,&rdrqlen,
					wrwq,&wrwqlen,rdwq,&rdwqlen,
					rstq,&raqlen,wstq,&waqlen,
					nstq,&naqlen);
			}
       			if ((ccode & 1) == 0)
			{
       				lnmlen = lntlen = iniquo = bufquo = 0;
				rdrqlen = wrwqlen = rdwqlen = 0;
				raqlen = waqlen = naqlen = 0;
			}
			/* fiddle refcnt's to exclude us */
			if (rrefc > 0)
				rrefc--;
			if (wrefc > 0)
				wrefc--;
       			putchar('\n');
			if (devsts & UCB$M_PRMMBX)
       				putmsg(MBU__MBUCHAR,&permmbx_dsc,&devname_dsc);
			else
       				putmsg(MBU__MBUCHAR,&tempmbx_dsc,&devname_dsc);

       			/* display data		*/
       			printf(fmt,devnam,opcnt,ownname,
       				uicbuf,ownpid,display_prot(promsk),
       				refcnt,bufsiz,iniquo,bufquo);
       			if (bytcnt_valid)
       				printf(fmt3,msgcnt2,bytcnt);
       			else
       				printf(fmt2,msgcnt);
#ifdef AGN$M_READONLY
			printf(fmt5,rrefc,wrefc);
#endif
			if (lnmlen != 0)
			{
				lnm[lnmlen] = '\0';
				lnt[lntlen] = '\0';
				printf(fmt4,lnm,lnt);
			}
       			putchar('\n');
			if (rdrqlen > 0)
			{
				/* display reader queue info	*/
				puts("Reader queue");
				display_irpq(rdrq,rdrqlen);
			}
			if (wrwqlen > 0)
			{
				/* display writer wait queue info	*/
				puts("Writer wait queue");
				display_irpq(wrwq,wrwqlen);
			}
			if (rdwqlen > 0)
			{
				/* display reader wait queue info	*/
				puts("Reader wait queue");
				display_irpq(rdwq,rdwqlen);
			}
			if (raqlen > 0)
			{
				/* display read attn Q info	*/
				puts("Read Attention ASTs");
				display_astq(rstq,raqlen);
			}
			if (waqlen > 0)
			{
				/* display write attn Q info	*/
				puts("Write Attention ASTs");
				display_astq(wstq,waqlen);
			}
			if (naqlen > 0)
			{
				/* display room notify AST Q 	*/
				puts("Room notify ASTs");
				display_astq(nstq,naqlen);
			}
       			/* all ok		*/
       			ccode = MBU__NORMAL;
      		}
        	SYS$DASSGN(chan);
                if (!(ccode & 1))
                        break;
        }
	/* delete dynamic string	*/
	LIB$SFREE1_DD(&value_dsc);
	LIB$SFREE1_DD(&param_dsc);
	return(ccode);
}

/*
	set_mailbox_cmd	- execute set mailbox command
*/
unsigned long
set_mailbox_cmd(void)
{
	extern unsigned long SYS$ASSIGN(), SYS$GETDVIW(), SYS$DASSGN();
	static unsigned long bufsiz,devclass,promsk,devtype;
	static ITEM dvi_list[] =
	{
		{sizeof(devtype),DVI$_DEVCLASS,&devclass,0},
		{sizeof(devtype),DVI$_DEVTYPE,&devtype,0},
		{sizeof(bufsiz),DVI$_DEVBUFSIZ,&bufsiz,0},
		{sizeof(promsk),DVI$_VPROT,&promsk,0},
		{0,0,0,0}
	};
	unsigned long ccode;
	static $DESCRIPTOR(maxmsg_dsc,"MAXMSG");
	static $DESCRIPTOR(bufquo_dsc,"BUFQUO");
	static $DESCRIPTOR(promsk_dsc,"PROMSK");
	$DESCRIPTOR_D(mbu_dsc);
	$DESCRIPTOR_D(value_dsc);
	unsigned long iniquo,bufquo,new_maxmsg=0,new_iniquo=0,new_promsk=0;
	unsigned short iosb[4], chan=0;

	/* get mailbox name	*/
	CLI$GET_VALUE(&name_dsc,&mbu_dsc);
	/* get buffer quota	*/
	LIB$SFREE1_DD(&value_dsc);
	ccode = CLI$GET_VALUE(&bufquo_dsc,&value_dsc);
	if ((ccode & 1) != 0)
		new_iniquo = atoi(value_dsc.dsc$a_pointer);
	/* get max msg size	*/
	LIB$SFREE1_DD(&value_dsc);
	ccode = CLI$GET_VALUE(&maxmsg_dsc,&value_dsc);
	if ((ccode & 1) != 0)
		new_maxmsg = atoi(value_dsc.dsc$a_pointer);
	/* get protection mask	*/
	LIB$SFREE1_DD(&value_dsc);
	ccode = CLI$GET_VALUE(&promsk_dsc,&value_dsc);
	if ((ccode & 1) != 0)
	{
		ccode = parse_prot(value_dsc.dsc$a_pointer,
			value_dsc.dsc$w_length,promsk,&new_promsk);
	}
	ccode = SYS$ASSIGN(&mbu_dsc,&chan,0,0,0);	
       	if (ccode & 1)
	{
        	/* get mailbox data	*/
        	ccode = SYS$GETDVIW(0,chan,0,&dvi_list,iosb,0,0,0);
                if (ccode & 1)
                        ccode = iosb[0];
	}
	if ((ccode & 1) && (devclass != DC$_MAILBOX))
		ccode = SS$_DEVNOTMBX;
	if ((ccode & 1) 
	&&  (devtype != DT$_MBX) 
	&&  (devtype != DT$_SHRMBX)
	&&  (devtype != 0))	/* first few MB have this */
	{
		ccode = SS$_DEVNOTMBX;
	}
	if (ccode & 1)
	{
		ccode = lockcode();	/* lock priv code in WS */
		if (ccode & 1)
		{
       			ccode = getmbx(chan,&iniquo,&bufquo,NULL,NULL,NULL,
				NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
				NULL,NULL,NULL,NULL,NULL,NULL);
		}
	}
	/* if new mailbox buffer quota specified and its less than current quota */
	if ((new_iniquo != 0) && (new_iniquo < iniquo))
		ccode = MBU__INVVAL; 	/* bad user !	*/
	/* if new max msg size specified and less than 32 bytes	*/
	if ((new_maxmsg != 0) && (new_maxmsg < 32))
		ccode = MBU__INVVAL;	/* bad user ! 	*/
	if (ccode & 1)
		ccode = setmbx(chan,new_iniquo,new_maxmsg);

	if (new_promsk != 0)
        	ccode = set_prot(chan,new_promsk);

	LIB$SFREE1_DD(&value_dsc);
	if (chan != 0)
        	SYS$DASSGN(chan);
	return ccode;
}

/*
	trnlog - translate mailbox logical name
*/
static unsigned long
trnlog(struct dsc$descriptor_s *name_dsc)
{
	static $DESCRIPTOR(perm_lnmtable_dsc,"LNM$PERMANENT_MAILBOX");
	static $DESCRIPTOR(temp_lnmtable_dsc,"LNM$TEMPORARY_MAILBOX");
	static $DESCRIPTOR(lnmtable_dsc,"LNM$DCL_LOGICAL");
	extern unsigned long SYS$TRNLNM(), STR$COPY_R();
	unsigned long ccode;
	int retlen;
	char logbuf[256];
	struct {
		short len;
		short code;
		char *buffer;
		int *retlen;
	} itemlist[2] =
	{
		{sizeof(logbuf)-1,LNM$_STRING,logbuf,&retlen},
		{0,0,0,0}
	};

	ccode = SYS$TRNLNM(0,&lnmtable_dsc,name_dsc,0,itemlist);
        if (ccode & 1)
	{
		/* overwrite logical name by translation */
		ccode = STR$COPY_R(name_dsc,&retlen,logbuf);
	}

	return ccode;	
}

/*
	getmsgcnt - get maibox message count and bytes used
*/
unsigned long 
getmsgcnt(unsigned short chan, unsigned long *msgcnt, unsigned long *bytcnt)
{
	extern unsigned long SYS$QIOW();
	unsigned long ccode;
	struct
	{
		unsigned short status;
		unsigned short msgcnt;
		unsigned long bytcnt;
	} iosb;
              		
	ccode = SYS$QIOW(GET_EFN,chan,IO$_SENSEMODE,&iosb,0,0,0,0,0,0,0,0);
       	if (ccode & 1)
       		ccode = iosb.status;
       	if (ccode & 1)
       	{
       		*msgcnt = iosb.msgcnt;
       		*bytcnt = iosb.bytcnt;
       	}
	return ccode;	
}

/*
	get_process_name
*/
void get_process_name(unsigned long pid, char *name, int namelen)
{
	$DESCRIPTOR(name_dsc,name);
	unsigned long ccode;
	unsigned short len;
	extern unsigned long LIB$GETJPI();

	if (pid != 0)
	{
		name_dsc.dsc$w_length = namelen;
	       	ccode  = LIB$GETJPI(&JPI$_PRCNAM,&pid,0,0,&name_dsc,&len);
		if (ccode & 1)
	        	name[len] = '\0';
		else
			name[0] = '\0';
	}
	else
	{
		name[0] = '\0';
	}
}

/*
	display_irpq - display a queue of IRPs
*/
static void
display_irpq(IRPI *q, int qlen)
{
	int i;
	char pname[17], fb[100];
	static char mode[4] = {'K','E','S','U'};

	printf("PID      Process Name     I/O Func BytCnt Mode EFN AST      ASTPRM\n");
/*              xxxxxxxx ................ xxxxxxxx dddddd    X ddd xxxxxxxx xxxxxxxx */
	for (i=0; i < qlen; i++)
	{
		if (q[i].pid & 0x80000000)
		{
			printf("System routine at address 0x%08x\n",
				q[i].pid);
		}
		else
		{
			q[i].pid = cvtpid(q[i].pid);
			get_process_name(q[i].pid,pname,sizeof(pname)-1);
			printf("%08x %-16.16s %08x %6d    %c %3d %08x %08x\n",
				q[i].pid, pname, q[i].func, q[i].bcnt, 
				mode[q[i].mode & 3],
				q[i].efn,q[i].ast,q[i].astprm);
			display_io_func(q[i].func,fb);
			printf("                          %-40.40s\n",fb);
		}
	}
}

/*
	display_io_func	- display a I/O function code and modifiers
*/
void 
display_io_func(unsigned long func, char *buf)
{
	unsigned int fc;

	buf[0] = '\0';		/* clear the output buffer */
	fc = func & IO$M_FCODE;	/* get I/O function code	*/

	switch (fc)
	{
	case IO$_WRITELBLK:
	case IO$_WRITEVBLK:
	case IO$_WRITEPBLK:
		strcpy(buf,"WRITE");
		break;
	case IO$_READLBLK:
	case IO$_READVBLK:
	case IO$_READPBLK:
		strcpy(buf,"READ");
		break;
	case IO$_WRITEOF:
		strcpy(buf,"WRITEEOF");
		break;
	case IO$_SETMODE:
		strcpy(buf,"SETMODE");
		break;
	case IO$_SENSEMODE:
		strcpy(buf,"SENSEMODE");
		break;
	}

	/* now check for various modifiers	*/
	if (func & IO$M_WRITERCHECK)
		strcat(buf," WRITERCHECK");
	if (func & IO$M_NOW)
		strcat(buf," NOW");
	if (func & IO$M_STREAM)
		strcat(buf," STREAM");
	if (func & IO$M_READERCHECK)
		strcat(buf," READERCHECK");
	if (func & IO$M_NORSWAIT)
		strcat(buf," NORSWAIT");
	if (func & IO$M_READATTN)
		strcat(buf," READATTN");
	if (func & IO$M_WRTATTN)
		strcat(buf," WRTATTN");
	if (func & IO$M_MB_ROOM_NOTIFY)
		strcat(buf," MB_ROOM_NOTIFY");
	if (func & IO$M_READERWAIT)
		strcat(buf," READERWAIT");
	if (func & IO$M_WRITERWAIT)
		strcat(buf," WRITERWAIT");
	if (func & IO$M_SETPROT)
		strcat(buf, "SETPROT");
}

/*
	display_astq - display a queue of ASTs
*/
static void
display_astq(ASTI *q, int qlen)
{
	int i;
	char pname[17];
	static char mode[4] = {'K','E','S','U'};

	printf("PID      Process Name     AST      ASTPRM   Mode Channel\n");
/*              xxxxxxxx ................ xxxxxxxx xxxxxxxx    A xxxx*/
	for (i=0; i < qlen; i++)
	{
		if (q[i].pid & 0x80000000)
		{
			printf("System routine at address 0x%08x\n",
				q[i].pid);
		}
		else
		{
			q[i].pid = cvtpid(q[i].pid);
			get_process_name(q[i].pid,pname,sizeof(pname)-1);
			printf("%08x %-16.16s %08x %08x    %c %04x\n",
				q[i].pid, pname,
				q[i].ast,q[i].astprm,
				mode[q[i].rmode & 3],
				q[i].chan);
		}
	}
}

static unsigned long 
set_prot(unsigned short chan, unsigned long promsk)
{
	extern unsigned long SYS$QIOW();
	unsigned short iosb[4];
	unsigned long ccode;

	ccode = SYS$QIOW(SET_EFN,chan,IO$_SETMODE|IO$M_SETPROT,iosb,0,0,
		0,promsk,0,0,0,0);
       	if (ccode & 1)
		ccode = iosb[0];
	return ccode;
}
