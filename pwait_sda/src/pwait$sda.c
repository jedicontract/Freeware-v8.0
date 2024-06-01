/*           
	PWAIT$SDA	SDA extention to look at a waiting process

 COPYRIGHT NOTICE

 This software is COPYRIGHT (c) 2004, Ian Miller. ALL RIGHTS RESERVED.

 Released under licence described in aaareadme.txt 

 DISCLAIMER

THIS PACKAGE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#define VERSION "V1.0"
#pragma module PWAIT$SDA VERSION

#define __NEW_STARLET 1
#include <arch_defs.h>
#include <cpudef.h>
#include <descrip.h>
#include <dmpdef.h>
#include <dyndef.h>
#include <gen64def.h>
#include <ints.h>
#include <lib$routines.h>
#include <libdtdef.h>
#include <lnmdef.h>
#include <lnmstrdef.h>
#include <statedef.h>
#include <pcbdef.h>
#include <ucbdef.h>
#include <phddef.h>
#include <sbdef.h>
#include <freddef.h>
#include <procstate.h>
#include <rmsdef.h>
#include <ccbdef.h>
#include <jibdef.h>
#include <irpdef.h>
#include <devdef.h>
#include <ddbdef.h>
#include <vcbdef.h>
#include <cebdef.h>
#include <dscdef.h>
#include <psldef.h>
#include <rsndef.h>
#include <fiddef.h>
#include <dcdef.h>
#include <statedef.h>
#include <sda_routines.h>
#include <ssdef.h>
#include <starlet.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stsdef.h>
#include <tqedef.h>
#include <pdscdef.h>
#include <efndef.h>
#include <iosadef.h>
#include <lkbdef.h>
#include <rsbdef.h>
#include <cli$routines.h>
#include <str$routines.h>
#include <climsgdef.h>
#include <lckdef.h>

#include "proc_read_writedef.h"

#include "sda_utils.h"

/*  Global variables	*/

int	sda$extend_version = SDA_FLAGS$K_VERSION;

/* local variables	*/
static SDA_FLAGS flags;
static char *statenames[] = 
	{"NULL","COLPG","MWAIT","CEF","PFW","LEF","LEFO","HIB","HIBO",
	 "SUSP","SUSPO","FPG","COM","COMO","CUR"};
static char *rsname[RSN$_MAX] = 
	{"RW???","RWAST","RWMBX","RWNPG","RWPFF","RWPAG","RWBRK","RWIMG",
	 "RWQUO","RWLCK","RWSWP","RWMPE","RWMPB","RWSCS","RWCLU","RWCAP",
	 "RWCSV","SNPFU","PSXFRK","RWINS","KTEXH"};
static char *rsndesc[RSN$_MAX] = 
	{"unknown resource","AST, I/O quota or interlock","Mailbox space",
	"non-paged dynamic memory","page file space","paged dynamic memory",
	"terminal broadcast","image activation interlock",
	"job pooled quota","lock ID","swap file space",
	"modified page list empty","modified page writer busy",
	"System Communication Services","Cluster locking",
	"CPU Capability","Cluster Server","Snapshot","Posix Fork Wait",
	"Inner mode access for Kthreads","Exit Handler for Kthread"
	};
static char * ast_modes[] = {"(none)","K","E","KE","S","KS","ES","KES","U","KU","EU","KEU","SU","KSU","ESU","KESU"};

static char *mode[4] = {"U","S","E","K"};
static char *rmode[4] = {"K","E","S","U"};

static char *mutex_names[][3] = 
{{"LNM$AQ_MUTEX","Shared logical name","Q"},
 {"IOC$GQ_MUTEX","I/O Database","Q"},
 {"EXE$GL_CEBMTX","Common event flag block list",NULL},
 {"EXE$GL_PGDYNMTX","Paged pool",NULL},
 {"EXE$GL_GSDMTX","Global Section Descriptor list",NULL},
 {"EXE$GL_ENQMTX","ENQ ?",NULL},
 {"EXE$GL_ACLMTX","ACL ?",NULL},
 {"CIA$GL_MUTEX","System intrusion database",NULL},
 {"EXE$GQ_BASIMGMTX","executive image data structures","Q"},
 {"QMAN$GL_MUTEX","QMAN ?",NULL},
 {"SMP$GL_CPU_MUTEX","CPU database",NULL}
};
#define MAX_MUTEX_NAMES (sizeof(mutex_names)/sizeof(mutex_names[0]))

typedef struct at
{
	char *name;
	char *note;
} AT;

/* display_timerq match values	*/
#define DISPLAY_TQ_ALL	0	/* display all entries for kernel thread */
#define DISPLAY_TQ_EF	1	/* display entries that match event flag */
#define	DISPLAY_TQ_WAKE	2	/* display scheduled wake entries	*/

static int cmd_flags;
#define PWAIT_M_ALL	1	/* analyze all processes		*/
#define PWAIT_M_FID_ONLY 2	/* don't try to translate fid to name	*/

static unsigned int pcbvecsize;	/* PCB vector size		*/
static PCB	**pcbvec = NULL;/* ptr to copy of PCB vector	*/
static unsigned int maxpix;		/* max process index		*/
static PCB	*nullpcb;		/* ptr to the null PCB		*/
static unsigned long swppid;		/* IPID of swapper		*/


/* local routines	*/
static int parse_cmd(struct dsc$descriptor_s *cmd_line);
static void analyze_all(void);
static int get_pcbvec(void);
static void analyze(PCB *pcb);
static void analyze_efw(KTB *ktb,PCB *pcb, PHD *phd);
static void analyze_fpg(KTB *ktb,PCB *pcb, PHD *phd);
static void analyze_hib(KTB *ktb,PCB *pcb, PHD *phd);
static void display_ceb(int efc,int efcp);
static void analyze_jibwait(KTB *ktb, PCB *pcb, PHD *phd);
static void analyze_rwast(PCB *curpcb, PCB *pcb, KTB *ktb, PHD *phd);
static void analyze_rwmbx(KTB *ktb, PHD *phd);
static void analyze_mutex_wait(KTB *ktb, PHD *phd);
static void analyze_lock(PCB *curpcb, PCB *pcb);
static void display_rsb(uint64 rsbptr);
static void display_rsb_grq(uint64 rsbptr, unsigned char rqmode);
static int getdvi_by_vollck(char *name, struct dsc$descriptor_s *devnam_dsc);
static void find_subprocesses(unsigned int pid);
static void display_addr(char *name, unsigned long addr);
static void get_device_name(DDB *ddb, UCB *ucb, char *devnam);
static void symbolize_addr(unsigned long addr, char *name, int max_name_len);
static void display_qaddr(char *name, uint64 addr);
static unsigned long get_registers(KTB *ktb,PHD *phd, PROCSTATE *procstate);
static unsigned long get_saved_registers(KTB *ktb,PHD *phd, PROCSTATE *procstate);
static unsigned long get_channels(CCB **ctpp, unsigned long *maxchan);
static int display_mb_ucb(char *msg, struct _ucb *ucbaddr);
static void display_disk_ucb(struct _ucb *ucbaddr);
static void display_busy_channels(CCB *ccb_table,unsigned long maxchan);
static void display_irpq(unsigned long count, 	IRP *irpqhdr, IRP *irpptr);
static unsigned long display_irp(IRP *irpq, IRP *irp);
static void display_timerq(KTB *ktb,int match);
static void display_time(char *name, unsigned long time);
static void display_io_func(unsigned int func);
static void display_io_sts(unsigned int sts);
static unsigned long get_ra(uint64 fp, uint64 *ra, uint64 *entry);
static int analyze_qaddr(char *name,uint64 addr,const AT at[],unsigned atlen);
static unsigned long pwait_get_regs(unsigned int ipid, uint64 regs[]);
#ifdef EFN$C_CTX
static void display_ctx_waitq(PCB *pcb);
#endif

/*

	This is the main entry point in SDA extension routine called from 
	within SDA.

	sda$extend	transfer_table, cmd_line, sda_flags

	transfer_table	- pointer to the routine transfer table
	cmd_line	- address of descriptor of the command line passed
	sda_flags	- flags 
*/
void sda$extend (int *transfer_table, 
		 struct dsc$descriptor_s *cmd_line, 
		 SDA_FLAGS sda_flags)
{
	static int sysdef_flag;
	int ccode;
	PCB *curpcb;

	/* Initialize the table and establish the condition handler */
	sda$vector_table = transfer_table;
	lib$establish(sda$cond_handler);
	flags = sda_flags;

	sda$print("PWAIT !AZ (c) 2004, Ian Miller (miller@encompasserve.org) built on VMS !AZ",
		&VERSION,__VMS_VERSION);


#if DEBUG
	lib$signal(SS$_DEBUG);
#endif
	if (sysdef_flag == FALSE)
	{
		sda$print("reading symbol tables");
	  	ccode = sda$read_symfile("SDA$READ_DIR:SYSDEF", SDA_OPT$M_READ_NOLOG );
	  	if (!(ccode & 1))
	    	{
	    		sda$print("READ SDA$READ_DIR:SYSDEF failed, ccode = !XL", ccode);
	    		return;
		}
	  	ccode = sda$read_symfile("SDA$READ_DIR",SDA_OPT$M_READ_NOLOG|SDA_OPT$M_READ_EXEC);
	  	if (!(ccode & 1))
	    	{
	    		sda$print("READ/EXEC failed, ccode = !XL", ccode);
	    		return;
		}
	  	sysdef_flag = TRUE;
	}

	ccode = parse_cmd(cmd_line);
	if (!(ccode & 1))
	{
		putmsg(ccode);
		return;
	}

	if (cmd_flags & PWAIT_M_ALL)
	{
		/* look at all processes	*/
		analyze_all();
	}
	else
	{
		/* look at current process	*/
		/* get current process pcb	*/
		sda$get_current_pcb (&curpcb);			/* get addr	*/

		analyze(curpcb);
	}
}

