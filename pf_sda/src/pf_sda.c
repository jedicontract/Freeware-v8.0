/*
	PF$SDA	SDA extension to display which processes are using a pagefile

 COPYRIGHT NOTICE

 This software is COPYRIGHT (c) 2006, Ian Miller. ALL RIGHTS RESERVED.

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
#define VERSION "V1.1"
#pragma module PF$SDA VERSION

#define __NEW_STARLET 1
#include <arch_defs.h>
#include <varargs.h>
#include <descrip.h>
#include <dmpdef.h>
#include <gen64def.h>
#include <ints.h>
#include <lib$routines.h>
#include <libdtdef.h>
#include <pcbdef.h>
#include <phddef.h>
#include <dscdef.h>
#include <rdedef.h>
#include <ptrdef.h>
#include <pfldef.h>
#include <dyndef.h>
#include <sda_routines.h>
#include <ssdef.h>
#include <rmsdef.h>
#include <stsdef.h>
#include <starlet.h>
#include <ptedef.h>
#include <vadef.h>
#include <pfndef.h>
#include <cli$routines.h>
#include <str$routines.h>
#include <climsgdef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <wcbdef.h>
#include <ucbdef.h>
#include <fcbdef.h>
#include <syspardef.h>

/* define symbol according to if we have the page file indices in the
   PHD or not (Alpha VMS 7.3 and later they are not).

   NEWPF is defined to be 1 if not.
*/
#ifdef phd$b_pagfil
#define NEWPF 0
#else
#define NEWPF 1
#endif

/*  Global variables	*/
int	sda$extend_version = SDA_FLAGS$K_VERSION;

/* local variables	*/
static SDA_FLAGS flags;

static unsigned long page_size;	/* = MMG$GQ_PAGE_SIZE 		*/
static unsigned int pcbvecsize;	/* PCB vector size		*/
static PCB	**pcbvec = NULL;/* ptr to copy of PCB vector	*/
static uint64 mmg$gq_pt_base;	/* base address of PPT		*/

static int cmd_flags;		/* command flags		*/
#define PF_M_FID_ONLY	2	/* /FID_ONLY qualifier		*/
#if NEWPF
#define PF_M_COUNT	1	/* /COUNT qualifier specified	*/
#define PF_M_LOCATION	4	/* /LOCATION qualifier		*/
#define PF_M_GLOBAL	8	/* /GLOBAL qualifier		*/
#endif

#define PF_C_BYTES_PER_BLOCK	512
           
/* local routines	*/
static void display_help(void);
static void sext(VOID_PQ *ap);
static void putmsg();
static int getvalue(char *name, void *vp, int size);
static int parse_cmd(struct dsc$descriptor_s *cmd_line, int *pgflx);
static int display_pf_info(int pgflx, PFL *pfl, int *refcnt);
static void display_pf_users(int pgflx, PFL *pfl);
#if NEWPF
static int search_region(RDE *rde, int pgflx, PFL *pfl, uint64 *count);
static int search_gpt(int pgflx,PFL *pfl);
static int search_addr_range(uint64 start, uint64 end, uint64 npte, int pgflx, PFL *pfl, uint64 *count);
static int search_ptes(PTE *pte, int n, int pgflx, PFL *pfl, uint64 *count);
static int get_pfn(unsigned long pfn, PFN *pfnentry);
static int search_dynamic_regions(int pgflx, PFL *pfl, uint64 *count);
static int search_dyn_region_list(RDE *rdelisthead, int pgflx, PFL *pfl, uint64 *count);
#endif

/* message codes	*/
globalvalue PF__NORMAL;
globalvalue PF__WARNING;
globalvalue PF__SYNTAX;
globalvalue PF__FATAL;
globalvalue PF__BUG;
globalvalue PF__GETVALUE;
globalvalue PF__GETVALUEA;
globalvalue PF__GETPCBVEC;
globalvalue PF__GETSYSPHD;
globalvalue PF__GETSPTE;
globalvalue PF__GETPPT;
globalvalue PF__INVPFI;
globalvalue PF__NOTINS;
globalvalue PF__GETPFN;
globalvalue PF__GETPSVECA;
globalvalue PF__GETPSVEC;
globalvalue PF__INVPSVEC;
globalvalue PF__GETPFL;
globalvalue PF__GETNULPFL;
globalvalue PF__INVPFL;
globalvalue PF__NOTINUSE;
globalvalue PF__LOOKING;
globalvalue PF__USERS;
globalvalue PF__READSYM;
globalvalue PF__NOTFOUND;
globalvalue PF__NOCOMMAND;
globalvalue PF__INFO;
globalvalue PF__PFREFS;
globalvalue PF__PFTOTALP;
globalvalue PF__PFTOTAL;
globalvalue PF__TOTALSWAP;
globalvalue PF__TOTALPAGE;
globalvalue PF__TOTALREFS;
globalvalue PF__TOTALALLO;
globalvalue PF__PINUSE;
globalvalue PF__GBLSEARCH;
globalvalue PF__GBLPAGES;

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
	int	ccode;
	static int sysdef_flag=0;	/* TRUE if SYSDEF.STB has been read 	*/
	int	pgflx, refcnt;
	PFL	pfl;	/* page file control block */
