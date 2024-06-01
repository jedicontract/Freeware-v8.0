/*
	GBLSEC$SDA	SDA extension to display information about a global section

 COPYRIGHT NOTICE

 This software is COPYRIGHT (c) 2005, Ian Miller. ALL RIGHTS RESERVED.

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
#define VERSION "V1.2"
#pragma module GBLSEC$SDA VERSION

#define __NEW_STARLET 1
#include <arch_defs.h>
#include <varargs.h>
#include <cpudef.h>
#include <descrip.h>
#include <dmpdef.h>
#include <dyndef.h>
#include <gen64def.h>
#include <ints.h>
#include <lib$routines.h>
#include <libdtdef.h>
#include <pcbdef.h>
#include <secdef.h>
#include <phddef.h>
#include <dscdef.h>
#include <wcbdef.h>
#include <fcbdef.h>
#include <rdedef.h>
#include <ucbdef.h>
#include <fiddef.h>
#include <psldef.h>
#include <sda_routines.h>
#include <ssdef.h>
#include <rmsdef.h>
#include <starlet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stsdef.h>
#include <gsddef.h>
#include <ctype.h>
#include <ptedef.h>
#include <vadef.h>
#include <cli$routines.h>
#include <str$routines.h>
#include <climsgdef.h>

/*  Global variables	*/

int	sda$extend_version = SDA_FLAGS$K_VERSION;

/* local variables	*/
static SDA_FLAGS flags;

static unsigned long page_size;	/* = MMG$GQ_PAGE_SIZE 		*/
static unsigned int pcbvecsize;	/* PCB vector size		*/
static PCB	**pcbvec = NULL;/* ptr to copy of PCB vector	*/
static uint64	mmg$gq_pt_base;	/* base address of PPT		*/

/* a buffer to hold a GSD with trailing name	*/
typedef struct
{
	GSD gsd;
	char namebuf[GSD$C_MAXNAMLEN];
} GSDBUF;

/* local routines	*/
static void display_help(void);
static void find_users(SECDEF *gste);
static int parse_cmd(struct dsc$descriptor_s *cmd_line, char *name, int maxlen,
	int *group, int *flags);
static int find_gs(char *gsname, int gsnamelen, GSDBUF *gsdbuf, int cmd_flags, int group, uint64 *gadaddr);
static void display_gsd(GSDBUF *gsdbuf);
static int get_gste(unsigned int gstx, SECDEF *gste, uint64 *gsteaddr);
static int get_gpte(SECDEF *gste, PTE **pte, unsigned int *ngpte);
static void display_gste(SECDEF *gste);
static void display_sec_flags(unsigned int flags, char *buf);
static void display_file_info(SECDEF *sec, int cmd_flags);
static int getvalue(char *name, void *vp, int size);
static void sext(VOID_PQ *ap);
static int search_region(RDE *rde, int gptx, PTE *gpte, int ngpte, uint64 *sva);
static int search_dynamic_regions(int gptx, PTE *gpte, int ngpte, uint64 *sva);
static int search_dyn_region_list(RDE *rdelisthead, int gptx, PTE *gpte, int ngpte, uint64 *sva);
static void putmsg();

/* command processing flags	*/
#define GBLSEC_M_GROUP 	1
#define GBLSEC_M_DELETE 2
#define GBLSEC_M_FIDONLY 4
#define GBLSEC_M_GSD	8
#define GBLSEC_M_GSTE	16
#define GBLSEC_M_USERS	32
#define GBLSEC_M_ALL	64

/* message codes	*/
globalvalue GBLSEC__NORMAL;
globalvalue GBLSEC__WARNING;
globalvalue GBLSEC__SYNTAX;
globalvalue GBLSEC__NTL;
globalvalue GBLSEC__NTS;
globalvalue GBLSEC__GNF;
globalvalue GBLSEC__GSDERR;
globalvalue GBLSEC__READSYM;
globalvalue GBLSEC__GETPCBVEC;
globalvalue GBLSEC__GETSYSPHD;
globalvalue GBLSEC__GETGSTE;
globalvalue GBLSEC__GETGPTE;
globalvalue GBLSEC__GETVALUE;
globalvalue GBLSEC__GETVALUEA;
globalvalue GBLSEC__GETSPTE;
globalvalue GBLSEC__GETPPT;
globalvalue GBLSEC__FATAL;
globalvalue GBLSEC__BUG;
globalvalue GBLSEC__NOTFOUND;
globalvalue GBLSEC__NOCOMMAND;
globalvalue GBLSEC__SEARCHING;

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
	int	found;
	int	ccode;
	int	group, cmd_flags;
	static int sysdef_flag;	/* TRUE if SYSDEF.STB has been read 	*/
	char	gsname[GSD$C_MAXNAMLEN+1]; /* Section name		*/
	int 	gsnamelen;	/* section name length			*/
	GSDBUF	gsdbuf;		/* global section descriptor + name	*/
	uint64	gsdaddr; 	/* address of GSD			*/
	SECDEF	gste;		/* Global Section Table Entry		*/
	uint64	gsteaddr;	/* address of GSTE			*/
	
	/* Initialize the table and establish the condition handler */
	sda$vector_table = transfer_table;
	lib$establish(sda$cond_handler);
	flags = sda_flags;

	sda$print("GBLSEC !AZ (c) 2005, Ian Miller (miller@encompasserve.org) built on VMS !AZ",
		&VERSION,__VMS_VERSION);

#if DEBUG
	lib$signal(SS$_DEBUG);