/*
	process the command line

	cmd_line	- commmand line by descriptor

	returns a condition code
*/
static int 
parse_cmd(struct dsc$descriptor_s *cmd_line)
{
	char value[256], cmd1[256], cmd2[256];
	$DESCRIPTOR(value_dsc,value);
	$DESCRIPTOR(cmd1_dsc,cmd1);
	$DESCRIPTOR(cmd2_dsc,cmd2);
	static $DESCRIPTOR(name_dsc,"NAME");
	static $DESCRIPTOR(id_dsc,"ID");
	static $DESCRIPTOR(allq_dsc,"ALL");
	static $DESCRIPTOR(fidq_dsc,"FID_ONLY");
	int ccode = SS$_NORMAL;	
	extern int *pwait_cld;
	unsigned short vlen, len;
	unsigned long pid;

        /* catch any exceptions         */
        VAXC$ESTABLISH(LIB$SIG_TO_RET);
 
	cmd_flags = 0;

	/* trim the command line	*/
	ccode = str$trim(&cmd1_dsc, cmd_line, &vlen);
	if (!(ccode & 1))
		return ccode;
	if (vlen == 0)
		return SS$_NORMAL;	/* no command	*/

	cmd1_dsc.dsc$w_length = vlen;
	cmd1[vlen] = '\0';	

	/* prefix command to make parsing easier	*/
	strcpy(cmd2, "PWAIT ");
	strcat(cmd2, cmd1);
	cmd2_dsc.dsc$w_length = strlen(cmd2);

	/* parse command			*/
	ccode = cli$dcl_parse(&cmd2_dsc, &pwait_cld);
	if (!(ccode & 1))
		return ccode;

	/* check for ALL qualifier		*/
	ccode = CLI$PRESENT(&allq_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
	{
		cmd_flags &= ~PWAIT_M_ALL;
	}
	else if (ccode & 1)
	{
		cmd_flags |= PWAIT_M_ALL;
	}

	/* check for FID_ONLY qualifier		*/
	ccode = CLI$PRESENT(&fidq_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
	{
		cmd_flags &= ~PWAIT_M_FID_ONLY;
	}
	else if (ccode & 1)
	{
		cmd_flags |= PWAIT_M_FID_ONLY;
	}

	/* check for ID qualifier		*/
	ccode = CLI$PRESENT(&id_dsc);
	if (ccode & 1)
	{
		/* get process id */
		ccode = cli$get_value(&id_dsc,&value_dsc, &vlen);
		if (!(ccode & 1))
			return ccode;
		value[vlen] = '\0';
		pid = strtoul(value,NULL,16);
		ccode = sda$set_process(NULL,pid,0);
	}
	else
	{
		/* check for process name 	*/
		ccode = CLI$PRESENT(&name_dsc);
		if (ccode & 1)
		{
			/* get process name */
			ccode = cli$get_value(&name_dsc,&value_dsc, &vlen);
			if (!(ccode & 1))
				return ccode;
			value[vlen] = '\0';
			ccode = sda$set_process(value,0,0);
		}
		else
			ccode = SS$_NORMAL;
	}

	return ccode;
}

/*
	analyze_all
*/
static void
analyze_all(void)
{
	PCB *curpcb;
	int i;
	int ccode;

	/* get a copy of a the PCB vector	*/
	ccode = get_pcbvec();
	if (!(ccode & 1))
		return;

	/* for each entry in the PCB vector		*/
	for (i=0; i < maxpix; i++)
	{
		if (pcbvec[i] == nullpcb)
			continue;	/* skip unused entry	*/

		curpcb = pcbvec[i];
		ccode = sda$set_process(0,0,(int)curpcb);
		if (!(ccode & 1))
		{
			sda$print("Failed to set process with PCB !XL",curpcb);
			break;
		}
		analyze(curpcb);
	}
}

static int
get_pcbvec(void)
{
	VOID_PQ sch$gl_pcbvec;		/* address of the PCB vector	*/
	int ccode;
	
	/* get the max PCB index			*/
	ccode = getvalue("SCH$GL_MAXPIX",&maxpix,sizeof(maxpix));
	if (!(ccode & 1))
	{
		sda$print("Failed to obtain maximum PCB index");
		return ccode;
	}

	/* get address of PCB vector			*/
	ccode = getvalue("SCH$GL_PCBVEC",&sch$gl_pcbvec, sizeof(sch$gl_pcbvec));
	if (!(ccode & 1))
	{
		sda$print("Failed to get address of PCB vector");
		return ccode;
	}
	sext(&sch$gl_pcbvec);

	/* get the address of the null PCB	*/
	ccode = getvalue("SCH$GL_NULLPCB",&nullpcb, sizeof(nullpcb));
	if (!(ccode & 1))
	{
		sda$print("Failed to get address of null PCB");
		return ccode;
	}

	/* get swapper IPID			*/
	ccode = getvalue("SCH$GL_SWPPID",&swppid,sizeof(swppid));
	if (!(ccode & 1))
	{
		sda$print("Failed to get SWAPPER IPID");
		return ccode ;
	}

	if (pcbvec == NULL)
	{
		/* allocate space for PCB vector		*/
		pcbvecsize = (maxpix + 1)*sizeof(long);
		sda$allocate(pcbvecsize, (void *)&pcbvec);
	}

	/* get the PCB vector				*/
	ccode = sda$trymem(sch$gl_pcbvec, pcbvec, pcbvecsize);
	if (!(ccode & 1))
	{
		sda$print("Failed to get PCB vector");
		return ccode;
	}
	return ccode;	
}

/*
	analyse the specified process
*/
static void
analyze(PCB *curpcb)
{
	int	ccode, i, t;
	PCB	pcb;
	PHD	*curphd, phd;
	KTB	*tktb, ktb, *ktbvec[PCB$K_MAX_KT_COUNT];
	unsigned short rsn;

	sda$format_heading("Process PCB @ !8XL",curpcb);

	if (cmd_flags & PWAIT_M_ALL)
		sda$new_page();

	ccode = sda$getmem(curpcb, &pcb, PCB$K_LENGTH);	/* get pcb	*/
	if (!(ccode & 1))
	{
		sda$print("Error !XL getting PCB",ccode);
		return;
	}

	/* sanity check - we could be looking at a crash dump with corruption*/
	if (pcb.pcb$b_type != DYN$C_PCB)
	{
		sda$print("PCB has invalid type !UB",pcb.pcb$b_type);
		return;
	}

	/* sometimes the process is COM - try again if it is */
	for (i=0; ((flags.sda_flags$v_current) && (pcb.pcb$l_state == SCH$C_COM) && (i<5)); i++)
	{
		sleep(5);
		sda$get_current_pcb (&curpcb);		/* get addr	*/
		sda$reqmem(curpcb, &pcb, PCB$K_LENGTH);	/* get pcb	*/
	}
	if (i>0)
		sda$print("!UL retries to get PCB",i);

	sda$format_heading("Process PID !XL name !AC No. Threads !UL",
                pcb.pcb$l_epid,pcb.pcb$t_lname, pcb.pcb$l_kt_count);

	sda$print("Process PID !XL name !AC No. Threads !UL",
                pcb.pcb$l_epid,pcb.pcb$t_lname, pcb.pcb$l_kt_count);

	/* get the addresses of the kernel thread blocks */
	ktbvec[0] = pcb.pcb$l_initial_ktb;	/* there is always 1 */
	if (pcb.pcb$l_kt_count > 1)
	{
		ccode = sda$getmem(pcb.pcb$l_ktbvec,ktbvec,
			pcb.pcb$l_kt_count*sizeof(ktbvec[0]));
		if (!(ccode & 1))
		{
			sda$print("Error !XL getting KTBVEC",ccode);
			return;
		}
	}
	for (t=0; t < pcb.pcb$l_kt_count; ++t) 
	{                                       
	tktb = ktbvec[t];
	ccode = sda$getmem(tktb,&ktb,sizeof(ktb));
	if (!(ccode & 1))
	{
		sda$print("Error !XL getting KTB",ccode);
		return;
	}

	sda$print("Thread !UL: state !AZ AST pending !AZ active !AZ blocked !AZ",
		t,statenames[ktb.ktb$l_state],
		ast_modes[ktb.ktb$l_ast_pending],
		ast_modes[ktb.ktb$l_astact],
		ast_modes[ktb.ktb$l_ast_blocked]);

	display_time("Process has been waiting for",ktb.ktb$l_waitime);
	if (ktb.ktb$l_sts & PCB$M_SSRWAIT)
		sda$print("Process thread resource wait is DISABLED");
	else
		sda$print("Process thread resource wait is ENABLED");

	if (ktb.ktb$l_sts & PCB$M_DELPEN)
		sda$print("Process is marked for deletion");

	if (ktb.ktb$l_sts & PCB$M_ERDACT)
		sda$print("Process exec mode rundown is active");

	if (pcb.pcb$l_sts2 & PCB$M_RWAST)
		sda$print("A thread is in RWAST");

	sda$format_heading("Process PID !XL name !AC Thread !UL",
                ktb.ktb$l_epid,pcb.pcb$t_lname, t);
	if ((t==0) && (pcb.pcb$v_single_threaded != 0))
	{
		sda$print("Process single threaded modes !AZ",
			ast_modes[pcb.pcb$v_single_threaded]);
	}

	/* and PHD			*/
	if (ktb.ktb$l_sts & PCB$M_PHDRES)
	{
		ccode = sda$getmem((VOID_PQ)&tktb->ktb$l_phd,&curphd,4); /* get addr */
		if (!(ccode & 1))
		{
			sda$print("Error !XL getting PHD address",ccode);
			return;
		}
		ccode = sda$getmem(curphd, &phd, PHD$K_LENGTH); /* get phd  */
		if (!(ccode & 1))
		{
			sda$print("Error !XL getting PHD",ccode);
			return;
		}
	}

	/* always check if the process is waiting for any lock requests */
	if (t == 0)	/* only for the first kthread	*/
		analyze_lock(curpcb,&pcb);

	switch(ktb.ktb$l_state)
	{
	case SCH$C_MWAIT:
		/* misc wait state or mutex wait	*/
		if (ktb.ktb$l_efwm == (int)ktb.ktb$l_jib)
		{
			analyze_jibwait(&ktb,&pcb,&phd);
		}
		else if ((rsn=ktb.ktb$l_efwm&0xFFFF) < RSN$_MAX)
		{
			/* resource wait	*/
			sda$print("!AZ - waiting for !AZ",
				rsname[rsn],
				rsndesc[rsn]);
			switch(rsn)
			{
			case RSN$_ASTWAIT:
				/* RWAST 	*/
				analyze_rwast(curpcb,&pcb,&ktb,&phd);
				break;
			case RSN$_MAILBOX:
				/* RWMBX	*/
				analyze_rwmbx(&ktb,&phd);
				break;
			case RSN$_NPDYNMEM:
			case RSN$_PGDYNMEM:
				/* RWNPG/RWPAG	*/
				sda$parse_command("SHOW MEMORY/POOL");
				break;
			case RSN$_PGFILE:
			case RSN$_SWPFILE:
			case RSN$_MPLEMPTY:
			case RSN$_MPWBUSY:
				sda$parse_command("SHOW MEMORY/FILE");
				break;
			case RSN$_SCS:
				sda$print("check cluster connections");
				break;
			case RSN$_CLUSTRAN:
				break;
			case RSN$_CPUCAP:
				sda$print("!_could be due to loss of cluster quorum"); 
				break;
			case RSN$_CLUSRV:
				sda$print("check CLUSTER_SERVER process");
				sda$print("- if not running then start it");
				sda$print("- if not in HIB state then reboot");
				break;
			case RSN$_INNER_MODE:
				sda$parse_command("SHOW PROCESS/SEMAPHORE");
				break;
			}
		}
		else
		{
			/* mutex wait	       		*/
			analyze_mutex_wait(&ktb,&phd);
		}
		break;
	case SCH$C_FPG:
		analyze_fpg(&ktb,&pcb,&phd);	/* Free Page wait */
		break;
	case SCH$C_CEF:
	case SCH$C_LEF:
	case SCH$C_LEFO:
		analyze_efw(&ktb,&pcb,&phd);
		break;
	case SCH$C_HIB:
	case SCH$C_HIBO:
		analyze_hib(&ktb,&pcb,&phd);
		break;
	default:
		/* not a wait state we are interested in */	
		break;
	}

	}
	return;
}

/*
	analyze_efw - display info about a process in LEF or CEF
*/
static void
analyze_efw(KTB *ktb, PCB *pcb, PHD *phd)
{
	unsigned long ccode, maxchan;
	int 	i, pos, efn;
	unsigned char size;
	char	outbuf[200], *p;
	CCB *ccb_table;
#if __alpha
	PROCSTATE pstate;
	uint64 ra, entry;
#endif

	sda$print("Event Flag Wait Mask EFWM !XL Wait Event Flag Cluster WEFC !UL",
		ktb->ktb$l_efwm,ktb->ktb$l_wefc);
	sda$print("Local Event flags 32-63 !XL 31-0 !XL",
		phd->phd$l_lefc_1, phd->phd$l_lefc_0);

	if (ktb->ktb$l_wefc < 4)	/* if a valid cluster number */
	{
		/* find out which event flag(s) process is waiting for */
		p = outbuf;
		pos = 0;
		size = 32;
		ccode = 1;
		while ((pos < size) && (ccode &1))
		{
			ccode = LIB$FFC(&pos,&size, &ktb->ktb$l_efwm, &efn);
			if ((ccode & 1) && (efn < size))
			{
				if (pos == 0)   /* if first EFN		*/
				{
					sprintf(p,"%d",efn+ktb->ktb$l_wefc*32);
				}
				else 		/* if not first EFN 	*/
				{
					if (ktb->ktb$l_sts & PCB$M_WALL)
					{
						sprintf(p," AND %d",
							efn+ktb->ktb$l_wefc*32);
					}
					else
					{
						sprintf(p," OR %d",
							efn+ktb->ktb$l_wefc*32);
					}
				} 
				p += strlen(p);
			}
			pos = efn + 1; 	/* resume search 1 bit after */
		} 
		*p = '\0';
		sda$print("waiting for event flag !AZ",outbuf);
		if ((ktb->ktb$l_wefc == 0) && (~(ktb->ktb$l_efwm) & 0xFF000000))
		{
			sda$print("process is waiting for an event flag which is reserved to HP (24-31)");
		}
	} /* endif wefc < 4 */
	if (ktb->ktb$l_wefc == 2)
	{
		/* display details of Common event block 2 */
		display_ceb(ktb->ktb$l_wefc,pcb->pcb$l_efc2p);
	}
	else if (ktb->ktb$l_wefc == 3)
	{
		/* display details of Common event block 3 */
		display_ceb(ktb->ktb$l_wefc,pcb->pcb$l_efc3p);
	}
	else if (ktb->ktb$l_wefc == 4)
	{
		sda$print("waiting for event flag 128 (EFN$C_ENF). EFWM is not relevant");

#ifdef EFN$C_CTX
		/* check for context wait (used by IO_PERFORM & UPDSEC_64) */
		if (pcb->pcb$l_ctx_waitq != NULL)
		{
			display_ctx_waitq(pcb);
		}
#endif
	}
#if __alpha
	/* display process waiting pc			*/
	ccode = get_registers(ktb,phd,&pstate);
	if (ccode & 1)
	{
		display_qaddr("process waiting pc",pstate.pstate$q_pc);

		/* get the return address	*/
		ccode = get_ra(pstate.pstate$q_r29,&ra,&entry);
		if (ccode & 1)
		{
			display_qaddr("current procedure entry address",
                                entry);
			display_qaddr("return address",
                                ra);
		}

	}
#endif

	/* now should try and work out what could set the EF */
// could be timer or I/O or something else 	*/
// could look at active timers to see if they are due to set this EF
// could look at busy channels to see if I/O completion are due to set EF
// could look at lock requests
// lots of system services use event flags :-(

	/* get the channel table	*/
	ccode = get_channels(&ccb_table,&maxchan);
	if (ccode & 1)
	{
		display_busy_channels(ccb_table,maxchan);
	}
	display_timerq(ktb,DISPLAY_TQ_ALL);
}

/*
	analyze_fpg - display info about a process in FPG
*/
static void
analyze_fpg(KTB *ktb, PCB *pcb, PHD *phd)
{
	unsigned long freecnt;
	unsigned long ccode;
	unsigned long idtblmax, lckcnt, lkidcnt, lkidfree;


	sda$print("===================== PWAIT$SDA Analysis ==============================");
	sda$print("Process in FPG - Free PaGe wait state");

#if __alpha
	ccode = getvalue("SCH$GL_FREECNT",&freecnt,sizeof(freecnt));
#endif
#if __ia64
	ccode = getvalue("SCH$GI_FREECNT",&freecnt,sizeof(freecnt));
#endif
	if (!(ccode & 1))
	{
		sda$print("**** can't get number of free pages	");
		return;
	}
	sda$print("Free page count !XL",freecnt);
	if ( freecnt == 0 )
	{
		sda$print("process is waiting for memory");
	}
	else
	{
		ccode = getvalue("LCK$GL_LKIDFREE",&lkidfree,sizeof(lkidfree));
		if (!(ccode & 1))
		{
			sda$print("**** can't get number of free lock IDs");
		}
		if (lkidfree == 0)
		{
			sda$print("process in RWFPG but FREECNT > 0. Process is waiting for allocation of LKB");
			sda$print("and no LOCK ID slots available in LOCK ID table");
			ccode = getvalue("LCK$GL_LCKCNT",&lckcnt,sizeof(lckcnt));
			ccode = getvalue("LCK$GL_LKIDCNT",&lkidcnt,sizeof(lkidcnt));
			ccode = getvalue("LCK$GL_IDTBLMAX",&idtblmax,sizeof(idtblmax));
			sda$print("There are !UL locks active",lckcnt);
			sda$print("There are !UL lock ID in use",lkidcnt);
			sda$print("IDTBLMAX !UL",idtblmax);
		}
	}
}

/*
	analyze_lock - check if process is waiting for any lock request
	Process-owned locks are kept in PCB$Q_LOCKQFL (waiting and converting
	locks first, then granted locks)
*/
static void
analyze_lock(PCB *curpcb, PCB *pcb)
{
	VOID_PQ pcb$q_lockqfl;
	unsigned long ccode,lck_cnt,wait_cnt,conv_cnt;
	uint64 queue_head, temp, lkb_ptr;
	int8 lkb_state;
	LKB$R_LKB lkb;

//	sda$print("Analyzing process locks PCB$Q_LOCKQFL = !XL.!XL !XL.!XL",
//		(uint64)(pcb->pcb$q_lockqfl)>>32, (uint64)(pcb->pcb$q_lockqfl) & 0xffffffff,
//		(uint64)(pcb->pcb$q_lockqbl)>>32, (uint64)(pcb->pcb$q_lockqbl) & 0xffffffff);

	sda$print("Analyzing process locks");

	queue_head = (uint64)&curpcb->pcb$q_lockqfl;
	temp = queue_head;
	lck_cnt = wait_cnt = conv_cnt = 0;

	/*
		Loop through all locks on the lock queue
	*/
	while ( 1 )
	{
	 	/* Get address of queue entry and return if we're done */
	  	ccode = sda$trymem ( (VOID_PQ) temp, &temp, sizeof(temp));
		if (!(ccode & 1)) 
	    	{
			sda$print("**** Error !XL getting LKB header @ !XL",ccode,temp);
			return;
	    	}
	  	if ( temp == queue_head ) 
			break;	/* back to beginning so exit the loop	*/	

		if ((lck_cnt > 0) && ((lck_cnt % 10000) == 0))
		{
			sda$print("analyzed !UL locks...",lck_cnt);
		}
		
	  	lck_cnt++;

	  	/* process-owned LKBs are linked via LKB$Q_OWNQFL/BL */

//	  	sda$print("temp= !XL.!XL",temp>>32, temp&0xffffffff);

		lkb_ptr = temp - offsetof(LKB$R_LKB,lkb$q_ownqfl);

//	  	sda$print("lkb_ptr= !XL.!XL",lkb_ptr>>32, lkb_ptr&0xffffffff);

	  	ccode = sda$trymem ( (UINT64_PQ) lkb_ptr, &lkb, sizeof(lkb));
	  	if (!(ccode & 1)) 
		{
			sda$print("**** Error !XL getting LKB @ !8XL",ccode,lkb_ptr);
			return;
	    	}

// should valididate lkb$b_type is DYN$C_LKB (could it be ACB ?)
// lkb$w_size should be sizeof(lkb)
	
		lkb_state = (int8)lkb.lkb$b_state;

//	  	sda$print("LKB state = !XL",lkb_state);
	    
          	switch (lkb_state)
            	{
            	case LKB$K_CONVERT:
	      		conv_cnt++;
	      		sda$print("!_converting lock LKB= !XL.!XL ID= !XL",
				lkb_ptr>>32,lkb_ptr&0xffffffff,
				lkb.lkb$l_lkid);
			display_rsb((uint64)lkb.lkb$q_rsb);
			display_rsb_grq((uint64)lkb.lkb$q_rsb,lkb.lkb$b_rqmode);
              		break;
            	case LKB$K_WAITING:
	      		sda$print("!_waiting lock LKB= !XL.!XL ID= !XL",
				lkb_ptr>>32,lkb_ptr&0xffffffff,
				lkb.lkb$l_lkid);
			display_rsb((uint64)lkb.lkb$q_rsb);
			display_rsb_grq((uint64)lkb.lkb$q_rsb,lkb.lkb$b_rqmode);
	      		wait_cnt++;
              		break;
            	}
	}

        if (lck_cnt == 0) 
	{
		sda$print("!_Process owns no locks"); 
	}
	else
	{
        	if (wait_cnt + conv_cnt == 0)
		{
			sda$print("!_Process owns !UL locks - none waiting or converting",lck_cnt);
		}
		else
		{
			sda$print("!_Process locks waiting/converting/total: !UL/!UL/!UL)",
				wait_cnt,conv_cnt,lck_cnt);
		}
	}
}

/*
	display granted locks with mode >= rqmode
*/
static void
display_rsb_grq(uint64 rsbptr, unsigned char rqmode)
{
	int ccode;
	RSB rsb;
	LKB$R_LKB lkb;
	uint64 qhead, ptr, a;

	/* get the RSB		*/
	ccode = sda$trymem((VOID_PQ)rsbptr,&rsb,sizeof(rsb));
	if (!(ccode & 1))
	{
		sda$print("error !XL reading RSB @ !XL.!XL",ccode,rsbptr>>32, rsbptr & 0xffffffff);
		return;
	}

	if (rsb.rsb$b_type != DYN$C_RSB)
	{
		sda$print("RSB @ !XL.!XL has invalid block type !UB", rsbptr>>32, rsbptr & 0xffffffff,
			rsb.rsb$b_type);
		return;
	}

	/* scan the granted queue displaying locks with mode >= rqmode	*/
	qhead = rsbptr + offsetof(RSB, rsb$q_grqfl);
	for (ptr = (uint64)rsb.rsb$q_grqfl; ptr != qhead; ptr = (uint64)lkb.lkb$q_sqfl)
	{
		ptr = ptr - offsetof(LKB$R_LKB,lkb$q_sqfl);
		ccode = sda$trymem((VOID_PQ)ptr, &lkb, sizeof(lkb));
		if (!(ccode & 1))
			break;	/* give up on error	*/
		if (lkb.lkb$b_grmode >= rqmode)
		{
			if (lkb.lkb$v_mstcpy) 
			{
				if (rsb.rsb$l_csid == 0)
				{
					sda$print("!_!_Blocking lock - LKB @ !XL.!XL - Master copy of lock Id !XL",
						ptr>>32, ptr & 0xffffffff, lkb.lkb$l_remlkid);			
				}
				else
				{
					sda$print("!_!_Blocking lock - LKB @ !XL.!XL - Remote lock Id !XL on node CSID !XL",
						ptr>>32, ptr & 0xffffffff, lkb.lkb$l_remlkid, rsb.rsb$l_csid);			
				}
			}
			else
			{
				if (rsb.rsb$l_csid == 0)
				{
					sda$print("!_!_Blocking lock - LKB @ !XL.!XL Id !XL IPID !XL",
						ptr>>32, ptr & 0xffffffff, lkb.lkb$l_lkid,lkb.lkb$l_tpid);
				}
				else
				{
					sda$print("!_!_Blocking lock - LKB @ !XL.!XL - process copy - Lock master is lock Id !XL on node CSID !XL",
						ptr>>32, ptr & 0xffffffff, lkb.lkb$l_remlkid,rsb.rsb$l_csid);			
				}
			}
		}
	}
}

/*
	display a resource block interpreting the resource name

*/
static void
display_rsb(uint64 rsbptr)
{
	int ccode;
	RSB rsb, parrsb;
	uint64 *rdb_ptr;
	char fs[256], devnam[256];
	$DESCRIPTOR(file_dsc,fs);
	$DESCRIPTOR(devnam_dsc,devnam);
	unsigned short buflen;
	FIDDEF fid;
	int *lckbasis;

	sda$print("!_!_RSB= !XL.!XL", rsbptr>>32, rsbptr & 0xffffffff);
	ccode = sda$trymem((VOID_PQ)rsbptr,&rsb,sizeof(rsb));
	if (!(ccode & 1))
	{
		sda$print("error !XL reading RSB",ccode);
		return;
	}

	if (rsb.rsb$b_type != DYN$C_RSB)
	{
		sda$print("RSB @ !XL.!XL has invalid block type !UB", rsbptr>>32, rsbptr & 0xffffffff,
			rsb.rsb$b_type);
		return;
	}
	sda$print("!_!_Resource  name !AF",rsb.rsb$b_rsnlen,rsb.rsb$t_resnam);

	/* interpret the resource name		*/
	if ((memcmp(rsb.rsb$t_resnam,"RMS$",4) != 0) &&  (rsb.rsb$t_resnam[10] == '\02'))
	{
		/* decode RMS name	*/
		getdvi_by_vollck(&rsb.rsb$t_resnam[10],&devnam_dsc);
		if (cmd_flags & PWAIT_M_FID_ONLY)
			ccode = 0;
		else
			ccode = lib$fid_to_name(&devnam_dsc, (USHORT_PQ)&rsb.rsb$t_resnam[4], &file_dsc, &buflen);
		if (ccode & 1)
		{
			fs[buflen] = '\0';
			sda$print("!_Filename: !AF",buflen, fs);
		}
	}
	rdb_ptr = (uint64 *)&rsb.rsb$t_resnam[0];
	if ( *rdb_ptr == 0x00000044000000dd ) 
	{
		/* decode RDB name	*/
		getdvi_by_vollck(&rsb.rsb$t_resnam[8], &devnam_dsc);
		if (cmd_flags & PWAIT_M_FID_ONLY)
			ccode = 0;
		else
			ccode = lib$fid_to_name(&devnam_dsc, (USHORT_PQ)&rsb.rsb$t_resnam[22], &file_dsc, &buflen);
		if (ccode & 1)
		{
			fs[buflen] = '\0';
			sda$print("!_!_Filename: !AF",buflen, fs);
		}
	}

	if (memcmp(rsb.rsb$t_resnam,"F11B$b",6) == 0)
	{
		ccode = getdvi_by_vollck(&rsb.rsb$t_resnam[6],&devnam_dsc);
		if (ccode & 1)
		{
			sda$print("!_!_!_File System Blocking lock on !AF",devnam_dsc.dsc$w_length,devnam_dsc.dsc$a_pointer);
		}
	}
	if (memcmp(rsb.rsb$t_resnam,"F11B$v",6) == 0)
	{
		ccode = getdvi_by_vollck(&rsb.rsb$t_resnam[6],&devnam_dsc);
		if (ccode & 1)
		{
			sda$print("!_!_!_File System Volume Allocation lock on !AF",&devnam_dsc.dsc$w_length,devnam_dsc.dsc$a_pointer);
		}
	}
	if ((memcmp(rsb.rsb$t_resnam,"F11B$s",6) == 0)
	||  (memcmp(rsb.rsb$t_resnam,"F11B$a",6) == 0))
	{
		/* decode XQP serialisation name	*/
		/* get volume allocation resource which is parent to this resource	*/
		ccode = sda$trymem((VOID_PQ)rsb.rsb$q_parent,&parrsb,sizeof(parrsb));
		if (ccode & 1)
		{
			parrsb.rsb$t_resnam[5] = '\02';
			ccode = getdvi_by_vollck(&parrsb.rsb$t_resnam[5],&devnam_dsc);
			if (cmd_flags & PWAIT_M_FID_ONLY)
			{
				ccode = 0;
			}
			else
			{
				lckbasis = (int *)&rsb.rsb$t_resnam[6];
				fid.fid$w_num = *lckbasis & 0xffff ;
				fid.fid$w_seq = 0;
				fid.fid$b_nmx = (*lckbasis & 0xff0000) >> 16;
				fid.fid$b_rvn = (*lckbasis & 0xff000000) >> 24;
				ccode = lib$fid_to_name(&devnam_dsc, (USHORT_PQ)&fid,&file_dsc, &buflen);
			}
		}
		if (ccode & 1)
		{
			fs[buflen] = '\0';
			if (rsb.rsb$t_resnam[5] == 's')
				sda$print("!_!_!_Serialisation lock on filename: !AF",buflen, fs);
			else if (rsb.rsb$t_resnam[5] == 'a')
				sda$print("!_!_!_Arbitration lock on filename: !AF",buflen, fs);
		}
	}
}

/*
	getdvi_by_volck - given a volume lock name find the device name
*/
static int
getdvi_by_vollck(char *vollock, struct dsc$descriptor_s *devnam_dsc)
{
	static char *prev_vollock = "";
	static char prev_devnam[64];
	static $DESCRIPTOR(prev_devnam_dsc,prev_devnam);
	VOID_PQ	ioc$gl_devlist, ddbptr, ucbptr, vcbptr;
	DDB 	ddb;
	UCB	ucb;
	VCB 	vcb;
	int 	ccode;
	int	found;

	// unlike lck$sda this routine should look at data in the dump only

	// the name can be one of the following formats
	//	<volume ID>
	//	<system name><UCB address>
	//	<volume set ID>
	//
	// GETDVI item DEVLOCKNAME returns this
	// VCB$T_VOLCKNAM contains this

	/* optimisation - should check if we have done this name already and if so return saved value	*/
	if (strncmp(prev_vollock,vollock,13) == 0)
	{
		*devnam_dsc = prev_devnam_dsc;
		return SS$_NORMAL;
	}

	/* loop through all DDB searching for disk class devices	*/
	ccode = getvalue("IOC$GL_DEVLIST",&ioc$gl_devlist,4);
	if (!(ccode & 1))
		return ccode;	/* give up on error	*/
	found = FALSE;
	for (ddbptr = ioc$gl_devlist; (ddbptr != 0) && !found; ddbptr = (VOID_PQ)ddb.ddb$ps_link)
	{
		sext(&ddbptr);
		ccode = sda$trymem((VOID_PQ)ddbptr,&ddb,DDB$K_LENGTH);
		if (!(ccode & 1))
			return ccode;	/* give up on error	*/
		/* loop through UCBs		*/
		for (ucbptr = (VOID_PQ)ddb.ddb$ps_ucb; ucbptr != 0; ucbptr = ucb.ucb$l_link)
		{
			sext(&ucbptr);
			ccode = sda$trymem((VOID_PQ)ucbptr,&ucb, UCB$K_LENGTH);
			if (!(ccode & 1))
				return ccode;	/* give up on error	*/
			if (ucb.ucb$b_devclass != DC$_DISK)
				break;		/* skip to next DDB	*/
			if (ucb.ucb$l_vcb == 0)
				continue;	/* skip to next UCB	*/
			vcbptr = ucb.ucb$l_vcb;
			sext(&vcbptr);
			ccode = sda$trymem(vcbptr,&vcb,sizeof(vcb));
			if (!(ccode & 1))
				return ccode;	/* give up on error	*/
			if (strncmp(vcb.vcb$t_volcknam,vollock,12) == 0)
			{
				found = TRUE;	/* eureka !	*/
				/* get device name	*/
				get_device_name(&ddb,&ucb,prev_devnam);
				prev_devnam_dsc.dsc$w_length = strlen(prev_devnam);
				*devnam_dsc = prev_devnam_dsc;
				break;
			}
		}
	}
	if (found)
		return SS$_NORMAL;
	else
		return 0;
}

/*
	get_device_name - create the name of a device
*/
static void
get_device_name(DDB *ddb, UCB *ucb, char *devnam)
{
	int ccode;
	unsigned long allocls;
	char nodename[16], dname[64];
	VOID_PQ sbptr;
	SB sb;
	unsigned long unit;

	*devnam = '\0';

	nodename[0] = '\0';
	unit = ucb->ucb$w_unit;

	memcpy(dname,&ddb->ddb$t_name[1],ddb->ddb$t_name[0]);
	dname[ddb->ddb$t_name[0]] = '\0';

	if (ucb->ucb$l_devchar2 & DEV$M_VRT)
	{
		allocls = 0;	/* no alloclass for shadow VU	*/
	}
	else
	{
		allocls = ddb->ddb$l_allocls;
		/* get scs node name	*/
		if (ddb->ddb$l_sb != 0)
		{
			sbptr = (VOID_PQ)ddb->ddb$l_sb;
			sext(&sbptr);
			ccode = sda$trymem(sbptr,&sb,sizeof(sb));
			if (ccode & 1)
			{
				memcpy(nodename,&sb.sb$t_nodename[1],sb.sb$t_nodename[0]);
				nodename[sb.sb$t_nodename[0]] = '\0';
			}
		}
	}
	if ((ucb->ucb$l_devchar & DEV$M_FOD)
	&&  !(ucb->ucb$l_devchar2 & DEV$M_NOCLU))
	{
		if (allocls != 0)
			sprintf(devnam,"$%u$%s%u:",allocls,dname,unit);
		else if(nodename[0] != '\0')
			sprintf(devnam,"%s$%s%u:",nodename,dname,unit);
		else
			sprintf(devnam,"%s%u:",dname,unit);
	}
	else
	{
			sprintf(devnam,"%s%u:",dname,unit);
	}
}

/*
	display_ceb - Display the specified common event block

*/
static void 
display_ceb(int efc, int efcp)
{
	unsigned long ccode;
	CEB ceb;

	if (efcp == 0)
		return; /*  nothing to do */

	ccode = sda$getmem((VOID_PQ)efcp, &ceb, CEB$C_LENGTH);		   
	if (!(ccode & 1))
	{
		sda$print("Error !XL getting CEB !XL",ccode,efcp);
		return;
	}

	if (ceb.ceb$b_type != DYN$C_CEB)
	{
		sda$print("CEF cluster !UL invalid CEB block type !UB",efc,ceb.ceb$b_type);
		return;
	}
	sda$print("EFC !UL !AZ CEF Cluster !AC Creator PID !XL UIC [!OW,!OW]",
		efc,(ceb.ceb$v_perm)?"Permanent":"Temporary",
		ceb.ceb$t_efcnam,ceb.ceb$l_pid,ceb.ceb$w_grp,ceb.ceb$l_uic & 0xFFFF);
	sda$print("	flags !XL",ceb.ceb$l_efc);
}

/*
	analyze_hib - display info about a process in HIB
*/
static void
analyze_hib(KTB *ktb, PCB *pcb, PHD *phd)
{
	unsigned long ccode, maxchan;
	CCB *ccb_table;
#if __alpha
	PROCSTATE pstate;
	uint64 ra, entry;

	/* display process waiting pc			*/
	ccode = get_registers(ktb,phd,&pstate);
	if (ccode & 1)
	{
		display_qaddr("process waiting pc",pstate.pstate$q_pc);

		/* get the return address	*/
		ccode = get_ra(pstate.pstate$q_r29,&ra,&entry);
		if (ccode & 1)
		{
			display_qaddr("current procedure entry address",
                                entry);
			display_qaddr("return address",
                                ra);
		}

	}
#endif
	/* get the channel table	*/
	ccode = get_channels(&ccb_table,&maxchan);
	if (ccode & 1)
	{
		display_busy_channels(ccb_table,maxchan);
	}
	display_timerq(ktb,DISPLAY_TQ_WAKE);
}

/*
	analyze_jibwait - display info about a process waiting for a pooled quota
*/
static void 
analyze_jibwait(KTB *ktb, PCB *pcb, PHD *phd)
{
	unsigned long ccode;
	JIB *curjib, jib;
	CCB *ccb_table;
	unsigned long maxchan;

	/* pooled quota wait		*/

	/* get jib        		*/
	ccode = sda$getmem(ktb->ktb$l_jib, &jib, JIB$K_LENGTH);		 
	if (!(ccode & 1))
	{
		sda$print("Error !XL getting JIB",ccode);
		return;
	}
	if (jib.jib$b_type != DYN$C_JIB)
	{
		sda$print("JIB has invalid type !UB",jib.jib$b_type);
		return;
	}

	/* check jib flags to see which	*/
	if (jib.jib$l_flags & JIB$M_BYTCNT_WAITERS)
	{
		/* job buffered I/O count exhausted */
		sda$print("waiting for buffered I/O byte count - Available !UL Quota !UL",
			jib.jib$l_bytcnt, jib.jib$l_bytlm);
	}
	if (jib.jib$l_flags & JIB$M_TQCNT_WAITERS)
	{
		/* job timer quota exhausted	*/
		sda$print("waiting for timer quota (TQ count !UL TQ Limit !UL)",
			jib.jib$l_tqcnt,jib.jib$l_tqlm);
		display_timerq(ktb,DISPLAY_TQ_ALL);
	}
	if (jib.jib$l_filcnt == 0)
	{
		sda$print("waiting for open file quota (FILLM !UL)",
			jib.jib$l_fillm);
	}
	if (jib.jib$l_pgflcnt == 0)
	{
		sda$print("waiting for page file quota (PGFLQOTA !UL)",
			jib.jib$l_pgflquota);
	}
}

/* 
	analyze_rwast 
*/ 
static void  
analyze_rwast(PCB *curpcb, PCB *pcb, KTB *ktb, PHD *phd)
{ 
/*
	address of known return addresses when in RWAST
*/
	static const AT rwast_ra_table[] = 
{{"EXE$DASSGN","process is waiting for an I/O request to complete"},
 {"EXE$MULTIQUOTA","process has exhausted a quota"},
 {"EXE$SNGLEQUOTA","process has exhausted a quota"},
 {"EXE$DELPRC","process may be waiting for a file system request or lock request"},
 {"PROCESS_MANAGEMENT","process may be waiting for a subprocess to end"},
};
	int c, display_channels = TRUE;
	CCB *ccb_table;
	PROCSTATE pstate;
	unsigned long ccode, maxchan;
	char outbuf[300];
	uint64 ra, entry;	

	/* check quotas	*/ 
	if (pcb->pcb$l_biocnt == 0) 
	{ 
		sda$print("!_process has exhausted buffered I/O quota (count !UL limit !UL)", 
			pcb->pcb$l_biocnt,pcb->pcb$l_biolm); 
	} 
	else if ((c = pcb->pcb$l_biocnt - pcb->pcb$l_biolm) > 0)
	{ 
		sda$print("!_process has !UL outstanding buffered I/O operations", 
			c); 
	} 
	if (pcb->pcb$l_diocnt == 0) 
	{ 
		sda$print("!_process has exhausted direct I/O quota (count !UL limit !UL)", 
			pcb->pcb$l_diocnt, pcb->pcb$l_diolm); 
	} 
	else if ((c = pcb->pcb$l_diocnt - pcb->pcb$l_diolm) > 0) 
	{ 
		sda$print("!_process has !UL outstanding direct I/O operations", 
			c); 
	} 
	if (pcb->pcb$l_prccnt != 0) 
	{ 
		sda$print("!_process has !UL subprocess!1%C!%Ees!%F", 
			pcb->pcb$l_prccnt); 
	} 
	if (pcb->pcb$l_dpc != 0) 
	{ 
		sda$print("!_process has !UL outstanding XQP events!%S", 
			pcb->pcb$l_dpc); 
// The XQP could be waiting for a lock or another blocked request or
// it is possible that a process in RWAST is waiting for XQP because the 
// paged dynamic memory is depleted. Look at the busy channels, look at
// the device AQB (ACP Queue Block) (address is in volume control block VCB)
// Look at the AQB$L_BUFCACHE field. This points to a F11BC structure.
// If the F11BC$Q_POOL_WAITQ field in this structure does not point to itself
// then the queue is not empty and the process XQP I/O request is waiting for
// paged pool.
	}
	if (ktb->ktb$l_sts & PCB$M_PHDRES)
	{
		/* check for outstanding ASTs	*/
		c = phd->phd$l_astlm - pcb->pcb$l_astcnt;
		if (c > 0)
		{
			sda$print("!_process has !UL outstanding AST!%S (ASTCNT !UL ASTLM !UL)",
				c,pcb->pcb$l_astcnt,phd->phd$l_astlm);
		}		
#if __alpha
		/* display process waiting pc			*/
		ccode = get_registers(ktb,phd,&pstate);
		if (ccode & 1)
		{
			display_qaddr("process waiting pc",pstate.pstate$q_pc);

			/* get the return address	*/
			ccode = get_ra(pstate.pstate$q_r29,&ra,&entry);
			if (ccode & 1)
			{
				display_qaddr("current procedure entry address",
                                	entry);
				c = analyze_qaddr("return address",ra,
					rwast_ra_table,
					sizeof(rwast_ra_table)/sizeof(AT));
				if (c > 0)
					display_channels = FALSE;
				if ((c == 3) && (pcb->pcb$l_dpc == 0))
				{
					sda$print("!_process may be waiting for a lock - use SHOW PROCESS/LOCK");
					sda$print("!_and look for locks 'waiting for' or 'converting'");
				}
				if ((c == 4) && (pcb->pcb$l_prccnt != 0))
				{
					sda$print("!_process is waiting for subprocess(es) to exit");
					find_subprocesses(pcb->pcb$l_pid);
				}
				if (pcb->pcb$l_dpc != 0)
				{
					sda$print("process !_is waiting for a XQP file system request");
					sda$print("!_- it may be due to waiting for a lock, paged pool, or another file system request");
					sda$print("!_- use SHOW PROCESS/LOCK and look for locks 'waiting for' or 'converting'");
					sda$print("!_- use SHOW POOL/SUMMARY and check if paged pool is available");
				}
			}
		}
#endif
	}
	if (display_channels)
	{
		/* get the channel table	*/
		ccode = get_channels(&ccb_table,&maxchan);
		if (ccode & 1)
		{
			display_busy_channels(ccb_table,maxchan);
		}
	}
}

/*
 	find the subprocesses of the specified process
*/
static void
find_subprocesses(unsigned int pid)
{
	PCB *curpcb;
	int i;
	int ccode;
	PCB pcb;

	/* get a copy of a the PCB vector	*/
	ccode = get_pcbvec();
	if (!(ccode & 1))
		return;

	/* for each entry in the PCB vector		*/
	for (i=0; i < maxpix; i++)
	{
		if (pcbvec[i] == nullpcb)
			continue;	/* skip unused entry	*/

		curpcb = pcbvec[i];
		ccode = sda$getmem(curpcb, &pcb, PCB$K_LENGTH);	/* get pcb	*/
		if (!(ccode & 1))
		{
			sda$print("Error !XL getting PCB @ !8XL",ccode,curpcb);
			return;
		}
		if (pcb.pcb$l_owner == pid)
		{
			sda$print("Subrocess PID !XL name !AC",
                		pcb.pcb$l_epid,pcb.pcb$t_lname);
		}
	}
}

/*
	analyze_rwmbx
*/
static void 
analyze_rwmbx(KTB *ktb, PHD *phd)
{
	PROCSTATE pstate;
	CCB *ccb_table;
	unsigned long ccode, maxchan, i, j, chan, nreg;
	uint64 *rptr;

	/* try and work out which mailbox	*/
	/* on a vax then in the PHD r3=IRP, R5=mailbox UCB	*/
	/* on an alpha then R? in the PHD = mailbox channel	*/
	/* on an itanium ?					*/

#if __alpha
	/* get the saved regs 		*/
	ccode = get_registers(ktb,phd,&pstate);	
	if (!(ccode & 1))
		return;
	display_qaddr("process waiting pc",pstate.pstate$q_pc);
	
	/* get the channel table	*/
	ccode = get_channels(&ccb_table,&maxchan);
	if (!(ccode & 1))
		return;

/*	
	for each valid channel (amod !=0, ucb != 0) compare channel number
	against the registers. 
	if a match is found then get the UCB using address in ccb
	if its a valid ucb (check btype) and its a mailbox
	display the device name, iniquo, bufquo
*/

	nreg = sizeof(pstate)/sizeof(uint64); /* number of saved regs */	
	for (i=0; i < maxchan; i++)
	{
		/* if ccb valid			*/
		if ((ccb_table[i].ccb$b_amod ==0)
		||  (ccb_table[i].ccb$l_ucb == NULL))
			continue;	/* skip unused ones */
		if (ccb_table[i].ccb$l_wind != NULL)
			continue;	/* skip if file structured device */

		/* get channel			*/
		chan = ccb_table[i].ccb$l_chan;

		/* check agains saved regs	*/
		for (j=0, rptr = &pstate.pstate$q_r8; j < nreg; j++, rptr++)
		{
			if (*rptr == chan)
			{
				/* a match !	*/
				if (display_mb_ucb("process is waiting for",ccb_table[i].ccb$l_ucb))
				{
					/* quit on successful display*/
					i=maxchan;
					break;	
				}
			}
		}
	}

	sda$deallocate((void *)ccb_table,maxchan*CCB$K_LENGTH);
#endif
#if __ia64
	/* get the channel table	*/
	ccode = get_channels(&ccb_table,&maxchan);
	if (ccode & 1)
	{
		display_busy_channels(ccb_table,maxchan);
		sda$deallocate((void *)ccb_table,maxchan*CCB$K_LENGTH);
	}
#endif
}

/*
	display_mb_ucb - display a UCB if its a mailbox

	returns TRUE if ok, FALSE if not
*/
static int
display_mb_ucb(char *msg, struct _ucb *ucbaddr)
{
	static MB_UCB *ucb;
	unsigned long ccode, irpoff;
	static LNMB	*lnmb;
	static int	lnmb_size;
	int new_lnmb_size;
	IRP *irpqhdr;

	if (ucb == NULL) 
		sda$allocate(UCB$K_MB_UCBLENGTH, (void *)&ucb);

	if (lnmb == NULL)
	{
		sda$allocate(sizeof (LNMB),(void *)&lnmb);
		lnmb_size = sizeof (LNMB);
	}

	ccode = sda$trymem(ucbaddr,ucb,UCB$K_MB_UCBLENGTH);
	if (!(ccode & 1))
		return FALSE;

	/* if is not a mailbox quit	*/
	if (ucb->ucb$r_ucb.ucb$b_devclass != DC$_MAILBOX)
		return FALSE;

	/*
		If the mailbox has a logical name associated with it, read in
		the logical name block (LNMB). Since we don't know the exact
		size of it then read the fixed header which contains
		the size, followed by another read of the whole LNMB into local
		memory.
	*/
	if (ucb->ucb$l_mb_logadr != NULL)
	{
		ccode = sda$trymem(ucb->ucb$l_mb_logadr,lnmb,sizeof(LNMB));
		if (!(ccode & 1))
			return FALSE;
		new_lnmb_size = lnmb->lnmb$w_size;
		/* make sure we have a big enough LNMB	*/
	      	if (new_lnmb_size > lnmb_size)
		{
			sda$deallocate((void*)lnmb,lnmb_size);
			sda$allocate(new_lnmb_size,(void *)&lnmb);
			lnmb_size = new_lnmb_size;
		}
	      	ccode  = sda$trymem(ucb->ucb$l_mb_logadr,lnmb,new_lnmb_size);
		if (!(ccode & 1))
			return FALSE;
	}
	else
		lnmb->lnmb$l_namelen = 0;
	if (msg != NULL)
		sda$print("!AZ",msg);
#ifdef UCB$L_MB_INIQUO
	sda$print("MBA!UW !AF msgcnt !UW, quota !UL, remaining !UL, rdr count !UL, wtr count !UL",
			ucb->ucb$r_ucb.ucb$w_unit,
			lnmb->lnmb$l_namelen, 
			&lnmb->lnmb$t_name,
			ucb->ucb$r_ucb.ucb$w_msgcnt,
			ucb->ucb$l_mb_iniquo, 
			ucb->ucb$l_mb_bufquo,
			ucb->ucb$l_mb_readerrefc,
			ucb->ucb$l_mb_writerrefc);
#else
	sda$print("MBA!UW !AF msgcnt !UW, quota !UW, remaining !UW, rdr count !UL, wtr count !UL",
			ucb->ucb$r_ucb.ucb$w_unit,
			lnmb->lnmb$l_namelen, 
			&lnmb->lnmb$t_name,
			ucb->ucb$r_ucb.ucb$w_msgcnt,
			ucb->ucb$r_ucb.ucb$w_iniquo, 
			ucb->ucb$r_ucb.ucb$w_bufquo,
			ucb->ucb$l_mb_readerrefc,
			ucb->ucb$l_mb_writerrefc);
#endif
	if (ucb->ucb$r_ucb.ucb$l_refc == 1)
		sda$print("no other process is accessing this mailbox");

	sda$print("mailbox read queue");
	irpoff = (char *)&ucb->ucb$l_mb_readqfl - (char *)ucb;
	irpqhdr = (IRP *)((char *)ucbaddr + irpoff);
	display_irpq(0,irpqhdr,ucb->ucb$l_mb_readqfl);
	
	return TRUE;	/* report the good news	*/
}

/*
	analyze_mutex_wait
*/
static void 
analyze_mutex_wait(KTB *ktb, PHD *phd)
{
	PCB *curpcb;
	PCB pcb;
	GENERIC_64 addr;
	int i, found = FALSE;
	unsigned long ccode;
	short lmutex[2];
	long qmutex[2];

	display_addr("	waiting for mutex",ktb->ktb$l_efwm);

/* 	use table of known mutex names, get value of each one and compare
	with mutex that process is waiting for. If known muxtex then say so
*/
	for (i=0;i < 0; i++)
	{
		ccode = sda$symbol_value(mutex_names[i][0],(uint64 *)&addr);
		if (ccode & 1)
		{
			if (addr.gen64$l_longword[0] == ktb->ktb$l_efwm)
			{
				found = TRUE;
				break;
			}
		}		
        }
	if (found)
	{
		/* sign extend the address	*/
		sext((VOID_PQ *)&addr);
		if (mutex_names[i][2] == NULL)
		{
			/* longword mutex	*/
			ccode = sda$trymem((VOID_PQ *)addr.gen64$q_quadword, 
				lmutex, 
				sizeof(lmutex));
			if (ccode & 1)
			{
				sda$print("	!AZ !AZ count=!ZW !AZ",
					mutex_names[i][0],
					mutex_names[i][1],
					lmutex[0],
					(lmutex[1]&1)?"writer":"nowriter");
			}
			else
			{
				sda$print("	!AZ !AZ",mutex_names[i][0],
					mutex_names[i][1]);
			}
		}
		else
		{
			/* quadword mutex	*/
			ccode = sda$trymem((VOID_PQ *)addr.gen64$q_quadword, 
				qmutex, 
				sizeof(qmutex));
			if (ccode & 1)
			{
				sda$print("	!AZ !AZ count=!ZL !AZ",
					mutex_names[i][0],
					mutex_names[i][1],
					qmutex[1],
					(qmutex[0]&1)?"writer":"nowriter");
			}
			else
			{
				sda$print("	!AZ !AZ",mutex_names[i][0],
					mutex_names[i][1]);
			}
		}
	}
	/*
		go looking for the owning process. It will have prio = 16 and pcb$l_mtxcnt > 0
	*/
	/* get a copy of a the PCB vector	*/
	ccode = get_pcbvec();
	if (!(ccode & 1))
		return;

	/* for each entry in the PCB vector		*/
	for (i=0; i < maxpix; i++)
	{
		if (pcbvec[i] == nullpcb)
			continue;	/* skip unused entry	*/

		curpcb = pcbvec[i];
		ccode = sda$getmem(curpcb, &pcb, PCB$K_LENGTH);	/* get pcb	*/
		if (!(ccode & 1))
		{
			sda$print("Error !XL getting PCB @ !8XL",ccode,curpcb);
			return;
		}
		if (pcb.pcb$l_mtxcnt == 1)
		{
			sda$print("Process PID !XL name !AC owns a mutex",
                		pcb.pcb$l_epid,pcb.pcb$t_lname);
		}
		else if (pcb.pcb$l_mtxcnt > 1)
		{
			sda$print("Process PID !XL name !AC owns !UL mutexes",
                		pcb.pcb$l_epid,pcb.pcb$t_lname,
				pcb.pcb$l_mtxcnt);
		}
	}
}

/*
	display_addr - display an address with a message and its symbol name if available
*/
static void
display_addr(char *name, unsigned long addr)
{
	char symname[300];

	symbolize_addr(addr, symname, sizeof(symname));
	sda$print("!AZ !XL !AZ",name,addr,symname);
}

/*
	symbolize_addr	- get symbol name for longword address
*/
static void 
symbolize_addr(unsigned long addr, char *name, int max_name_len)
{
	unsigned long ccode;
	GENERIC_64 value;

	/* copy and sign extend addr 	*/
	value.gen64$l_longword[0] = addr;
	sext((VOID_PQ *)&value);

	ccode = sda$symbolize(value.gen64$q_quadword, name, max_name_len);
	if (!(ccode & 1))
		*name = '\0';
}

/*
	display_qaddr - display an address with a message and its symbol name if available
*/
static void
display_qaddr(char *name, uint64 addr)
{
	unsigned long ccode;
	char symname[300];
	GENERIC_64 value;

	value.gen64$q_quadword = addr;
	ccode = sda$symbolize(addr, symname, sizeof(symname));
	if (ccode & 1)
	{
		sda$print("!AZ !XL.!XL !AZ",name,
			value.gen64$l_longword[1],value.gen64$l_longword[0],
			symname);
	}
	else
	{
		sda$print("!AZ !XL.!XL", name,
			value.gen64$l_longword[1],value.gen64$l_longword[0]);
	}	
}

/*
	get_registers - get the process registers
*/
static unsigned long
get_registers(KTB *ktb, PHD *phd, PROCSTATE *procstate)
{
	unsigned long ccode;
	uint64 regs[32];

	if (flags.sda_flags$v_current)
	{
		/* need to get current registers from the process */
		ccode = pwait_get_regs(ktb->ktb$l_pid,regs);
		if (ccode & 1)
		{
			/* copy registers to pstate structure	*/
			procstate->pstate$q_r0 = regs[0];
			procstate->pstate$q_r1 = regs[1];
			procstate->pstate$q_r2 = regs[2];
			procstate->pstate$q_r3 = regs[3];
			procstate->pstate$q_r4 = regs[4];
			procstate->pstate$q_r5 = regs[5];
			procstate->pstate$q_r6 = regs[6];
			procstate->pstate$q_r7 = regs[7];
			procstate->pstate$q_r8 = regs[8];
			procstate->pstate$q_r9 = regs[9];
			procstate->pstate$q_r10 = regs[10];
			procstate->pstate$q_r11 = regs[11];
			procstate->pstate$q_r12 = regs[12];
			procstate->pstate$q_r13 = regs[13];
			procstate->pstate$q_r14 = regs[14];
			procstate->pstate$q_r15 = regs[15];
			procstate->pstate$q_r16 = regs[16];
			procstate->pstate$q_r17 = regs[17];
			procstate->pstate$q_r18 = regs[18];
			procstate->pstate$q_r19 = regs[19];
			procstate->pstate$q_r20 = regs[20];
			procstate->pstate$q_r21 = regs[21];
			procstate->pstate$q_r22 = regs[22];
			procstate->pstate$q_r23 = regs[23];
			procstate->pstate$q_r24 = regs[24];
			procstate->pstate$q_r25 = regs[25];
			procstate->pstate$q_r26 = regs[26];
			procstate->pstate$q_r27 = regs[27];
			procstate->pstate$q_r28 = regs[28];
			procstate->pstate$q_r29 = regs[29];
			procstate->pstate$q_pc = regs[30];
			procstate->pstate$q_ps = regs[31];
		}
	}
	else
	{
		/* looking at a dump - get the saved registers	*/
		ccode = get_saved_registers(ktb,phd,procstate);
	}
	return ccode;
}

/*
	pwait_get_regs - get registers from a current process
*/
static unsigned long 
pwait_get_regs(unsigned int ipid, uint64 regs[])
{
	static unsigned long ast_cnt;	// AST Counter - used internally by exe$read_process
	struct
	{
		unsigned long arg_count;
		unsigned int ipid;
		unsigned long buffer_size;
		unsigned long target_address;
		uint64 *local_address;
		unsigned long target_address_type;
		unsigned long *ast_counter_address;
	} arg_list = {6,0,32*8,eacb$k_r0,NULL,eacb$k_general_register,&ast_cnt};
	unsigned long ccode;
	extern int EXE$READ_PROCESS();

	arg_list.ipid = ipid;
	arg_list.local_address = regs;

	ccode = SYS$CMKRNL(EXE$READ_PROCESS,(unsigned int *)&arg_list);

	return ccode;
}

/*
	get_saved_registers - get the process saved registers
*/
static unsigned long
get_saved_registers(KTB *ktb,PHD *phd, PROCSTATE *procstate)
{
	unsigned long ccode;
	FRED fred;
	GENERIC_64 fredaddr;

#if __alpha
	/*
		for alpha some registers are saved on the Kernal stack
		and others are in the PHD/FRED (floating point, stack pointers)
	*/
	if (ktb->ktb$l_sts & PCB$M_PHDRES)
	{
		/* get the kernel stack pointer from the FRED 	*/
#ifdef ktb$q_fred
		fredaddr.gen64$q_quadword = (uint64)ktb->ktb$q_fred;
#else
		fredaddr.gen64$l_longword[0] = (unsigned int)ktb->ktb$l_fred;
		fredaddr.gen64$l_longword[1] = 0xFFFFFFFF;
#endif
		ccode = sda$trymem((VOID_PQ)fredaddr.gen64$q_quadword,
			&fred,FRED$K_HWPCBLEN);
		if (ccode & 1)
		{
			/* get the saved process state from the kernal stack */
			ccode = sda$trymem((VOID_PQ)fred.fred$q_ksp,
				procstate,PSTATE$K_LENGTH);
		}
	}
	else
	{
		ccode = 0;	/* if PHD not resident then can't access FRED*/
	}
#else
	ccode = 0;	/* don't know if not alpha */
#endif
	return ccode;
}

/*
	get_channels - copy the channel table

	ctpp is the address of a ptr to the allocated memory containing
		a copy of the channel table.
	maxchan is a ptr to a longword for the max channel number in use.
*/
static unsigned long
get_channels(CCB **ctpp, unsigned long *maxchan)
{
	CCB *ctp;
	unsigned long ccode, chindx;
	VOID_PQ	ctl$ga_ccb_table, ctl$gl_chindx;
	GENERIC_64 ccb_table;

	/* address of start of channel table	*/
	ccode = sda$symbol_value("CTL$GA_CCB_TABLE",(uint64 *)&ctl$ga_ccb_table);
	if (!(ccode & 1))
	{
		sda$print("can't get channel table start address");
		return ccode;
	}

	ccode = sda$getmem(ctl$ga_ccb_table,&ccb_table.gen64$l_longword[0],4);
	if (!(ccode & 1))
	{
		sda$print("can't get channel table start address");
		return ccode;
	}
	/* validate address	*/
	if ((ccb_table.gen64$l_longword[0] == 0)
	||  (ccb_table.gen64$l_longword[0] >= 0x80000000))
	{
		sda$print("can't get channel table");
		return 0;
	}
	ccb_table.gen64$l_longword[1] = 0;

	/* get channel high water mark	*/
	ccode = sda$symbol_value("CTL$GL_CHINDX",(uint64 *)&ctl$gl_chindx);
	if (!(ccode & 1))
	{
		sda$print("can't get channel table size");
		return ccode;
	}
	ccode = sda$getmem(ctl$gl_chindx,&chindx,4);
	if (!(ccode & 1))
	{
		sda$print("can't get channel table size");
		return ccode;
	}
	/* allocate memory for channel table	*/
	sda$allocate(chindx*CCB$K_LENGTH,(void *)&ctp);

	/* copy table				*/
	ccode = sda$getmem((VOID_PQ)ccb_table.gen64$q_quadword,ctp,chindx*CCB$K_LENGTH);
	if (ccode & 1)
	{
		*ctpp = ctp;
		*maxchan = chindx;
	}
	else
	{
		sda$deallocate(ctp,chindx*CCB$K_LENGTH);
		*ctpp = NULL;
		*maxchan = 0;
	}

	return ccode;
}

/*
	display_busy_channels

	INPUT
*/
static void
display_busy_channels(CCB *ccb_table,unsigned long maxchan)
{
	int i,c,u;
	char device_name[32], outbuf[300];
	unsigned long ccode, irpoff;
	UCB *ucbptr, ucb;
	IRP *irpq, irp, *irpqhdr;

	for (i=0,u=0,c=0; i < maxchan; i++)
	{
		/* if ccb valid			*/
		if ((ccb_table[i].ccb$b_amod ==0)
		||  (ccb_table[i].ccb$l_ucb == NULL))
			continue;	/* skip	*/
		u++;	/* one more inuse */
		if (ccb_table[i].ccb$l_ioc == 0) 
			continue;	/* skip	idle 	*/
		c++;		/* one more busy	*/
		if (c==1)	/* if first	*/
		{
			/* output header */
			sda$print("!/Process busy channels!/_____________________");
			sda$print("Index  Chan     Mode I/O cnt WIND     Device");
			sda$format_heading("Index  Chan     Mode I/O cnt WIND     Device");
		}
#ifdef sda$get_device_name
		/* get device name	*/
		ccode = sda$get_device_name(ccb_table[i].ccb$l_ucb,
			device_name, sizeof(device_name));
		if (!(ccode & 1))
#endif
			device_name[0] = '\0';		/* no name	*/

// should get the WCB and display the FID (or name using LIB$FID_TO_NAME?)
// if wind < 0x80000000 then its a process section index
// but SDA SHOW CHANNEL command does this so is it worth the trouble

		sprintf(outbuf,"%-6d %08x    %s %7d %08x %s",
			i,ccb_table[i].ccb$l_chan,
			mode[ccb_table[i].ccb$b_amod & 3],
			ccb_table[i].ccb$l_ioc,
			ccb_table[i].ccb$l_wind,
			device_name);
		sda$print(outbuf);
		if ((unsigned long)ccb_table[i].ccb$l_wind & 1)
			sda$print("  (de)access pending for this channel");
		if ((unsigned long)ccb_table[i].ccb$l_dirp > 0x80000000)
		{
			sda$print("  deaccess irp");
			display_irp(ccb_table[i].ccb$l_dirp,&irp);
		}
	
		ucbptr = ccb_table[i].ccb$l_ucb;
		ccode = sda$getmem((VOID_PQ)ucbptr, &ucb, sizeof(UCB));
		if (!(ccode & 1))
		{
			sda$print("error getting UCB");
			continue;
		}
		if (ucb.ucb$l_irp)
		{
			sda$print("!/    Current IRP for this chan is @ !XL!/    _______________________________________",
				ucb.ucb$l_irp);
			display_irp(ucb.ucb$l_irp, &irp);
		}
		if (ucb.ucb$l_qlen != 0)
		{
			/* determine address of irpq */
			irpoff = (char *)&ucb.ucb$l_ioqfl - (char *)&ucb;
			irpqhdr = (IRP *)((char *)ucbptr + irpoff);
			sda$print("!/    I/O Queue for this chan!/    _______________________");
			display_irpq(ucb.ucb$l_qlen,irpqhdr,ucb.ucb$l_ioqfl);
		}

// here could look at other queues in UCB depending on class of device
// may have to get extended UCB. For mailbox devices could scan
// message queue and get irp address from message buffer header

		if (ucb.ucb$b_devclass == DC$_MAILBOX)
		{
			display_mb_ucb(NULL,ucbptr);
		}
		if (ucb.ucb$b_devclass == DC$_DISK)
		{
			display_disk_ucb(ucbptr);
		}
	}
	sda$print("process has !UL channel!%S !UL of which are busy",u,c);
}

/*
	display_irpq - display a queue of IRP
*/
static void
display_irpq(unsigned long count, IRP *irpq, IRP *irpptr)
{
	unsigned long ccode;
	int j, maxirp;
	IRP irp;

	/* display up to 5	*/
	if (count > 5)
	{
		maxirp = 5;
		sda$print("displaying first 5 of !UL",count);
	}
	else if (count == 0)
	{
		maxirp = 5;	/* display up to 5	*/
	}
	else
	{
		maxirp = count;
	}

	for (j=0; (j < maxirp); j++)
	{
		if (irpptr == irpq)
			break;
		ccode = display_irp(irpptr,&irp);
		irpptr = irp.irp$l_ioqfl;
	}
}

/*
	display_irp	- display one irp

	INPUT
		irpptr - address of the irp
		irp	- address of a IRP structure to use
*/
static unsigned long
display_irp(IRP *irpptr, IRP *irp)
{
	unsigned long ccode;

	ccode = sda$getmem((VOID_PQ)irpptr, irp, sizeof(IRP));
	if (!(ccode & 1))
	{
		sda$print("**** error getting IRP");
		return ccode;
	}
	/* sanity check structure type	*/
	if (irp->irp$b_type != DYN$C_IRP)
	{
		return 0;	/* its not a IRP */
	}
	sda$print("    IRP @!XL: tPID !XL chan !XL mode !AZ EFN !UB FUNC !XL",
		irpptr,irp->irp$l_thread_pid,irp->irp$l_chan,
		rmode[irp->irp$v_mode],
		irp->irp$b_efn,
		irp->irp$l_func);
	display_io_func(irp->irp$l_func);
	display_io_sts(irp->irp$l_sts);
	sda$print("        P1-6 !XL !XL !XL !XL !XL !XL",
		irp->irp$l_qio_p1,irp->irp$l_qio_p2,
		irp->irp$l_qio_p3,irp->irp$l_qio_p4,
		irp->irp$l_qio_p5,irp->irp$l_qio_p6);

	display_qaddr("        AST",(uint64)irp->irp$pq_acb64_ast);
	display_qaddr("        ASTPRM",irp->irp$q_acb64_astprm);
	return 1;
}

/*
	display the name of the io function
*/
static void 
display_io_func(unsigned int func)
{
	unsigned long fcode;

	fcode = func & IRP$M_FCODE;	/* get the function code part 	*/

// the problem here is that the same value as different names dependingon the driver

}

/*
	display the translation of the io request status
*/
static void 
display_io_sts(unsigned int sts)
{
	static readonly struct
	{
		unsigned int value;
		char *name;
	} stsnames[] =
	{{IRP$M_BUFIO,"bufio"},
	 {IRP$M_FUNC,"read-func"},
	 {IRP$M_PAGIO,"pagio"},
	 {IRP$M_COMPLX,"complx"},
	 {IRP$M_VIRTUAL,"virtual"},
	 {IRP$M_CHAINED,"chained"},
	 {IRP$M_SWAPIO,"swapio"},
	 {IRP$M_DIAGBUF,"diagbuf"},
	 {IRP$M_PHYSIO,"physio"},
	 {IRP$M_TERMIO,"termio"},
	 {IRP$M_MBXIO,"mbxio"},
	 {IRP$M_EXTEND,"extend"},
	 {IRP$M_FILACP,"filacp"},
	 {IRP$M_MVIRP,"mvirp"},
	 {IRP$M_SRVIO,"srvio"},
	 {IRP$M_CCB_LOOKED_UP,"ccb_looked_up"},
	 {IRP$M_CACHE_PAGIO,"cache_pagio"},
	 {IRP$M_BUFOBJ,"bufobj"},
	 {IRP$M_TRUSTED,"trusted"},
	 {IRP$M_FASTIO_DONE,"fastio_done"},
	 {IRP$M_FASTIO,"fastio"},
	 {IRP$M_FAST_FINISH,"fast_finish"},
	 {IRP$M_DOPMS,"dopms"},
	 {IRP$M_HIFORK,"hifork"},
	 {IRP$M_SRV_ABORT,"srv_abort"},
	 {IRP$M_LOCK_RELEASEABLE,"lock_releasable"},
	 {IRP$M_DID_FAST_FDT,"did_fast_fdt"},
	 {IRP$M_SYNCSTS,"syncsts"},
	 {IRP$M_FINIPL8,"finipl8"},
	 {IRP$M_FILE_FLUSH,"file_flush"},
	 {IRP$M_BARRIER,"barrier"}
	};
#define MAX_STSNAMES (sizeof(stsnames)/sizeof(stsnames[0]))
	char line[133], *p;
	int i, len, namelen;

	/* setup to make a line of names of the status flags that are set */
	line[0] = '\t';
	p = &line[1];
	len = sizeof(line) - 2;

	/* scan the table adding names for flags that are set 	*/
	for (i=0; i < MAX_STSNAMES; i++)
	{
		/* if the flag is set */
		if (sts & stsnames[i].value)
		{
			namelen = strlen(stsnames[i].name);
			if (namelen < len) 	/* if there is enough room */
			{
				strcpy(p, stsnames[i].name);
				p += namelen;
				*p++ = ' ';
				len -= namelen+1;
			}
			else
			{
				break;		/* line is full */
			}
		}
	}	
	*p = '\0';	/* terminate the line	*/
	if (line[1] != '\0')	/* if there is something to output */
	{
		sda$print(line);
	}
}

/*
	display_timerq	- display timer queue 

	ktb	kernel thread block for which to display timers
	match	= 0	display all entries for kernel thread
		= 1	display entries that match event flag
		= 2	display scheduled wake entries

 TBD:   Only display timer queue entries if any timers are outstanding,
	i.e. jib.jib$l_tqcnt .NE. jib.jib$l_tqlm

*/
static void
display_timerq(KTB *ktb, int match)
{
	VOID_PQ exe$gl_tqfl;
	TQE *tqfl, *tqeptr, tqe;
	unsigned long ccode, count;
	char name[300];
		
	if (strcmp(__VMS_VERSION+1,"7.3-1  ") >= 0) /* V7.3-1 or later */
	{
		sda$parse_command("SHOW PROCESS/TQE=ALL");
	}
	else
	{
#if 0
		/* if working on live system and have therefore time or synchronistion issues */
		if (flags.sda_flags$v_current)
		{
			sda$print("**** Can't display TQEs for live system");
			return;
		}
#endif
		/* walk the TQE list displaying entries for this process */
		ccode = sda$symbol_value("EXE$GL_TQFL",(uint64 *)&exe$gl_tqfl);
		if (!(ccode & 1))
		{
			sda$print("**** can't get address of tqe list");
			return;
		}
		ccode = sda$getmem(exe$gl_tqfl,&tqeptr,4);
		if (!(ccode & 1))
		{
			sda$print("**** can't get address of tqe list");
			return;
		}
		for (count=0; tqeptr != exe$gl_tqfl; tqeptr=tqe.tqe$l_tqfl)
		{
			ccode = sda$getmem(tqeptr,&tqe,sizeof(tqe));
			if (!(ccode & 1))
				break;	/* give up on error 	*/
			/* validate TQE block type */
			if (tqe.tqe$b_type != DYN$C_TQE)
			{
				sda$print("unknown TQE type !XB in TQE @ !XL",
					tqe.tqe$b_type,tqeptr);
				break;
			}
			if (tqe.tqe$b_rqtype == TQE$C_SSSNGL)
				continue;	/* skip subroutine entries */
			if (tqe.tqe$l_pid != ktb->ktb$l_pid)
				continue;	/* skip entries for other processes */
			if (count == 0)
			{
				sda$print(" ");
				sda$print("Process Timer Requests");
				sda$print("TQE      Type Mode EFN ASTPRM   AST");
				sda$format_heading("TQE      Type Mode EFN ASTPRM   AST");
			}
			count++;
			symbolize_addr((unsigned long )tqe.tqe$l_ast,name,sizeof(name));

			sda$print("!XL  !XB     !AZ !3UB !8XL !XL !AZ",
				tqeptr, tqe.tqe$b_rqtype, 
				rmode[tqe.tqe$l_rmod&3],
				tqe.tqe$l_efn,
				tqe.tqe$l_astprm,
				tqe.tqe$l_ast,
				name);
			if (match == DISPLAY_TQ_EF)
			{
				/* check for matching waiting event flag   */
				if (((tqe.tqe$l_efn / 32) == ktb->ktb$l_wefc)
				&& (((tqe.tqe$l_efn % 32)<<1) & ktb->ktb$l_efwm))
				{
					sda$print("NOTE: this TQE matches waiting event flag");
				}                               
			}
			if (match == DISPLAY_TQ_WAKE)
			{
				/* if not a wakeup entry	*/
				if (tqe.tqe$b_rqtype == TQE$C_WKSNGL)
					sda$print("NOTE: this TQE could wake your hibernating process");
			}
		}
	}
}

/*
	display_time
*/
static void 
display_time(char *name, unsigned long time)
{
	VOID_PQ exe$gl_abstim_tics;
	unsigned long ccode, abstim_tics;
	float t;
	GENERIC_64 qt;
	static unsigned int m=100000, op = LIB$K_DELTA_SECONDS_F, cvtflag=1;
	unsigned short timlen;
	static char timbuf[64];
	$DESCRIPTOR(timbuf_dsc,timbuf);

	ccode = sda$symbol_value("EXE$GL_ABSTIM_TICS",
		(uint64 *)&exe$gl_abstim_tics);
	if (!(ccode & 1))
	{
		sda$print("error !XL getting abstim_tics",ccode);
		return;
	}
	ccode = sda$getmem(exe$gl_abstim_tics,&abstim_tics,sizeof(abstim_tics));
	t = abstim_tics - time;		/* wait time in 10ms tics */
	t = t / 100.;			/* convert to no. of seconds */
	ccode = LIB$CVTF_TO_INTERNAL_TIME(&op, &t, &qt.gen64$q_quadword);
	if (!(ccode & 1))
	{
		sda$print("error !XL converting time 0x!XL",ccode,time);
		return;
	}
	ccode = sys$asctim(&timlen,&timbuf_dsc,&qt,1);
	if (!(ccode & 1))
	{
		sda$print("error !XL converting time",ccode);
		return;
	}
	timbuf[timlen] = '\0';
	sda$print("!AZ !AZ",name,timbuf);	
}

/*
	get_ra - get return address and procedure entry address from frame 
*/
unsigned long 
get_ra(uint64 fp, uint64 *ra, uint64 *entry)
{
	GENERIC_64 qw;
	unsigned long ccode;
	PDSCDEF pdsc;
	VOID_PQ pdaddr, regaddr;
	unsigned kind, framelen;
	unsigned short flags;

	qw.gen64$q_quadword = fp;

	/* ensure fp is quadword aligned	*/
	if ((qw.gen64$l_longword[0] & 7) != 0)
	{
		sda$print("fp is not quadword aligned : FP = !XL.!XL",
			qw.gen64$l_longword[1], qw.gen64$l_longword[0]);
		return 0;
	}

	/* get initial part of PDSC		*/
	pdaddr = (VOID_PQ *)fp;
	ccode = sda$trymem(pdaddr,&qw,8);
	if ((ccode & 1) == 0)
	{
		qw.gen64$q_quadword = fp;
		sda$print("failed to get procedure descriptor @ !XL.!XL",
			qw.gen64$l_longword[1], qw.gen64$l_longword[0]);
		return ccode;
	}

	/* check for pointer to procedure descriptor	*/
	if ((qw.gen64$l_longword[0] & 7) == 0)
	{
		pdaddr = (VOID_PQ *)qw.gen64$q_quadword;

		/* get initial part of PDSC		*/
		ccode = sda$trymem(pdaddr,&qw,8);
		if ((ccode & 1) == 0)
		{
			qw.gen64$q_quadword = (uint64)pdaddr;
			sda$print("failed to get procedure descriptor @ !XL.XL",
				qw.gen64$l_longword[1], qw.gen64$l_longword[0]);
			return ccode;
		}
	}
	else
	{
		pdaddr = (VOID_PQ *)fp;
	}

	/* get the kind of procedure	*/
	memcpy(&pdsc,&qw,sizeof(qw));
	flags = pdsc.pdsc$w_flags;
	kind = pdsc.pdsc$v_kind;
	if (kind == PDSC$K_KIND_FP_REGISTER)
	{
		/* register frame procedure */
		framelen = PDSC$K_MIN_LENGTH_RF;
	}
	else if (kind == PDSC$K_KIND_FP_STACK)
	{
		/* stack frame procedure */
		framelen = PDSC$K_MIN_LENGTH_SF;
	}
	else
	{
		qw.gen64$q_quadword = (uint64)pdaddr;

		sda$print("unknown kind !UW of procedure @ !XL.!XL",
			kind,qw.gen64$l_longword[1], qw.gen64$l_longword[0]);
		return 0;
	}

	/* determine length of basic frame	*/
	if (flags & PDSC$M_HANDLER_VALID)
		framelen += 8;
	if (flags & PDSC$M_HANDLER_DATA_VALID)
		framelen += 8;

	/* get the pdsc		*/
	ccode = sda$trymem(pdaddr,&pdsc,framelen);
	if ((ccode & 1) == 0)
	{	
		qw.gen64$q_quadword = (uint64)pdaddr;
		sda$print("failed to get procedure descriptor @ !XL.!XL",
			qw.gen64$l_longword[1], qw.gen64$l_longword[0]);
		return ccode;
	}
	/* get procedure entry address if wanted 	*/
	if (entry != NULL)
		*entry = pdsc.pdsc$q_entry;

	if (kind == PDSC$K_KIND_FP_REGISTER)
	{
		/* register frame procedure - RA is in a register */
		ccode = 0;	/* not yet implemented so return fail */
	}
	else if (kind == PDSC$K_KIND_FP_STACK)
	{
		/* stack frame procedure - RA is on the stack 	*/
		/* find the register save area			*/
		if (flags & PDSC$M_BASE_REG_IS_FP)
		{
			/* get the RA from the register save area */
			/* its always the first quadword	*/
			regaddr = (VOID_PQ)(fp + pdsc.pdsc$w_rsa_offset);
			ccode = sda$trymem(regaddr,ra,8);
		}
		else
		{
			/* base reg is not FP	*/
			/* I don't know what to do in this case so return failure */
//base reg will be SP so I suppose I add the value of the SP to rsa_offset
//and fetch the RA from the resultant address
			ccode = 0;
		}
	}
	else
	{
		qw.gen64$q_quadword = (uint64)pdaddr;

		sda$print("unknown kind !UW of procedure @ !XL.!XL",
			kind,qw.gen64$l_longword[1], qw.gen64$l_longword[0]);
		return 0;
	}

	return ccode;
}

/*
	analzye_qaddr
	- display an address and corresponding symbol value
	- check the symbol value against a table and if found display comment
	- returns the matching entry number or -1 if no match
*/
static int
analyze_qaddr(char *name,uint64 addr,const AT at[], unsigned atlen)
{
	int i;
	unsigned long ccode;
	char symname[300];
	GENERIC_64 value;

	value.gen64$q_quadword = addr;
	ccode = sda$symbolize(addr, symname, sizeof(symname));
	if (ccode & 1)
	{
		sda$print("!AZ !XL.!XL !AZ",name,
			value.gen64$l_longword[1],value.gen64$l_longword[0],
			symname);
		/* now check the name against the table	*/
		for (i=0; i < atlen; i++)
		{
			/* note it's a substring match 	*/
			if (strstr(symname,at[i].name) != NULL)
				break;	/* eureka !	*/
		}
		if (i < atlen)
		{
			/* display note from matching entry	*/
			sda$print("NOTE: !AZ",at[i].note);
		}
		else
		{
			i = -1;	/* indicate no match		*/
		}
	}
	else
	{
		sda$print("!AZ !XL.!XL", name,
			value.gen64$l_longword[1],value.gen64$l_longword[0]);
		i = -1;	/* indicate no match		*/
	}	
	return i;
}

#ifdef EFN$C_CTX
/*
	display_ctx_waitq - display info about a ctx wait queue

	The context wait queue in the pcb is a single linked list of structures
	which are a IOSA followed by a ptr to the next entry;
*/
static void
display_ctx_waitq(PCB *pcb)
{
	struct
	{
		IOSA iosa;
		void *next;
	} entry;
	unsigned long ccode;
	void *p;
	GENERIC_64 value;
	
	p = pcb->pcb$l_ctx_waitq;
	while (p != NULL)
	{
		ccode = sda$trymem((VOID_PQ *)p,&entry,sizeof(entry));
		if (!(ccode & 1))
			break;	/* if failed to get entry give up */
		p = entry.next;
		display_qaddr("ctx wait block @ ",(uint64)p);
		display_qaddr(" - context id",entry.iosa.iosa$q_context_id);
		sda$print(" - iosa$l_status !XL",entry.iosa.iosa$l_status);
		sda$print(" - iosa$l_resd !XL",entry.iosa.iosa$l_resd);

		value.gen64$q_quadword = entry.iosa.iosa$ih_count;
		sda$print(" - iosa$ih_count !XL.!XL",
			value.gen64$l_longword[1],value.gen64$l_longword[0]);
	}
}
#endif

/*
	display interesting info from a disk UCB
*/
static void 
display_disk_ucb(struct _ucb *ucbaddr)
{
#ifdef  UCB$C_DK_LENGTH
	unsigned long ccode, irpoff;
	IRP 	*irpqhdr;
	UCB 	ucb;
	DK_UCB 	dk_ucb;
	DDB	ddb;
	VCB	vcb;

	/* read the UCB		*/
	ccode = sda$trymem(ucbaddr,&ucb,UCB$K_LENGTH);
	if (!(ccode & 1))
		return;	/* give up on error		*/

	if (ucb.ucb$b_devclass != DC$_DISK)
		return;	/* give up if not a disk	*/

	/* read the DDB		*/
	ccode = sda$trymem(ucb.ucb$l_ddb,&ddb,DDB$K_LENGTH);
	if (!(ccode & 1))
		return;	/* give up on error		*/
	
	/* check its a DK or DG by checking DDB$T_NAME	*/
// is there a better way of checking its a SCSI or FC disk	?
	if ((ddb.ddb$t_name[1] == 'D')
	&&  ((ddb.ddb$t_name[2] == 'K')
	  || (ddb.ddb$t_name[2] == 'G')))
	{
		/* read the full DK UCB	*/
		ccode = sda$trymem(ucbaddr,&dk_ucb,sizeof(dk_ucb));
		if (!(ccode & 1))
			return;
		if (dk_ucb.ucb$l_dk_queued_io_count != 0)
		{	
			/* display normal path Active IRP list	*/
			sda$print("!/    Normal path Active IRP list!/    ___________________________");
			irpoff = (char *)&dk_ucb.ucb$ps_dk_active_irp_qfl - (char *)&dk_ucb;
			irpqhdr = (IRP *)((char *)ucbaddr + irpoff);
			display_irpq(dk_ucb.ucb$l_dk_queued_io_count,irpqhdr,dk_ucb.ucb$ps_dk_active_irp_qfl);
		}
	}
#endif
}