#if NEWPF
	uint64	mmg$gq_swapfile_pages, mmg$gq_pagefile_pages, mmg$gq_pagefile_refs, mmg$gq_pagefile_allocs;
#endif
	SYS_FLAGS exe$gl_flags;
	float pinuse;
	char pbuf[50];
                                                    
	/* Initialize the table and establish the condition handler */
	sda$vector_table = transfer_table;
	lib$establish(sda$cond_handler);
	flags = sda_flags;

	sda$print("PF !AZ (c) 2006, Ian Miller (miller@encompasserve.org) built on VMS !AZ",
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
	ccode = parse_cmd(cmd_line, &pgflx);
	if (ccode == PF__NOCOMMAND)
	{
		/* no parameters - give brief help	*/
		display_help();
		return;		/* exit			*/
	}
	if (!(ccode & 1))	/* if error with cmd	*/
	{
		putmsg(ccode);	/* report the bad news	*/
		return;		/* exit			*/
	}

	/* add useful stuff to the symbol table if we have not done it already*/
	if (!sysdef_flag)
	{
	  	ccode = sda$read_symfile("SDA$READ_DIR:SYSDEF", SDA_OPT$M_READ_NOLOG );
	  	if (!(ccode & 1))
	    	{
			putmsg(PF__READSYM,ccode);
	    		return;
		}
	  	sysdef_flag = 1;
	}

	/* get system page size in bytes		*/
	ccode = getvalue("MMG$GL_PAGE_SIZE",&page_size,sizeof(page_size));
	if (!(ccode & 1))
		return;

	/* display information about the pagefile	*/
	ccode = display_pf_info(pgflx, &pfl, &refcnt);
	if (!(ccode & 1))
		return;

	/* if pagefile is used				*/
	if (refcnt != 0)
	{
		display_pf_users(pgflx,&pfl);	/* display names of the processes	*/
#ifdef PF_M_GLOBAL
		if (cmd_flags & PF_M_GLOBAL)
			search_gpt(pgflx,&pfl);	/* search for global pages in pagefile	*/
#endif
//
// what about pagedyn (paged pool) pages? It is possible that a page in paged pool has space allocated in 
// this page file.
// Paged parts of the exec ?
// 
	}
	else
	{
		/* if no pages are in use there are no users to find	*/
		putmsg(PF__NOTINUSE);
	}
#if NEWPF
	/* display some system wide totals	*/
	ccode = getvalue("MMG$GQ_SWAPFILE_PAGES",&mmg$gq_swapfile_pages,sizeof(mmg$gq_swapfile_pages));
	if (ccode & 1)
		putmsg(PF__TOTALSWAP,mmg$gq_swapfile_pages);
	ccode = getvalue("MMG$GQ_PAGEFILE_PAGES",&mmg$gq_pagefile_pages,sizeof(mmg$gq_pagefile_pages));
	if (ccode & 1)
		putmsg(PF__TOTALPAGE,mmg$gq_pagefile_pages);
	ccode = getvalue("MMG$GQ_PAGEFILE_REFS",&mmg$gq_pagefile_refs,sizeof(mmg$gq_pagefile_refs));
	if (ccode & 1)                                                  
		putmsg(PF__TOTALREFS,mmg$gq_pagefile_refs);
	ccode = getvalue("MMG$GQ_PAGEFILE_ALLOCS",&mmg$gq_pagefile_allocs,sizeof(mmg$gq_pagefile_allocs));
	if (ccode & 1)                                                  
		putmsg(PF__TOTALALLO,mmg$gq_pagefile_allocs);
	pinuse = (100.0*mmg$gq_pagefile_allocs)/mmg$gq_pagefile_pages;
	sprintf(pbuf,"%2.2f",pinuse); 	
	putmsg(PF__PINUSE,pbuf);
#endif
	ccode = getvalue("EXE$GL_FLAGS",&exe$gl_flags,sizeof(exe$gl_flags));
	if (ccode & 1)
	{
		if (exe$gl_flags.exe$v_pgflfrag)
			sda$print("%SYSTEM-W-PAGEFRAG, page file filling up");
		if (exe$gl_flags.exe$v_pgflcrit)
			sda$print("%SYSTEM-W-PAGECRIT, page file nearly full");
	}
}