#endif

	if (cmd_line->dsc$w_length == 0)
	{
		/* no parameters - give brief help	*/
		display_help();
		return;
	}

	/* process command line	*/
	ccode = parse_cmd(cmd_line, gsname, GSD$C_MAXNAMLEN, &group, &cmd_flags);
	if (ccode == GBLSEC__NOCOMMAND)
	{
		/* no parameters - give brief help	*/
		display_help();
		return;
	}

	if (!(ccode & 1))
	{
		putmsg(GBLSEC__SYNTAX);
		return;
	}

	gsnamelen = strlen(gsname);

	if (gsnamelen > GSD$C_MAXNAMLEN)
	{
		putmsg(GBLSEC__NTL);
		return;
	}

	/* check name is ok			*/
	if (gsnamelen <= 0)
	{
		putmsg(GBLSEC__NTS);
		return;
	}

	/* add useful stuff to the symbol table if we have not done it already*/
	if (sysdef_flag == FALSE)
	{
	  	ccode = sda$read_symfile("SDA$READ_DIR:SYSDEF", SDA_OPT$M_READ_NOLOG );
	  	if (!(ccode & 1))
	    	{
			putmsg(GBLSEC__READSYM,ccode);
	    		return;
		}
	  	sysdef_flag = TRUE;
	}

	gsdaddr = 0;
	found = 0;

	while (1)
	{
		/* look for the global section descriptor	*/
		if (!find_gs(gsname,gsnamelen,&gsdbuf, cmd_flags, group,&gsdaddr))
		{
			if (found == 0)
				putmsg(GBLSEC__GNF);
			break;	
		}

		found++;
	
		/* set page heading and start on a new page	*/
		sda$format_heading("Global section: !AZ", gsname);
		sda$new_page();

		sda$add_symbol("GSD",gsdaddr);

		if (cmd_flags & GBLSEC_M_GSD)
		{
			/* display info from GSD			*/
			display_gsd(&gsdbuf);
		}

	// could get and display the ORB at this point

		/* get the GSTE 				*/
		ccode = get_gste(gsdbuf.gsd.gsd$l_gstx,&gste,&gsteaddr);
		if (!(ccode & 1))
			return;

		sda$add_symbol("GSTE",gsteaddr);

		if (cmd_flags & GBLSEC_M_GSTE)
		{
			/* if there is a associated file then display 	*/
			if (gste.sec$l_window != 0)
			{
				sda$print(" ");
				display_file_info(&gste, cmd_flags);
			}
		
			/* display info from GSTE			*/
			sda$print(" ");
			display_gste(&gste);
		}

		if (cmd_flags & GBLSEC_M_USERS)
		{
			find_users(&gste);
		}
		if (!(cmd_flags & GBLSEC_M_ALL))
			break;	
	}
}

/*
	find_users - find processes mapped to the specified globl section
*/
static void
find_users(SECDEF *gste)
{
	PCB	*curpcb, pcb;
	PHD	*curphd, phd;
	PTE	*gpte;	/* ptr to array containing copy of GPTEs	*/
	unsigned int ngpte;		/* Number of GPTE		*/
	unsigned int maxpix;		/* max process index		*/
	VOID_PQ sch$gl_pcbvec;		/* address of the PCB vector	*/
	PCB	*nullpcb;		/* ptr to the null PCB		*/
	unsigned long swppid;		/* IPID of swapper		*/
	unsigned int ngsusers;	/* No. of users of the global section	*/
	int ccode;
	int i;
	uint64 sva;

	if (page_size == 0)
	{
		/* get system page size in bytes		*/
		ccode = getvalue("MMG$GL_PAGE_SIZE",&page_size,sizeof(page_size));
		if (!(ccode & 1))
			return;
	}

	/* get the GPTEs for this global section	*/
	ccode = get_gpte(gste,&gpte,&ngpte);
	if (!(ccode & 1))
		return;

	/* get the max PCB index			*/
	ccode = getvalue("SCH$GL_MAXPIX",&maxpix,sizeof(maxpix));
	if (!(ccode & 1))
		return;

	/* get address of PCB vector			*/
	ccode = getvalue("SCH$GL_PCBVEC",&sch$gl_pcbvec, sizeof(sch$gl_pcbvec));
	if (!(ccode & 1))
		return;
	sext(&sch$gl_pcbvec);

	/* get the address of the null PCB	*/
	ccode = getvalue("SCH$GL_NULLPCB",&nullpcb, sizeof(nullpcb));
	if (!(ccode & 1))
		return;

	/* get swapper IPID			*/
	ccode = getvalue("SCH$GL_SWPPID",&swppid,sizeof(swppid));
	if (!(ccode & 1))
		return;

	/* get address of Process Page Table		*/
	ccode = getvalue("MMG$GQ_PT_BASE",&mmg$gq_pt_base,sizeof(mmg$gq_pt_base));
	if (!(ccode & 1))
		return;

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
		putmsg(GBLSEC__GETPCBVEC);
		sda$deallocate((void *)pcbvec, pcbvecsize);
		pcbvec = NULL;
		return;
	}