/*
	process the command line

	cmd_line	- commmand line by descriptor
	pgflx		- page file index by reference

	returns a condition code
*/
static int 
parse_cmd(struct dsc$descriptor_s *cmd_line, int *pgflx)
{
	char value[256], cmd1[256], cmd2[256], ibuf[256];
	$DESCRIPTOR(value_dsc,value);
	$DESCRIPTOR(cmd1_dsc,cmd1);
	$DESCRIPTOR(cmd2_dsc,cmd2);
	static $DESCRIPTOR(index_dsc,"INDEX");
	static $DESCRIPTOR(countq_dsc,"COUNT");
	static $DESCRIPTOR(globalq_dsc,"GLOBAL");
	static $DESCRIPTOR(locq_dsc,"LOCATION");
	static $DESCRIPTOR(fidoq_dsc,"FID_ONLY");
	int ccode = SS$_NORMAL;	
	extern int *pf_cld;
	unsigned short vlen, len;

        /* catch any exceptions         */
        VAXC$ESTABLISH(LIB$SIG_TO_RET);
 

	cmd_flags = 0;

	/* trim the command line	*/
	ccode = str$trim(&cmd1_dsc, cmd_line, &vlen);
	if (!(ccode & 1))
		return ccode;
	if (vlen == 0)
		return PF__NOCOMMAND;	/* no command	*/
	cmd1_dsc.dsc$w_length = vlen;
	cmd1[vlen] = '\0';	

	/* prefix command with PF to make parsing easier	*/
	strcpy(cmd2, "PF ");
	strcat(cmd2, cmd1);
	cmd2_dsc.dsc$w_length = strlen(cmd2);

	/* parse command			*/
	ccode = cli$dcl_parse(&cmd2_dsc, &pf_cld);
	if (!(ccode & 1))
		return ccode;

#ifdef PF_M_COUNT
	/* check for COUNT qualifier		*/
	ccode = CLI$PRESENT(&countq_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		cmd_flags &= ~PF_M_COUNT;
	else if (ccode & 1)
	{
		cmd_flags |= PF_M_COUNT;
	}
#endif
#ifdef PF_M_GLOBAL
	/* check for GLOBAL qualifier		*/
	ccode = CLI$PRESENT(&globalq_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		cmd_flags &= ~PF_M_GLOBAL;
	else if (ccode & 1)
	{
		cmd_flags |= PF_M_GLOBAL;
	}
#endif
#ifdef PF_M_LOCATION
	/* check for LOCATION qualifier		*/
	ccode = CLI$PRESENT(&locq_dsc);
      	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		cmd_flags &= ~PF_M_LOCATION;
	else if (ccode & 1)
	{
		cmd_flags |= PF_M_LOCATION;
		cmd_flags |= PF_M_COUNT; 	/* need this to scan all pages	*/
	}
#endif

	/* check for FID_ONLY qualifier		*/
	ccode = CLI$PRESENT(&fidoq_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
	{
		cmd_flags &= ~PF_M_FID_ONLY;
	}
	else if (ccode & 1)
	{
		cmd_flags |= PF_M_FID_ONLY;
	}

	/* get page file index parameter	*/
	ccode = cli$get_value(&index_dsc,&value_dsc, &vlen);
	if (!(ccode & 1))
		return ccode;

	/* add null terminator to param value	*/
	len = (vlen<sizeof(ibuf))?vlen:sizeof(ibuf)-1;
	memcpy(ibuf, value_dsc.dsc$a_pointer, len);
	ibuf[len] = '\0';

	/* get parameter value			*/
	*pgflx = atoi(ibuf);

	/* range check pagefile index	*/
	if ((*pgflx > 0) && (*pgflx < 255))
		ccode = PF__NORMAL;
	else
		ccode = PF__INVPFI;

	return ccode;
}

/*
	display_help - display brief help
*/
static void
display_help(void)
{
	sda$print(" ");
	sda$print("PF SDA Extension - displays which processes are using a specified page file");
	sda$print(" ");
	sda$print("Usage: PF pagefileindex [/qualifiers]");
	sda$print(" ");
	sda$print("pagefileindex can be found by the SHOW MEMORY/FILE command or CLUE MEMORY/FILES");
	sda$print(" ");
	sda$print("Qualifiers:");
#ifdef PF_M_COUNT
	sda$print("/COUNT  to count the number of pages used by each process in the file");
#endif
#ifdef PF_M_LOCATION
	sda$print("/LOCATION to display where in the file the pages are");
#endif
#ifdef PF_M_GLOBAL
	sda$print("/GLOBAL to search for global pages in the pagefile");
#endif
      	sda$print("/FID_ONLY prevents attempting to determine filename from file id");
	sda$print(" ");
}

/*
	display information about a pagefile
*/
static int
display_pf_info(int pgflx, PFL *pfl, int *refcnt)
{
	int ccode;
#if NEWPF
#else
	unsigned long mmg$gl_pagswpvc;
#endif
	VOID_PQ mmg$gpq_page_swap_vector;
	VOID_PQ mmg$ar_nullpfl;
	struct
	{
		PTR hdr;
		unsigned long vec[254];
	} psvec;	/* buffer for ps vector	*/
	unsigned long *vp;
	VOID_PQ pfladdr, wcbaddr, ucbaddr, fcbaddr;
	char pfbuf[256];
	WCB wcb;
	FCB fcb;
	char device_name[32];
	$DESCRIPTOR(devname_dsc,device_name);
	char ftype[5];
	char fs[4096];
	unsigned short fslen;
	$DESCRIPTOR(fs_dsc,fs);
	unsigned long acp_status;
	unsigned int b;

	/* range check pagefile index	*/
	if ((pgflx < 1) && (pgflx > 254))
		return PF__INVPFI;

	ccode = getvalue("MMG$AR_NULLPFL",&mmg$ar_nullpfl,sizeof(mmg$ar_nullpfl));
	if (!(ccode & 1))
	{
		putmsg(PF__GETNULPFL);
		return ccode;
	}
	sext(&mmg$ar_nullpfl);
#if NEWPF
	/* get address of page and swap file vector	*/
	ccode = getvalue("MMG$GPQ_PAGE_SWAP_VECTOR",&mmg$gpq_page_swap_vector,sizeof(mmg$gpq_page_swap_vector));
	if (!(ccode & 1))
	{
		putmsg(PF__GETPSVECA);
		return ccode;
	}
#else
	/* get address of page and swap file vector	*/
	ccode = getvalue("MMG$GL_PAGSWPVC",&mmg$gl_pagswpvc,sizeof(mmg$gl_pagswpvc));
	if (!(ccode & 1))
	{
		putmsg(PF__GETPSVECA);
		return ccode;
	}
	mmg$gl_pagswpvc -= 16;	/* backup to addr of header */
	mmg$gpq_page_swap_vector = (VOID_PQ)mmg$gl_pagswpvc;
#endif
	sext(&mmg$gpq_page_swap_vector);

	/* get the page and swap file vector header	*/
	ccode = sda$trymem(mmg$gpq_page_swap_vector, &psvec.hdr, sizeof(psvec.hdr));
	if (!(ccode & 1))
	{
		putmsg(PF__GETPSVEC);
		return ccode;
	}

	/* validate psvec hdr	*/
	if ((psvec.hdr.ptr$b_type != DYN$C_PTR)
	||  (psvec.hdr.ptr$b_ptrtype != DYN$C_PFL)
#ifdef ptr$l_info_long1 
	||  (psvec.hdr.ptr$l_info_long1 > 255)

#endif
           )
	{
		putmsg(PF__INVPSVEC);
		return PF__INVPSVEC;
	}

	/* range check pagefile index	*/
	if (pgflx >= psvec.hdr.ptr$l_ptrcnt)
	{
		putmsg(PF__INVPFI);
		return PF__INVPFI;
	}

#ifdef ptr$l_info_long1 
	/* check a pagefile with the specified index is installed	*/
	if ((pgflx < psvec.hdr.ptr$l_info_long1)
	&&  (pgflx > psvec.hdr.ptr$l_info_long0))
	{
		putmsg(PF__NOTINS);
		return PF__NOTINS;
	}
#endif

	/* get the whole page and swap file vector	*/
	ccode = sda$trymem(mmg$gpq_page_swap_vector, &psvec, 
		psvec.hdr.ptr$w_size);
	if (!(ccode & 1))
	{
		putmsg(PF__GETPSVEC);
		return ccode;
	}

	/* determine address of pagefile control block			*/
	vp = (unsigned long *)&psvec.hdr.ptr$l_ptr0;
	pfladdr = (VOID_PQ)(vp[pgflx]);
	sext(&pfladdr);

	sda$add_symbol("PFL",(uint64)pfladdr);

	/* if PFL address is address of null PFL 	*/
	/* then the page file is not installed		*/
	if (pfladdr == mmg$ar_nullpfl)
	{
		putmsg(PF__NOTINS);
		return PF__NOTINS;
	}

	/* get the page file control block		*/
	ccode = sda$trymem(pfladdr, pfl, sizeof(PFL));
	if (!(ccode & 1))
	{
		putmsg(PF__GETPFL);
		return PF__GETPFL;
	}

	/* sanity check page file control block		*/
	if ((pfl->pfl$b_type != DYN$C_PFL)
	||  (pfl->pfl$l_pgflx != pgflx)
	   )
	{
		putmsg(PF__INVPFL,pfladdr);
		return PF__INVPFL;
	}


	putmsg(PF__INFO,pgflx);

	device_name[0] = '\0';	/* assume no name	*/

	/* get the WCB		*/
	wcbaddr = pfl->pfl$l_window;
	sext(&wcbaddr);
	sda$add_symbol("WCB",(uint64)wcbaddr);
	ccode = sda$trymem(wcbaddr, &wcb, sizeof(wcb));
	if (ccode & 1)
	{
		ucbaddr = wcb.wcb$l_orgucb;
		sext(&ucbaddr);
		sda$add_symbol("UCB",(uint64)ucbaddr);
		fcbaddr = wcb.wcb$l_fcb;
		sext(&fcbaddr);
		sda$add_symbol("FCB",(uint64)fcbaddr);
		if (fcbaddr != 0)
		{
			ccode = sda$trymem(fcbaddr, &fcb, sizeof(fcb));
		}
		else
		{
			/* default page & swap files don't have a FCB */
			fcb.fcb$w_fid[2] = 0;
			fcb.fcb$w_fid[1] = 0;
			fcb.fcb$w_fid[0] = 0;
		}
	}
	if (ccode & 1)
	{
#ifdef sda$get_device_name
		/* get device name	*/
		ccode = sda$get_device_name(ucbaddr,
			device_name, sizeof(device_name));
#endif
	}
	if (ccode & 1)
	{
		if (device_name[0] == '\0')
		{
			sprintf(device_name,"UCB %08x",ucbaddr);
		}
		
		if (fcb.fcb$w_fid[0] == 0)
		{
			strcpy(ftype,"%%%%");
#ifdef pfl$v_swap_file
			if (pfl->pfl$v_swap_file)
				strcpy(ftype,"SWAP");
			else
				strcpy(ftype,"PAGE");
#endif
			sda$print("Device: !AZ (default filespec SYS$SYSROOT:[SYSEXE]!AZFILE.SYS)",
				device_name,ftype);
		}
		else
		{
			if (cmd_flags & PF_M_FID_ONLY)
			{
				ccode = 0;	/* don't try to get filename from FID	*/
			}
			else
			{
				/* try to get filename from FID		*/
				devname_dsc.dsc$w_length = strlen(device_name);
				ccode = lib$fid_to_name(&devname_dsc,fcb.fcb$w_fid,&fs_dsc,&fslen,NULL,&acp_status);
				if (ccode & 1)
					ccode = acp_status;
			}
			if (ccode & 1)
			{
				fs[fslen] = '\0';
				sda$print("!AZ",fs);
			}
			else
			{
				sda$print("Device: !AZ FID (!UW, !UW, !UW)",
					device_name,
					fcb.fcb$w_fid[2],
					fcb.fcb$w_fid[1],
					fcb.fcb$w_fid[0]);
			}
		}
	}

	/* display page file state	*/
	strcpy(pfbuf, "Pagefile flags: ");
	if (pfl->pfl$v_inited)
		strcat(pfbuf, "INITED ");
	if (pfl->pfl$v_pagfilful)
		strcat(pfbuf, "PAGFILFUL ");
	if (pfl->pfl$v_swpfilful)
		strcat(pfbuf, "SWPFILFIL ");
#ifdef pfl$v_swap_file
	if (pfl->pfl$v_swap_file)
		strcat(pfbuf, "SWAP_FILE ");
#endif
	if (pfl->pfl$v_dinspen)
		strcat(pfbuf, "DINSPEN ");
	sda$print(pfbuf);
	if (pfl->pfl$v_pagfilful)
		sda$print("!_a request for paging space has failed");
	if (pfl->pfl$v_swpfilful)
		sda$print("!_a request for swapping space has failed");
	if (pfl->pfl$v_dinspen)
		sda$print("!_file deinstall pending");

	b = page_size / PF_C_BYTES_PER_BLOCK;

#if NEWPF
	sda$print("Total size (in pages) !_!10UL!_Pages in use          !_!_!10UL",
		pfl->pfl$l_bitmapsiz*8,pfl->pfl$l_refcnt);
	sda$print("Free Pages !_!_!10UL!_Minimum free pages!_!_!10UL",
		pfl->pfl$l_frepagcnt,pfl->pfl$l_minfrepagcnt);
#else
	sda$print("Total size (in blocks) !_!10UL!_Reserveable space(in blocks)!_!10UL",
		pfl->pfl$l_bitmapsiz*8*b,pfl->pfl$l_rsrvpagcnt*b);
	sda$print("Swap Usage (processes)!_!UL!_Paging Usage (processes) !_!_!10UL",
		pfl->pfl$l_swprefcnt,pfl->pfl$l_refcnt);
	sda$print("Free Blocks !_!_!10UL!_Minimum free blocks!_!_!10UL",
		pfl->pfl$l_frepagcnt*b,pfl->pfl$l_minfrepagcnt*b);
#endif
	sda$print("Total write count !_!10UL!_Total read count !_!_!10UL",
		wcb.wcb$l_writes,wcb.wcb$l_reads);

// here should get the bitmap and scan it to find smallest and largest block (group of 1 bits)

	*refcnt = pfl->pfl$l_refcnt;

	return PF__NORMAL;
}

/*
	find the processes using the specified page file

	pgflx	page file index
	pfl	page file controlo block
*/
static void 
display_pf_users(int pgflx, PFL *pfl)
{
	PCB	*curpcb, pcb;
	PHD	*curphd, phd;
	unsigned int maxpix;		/* max process index		*/
	VOID_PQ sch$gl_pcbvec;		/* address of the PCB vector	*/
	PCB	*nullpcb;		/* ptr to the null PCB		*/
	unsigned long swppid;		/* IPID of swapper		*/
	int ccode;
	int i, j;
	int npfusers;			/* No. of procesess using pf	*/
#ifdef phd$q_pagefile_refs
	uint64	total_pfr = 0;
#endif
	uint64 tpages;			/* No. pages a process has in the pagefile	*/
	uint64 rpages;			/* No. pages a process has in the pagefile in a region	*/
	uint64 stpages;			/* Total pages used in the pagefile	*/

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
		putmsg(PF__GETPCBVEC);
		return;
	}

	npfusers = 0;
	stpages = 0;
	putmsg(PF__LOOKING);

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
#ifdef phd$q_pagefile_refs
		/* if process has no pages in a page file	*/
		if (phd.phd$q_pagefile_refs == 0)
			continue;	/* skip			*/
#endif
		/* set this process as current process		*/
		ccode = sda$set_process(NULL,0,(int)curpcb);
		tpages = 0;
#if NEWPF

		/* VMS V7.3 or later - need to search the process PTEs	*/
		if (ccode & 1)
		{
			/* search P0 region	*/
			rpages = 0;
			ccode = search_region((RDE *)&phd.phd$q_p0_rde, pgflx, pfl, &rpages);
			tpages += rpages;
		}
		if ((ccode == PF__NOTFOUND)	/* if not found		*/
		||  (cmd_flags & PF_M_COUNT))
		{                   
			/* search P1 region	*/
			rpages = 0;
			ccode = search_region((RDE *)&phd.phd$q_p1_rde, pgflx, pfl, &rpages);
			tpages += rpages;
		}
		if ((ccode == PF__NOTFOUND)	/* if not found		*/
		||  (cmd_flags & PF_M_COUNT))
		{
			/* search P2 region	*/
			rpages = 0;
			ccode = search_region((RDE *)&phd.phd$q_p2_rde, pgflx, pfl, &rpages);
			tpages += rpages;
		}
		if ((ccode == PF__NOTFOUND)	/* if not found		*/
		||  (cmd_flags & PF_M_COUNT))
		{
			rpages = 0;
			ccode = search_dynamic_regions(pgflx, pfl, &rpages);
			tpages += rpages;
		}
		if (cmd_flags & PF_M_COUNT)
		{
			if (tpages != 0)
			{
				npfusers++;
				stpages += tpages;
#ifdef phd$q_pagefile_refs
				sda$print("PID !8XL !AC has !UQ pages and !UQ pagefile references",
					pcb.pcb$l_epid,pcb.pcb$t_lname,tpages,
					phd.phd$q_pagefile_refs);
				total_pfr += phd.phd$q_pagefile_refs;
#else
				sda$print("PID !8XL !AC has !UQ pages",
					pcb.pcb$l_epid,pcb.pcb$t_lname,tpages);
#endif
			}
		}
		else
		if (ccode & 1)		/* if found		*/
		{
			npfusers++;
#ifdef phd$q_pagefile_refs
			sda$print("PID !8XL !AC has !UQ pagefile refs",
				pcb.pcb$l_epid,pcb.pcb$t_lname,
				phd.phd$q_pagefile_refs);
			total_pfr += phd.phd$q_pagefile_refs;
#else
			sda$print("PID !8XL !AC",
				pcb.pcb$l_epid,pcb.pcb$t_lname);
#endif
		}
#else
		/* before VMS V7.3 - look in the PHD PHD$B_PRCPGFL	*/
		/* for a page file index match				*/
		if ((phd.phd$b_prcpgfl[0] == pgflx)
		||  (phd.phd$b_prcpgfl[1] == pgflx)
		||  (phd.phd$b_prcpgfl[2] == pgflx)
		||  (phd.phd$b_prcpgfl[3] == pgflx))
		{
			ccode = PF__NORMAL;	/* eureka !		*/
			npfusers++;
			sda$print("PID !8XL !AC",pcb.pcb$l_epid,pcb.pcb$t_lname);
		}
#endif
	}
	putmsg(PF__USERS,npfusers);
#ifdef phd$q_pagefile_refs
	putmsg(PF__PFREFS,total_pfr);
#endif
#ifdef PF_M_COUNT
	if (cmd_flags & PF_M_COUNT)
		putmsg(PF__PFTOTALP,stpages);
#endif
}

#if NEWPF
/*
	search_gpt - search the global page table for pages in the page file
*/
static int
search_gpt(int pgflx, PFL *pfl)
{
	int ccode;
	uint64  mmg$gq_gpt_base, mmg$gq_max_gpte, npte, count, n, paddr, n1, pptsize;
	PTE *pte = NULL; /* ptr to array containing copy of PTEs	*/

	ccode = getvalue("MMG$GQ_GPT_BASE",&mmg$gq_gpt_base,sizeof(mmg$gq_gpt_base));
        if (!(ccode & 1))
		return ccode;
	ccode = getvalue("MMG$GQ_MAX_GPTE",&mmg$gq_max_gpte, sizeof(mmg$gq_max_gpte));
        if (!(ccode & 1))
		return ccode;

	npte = mmg$gq_max_gpte - mmg$gq_gpt_base;
	if (npte == 0)
		return PF__NOTFOUND;

	n1 = page_size / PTE$C_BYTES_PER_PTE;	/* number of L3 PTE in a page	*/

	/* allocate a buffer to hold L3 PTE 	*/
	pptsize = n1*PTE$C_BYTES_PER_PTE;
	sda$allocate(pptsize, (void *)&pte);

	putmsg(PF__GBLSEARCH);
	paddr = mmg$gq_gpt_base;
	count = 0;

	cmd_flags |= PF_M_COUNT;	/* need this so search_ptes will count	*/

	while (npte > 0)
	{
		n = (npte > n1)?n1:npte;
		ccode = sda$trymem((VOID_PQ)paddr,pte,n);
		if (ccode & 1)
			ccode = search_ptes(pte, n, pgflx, pfl, &count);
		else
			break;	/* give up if we can't read the GPTEs	*/
		npte -= n;
		paddr += n * PTE$C_BYTES_PER_PTE;
	}

	putmsg(PF__GBLPAGES,count);

	if (count != 0)
	{
		ccode = PF__NORMAL;	
	}
	else
	{
		ccode = PF__NOTFOUND;
	}
	sda$deallocate((void *)pte, pptsize);

	return ccode;	
}

/*
	search a region of the process page table to see if
	any PTE match the pagefile index

	rde	region descriptor
	pgflx	

	returns success condition code if found

*/
static int
search_region(RDE *rde, int pgflx, PFL *pfl, uint64 *count)
{
	int ccode;
	uint64 start, end, npte;

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
		ccode =  PF__NOTFOUND;	/* nothing to look at	*/
	else
		ccode = search_addr_range(start,end,npte,pgflx,pfl,count);

	return ccode;
}

/*
	search_addr_range
*/
static int
search_addr_range(uint64 start, uint64 end, uint64 npte, int pgflx, PFL *pfl, uint64 *count)
{
  	static uint64 mmg$gq_l2_base;
  	static uint64 mmg$gq_non_va_mask;
  	static uint64 mmg$gq_non_pt_mask;
  	static uint64 mmg$gq_level_width;
	PTE *pte = NULL; /* ptr to array containing copy of PTEs	*/
  	int vrnx;
	int found;
	uint64	va_pte;
	VA v;
	uint64 a, b2;
	unsigned int n1, n2, n, b;
	int j, g;
	PTE	l2pte;
	uint64 pptsize;		/* size of PPT			*/
	uint64 pt_base;		/* base addr of PPT		*/
	int	ccode;
	uint64	c;
	

	found = FALSE;
	c = 0;

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


	n1 = page_size / PTE$C_BYTES_PER_PTE;	/* number of L3 PTE in a page	*/
	n2 = n1 * n1;		/* number of PTE mapped by a L2 page*/
	b2 = n2 * PTE$C_BYTES_PER_PTE;

	/* allocate a buffer to hold L3 PTE 	*/
	pptsize = n1*PTE$C_BYTES_PER_PTE;
	if (pte == NULL)
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
			putmsg(PF__GETPPT);
			sda$deallocate((void *)pte, pptsize);
			pte = NULL;
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
			putmsg(PF__GETPPT);
			sda$deallocate((void *)pte, pptsize);
			pte = NULL;
			return ccode;
		}   
		/* search these L3 PTEs	*/            
		if ((search_ptes(pte,n,pgflx,pfl,&c) & 1)
		&&  !(cmd_flags & PF_M_COUNT))
			found = TRUE;
	}

	if ((found) || (c != 0))
	{
		ccode = PF__NORMAL;	
		*count = *count + c;
	}
	else
	{
		ccode = PF__NOTFOUND;
	}

	return ccode;
}

/*
	search_ptes - search a range of PTEs for pages in the page file
*/
static int
search_ptes(PTE *pte, int n, int pgflx, PFL *pfl,uint64 *count)
{
	int b,c, found,j, ccode;
	PFN	pfn;		/* PFN database entry		*/

	found = FALSE;

	c = 0;

	b = page_size / PF_C_BYTES_PER_BLOCK;	/* number of disk blocks in a page */

	/* for each pte in the page table		*/
	for (j=0; (j < n) && !found; j++)
 	{
		/* if PTE contains the right page file index		*/
		if ((!pte[j].pte$v_valid)	/* not valid		*/
	    	&&  (!pte[j].pte$v_typ0)	/* typ0 = 0		*/
		&&  (pte[j].pte$v_typ1)		/* typ1 = 1		*/
	    	&&  (!pte[j].pte$v_partial_section)	/* not partial	*/
		&&  (!pte[j].pte$v_s0_mbz)	/* must be zero		*/
		&&  (pte[j].pte$v_pgflx == pgflx) /* matches pagefile	*/
	  	   )
		{
			/* eureka !		*/
			if (cmd_flags & PF_M_COUNT) 
			{	
				c++;
#if PF_M_LOCATION
				if (cmd_flags & PF_M_LOCATION)
				{
					sda$print("!_Page starting at VBN !UL",
#ifdef pfl$q_vbn
						pfl->pfl$q_vbn + (pte[j].pte$v_pgflpag & 0xFFFFFFFF)*b);
#else                                  
			 			pfl->pfl$l_vbn + (pte[j].pte$v_pgflpag & 0xFFFFFFFF)*b); 
#endif                               
				}
#endif
			}
			else
			{
				found = TRUE;		
				break;			/* no need to go on	*/
			}
       		}
		else
		if ((!pte[j].pte$v_valid)	/* not valid		*/
		&&  (!pte[j].pte$v_typ0)	/* typ0 = 0		*/
		&&  (!pte[j].pte$v_typ1)	/* typ1 = 0		*/
		&&  (!pte[j].pte$v_partial_section)	/* not partial	*/
		&&  (!pte[j].pte$v_s0_mbz)	/* must be zero		*/
		&&  (pte[j].pte$v_pfn != 0)
 	 	   )
		{
			/* transition PTE	*/
			/* get PFN database entry for this page		*/
			ccode = get_pfn(pte[j].pte$v_pfn, &pfn);
	
			// should there be more validation of the PFN db entry here ?		
	
			/* check to see if it matches			*/
			if ((ccode & 1)
			&&  !(pfn.pfn$v_typ0)	/* typ0 = 0 indicates page in pagefile	*/
			&&  !(pfn.pfn$v_gblbak)
			&&  (pfn.pfn$v_pgflx == pgflx)
	 	   	   )
			{
				/* eureka !		*/
				if (cmd_flags & PF_M_COUNT)
				{
					c++;
#if PF_M_LOCATION
	     				if (cmd_flags & PF_M_LOCATION)
					{
						sda$print("!_Page starting at VBN !UL",
#ifdef pfl$q_vbn
							pfl->pfl$q_vbn + (pfn.pfn$v_pgflpag & 0xFFFFFFFF)*b);
#else                                  
				 			pfl->pfl$l_vbn + (pfn.pfn$v_pgflpag & 0xFFFFFFFF)*b); 
#endif                               
			       		}
#endif
				}
				else
				{
					found = TRUE; 	
					break;		/* no need to go on	*/
				}
			}
		}
	}
	if ((found) || (c != 0))
	{
		ccode = PF__NORMAL;	
		*count = *count + c;
	}
	else
	{
		ccode = PF__NOTFOUND;
	}

	return ccode;

}