// to find the users of a global section then for each process in the 
// system the process page table has to be searched to find pte that match
// the gpte for the global section. If a matching pte is found then 
// the process does use the global section.
// A matching pte would either contain the gstx (if pte is invalid)
// or, if valid, contain a pfn matching a pfn in a gpte for the global section

	ngsusers = 0;	/* no users of this global section found yet	*/
	putmsg(GBLSEC__SEARCHING);

	/* for each entry in the PCB vector		*/
	for (i=0; i < maxpix; i++)
	{
		if (pcbvec[i] == nullpcb)
			continue;	/* skip unused entry	*/

		/* get the PCB		*/
		curpcb = pcbvec[i];
		ccode = sda$trymem(curpcb,&pcb,sizeof(pcb));
		if (!(ccode & 1))
			continue;	/* skip on error	*/

		if (pcb.pcb$l_pid == swppid)
			continue;	/* skip swapper		*/

		if (!(pcb.pcb$l_sts & PCB$M_PHDRES))
			continue;	/* skip if PHD not resident */

		/* get the PHD	*/
		curphd = pcb.pcb$l_phd;
		ccode = sda$trymem(curphd, &phd, PHD$K_LENGTH);
		if (!(ccode & 1))
			continue;	/* skip on error	*/

		/* set this process as current process		*/
		ccode = sda$set_process(NULL,0,(int)curpcb);
		if (ccode & 1)
		{
			/* search P0 region	*/
			ccode = search_region((RDE *)&phd.phd$q_p0_rde, gste->sec$l_vpx, gpte, ngpte, &sva);
		}
		if (!(ccode & 1))	/* if not found		*/
		{
			/* search P1 region	*/
			ccode = search_region((RDE *)&phd.phd$q_p1_rde, gste->sec$l_vpx, gpte, ngpte, &sva);
		}
		if (!(ccode & 1))	/* if not found		*/
		{
			/* search P2 region	*/
			ccode = search_region((RDE *)&phd.phd$q_p2_rde, gste->sec$l_vpx, gpte, ngpte, &sva);
		}
		if (!(ccode & 1))	/* if not found		*/
		{
			ccode = search_dynamic_regions(gste->sec$l_vpx, gpte, ngpte, &sva);
		}
		
		if (ccode & 1)		/* if found		*/
		{
			ngsusers++;
			sda$print("PID !8XL !AC mapped starting at !XQ",pcb.pcb$l_epid,pcb.pcb$t_lname,sva);
		}
	}

	sda$print("!UL processes found to be mapped to this global section",ngsusers);
}

/*
	display_help - display brief help
*/
static void
display_help(void)
{
	sda$print("GBLSEC SDA Extension - Displays information about a named global section");
	sda$print(" ");
	sda$print("Usage: GBLSEC section-name");
	sda$print("!_GBLSEC section-name /GROUP[=n]");
	sda$print("!_GBLSEC section-name /DELETE_PENDING");
	sda$print(" ");
	sda$print("Global section names containing lower case letters have to be specified in quotes e.g \"DnsClarkCache\"");
	sda$print(" ");
	sda$print("Data about the global section and the processes mapped to it is displayed");
	sda$print("including the filename (if any). To prevent lookup of the filename");
	sda$print("specify the /FID_ONLY qualifier.");
	sda$print(" ");
	sda$print("Specify /NOGSD to supress display of the global section descriptor");
	sda$print("Specify /NOGSTE to supress display of the Global Section Table Entry");
	sda$print("Specify /NOUSERS to not look for processes using this global section");
	sda$print(" ");
}