/*
	get PFN database entry for specified page

	pfn		- pfn to return
	pfnentry	- buffer to return it in

	returns condition value
*/
static int 
get_pfn(unsigned long pfn, PFN *pfnentry)
{
	int ccode = PF__NORMAL;
	uint64	pfn$pq_database;	/* PFN database base address	*/
	uint64	pfnaddr;		/* PFN database entry address	*/

	/* get address of PFN database	*/
	ccode = getvalue("PFN$PQ_DATABASE",&pfn$pq_database,sizeof(pfn$pq_database));
	if (!(ccode & 1))
		return ccode;
	
	/* get address of PFN db entry	*/
	pfnaddr = pfn$pq_database + pfn*PFN$C_ENTRY_SIZE;

	/* get PFN database entry	*/
	ccode = sda$trymem((VOID_PQ)pfnaddr, pfnentry, sizeof(PFN));
	if (!(ccode & 1))
		putmsg(PF__GETPFN,pfn);

	return ccode;
}

/*
	search_dynamic_regions - search all process dynamic regions
	 to see of any PTE match the pgflx.

	pgflx	page file system index

	returns condition code
*/
static int 
search_dynamic_regions(int pgflx, PFL *pfl, uint64 *count)
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
	/* (with all these casts sometimes I wish I was writing BLISS:-)*/
	offset = (char *)&phd.phd$ps_p0_va_list_flink - (char *)&phd;
	rdelisthead = (RDE *)((char *)ctl$gl_phd + offset);

        /* search P0 dynamic regions					*/
	ccode = search_dyn_region_list(rdelisthead, pgflx, pfl, count);
	if ((ccode & 1) && !(cmd_flags & PF_M_COUNT))
		return ccode;	/* eureka !				*/

	/* determine address of P1 dynamic region descriptor listhead	*/	
	offset = (char *)&phd.phd$ps_p1_va_list_flink - (char *)&phd;
	rdelisthead = (RDE *)((char *)ctl$gl_phd + offset);

        /* search P1 dynamic regions					*/
	ccode = search_dyn_region_list(rdelisthead, pgflx, pfl, count);
	if (ccode & 1)
		return ccode;	/* eureka !				*/

	/* determine address of P2 dynamic region descriptor listhead	*/	
	offset = (char *)&phd.phd$ps_p2_va_list_flink - (char *)&phd;
	rdelisthead = (RDE *)((char *)ctl$gl_phd + offset);

        /* search P2 dynamic regions					*/
	ccode = search_dyn_region_list(rdelisthead, pgflx, pfl, count);
	if (ccode & 1)
		return ccode;	/* eureka !				*/

	return ccode;
}

/*
	search_dyn_region_list - search a list of regions for matching PTE

*/	
static int 
search_dyn_region_list(RDE *rdelisthead, int pgflx, PFL *pfl, uint64 *count)
{
	int ccode, ccode2;
	RDE rde, *rdeaddr;

	/* read listhead	*/
	ccode = sda$trymem((VOID_PQ)rdelisthead,&rde,sizeof(RDE));
	if (!(ccode & 1))
		return ccode;
	
	ccode = PF__NOTFOUND;
	rdeaddr = rde.rde$ps_va_list_flink;	
	while (rdeaddr != rdelisthead && ccode == PF__NOTFOUND)
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
		ccode = search_region(&rde, pgflx, pfl, count);
		rdeaddr = rde.rde$ps_va_list_flink;	
	}

	return ccode;
}
#endif

/*
	sext - sign extend an address to 64 bits

	ap		- ap ptr to address 
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
	getvalue - get the value at a named location	

	name		- symbol name
	vp		- address of buffer for value
	size		- size of value

	returns a condition value
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
			putmsg(PF__GETVALUE);
		}
	}
	else
	{
		putmsg(PF__GETVALUEA);
	}
	return ccode;
}
	
/*
	output routine for putmsg

	msg_dsc		- message to output (by descriptor)
	arg		- ignored argument
*/
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

	note this function needs to be placed after where its called 
	otherwise the C compiler complains.
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