/*
	parse_cmd
*/
static int 
parse_cmd(struct dsc$descriptor_s *cmd_line, char *name, int max_len,
	int *group, int *flags)
{
	char value[256], cmd1[256], cmd2[256];
	$DESCRIPTOR(value_dsc,value);
	$DESCRIPTOR(cmd1_dsc,cmd1);
	$DESCRIPTOR(cmd2_dsc,cmd2);
	static $DESCRIPTOR(name_dsc,"NAME");
	static $DESCRIPTOR(group_dsc,"GROUP");
	static $DESCRIPTOR(delete_dsc,"DELETE_PENDING");
	static $DESCRIPTOR(fidonly_dsc, "FID_ONLY");
	static $DESCRIPTOR(gsd_dsc, "GSD");
	static $DESCRIPTOR(gste_dsc, "GSTE");
	static $DESCRIPTOR(users_dsc, "USERS");	
	static $DESCRIPTOR(all_dsc, "ALL");
	int ccode = SS$_NORMAL;	
	extern int *gblsec_cld;
	unsigned short vlen;

	*group = 0;
	*flags = 0;

        /* catch any exceptions         */
        VAXC$ESTABLISH(LIB$SIG_TO_RET);
 
	/* trim the command line	*/
	ccode = str$trim(&cmd1_dsc, cmd_line, &vlen);
	if (!(ccode & 1))
		return ccode;
	if (vlen == 0)
		return GBLSEC__NOCOMMAND; /* no command	*/

	cmd1_dsc.dsc$w_length = vlen;
	cmd1[vlen] = '\0';

	/* prefix command with GBLSEC to make parsing easier	*/
	strcpy(cmd2, "GBLSEC ");
	strcat(cmd2, cmd1);
	cmd2_dsc.dsc$w_length = strlen(cmd2);

	ccode = cli$dcl_parse(&cmd2_dsc, &gblsec_cld);
	if (!(ccode & 1))
		return ccode;
	ccode = cli$get_value(&name_dsc,&value_dsc, &vlen);
	if (!(ccode & 1))
		return ccode;
	if (vlen > max_len)
	{
		putmsg(GBLSEC__NTL);
		return GBLSEC__NTL;
	}
	memcpy(name,value_dsc.dsc$a_pointer,vlen);
	name[vlen] = '\0';

	/* check for group qualifier and get value if present	*/
	ccode = CLI$PRESENT(&group_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		*flags &= ~GBLSEC_M_GROUP;
	else if (ccode & 1)
	{
		*flags |= GBLSEC_M_GROUP;
		ccode = CLI$GET_VALUE(&group_dsc,&value_dsc,&vlen);
		if (ccode & 1)
			*group = strtoul(value_dsc.dsc$a_pointer,NULL,8);
	}

	/* check for delete_pending qualifier and set flag if present	*/
	ccode = CLI$PRESENT(&delete_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		*flags &= ~GBLSEC_M_DELETE;
	else if (ccode & 1)
		*flags |= GBLSEC_M_DELETE;
	
	/* check for fid_only qualifier and set flag if present	*/
	ccode = CLI$PRESENT(&fidonly_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		*flags &= ~GBLSEC_M_FIDONLY;
	else if (ccode & 1)
		*flags |= GBLSEC_M_FIDONLY;

	/* check for GSD qualifier and set flag if present	*/
	ccode = CLI$PRESENT(&gsd_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		*flags &= ~GBLSEC_M_GSD;
	else if (ccode & 1)
		*flags |= GBLSEC_M_GSD;

	/* check for gste qualifier and set flag if present	*/
	ccode = CLI$PRESENT(&gste_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		*flags &= ~GBLSEC_M_GSTE;
	else if (ccode & 1)
		*flags |= GBLSEC_M_GSTE;

	/* check for users qualifier and set flag if present	*/
	ccode = CLI$PRESENT(&users_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		*flags &= ~GBLSEC_M_USERS;
	else if (ccode & 1)
		*flags |= GBLSEC_M_USERS;

	/* check for all qualifier and set flag if present	*/
	ccode = CLI$PRESENT(&all_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		*flags &= ~GBLSEC_M_ALL;
	else if (ccode & 1)
		*flags |= GBLSEC_M_ALL;
	
	ccode = SS$_NORMAL;

	return ccode;
}

/*
	find global section by name	

	Input:
		gsname	- name of global section
		gsnamlen- length of name
		gsdaddr - 0 or address of previously found GSD

	Output:
		gsdbuf - copy of GSD

	Returns:
		TRUE if found, FALSE if not
*/
static int
find_gs(char *gsname, int gsnamelen, GSDBUF *gsdbuf, int cmd_flags, int group, uint64 *gsdaddr)
{
	int ccode, found;
	char	*gsnp;
	int 	gsnl;
	GSD	*gsdlist, *gsdptr;
	VOID_PQ	*exe$gl_gsdxxxfl;

	found = FALSE;

	/* get start of global section list	*/
	if (cmd_flags & GBLSEC_M_GROUP)
	{
		/* group list		*/
		ccode = sda$symbol_value("EXE$GL_GSDGRPFL",(uint64 *)&exe$gl_gsdxxxfl);
	}
	else if (cmd_flags & GBLSEC_M_DELETE)
	{	
		/* delete pending list	*/
		ccode = sda$symbol_value("EXE$GL_GSDDELFL",(uint64 *)&exe$gl_gsdxxxfl);
	}
	else
	{
		/* system list		*/
		ccode = sda$symbol_value("EXE$GL_GSDSYSFL",(uint64 *)&exe$gl_gsdxxxfl);
	}
	if (!(ccode & 1))
	{
		putmsg(GBLSEC__GSDERR);
		return FALSE;
	}
	gsdlist = (GSD *)exe$gl_gsdxxxfl;

	if (*gsdaddr == 0)
	{
		/* start GSD list	*/
		ccode = sda$trymem(gsdlist,&gsdptr,sizeof(gsdptr));
		gsdbuf->gsd.gsd$l_gsdfl = gsdptr;
	}
	else
	{
		/* continue from previous entry	*/
		gsdptr = gsdbuf->gsd.gsd$l_gsdfl;
	}

	while ((ccode & 1) && (gsdptr != gsdlist))
	{
		/* get GSD	*/
		ccode = sda$trymem(gsdptr,gsdbuf,sizeof(GSDBUF));
		if (!(ccode & 1))
			break;	/* give up on error	*/
		if (gsdbuf->gsd.gsd$l_flags & SEC$M_PFNMAP)
		{
			gsnp = &gsdbuf->gsd.gsd$t_pfngsdnam;
			gsnl = gsdbuf->gsd.gsd$t_pfngsdnam;
		}
		else
		{
			gsnp = &gsdbuf->gsd.gsd$t_gsdnam;
			gsnl = gsdbuf->gsd.gsd$t_gsdnam;
		}
		gsnp++;	/* skip length byte	*/
		if ((gsdbuf->gsd.gsd$b_type == DYN$C_GSD)
		&&  (gsnl == gsnamelen))
		{
			if ((group == 0)
			||  ((group != 0) && (group == gsdbuf->gsd.gsd$w_pcbgrp)))
			{
				/* compare names	*/
				// really here we should be using the hash value to speed this up
				if (memcmp(gsname,gsnp,gsnamelen) == 0)
				{
					found = TRUE;	/* eureka !	*/
					*gsdaddr = (uint64)gsdptr;
					break;
				}
			}
		}
		/* follow the forward pointer	*/
		gsdptr = gsdbuf->gsd.gsd$l_gsdfl;
	}

	return found;
}

/*
	display data from GSD
*/
static void 
display_gsd(GSDBUF *gsdbuf)
{
	int gsnamelen;
	char flags[200];
	char *gsnp;
	int gsnl;

	sda$print("GSD: Global Section Descriptor");
	if (gsdbuf->gsd.gsd$l_flags & SEC$M_PFNMAP)
	{
		gsnp = &gsdbuf->gsd.gsd$t_pfngsdnam;
		gsnl = gsdbuf->gsd.gsd$t_pfngsdnam;
	}
	else
	{
		gsnp = &gsdbuf->gsd.gsd$t_gsdnam;
		gsnl = gsdbuf->gsd.gsd$t_gsdnam;
	}
	gsnp++;	/* skip length byte	*/
	sda$print("GSName:!_!AF",gsnl,gsnp);

// could look for special names and interpret them here e.g. FSB$, GBL$
// INS$xxxxxx_nnn for installed images where xxxxxx is the KFE address 

	sda$print("PCBUIC:!_!%U", gsdbuf->gsd.gsd$l_pcbuic);
	sda$print("FILUIC:!_!%U", gsdbuf->gsd.gsd$l_filuic);
	sda$print("ORB:!_!8XL",gsdbuf->gsd.gsd$l_orb);
	sda$print("GSTX:!_!8XL",gsdbuf->gsd.gsd$l_gstx);
	sda$print("PROT:!_!8XL",gsdbuf->gsd.gsd$l_prot);
	sda$print("IDENT:!_!8XL",gsdbuf->gsd.gsd$l_ident);
	sda$print("IPID/RGSTX: !8XL",gsdbuf->gsd.gsd$l_ipid);
	sda$print("FLAGS:!_!8XL",gsdbuf->gsd.gsd$l_flags);
	display_sec_flags(gsdbuf->gsd.gsd$l_flags,flags);
	sda$print("!_!AZ",flags);
}

/*
	get_gste - get the global section table entry

	Input:
		gstx
	Output:
		gste
	returns: condition code
*/
static int 
get_gste(unsigned int gstx, SECDEF *gste,uint64 *gsteaddr)
{
	int ccode = SS$_NORMAL;
	uint64 pst;		/* address of gste		*/
	uint64 mmg$gl_sysphd;	/* address of system PHD	*/
	PHD sysphd;		/* fixed part of system PHD	*/

	/* get address of MMG$GL_SYSPHD	*/
	ccode = getvalue("MMG$GL_SYSPHD",&mmg$gl_sysphd,sizeof(mmg$gl_sysphd));
	if (!(ccode & 1))
		return ccode;

	sext((VOID_PQ *)&mmg$gl_sysphd);

	/* read the fixed part of the system PHD	*/
	ccode = sda$trymem((VOID_PQ)mmg$gl_sysphd,&sysphd,PHD$K_LENGTH);
	if (!(ccode & 1))
	{
		putmsg(GBLSEC__GETSYSPHD);
		return ccode;
	}
	pst = mmg$gl_sysphd + sysphd.phd$l_pst_base_offset
		- (SEC$C_LENGTH*gstx);

	ccode = sda$trymem((VOID_PQ)pst,gste,SEC$C_LENGTH);
	if (!(ccode & 1))
	{
		putmsg(GBLSEC__GETGSTE);
		return ccode;
	}
	*gsteaddr = pst;

	return ccode;
}

/*
	get_gpte - get GPTEs for the global sections

	Input:
		gste	- global section table entry

	Output:
		gpte	- array containing a copy of the GPTEs
		ngpte	- number of GPTE

	Returns:
		condition value
*/
static int 
get_gpte(SECDEF *gste, PTE **gpte, unsigned int *ngpte)
{
	int ccode;
	unsigned int ng;
	PTE_PQ gpt_base, gpte_addr;

	/* calcuate number of GPTE			*/
	if (gste->sec$l_flags & SEC$M_PFNMAP)
	{
		/* unit_cnt is number of physical pages	*/
		ng = *ngpte = gste->sec$l_unit_cnt;
	}
	else
	{
		/* unit_cnt is number of 512 byte pagelets */
		ng = *ngpte = gste->sec$l_unit_cnt / (page_size / 512);
	}	

	if (ng == 0)		
		ng = 1;
	sda$add_symbol("NGPTE",ng);

	/* get address of global page table		*/
	ccode = getvalue("MMG$GQ_GPT_BASE",&gpt_base,sizeof(gpt_base));
	if (!(ccode & 1))
		return ccode;

	sext((VOID_PQ *)&gpt_base);

	/* get address of GPTEs				*/
	gpte_addr = gpt_base + gste->sec$l_vpx;

	sda$add_symbol("GPTE",(uint64)gpte_addr);
	
	/* allocate array to hold the GPTEs	*/	
	sda$allocate(ng * sizeof(PTE),(void *)gpte);

	/* get the GPTEs			*/
	ccode = sda$trymem((VOID_PQ)gpte_addr,*gpte,ng*sizeof(PTE));
	if (!(ccode & 1))
	{
		putmsg(GBLSEC__GETGPTE);
		sda$deallocate((void *)*gpte, ng*sizeof(PTE));
		*gpte = NULL;
	}

	return ccode;
}

/*
	display_gste
*/
static void 
display_gste(SECDEF *gste)
{
	char buf[200];

	sda$print("GSTE: Global Section Table Entry");
	sda$print("GSD:!_!8XL",gste->sec$l_gsd);
	sda$print("PFC:!_!8UL.",gste->sec$l_pfc);
	sda$print("WINDOW:!_!8XL",gste->sec$l_window);
	sda$print("VBN:!_!8UL",gste->sec$l_vbn);
	sda$print("REFCNT:!_!8UL.",gste->sec$l_refcnt);
	sda$print("UNITCNT:!8UL.",gste->sec$l_unit_cnt);
	sda$print("VPX:!_!8UL.",gste->sec$l_vpx);
	sda$print("FLAGS:!_!8XL",gste->sec$l_flags);
	display_sec_flags(gste->sec$l_flags,buf);
	sda$print("!_!AZ",buf);
}

/*
	display_sec_flags - display interpretation of section flags
*/
static void
display_sec_flags(unsigned int flags, char *buf)
{
	static const char *mod[4] = {"KERNEL","EXEC","SUPER","USER"};
	int wrtmod, amod;	

	wrtmod = (flags >> 6) & 3;
	amod = (flags >> 8) & 3;

	sprintf(buf,"WRTMOD = %s, AMOD = %s,",
		mod[wrtmod],mod[amod]);

	if (flags & SEC$M_GBL)
		strcat(buf," GBL");
	if (flags & SEC$M_CRF)
		strcat(buf," CRF");
	if (flags & SEC$M_DZRO)
		strcat(buf," DZERO");
	if (flags & SEC$M_WRT)
		strcat(buf," WRT");
	if (flags & SEC$M_SHMGS)
		strcat(buf," SHMGS");
	if (flags & SEC$M_READ_ONLY_SHPT)
		strcat(buf," READ_ONLY_SHPT");
	if (flags & SEC$M_SHARED_PTS)
		strcat(buf," SHARED_PTS");
	if (flags & SEC$M_MRES)
		strcat(buf," MRES");
	if (flags & SEC$M_MRES_ALLOC)
		strcat(buf," MRES_ALLOC");
	if (flags & SEC$M_PROTECT)
		strcat(buf," PROTECT");
	if (flags & SEC$M_PERM)
		strcat(buf," PERM");
	if (flags & SEC$M_PFNMAP)
		strcat(buf," PFNMAP");
	if (flags & SEC$M_PAGFIL)
		strcat(buf," PAGFIL");
	if (flags & SEC$M_SYSGBL)
		strcat(buf," SYS");
	else
		strcat(buf," GRP");
}

/*
	display_file_info - display filename for section
*/
static void
display_file_info(SECDEF *sec, int cmd_flags)
{
	WCB wcb, *wcbptr;
	FCB fcb, *fcbptr;
	UCB *ucbptr;
	int ccode;
	char devname[32];
	$DESCRIPTOR(devname_dsc,devname);
	char fs[4096];
	unsigned short fslen;
	$DESCRIPTOR(fs_dsc,fs);
	unsigned long acp_status;

	/* sanity check	*/
	if (sec->sec$l_window == 0)
		return;		/* nothing to do	*/

	/* get the WCB	*/
	wcbptr = (WCB *)sec->sec$l_window;
	ccode = sda$trymem(wcbptr,&wcb,sizeof(wcb));
	if (!(ccode & 1))
		return;
	sda$add_symbol("WCB",(uint64)wcbptr);

	/* get the FCB	*/
	fcbptr = wcb.wcb$l_fcb;
	ccode = sda$trymem(fcbptr,&fcb,sizeof(fcb));
	if (!(ccode & 1))
		return;
	sda$add_symbol("FCB",(uint64)fcbptr);

	/* get the device name using the UCB ptr in the WCB	*/
	ucbptr = wcb.wcb$l_orgucb;
	sda$add_symbol("UCB",(uint64)ucbptr);

#ifdef sda$get_device_name
	ccode = sda$get_device_name(ucbptr, devname, sizeof(devname));

	if (ccode & 1)
	{
		/* get the filename from the fid and device name	*/
		/* issues with this are					*/
		/* 1. we could be looking at a dump from another system	*/
		/* 2. we could be looking at the live system and a file	*/
		/*    system lock could block us 			*/
		/* hence the FID_ONLY qualifier can be used		*/

		if (cmd_flags & GBLSEC_M_FIDONLY)
		{
			ccode = 0;	/* didn't lookup the fid	*/
		}
		else
		{
			devname_dsc.dsc$w_length = strlen(devname);
			ccode = lib$fid_to_name(&devname_dsc,fcb.fcb$w_fid,&fs_dsc,&fslen,NULL,&acp_status);
		}
		if (ccode & 1)
		{
			fs[fslen] = '\0';
			sda$print("File:!_!AZ",fs);
		}
		else
		{
			sda$print("File:!_!AZ (!UW,!UW,!UW)",devname,
				fcb.fcb$w_fid[0],fcb.fcb$w_fid[1],fcb.fcb$w_fid[2]);
		}
	}		
#endif
}

/*
	get the value at a named location	
*/
static int 
getvalue(char *name, void *vp, int size)
{
	int ccode;
	VOID_PQ ptr;

	/* translate the name to an address	*/
	ccode = sda$symbol_value(name,(uint64 *)&ptr);
	if (ccode & 1)
	{
		/* get the value at that address	*/
		ccode = sda$trymem(ptr,vp,size);
		if (!(ccode & 1))
		{
			putmsg(GBLSEC__GETVALUE);
		}
	}
	else
	{
		putmsg(GBLSEC__GETVALUEA);
	}
	return ccode;
}

/*
	sext - sign extend an address
*/
static void
sext(VOID_PQ *ap)
{
	GENERIC_64 a;

	a.gen64$q_quadword = (uint64)*ap;

	if (a.gen64$l_longword[1] == 0)
	{
		if (a.gen64$l_longword[0] & 0x80000000)
			a.gen64$l_longword[1] = 0xFFFFFFFF;
		else
			a.gen64$l_longword[1] = 0;
		*ap = (VOID_PQ)a.gen64$q_quadword;
	}
}

/*
	search a region of the process page table to see if
	any PTE match the global section

	rde	region descriptor
	gptx	global section table entry index
	gpte	GPTEs
	ngpte	number of GPTE

	returns success condition code if found

*/
static int
search_region(RDE *rde, int gptx, PTE *gpte,int ngpte,uint64 *sva)
{
  	static uint64 mmg$gq_l2_base;
  	static uint64 mmg$gq_non_va_mask;
  	static uint64 mmg$gq_non_pt_mask;
  	static uint64 mmg$gq_level_width;
  	int vrnx;
	int found;
	uint64	va_pte;
	VA v;
	uint64	start, end, a, npte, b2;
	unsigned int n1, n2, n;
	int j, g;
	PTE	*pte;	/* ptr to array containing copy of PTEs	*/
	PTE	l2pte;
	unsigned int pptsize;	/* size of PPT			*/
	uint64 pt_base;		/* base addr of PPT		*/
	int	ccode;

	if (mmg$gq_l2_base == 0)
	{
		/* get address of L2 Page Table		*/
		ccode = getvalue("MMG$GQ_L2_BASE",&mmg$gq_l2_base,sizeof(mmg$gq_l2_base));
		if (!(ccode & 1))
			return ccode;
	}

	if (mmg$gq_non_va_mask == 0)
	{
		ccode = getvalue("MMG$GQ_NON_VA_MASK",&mmg$gq_non_va_mask,sizeof(mmg$gq_non_va_mask));
		if (!(ccode & 1))
			return ccode;
	}

	if (mmg$gq_non_pt_mask == 0)
	{
		ccode = getvalue("MMG$GQ_NON_PT_MASK",&mmg$gq_non_pt_mask,sizeof(mmg$gq_non_pt_mask));
		if (!(ccode & 1))
			return ccode;
	}

	if (mmg$gq_level_width == 0)
	{
		ccode = getvalue("MMG$GQ_LEVEL_WIDTH",&mmg$gq_level_width,sizeof(mmg$gq_level_width));
		if (!(ccode & 1))
			return ccode;
	}

	/* determine start and end address		*/
	if (rde->rde$v_descend)
	{
		/* calculate how many PTEs 		*/
		npte = (((uint64)rde->rde$pq_start_va + rde->rde$q_region_size - 1) - (uint64)rde->rde$pq_first_free_va)/page_size;

		start = (uint64)rde->rde$pq_first_free_va + 1;

		end = (uint64)rde->rde$pq_start_va + rde->rde$q_region_size;

	}
	else
	{
		/* calculate how many PTEs 		*/
		npte = ((uint64)rde->rde$pq_first_free_va - (uint64)rde->rde$pq_start_va)/page_size;

		start = (uint64)rde->rde$pq_start_va;

		end = (uint64)rde->rde$pq_first_free_va;
	}


	if (npte == 0)
		return GBLSEC__NOTFOUND; /* nothing to look at	*/


	n1 = page_size / PTE$C_BYTES_PER_PTE;	/* number of L3 PTE in a page	*/
	n2 = n1 * n1;		/* number of PTE mapped by a L2 page*/
	b2 = n2 * PTE$C_BYTES_PER_PTE;

	/* allocate a buffer to hold L3 PTE 	*/
	pptsize = n1*page_size;
	sda$allocate(pptsize, (void *)&pte);

	found = FALSE;

	/* loop through the L3 PTE n1 entries at a time	*/
	for (a = start; a < end && !found; a += b2,npte -= n)
	{
		/* round address to start of L2 boundry	*/
		a = (a/b2)*b2;

		/* get L2 PTE				*/
#ifdef VA$C_VRNX_COUNT
		v.va$q_quad = a;
		vrnx = v.va$v_vrnx;
		va_pte = mmg$gq_l2_base + vrnx*PTE$C_BYTES_PER_PTE + ((a & ~mmg$gq_non_va_mask) >> 2*mmg$gq_level_width) 
			& (uint64) ~(PTE$C_BYTES_PER_PTE-1);
#else
		va_pte = mmg$gq_l2_base + (((a & ~mmg$gq_non_va_mask) >> 2*mmg$gq_level_width) 
			& (uint64) ~(PTE$C_BYTES_PER_PTE-1));
#endif
		ccode = sda$trymem((VOID_PQ)va_pte, &l2pte, sizeof(PTE));
		if (!(ccode & 1))
		{
			putmsg(GBLSEC__GETPPT);
			sda$deallocate((void *)pte, pptsize);
			return ccode;
		}               


		/* if L2 PTE not valid then skip	*/
		if (!l2pte.pte$v_valid)
			continue;

#ifdef VA$C_VRNX_COUNT
		/* determine base address of L3 PTEs	*/
		pt_base = mmg$gq_pt_base + vrnx*PTE$C_BYTES_PER_PTE + ((a & ~mmg$gq_non_pt_mask) >> mmg$gq_level_width);
#else
		pt_base = mmg$gq_pt_base + ((a & ~mmg$gq_non_pt_mask) >> mmg$gq_level_width);
#endif		
		n = (npte > n1)?n1:npte;

		/* get L3 PTEs 				*/
		ccode = sda$trymem((VOID_PQ)pt_base, pte, n*PTE$C_BYTES_PER_PTE);
		if (!(ccode & 1))
		{
			putmsg(GBLSEC__GETPPT);
			sda$deallocate((void *)pte, pptsize);
			return ccode;
		}               

		/* for each pte in the page table		*/
		for (j=0; (j < n) && !found; j++)
		{
			/* check for match with global section	*/
			/* if a valid PTE			*/
			if (pte[j].pte$v_valid)
			{
				/* here we need to compare the pfn against the gptes	*/
				for (g=0;g < ngpte; g++)
				{
					if ((gpte[g].pte$v_valid)
					&&  (pte[j].pte$v_pfn == gpte[g].pte$v_pfn))
					{
						found = TRUE;	/* eureka !	*/
						break;		/* stop looking	*/
					}
				}
			}
			else if ((pte[j].pte$v_typ0)		/* if type0	*/
			&&  (!pte[j].pte$v_partial_section)	/* not partial	*/
			&&  (!pte[j].pte$v_typ1)		/* not type1	*/
			&&  (!pte[j].pte$v_s0_mbz)
			&&  (pte[j].pte$v_gptx == gptx))
			{
				found = TRUE;	/* eureka !	*/
				break;		/* stop looking	*/
			}
		}
	}
        /* free PPT buffer	*/
	sda$deallocate((void *)pte, pptsize);

	if (found)
	{
		*sva = (uint64)rde->rde$pq_start_va + (j-1)*page_size;
		ccode = GBLSEC__NORMAL;
	}
	else
	{
		ccode = GBLSEC__NOTFOUND;
	}

	return ccode;
}

/*
	search_dynamic_regions - search all process dynamic regions
	 to see of any PTE match the gptx.

	returns condition code
*/
static int 
search_dynamic_regions(int gptx, PTE *gpte,int ngpte, uint64 *sva)
{
	int ccode;
	int offset;
	PHD	phd;
	static PHD *ctl$gl_phd = NULL;
	RDE    *rdelisthead;

	if (ctl$gl_phd == NULL)
	{
		/* get P1 address of PHD */
		ccode = getvalue("CTL$GL_PHD",&ctl$gl_phd,sizeof(ctl$gl_phd));
		if (!(ccode & 1))
			return ccode;
	}

	/* determine address of P0 dynamic region descriptor listhead	*/	
	offset = (char *)&phd.phd$ps_p0_va_list_flink - (char *)&phd;
	rdelisthead = (RDE *)((char *)ctl$gl_phd + offset);

        /* search P0 dynamic regions					*/
	ccode = search_dyn_region_list(rdelisthead, gptx, gpte, ngpte, sva);
	if (ccode & 1)
		return ccode;	/* eureka !				*/

	/* determine address of P1 dynamic region descriptor listhead	*/	
	offset = (char *)&phd.phd$ps_p1_va_list_flink - (char *)&phd;
	rdelisthead = (RDE *)((char *)ctl$gl_phd + offset);

        /* search P1 dynamic regions					*/
	ccode = search_dyn_region_list(rdelisthead, gptx, gpte, ngpte, sva);
	if (ccode & 1)
		return ccode;	/* eureka !				*/

	/* determine address of P2 dynamic region descriptor listhead	*/	
	offset = (char *)&phd.phd$ps_p2_va_list_flink - (char *)&phd;
	rdelisthead = (RDE *)((char *)ctl$gl_phd + offset);

        /* search P2 dynamic regions					*/
	ccode = search_dyn_region_list(rdelisthead, gptx, gpte, ngpte, sva);
	if (ccode & 1)
		return ccode;	/* eureka !				*/

	return ccode;
}

/*
	search_dyn_region_list - search a list of regions for matching PTE

*/	
static int 
search_dyn_region_list(RDE *rdelisthead, int gptx, PTE *gpte,int ngpte, uint64 *sva)
{
	int ccode, ccode2;
	RDE rde, *rdeaddr;

	/* read listhead	*/
	ccode = sda$trymem((VOID_PQ)rdelisthead,&rde,sizeof(RDE));
	if (!(ccode & 1))
		return ccode;
	
	ccode = GBLSEC__NOTFOUND;
	rdeaddr = rde.rde$ps_va_list_flink;	
	while (rdeaddr != rdelisthead && ccode == GBLSEC__NOTFOUND)
	{
		if (rdeaddr == rde.rde$ps_va_list_flink)
			break;
		ccode2 = sda$trymem((VOID_PQ)rdelisthead,&rde,sizeof(RDE));
		if (!(ccode2 & 1))
			break;
		if (rde.rde$q_region_id < RDE$C_MIN_USER_ID)
			break;	/* invalid region id	*/
		if ((rde.rde$b_type != DYN$C_MISC)
		||  (rde.rde$b_subtype != DYN$C_RDE))
			break;	/* invalid RDE		*/
		ccode = search_region(&rde, gptx, gpte, ngpte, sva);
		rdeaddr = rde.rde$ps_va_list_flink;	
	}

	return ccode;
}

static int
put_output(struct dsc$descriptor_s *msg_dsc, unsigned long arg)
{
	char line[133];
	int len;

	len = (msg_dsc->dsc$w_length < sizeof(line)-1)?msg_dsc->dsc$w_length:sizeof(line)-1;

	memcpy(line,msg_dsc->dsc$a_pointer,len);
	line[len] = '\0';

	sda$print(line);
	
	return 0;	/* no output to SYS$OUTPUT	*/
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

	SYS$PUTMSG(args,(void(*)())put_output,0,0);
}
                                                                                                   