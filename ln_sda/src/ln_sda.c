/*
	LN$SDA	SDA extension to display logical names

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
#define VERSION "V1.0"
#pragma module LN$SDA VERSION

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
#include <phddef.h>
#include <dscdef.h>
#include <psldef.h>
#include <sda_routines.h>
#include <ssdef.h>
#include <rmsdef.h>
#include <starlet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stsdef.h>
#include <ctype.h>
#include <cli$routines.h>
#include <str$routines.h>
#include <climsgdef.h>
#include <lnmstrdef.h>

/*  Global variables	*/

int	sda$extend_version = SDA_FLAGS$K_VERSION;

/* local variables	*/
static SDA_FLAGS flags;
static int cmd_flags;
static LNMTH *sdir, *pdir;		/* system, process directory addressses	*/
static uint32 *phash, *shash;		/* process and system hash tables	*/
static int htblsizp, htblsizs;
static int cmd_acmode;			/* access mode specified on cmd line	*/

#define LN_C_MAXLEN	255		/* max length of a logical name		*/
#define LN_C_MAXDEPTH	10		/* max recursion depth			*/
#define LN_C_MAXWIDTH	255		/* max width				*/
#define LN_C_MAXTABLES	30		/* max number of tables			*/
#define LN_C_MAXVLEN	255		/* max len of logical name value	*/

static char *mode_names[4] = {"kernel","exec","super","user"};

typedef struct flagd
{
	unsigned int mask;
	char *desc;
} FLAGD;

/* table to keep track of table search	*/
typedef struct rt
{
	int depth;	/* recursion depth	*/
	int width;	/* search width		*/
	int matches;	/* number of matches	*/
	struct
	{
		LNMTH	*lnmth;
		char	name[LN_C_MAXLEN+1];
	} tables[LN_C_MAXTABLES];
} RT;

typedef struct
{
	LNMX	lnmx;
	char	bufp[LN_C_MAXVLEN+1];
} LNMXBUF;

/* local routines	*/
static void display_help(void);
static int parse_cmd(struct dsc$descriptor_s *cmd_line, char *name, char *table,int *flags);
static int search_t(char *name, int namelen,RT *rt);
static int search_hash_t(char *name, int namelen, uint32 *hash, int nentries, LNMTH *dir, RT *rt);
static int search_hash_t_chain(char *name, int namelen, VOID_PQ ptr, LNMTH *dir, RT *rt);
static int search_name(char *name, int namelen, RT *rt);
static int search_hash(char *name, int namelen, uint32 *hash, int nentries, RT *rt);
static int search_hash_chain(char *name, int namelen, VOID_PQ ptr, RT *rt);
static int search_hash_cv(char *value, int valuelen, VOID_PQ ptr, RT *rt);
static int wildcard_cmp(char *pattern, char *name, int patternlen, int namelen);
static int case_blind_cmp(char *n1, char *n2, int len);
static void display_lnm(LNMB *lnmb);
static void dump_cache(void);
static int getvalue(char *name, void *vp, int size);
static void sext(VOID_PQ *ap);
static void putmsg();
static int get_directory(char *name, LNMTH **dpp);
static void display_hex(char *buf, int len, char *outbuf);
static void display_flags(unsigned int flags, FLAGD *fd, int nf, char *outbuf);
static void display_table(LNMTH *th, char *name);

/* command processing flags	*/
#define LN_M_SYSTEM	0x001
#define LN_M_GROUP	0x002
#define LN_M_JOB	0x004
#define LN_M_PROCESS	0x008
#define LN_M_VALUE	0x010
#define LN_M_WILDCARD	0x020
#define LN_M_CASEBLIND	0x040
#define LN_M_ALL	0x080
#define LN_M_DUMPC	0x100

/* access modes from cmd line	*/
#define LN_C_ALLMODE	-1
#define LN_C_KERNEL	0
#define LN_C_EXEC	1
#define LN_C_SUPER	2
#define LN_C_USER	3

/* message codes	*/
globalvalue LN__NORMAL;
globalvalue LN__WARNING;
globalvalue LN__SYNTAX;
globalvalue LN__FATAL;
globalvalue LN__BUG;
globalvalue LN__NOCOMMAND;
globalvalue LN__READSYM;
globalvalue LN__WISHLIST;
globalvalue LN__GETVALUE;
globalvalue LN__GETVALUEA;
globalvalue LN__NTL;
globalvalue LN__GETSDIR;
globalvalue LN__GETPDIR;
globalvalue LN__GETHTA;
globalvalue LN__GETHT;
globalvalue LN__GETPHTA;
globalvalue LN__GETPHT;
globalvalue LN__GETSHTA;
globalvalue LN__GETSHT;
globalvalue LN__INVLNMB;
globalvalue LN__INVLNMH;
globalvalue LN__MAXTABLES;
globalvalue LN__NOTFOUND;
globalvalue LN__TNOTFOUND;
globalvalue LN__TOODEEP;
globalvalue LN__GETCHDR;
globalvalue LN__GETCB;
globalvalue LN__INVCB;
globalvalue LN__VTL;
globalvalue LN__GETTH;
globalvalue LN__GETLNMB;
globalvalue LN__LNTL;
globalvalue LN__GETV;

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
	int phashsize, shashsize; 
	int	found;
	int	ccode;
	static int sysdef_flag;	/* TRUE if SYSDEF.STB has been read 	*/
	char	name[LN_C_MAXLEN+1], table[LN_C_MAXLEN+1];
	int 	namelen;
	VOID_PQ	lnm$al_hashtbl, ptr;
	uint32	hashtbl[2];
	RT	rt;

	/* Initialize the table and establish the condition handler */
	sda$vector_table = transfer_table;
	lib$establish(sda$cond_handler);
	flags = sda_flags;

	sda$ensure(5);
	sda$print("LN$SDA !AZ (c) 2006, Ian Miller (miller@encompasserve.org) built on VMS !AZ",
		&VERSION,__VMS_VERSION);
	sda$format_heading("LN$SDA !AZ (c) 2006, Ian Miller (miller@encompasserve.org) built on VMS !AZ",
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
	ccode = parse_cmd(cmd_line, name, table, &cmd_flags); 
	if (ccode == LN__NOCOMMAND) 
	{ 
		/* no parameters - give brief help	*/ 
		display_help(); 
		return; 
	}

	if (!(ccode & 1))
	{
		putmsg(ccode);
		return;
	}

	/* add useful stuff to the symbol table if we have not done it already*/
	if (sysdef_flag == FALSE)
	{
	  	ccode = sda$read_symfile("SDA$READ_DIR:SYSDEF", SDA_OPT$M_READ_NOLOG );
	  	if (!(ccode & 1))
	    	{
			putmsg(LN__READSYM,ccode);
	    		return;
		}
	  	sysdef_flag = TRUE;
	}

	/* get logical name table directories		*/
	ccode = get_directory("LNM$AR_SYSTEM_DIRECTORY",&sdir);
	if (!(ccode & 1))
	{
		putmsg(LN__GETSDIR);
		sdir = NULL;
	}
	ccode = get_directory("CTL$GL_LNMDIRECT",&pdir);
	if (!(ccode & 1))
	{
		putmsg(LN__GETPDIR);
		pdir = NULL;
	}

	/* get size of system hash table		*/
	ccode = getvalue("LNM$GL_HTBLSIZS",&htblsizs,4);

	/* get size of process hash table		*/
	ccode = getvalue("LNM$GL_HTBLSIZP",&htblsizp,4);

	/* get addresses of hash tables			*/
	ccode = sda$symbol_value("LNM$AL_HASHTBL",(uint64 *)&lnm$al_hashtbl);
	if (!(ccode & 1))
	{
		putmsg(LN__GETHTA);
		return;
	}
	sext(&lnm$al_hashtbl);
	ccode = sda$trymem(lnm$al_hashtbl, hashtbl, sizeof(hashtbl));
	if (!(ccode & 1))
	{
		putmsg(LN__GETHT);
		return;
	}

	ptr = (VOID_PQ)hashtbl[0];
	ccode = sda$trymem(ptr, &hashtbl[0], sizeof(hashtbl[0]));
	if (!(ccode & 1))
	{
		putmsg(LN__GETSHTA);
		shash = NULL;
	}
	else
	{
		/* get system hash table			*/
		shashsize = htblsizs * sizeof(uint32) +  LNMHSH$S_LNMHSHDEF;
		sda$allocate(shashsize, (void *)&shash); /* allocate space for the table 	*/
		ptr = (VOID_PQ)hashtbl[0];
		ccode = sda$trymem(ptr, shash, shashsize);
		if (!(ccode & 1))
		{
			putmsg(LN__GETSHT);
			sda$deallocate(shash, shashsize);
			shash = NULL;
		}
	}

	ptr = (VOID_PQ)hashtbl[1];
	ccode = sda$trymem(ptr, &hashtbl[1], sizeof(hashtbl[1]));
	if (!(ccode & 1))
	{
		putmsg(LN__GETPHTA);
		phash = NULL;
	}
	else
	{
		/* get process hash table			*/
		phashsize = htblsizp * sizeof(uint32) +  LNMHSH$S_LNMHSHDEF;
		sda$allocate(phashsize, (void *)&phash); /* allocate space for the table 	*/
		ptr = (VOID_PQ)hashtbl[1];
		ccode = sda$trymem(ptr, phash, phashsize);
		if (!(ccode & 1))
		{
			putmsg(LN__GETPHT);	
			sda$deallocate(phash, phashsize);
			phash = NULL;
		}	
	}

	if (cmd_flags & LN_M_DUMPC)
		dump_cache();

	if (!(cmd_flags & LN_M_ALL))
	{
		/* setup to start search for tables		*/
		rt.depth = 0;
		rt.width = 0;
		rt.matches = 0;

		/* search for tables				*/
		ccode = search_t(table, strlen(table), &rt);	
		if (!(ccode & 1))
		{
			putmsg(ccode);
			return;
		}
	}

	namelen = strlen(name);

	ccode = search_name(name, namelen, &rt);
	if (!(ccode & 1))
		putmsg(ccode);

	if (phash != NULL)
		sda$deallocate(phash, phashsize); 
	if (shash != NULL)
		sda$deallocate(shash, shashsize); 
}

/*
	display_help - display brief help
*/
static void
display_help(void)
{
	sda$print("LN SDA Extension - Displays logical names");
	sda$print(" ");
	sda$print("Usage: LN name [table]");
	sda$print(" ");
	sda$print("Specify table to search a specific logical name table or use the qualifiers");
	sda$print("/SYSTEM to search LNM$SYSTEM only");
	sda$print("/GROUP to search LNM$GROUP only");
	sda$print("/JOB to search LNM$JOB only");
	sda$print("/PROCESS to search LNM$PROCESS only");
	sda$print("/ALL to search this process and all shared tables");
	sda$print(" ");
	sda$print("Only one of /SYSTEM,  /GROUP, /JOB, /PROCESS and /ALL can be specified.");
	sda$print(" ");
	sda$print("If no table is specified then the default is to search the tables specified by the ");
	sda$print("LNM$DCL_LOGICAL search list.");
	sda$print(" ");
	sda$print("Names containing lower case letters have to be specified in quotes e.g \"FooBar\"");
	sda$print(" ");
	sda$print("Use");
	sda$print("/CASE_BLIND for case blind matching of name");
	sda$print("/WILDCARD if name contains wildcards");
	sda$print(" ");
	sda$print("The qualifier /VALUE specifies that the name is a value of the logical name");
	sda$print("to search for. The value should be a string. ");
	sda$print("/WILDCARD and /CASE_BLIND can be used with /VALUE.");
	sda$print(" ");	
	sda$print("The qualifiers /USER, /SUPERVISOR, /EXECUTIVE, and /KERNEL restrict the");
	sda$print("search to only logical names that match the mode specifed. ");
	sda$print("The default is to match any mode.");
	sda$print(" ");
	sda$print("Only one of /USER, /SUPERVISOR, /EXECUTIVE, and /KERNEL can be specified");
	sda$print(" ");
	sda$print("Use /DUMP_CACHE to request that the process logical name table translation cache is displayed.");
	sda$print(" ");
}

/*
	parse_cmd
*/
static int 
parse_cmd(struct dsc$descriptor_s *cmd_line, char *name, char *table,int *flags)
{
	char value[256], cmd1[256], cmd2[256];
	$DESCRIPTOR(value_dsc,value);
	$DESCRIPTOR(cmd1_dsc,cmd1);
	$DESCRIPTOR(cmd2_dsc,cmd2);
	static $DESCRIPTOR(name_dsc,"NAME");
	static $DESCRIPTOR(group_dsc,"GROUP");
	static $DESCRIPTOR(system_dsc,"SYSTEM");
	static $DESCRIPTOR(job_dsc,"JOB");
	static $DESCRIPTOR(process_dsc, "PROCESS");
	static $DESCRIPTOR(user_dsc,"USER");
	static $DESCRIPTOR(supervisor_dsc,"SUPERVISOR");
	static $DESCRIPTOR(executive_dsc,"EXECUTIVE");
	static $DESCRIPTOR(kernel_dsc,"KERNEL");
	static $DESCRIPTOR(wildcard_dsc, "WILDCARD");
	static $DESCRIPTOR(caseblind_dsc, "CASE_BLIND");
	static $DESCRIPTOR(all_dsc, "ALL");
	static $DESCRIPTOR(table_dsc, "TABLE");
	static $DESCRIPTOR(valueq_dsc, "VALUE");
	static $DESCRIPTOR(dump_dsc,"DUMP_CACHE");
	int ccode = SS$_NORMAL;	
	extern int *ln_cld;
	unsigned short vlen;

	*flags = 0;

	cmd_acmode =  LN_C_ALLMODE;

        /* catch any exceptions         */
        VAXC$ESTABLISH(LIB$SIG_TO_RET);
 
	/* trim the command line	*/
	ccode = str$trim(&cmd1_dsc, cmd_line, &vlen);
	if (!(ccode & 1))
		return ccode;
	if (vlen == 0)
		return LN__NOCOMMAND; /* no command	*/

	cmd1_dsc.dsc$w_length = vlen;
	cmd1[vlen] = '\0';

	/* prefix command with LN to make parsing easier	*/
	strcpy(cmd2, "LN ");
	strcat(cmd2, cmd1);
	cmd2_dsc.dsc$w_length = strlen(cmd2);

	ccode = cli$dcl_parse(&cmd2_dsc, &ln_cld);
	if (!(ccode & 1))
		return ccode;

	/* get logical name	*/
	ccode = cli$get_value(&name_dsc,&value_dsc, &vlen);
	if (!(ccode & 1))
		return ccode;
	if (vlen > LN_C_MAXLEN)
	{
		putmsg(LN__NTL);
		return LN__NTL;
	}
	memcpy(name,value_dsc.dsc$a_pointer,vlen);
	name[vlen] = '\0';

	/* get table name */
	ccode = cli$get_value(&table_dsc,&value_dsc, &vlen);
	if (ccode & 1)
	{
		if (vlen > LN_C_MAXLEN)
		{
			putmsg(LN__NTL);
			return LN__NTL;
		}
		memcpy(table,value_dsc.dsc$a_pointer,vlen);
		table[vlen] = '\0';
	}
	else
	{
		strcpy(table, "LNM$DCL_LOGICAL");
	}

	/* check for group qualifier and get value if present	*/
	ccode = CLI$PRESENT(&group_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
	{
		*flags &= ~LN_M_GROUP;
	}
	else if (ccode & 1)
	{
		*flags |= LN_M_GROUP;
		strcpy(table, "LNM$GROUP");
	}

	/* check for SYSTEM qualifier and set flag if present	*/
	ccode = CLI$PRESENT(&system_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
	{
		*flags &= ~LN_M_SYSTEM;
	}
	else if (ccode & 1)
	{
		*flags |= LN_M_SYSTEM;
		strcpy(table, "LNM$SYSTEM");
	}

	/* check for process qualifier and set flag if present	*/
	ccode = CLI$PRESENT(&process_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
	{
		*flags &= ~LN_M_PROCESS;
	}
	else if (ccode & 1)
	{
		*flags |= LN_M_PROCESS;
		strcpy(table, "LNM$PROCESS");
	}

	/* check for JOB qualifier and set flag if present	*/
	ccode = CLI$PRESENT(&job_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
	{
		*flags &= ~LN_M_JOB;
	}
	else if (ccode & 1)
	{
		*flags |= LN_M_JOB;
		strcpy(table, "LNM$JOB");
	}

	/* check for DUMP_CACHE qualifier and flag if present	*/
	ccode = CLI$PRESENT(&dump_dsc);
	if (ccode & 1)
	{
		*flags |= LN_M_DUMPC;
	}

	/* check for USER qualifier and set mode if present	*/
	ccode = CLI$PRESENT(&user_dsc);
	if (ccode & 1)
	{
		cmd_acmode = LN_C_USER;
	}

	/* check for SUPER qualifier and set mode if present	*/
	ccode = CLI$PRESENT(&supervisor_dsc);
	if (ccode & 1)
	{
		cmd_acmode = LN_C_SUPER;
	}

	/* check for EXEC qualifier and set mode if present	*/
	ccode = CLI$PRESENT(&executive_dsc);
	if (ccode & 1)
	{
		cmd_acmode = LN_C_EXEC;
	}

	/* check for KERNEL qualifier and set mode if present	*/
	ccode = CLI$PRESENT(&kernel_dsc);
	if (ccode & 1)
	{
		cmd_acmode = LN_C_KERNEL;
	}

	/* check for value qualifier and set flag if present	*/
	ccode = CLI$PRESENT(&valueq_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		*flags &= ~LN_M_VALUE;
	else if (ccode & 1)
		*flags |= LN_M_VALUE;

	/* check for wildcard qualifier and set flag if present	*/
	ccode = CLI$PRESENT(&wildcard_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		*flags &= ~LN_M_WILDCARD;
	else if (ccode & 1)
		*flags |= LN_M_WILDCARD;

	/* check for caseblind qualifier and set flag if present	*/
	ccode = CLI$PRESENT(&caseblind_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		*flags &= ~LN_M_CASEBLIND;
	else if (ccode & 1)
		*flags |= LN_M_CASEBLIND;

	/* check for all qualifier and set flag if present	*/
	ccode = CLI$PRESENT(&all_dsc);
	if ((ccode == CLI$_NEGATED) || (ccode == CLI$_LOCNEG))
		*flags &= ~LN_M_ALL;
	else if (ccode & 1)
		*flags |= LN_M_ALL;

	ccode = SS$_NORMAL;

	return ccode;
}

/*
	search_t - search the hash tables to translate a table name
*/
static int
search_t(char *name, int namelen,RT *rt)
{
	int ccode1, ccode2;

	if (pdir != NULL)
	{
		/* search process hash table */
		ccode1 = search_hash_t(name, namelen, phash, htblsizp, pdir, rt);
	}
	else
	{
		ccode1 = LN__TNOTFOUND;
	}

	if ((ccode1 & 1) || (ccode1 == LN__TNOTFOUND))
	{
		if (sdir != NULL)
		{
			/* search system directory	*/
			ccode2 = search_hash_t(name, namelen, shash, htblsizs, sdir, rt);
		}
		else
		{
			ccode2 = LN__TNOTFOUND;
		}
	}
	else
		return ccode1;

	if ((ccode1 & 1) || (ccode2 & 1))
		return LN__NORMAL;
	else
		return ccode2;
}

/*
	search_hash_t - search a hash table for a table name
*/
static int 
search_hash_t(char *name, int namelen, uint32 *hash, int nentries, LNMTH *dir, RT *rt)
{
	VOID_PQ ptr;
	int i;
	int ccode;
	extern unsigned int lnm_std$hash(int namelen, char *name);
	unsigned int hashvalue;
	LNMHSH *hsh;

	/* if no hash table to search	*/
	if (hash == NULL)
		return LN__TNOTFOUND;

	hsh = (LNMHSH *)hash;

	if (hsh->lnmhsh$b_type != DYN$C_RSHT)
	{
		putmsg(LN__INVLNMH, hash, hsh->lnmhsh$b_type);
		return LN__INVLNMH;
	}

	hashvalue = lnm_std$hash(namelen,name) & ~hsh->lnmhsh$l_mask;

	ptr = (VOID_PQ)hash[(LNMHSH$S_LNMHSHDEF/sizeof(uint32))+hashvalue];
	sext(&ptr);
	ccode = search_hash_t_chain(name, namelen, ptr, dir, rt);

        return ccode;
}

/*
	search_hash_t_chain - search a hash table for a table name
*/
static int 
search_hash_t_chain(char *name, int namelen, VOID_PQ ptr, LNMTH *dir, RT *rt)
{
	LNMXBUF lnmxbuf;
	LNMX *lnmxptr;
	LNMB *lnmbuf;
	LNMB lnmb;
	int c, j, i;
	int ccode;
	int result;
	LNMTH *lnmth;

	lnmbuf = NULL;
	result = 0;

	while (ptr != NULL)
	{
		/* get the LNMB	*/
	 	ccode = sda$trymem(ptr,&lnmb, sizeof(LNMB));
		if (!(ccode & 1))
			return ccode;	/* return the bad news	*/
		if (lnmb.lnmb$b_type == DYN$C_LNM)
		{
			sda$allocate(lnmb.lnmb$w_size, (void *)&lnmbuf);
			ccode = sda$trymem(ptr, lnmbuf, lnmb.lnmb$w_size);
		}
		else
		{
			putmsg(LN__INVLNMB, ptr, lnmb.lnmb$b_type);
			return LN__INVLNMB;
		}

		/* compare the name	*/
		if (namelen == lnmbuf->lnmb$l_namelen)
		{
			c = memcmp(name, &lnmbuf->lnmb$t_name, namelen);
			if ((c == 0)
			&& (lnmbuf->lnmb$l_table == dir))
			{
				/* eureka !	*/
				/* look at the translations of the table name	*/
				for (lnmxptr = lnmbuf->lnmb$l_lnmx; lnmxptr != NULL; lnmxptr = lnmxbuf.lnmx.lnmx$l_next)
				{
					/* get the translation			*/
					ccode = sda$trymem(lnmxptr, (void *)&lnmxbuf, sizeof(LNMX));
					if (!(ccode & 1))
					{
						putmsg(LN__GETV,lnmxptr);
						break;
					}
					if (lnmxbuf.lnmx.lnmx$l_xlen > LN_C_MAXVLEN)
					{
						putmsg(LN__VTL,lnmxbuf.lnmx.lnmx$l_index,lnmxbuf.lnmx.lnmx$l_xlen);
						break;
					}
					ccode = sda$trymem(lnmxptr, (void *)&lnmxbuf, 
						lnmxbuf.lnmx.lnmx$l_xlen - 1 + sizeof(LNMX));
					if (!(ccode & 1))
					{
						putmsg(LN__GETV,lnmxptr);
						break;
					}
					if (lnmxbuf.lnmx.lnmx$l_index >=0 )
					{
						/* value is a name - recursive search	*/
						rt->depth++;
						if (rt->depth > LN_C_MAXDEPTH)
						{
							putmsg(LN__TOODEEP, lnmxbuf.lnmx.lnmx$l_index,
								&lnmxbuf.lnmx.lnmx$t_xlation,
								lnmxbuf.lnmx.lnmx$l_xlen);
						}
						else
						{
							ccode = search_t(&lnmxbuf.lnmx.lnmx$t_xlation,
								lnmxbuf.lnmx.lnmx$l_xlen,rt);
						}
						rt->depth--;
					}
					else if (lnmxbuf.lnmx.lnmx$l_index == LNMX$C_TABLE)
					{
						result++;
						lnmth = (LNMTH *)((char *)lnmxptr + LNMX$S_LNMXDEF - 1); 

						/* check if already in result table */
						for (i=0; i < rt->matches; i++)
						{
						    if (rt->tables[i].lnmth == lnmth)
							break;
						}
						/* if not then add		*/
						if (i >= rt->matches)
						{
						    rt->matches++; 
						    rt->tables[rt->matches-1].lnmth = lnmth;
						    memcpy(rt->tables[rt->matches-1].name, name, namelen);
						    rt->tables[rt->matches-1].name[namelen] = '\0';
						    if (rt->matches == LN_C_MAXTABLES)
							return LN__MAXTABLES;
						}
						break;
					}
				}
				result++;
			}	
			else if (c < 0)
			{
				break;	/* give up - name in LNMB is alphabetically later	*/
			}
		}
		else if (namelen < lnmbuf->lnmb$l_namelen)
		{
			break;
		}

		sda$deallocate(lnmbuf, lnmb.lnmb$w_size);
		lnmbuf = NULL;

		/* next entry		*/
		ptr = (VOID_PQ)lnmb.lnmb$l_flink;
		sext(&ptr);
	}
	if (lnmbuf != NULL)
	{
		sda$deallocate(lnmbuf, lnmb.lnmb$w_size);
		lnmbuf = NULL;
	}

	if (result == 0)
		return LN__TNOTFOUND;
	else
		return LN__NORMAL;
}

/*
	search for a name
*/
static int 
search_name(char *name, int namelen, RT *rt)
{
	int ccode1, ccode2;

	/* search the process hash table		*/
	ccode1 = search_hash(name, namelen, phash, htblsizp, rt);
	
	if ((ccode1 & 1) || (ccode1 == LN__NOTFOUND))
	{
		/* search the shareable hash table		*/
		ccode2 = search_hash(name, namelen, shash, htblsizs, rt);
	}
	else
		return ccode1;

	if ((ccode1 & 1) || (ccode2 & 1))
		return LN__NORMAL;
	else
		return ccode2;
}

/*
	search_hash
*/
static int 
search_hash(char *name, int namelen, uint32 *hash, int nentries, RT *rt)
{
	VOID_PQ ptr;
	int i;
	int ccode;
	int matches = 0;
	extern unsigned int lnm_std$hash(int namelen, char *name);
	unsigned int hashvalue;
	LNMHSH *hsh;

	/* if no hash table to search	*/
	if (hash == NULL)
		return LN__NOTFOUND;

	hsh = (LNMHSH *)hash;

	if (hsh->lnmhsh$b_type != DYN$C_RSHT)
	{
		putmsg(LN__INVLNMH, hash, hsh->lnmhsh$b_type);
		return LN__INVLNMH;
	}

	if (cmd_flags & LN_M_VALUE)
	{
		for (i=0; i < nentries; i++)
		{
			ptr = (VOID_PQ)hash[(LNMHSH$S_LNMHSHDEF/sizeof(uint32))+i];
			ccode = search_hash_cv(name,namelen,ptr,rt);
			if (ccode & 1)
				matches++;
			else if (ccode != LN__NOTFOUND)
				break;
		}
		if ((matches == 0) && (ccode & 1))
			ccode = LN__NOTFOUND;
		if ((matches != 0) && (ccode == LN__NOTFOUND))
			ccode = LN__NORMAL;
	}
	else if (cmd_flags & LN_M_WILDCARD)
	{
		for (i=0; i < nentries; i++)
		{
			ptr = (VOID_PQ)hash[(LNMHSH$S_LNMHSHDEF/sizeof(uint32))+i];
			ccode = search_hash_chain(name,namelen,ptr,rt);
			if (ccode & 1)
				matches++;
			else if (ccode != LN__NOTFOUND)
				break;
		}
		if ((matches == 0) && (ccode & 1))
			ccode = LN__NOTFOUND;
		if ((matches != 0) && (ccode == LN__NOTFOUND))
			ccode = LN__NORMAL;
	}
	else
	{
		hashvalue = lnm_std$hash(namelen,name) & ~hsh->lnmhsh$l_mask;
		ptr = (VOID_PQ)hash[(LNMHSH$S_LNMHSHDEF/sizeof(uint32))+hashvalue];
		ccode = search_hash_chain(name,namelen,ptr,rt);
	}

	return ccode;
}

/*
	search_hash_chain - search a hash table for a name
*/
static int 
search_hash_chain(char *name, int namelen, VOID_PQ ptr, RT *rt)
{
	LNMB *lnmbuf;
	LNMB lnmb;
	int c, j, i;
	int ccode;
	int result;

	lnmbuf = NULL;
	result = 0;

	for (; ptr != NULL; ptr = (VOID_PQ)lnmb.lnmb$l_flink)
	{
		sext(&ptr);

		/* get the fixed part of the LNMB	*/
		ccode = sda$trymem(ptr,&lnmb, sizeof(LNMB));
		if (!(ccode & 1))
			return ccode;	/* return the bad news	*/
		if (lnmb.lnmb$b_type != DYN$C_LNM)
		{
			putmsg(LN__INVLNMB, ptr, lnmb.lnmb$b_type);
			return LN__INVLNMB;
		}

		/* check the name access mode matches	*/
		if ((cmd_acmode != LN_C_ALLMODE)
		&&  (cmd_acmode != lnmb.lnmb$l_acmode))
			continue;	/* ignore as not the specified mode	*/

		/* get the whole LNMB			*/
		sda$allocate(lnmb.lnmb$w_size, (void *)&lnmbuf);
		ccode = sda$trymem(ptr, lnmbuf, lnmb.lnmb$w_size);

		/* compare the name	*/
		if (cmd_flags & LN_M_WILDCARD)
		{
			if (wildcard_cmp(name, &lnmbuf->lnmb$t_name, namelen, lnmbuf->lnmb$l_namelen) & 1)
			{
				/* eureka	*/
				if (cmd_flags & LN_M_ALL)
				{
					display_lnm(lnmbuf);
					result++;
				}
				else
				{
					/* check if its in the right table	*/
					for (j=0; j < rt->matches; j++)
					{
						if (lnmbuf->lnmb$l_table == rt->tables[j].lnmth)
						{
							display_lnm(lnmbuf);
							result++;
							break;
						}
					}
				}
			}
		}
		else
		{
			if (namelen == lnmbuf->lnmb$l_namelen)
			{
				c = memcmp(name, &lnmbuf->lnmb$t_name, namelen);
				if ((c != 0) && (cmd_flags & LN_M_CASEBLIND))
					c = case_blind_cmp(name, &lnmbuf->lnmb$t_name, namelen);
				if (c == 0)
				{
					/* eureka !	*/
					if (cmd_flags & LN_M_ALL)
					{
						display_lnm(lnmbuf);
						result++;
					}
					else
					{
						/* check if its in the right table	*/
						for (j=0; j < rt->matches; j++)
						{
							if (lnmbuf->lnmb$l_table == rt->tables[j].lnmth)
							{
								display_lnm(lnmbuf);
								result++;
								break;
							}
						}
					}
				}	
				else if (c < 0)
				{
					break;	/* give up - name in LNMB is alphabetically later	*/
				}
			}
			else if (namelen < lnmbuf->lnmb$l_namelen)
			{
				break;
			}
		}
		sda$deallocate(lnmbuf, lnmb.lnmb$w_size);
		lnmbuf = NULL;
	}
	if (result == 0)
		return LN__NOTFOUND;
	else
		return LN__NORMAL;
}

/*
	search_hash_cv - search a hash chain for names who have a matching value
*/
static int 
search_hash_cv(char *value, int valuelen, VOID_PQ ptr, RT *rt)
{
	LNMB *lnmbuf;
	LNMB lnmb;
	int c, j, i;
	int ccode;
	int result;
	LNMXBUF lnmxbuf;
	LNMX *lnmxptr;

	lnmbuf = NULL;
	result = 0;
	for (; ptr != NULL; ptr = (VOID_PQ)lnmb.lnmb$l_flink)
	{
		sext(&ptr);

		/* get the fixed part of the LNMB	*/
		ccode = sda$trymem(ptr,&lnmb, sizeof(LNMB));
		if (!(ccode & 1))
			return ccode;	/* return the bad news	*/
		if (lnmb.lnmb$b_type != DYN$C_LNM)
		{
			putmsg(LN__INVLNMB, ptr, lnmb.lnmb$b_type);
			return LN__INVLNMB;
		}

		/* check the name access mode matches	*/
		if ((cmd_acmode != LN_C_ALLMODE)
		&&  (cmd_acmode != lnmb.lnmb$l_acmode))
			continue;	/* ignore as not the specified mode	*/

		/* get the whole LNMB			*/
		sda$allocate(lnmb.lnmb$w_size, (void *)&lnmbuf);
		ccode = sda$trymem(ptr, lnmbuf, lnmb.lnmb$w_size);

		/* loop through the translations of this name looking for a match	*/
		for (lnmxptr = lnmbuf->lnmb$l_lnmx; lnmxptr != NULL; lnmxptr = lnmxbuf.lnmx.lnmx$l_next)
		{
			/* get the translation value	*/
			ccode = sda$trymem(lnmxptr, (void *)&lnmxbuf, sizeof(LNMX));
			if (!(ccode & 1))
				break;
			if (lnmxbuf.lnmx.lnmx$l_xlen > LN_C_MAXVLEN)
			{
				putmsg(LN__VTL,lnmxbuf.lnmx.lnmx$l_index,lnmxbuf.lnmx.lnmx$l_xlen);
				break;
			}
			if (lnmxbuf.lnmx.lnmx$l_index < 0)
				continue;	/* skip nonstandard translations	*/
			ccode = sda$trymem(lnmxptr, (void *)&lnmxbuf, lnmxbuf.lnmx.lnmx$l_xlen - 1 + sizeof(LNMX));
			if (!(ccode & 1))
				break;

			/* check for matches		*/
			if (cmd_flags & LN_M_WILDCARD)
			{
				if (wildcard_cmp(value,&lnmxbuf.lnmx.lnmx$t_xlation,valuelen,lnmxbuf.lnmx.lnmx$l_xlen)&1)
				{
					/* eureka	*/
					if (cmd_flags & LN_M_ALL)
					{
						display_lnm(lnmbuf);
						result++;
					}
					else
					{
						/* check if its in the right table	*/
						for (j=0; j < rt->matches; j++)
						{
							if (lnmbuf->lnmb$l_table == rt->tables[j].lnmth)
							{
								display_lnm(lnmbuf);
								result++;
								break;
							}
						}
					}
				}
			}
			else if (valuelen == lnmxbuf.lnmx.lnmx$l_xlen)
			{
				c = memcmp(value, &lnmxbuf.lnmx.lnmx$t_xlation, valuelen);
				if ((c != 0) && (cmd_flags & LN_M_CASEBLIND))
					c = case_blind_cmp(value, &lnmxbuf.lnmx.lnmx$t_xlation, valuelen);
				if (c == 0)
				{
					/* eureka !	*/
					if (cmd_flags & LN_M_ALL)
					{
						display_lnm(lnmbuf);
						result++;
					}
					else
					{
						/* check if its in the right table	*/
						for (j=0; j < rt->matches; j++)
						{
							if (lnmbuf->lnmb$l_table == rt->tables[j].lnmth)
							{
								display_lnm(lnmbuf);
								result++;
								break;
							}
						}
					}
				}	
			}
		}
		sda$deallocate(lnmbuf, lnmb.lnmb$w_size);
		lnmbuf = NULL;
	}
	if (lnmbuf != NULL)
		sda$deallocate(lnmbuf, lnmb.lnmb$w_size);

	if (result == 0)
		return LN__NOTFOUND;
	else
		return LN__NORMAL;
}

/*
	wildcard name comparision
*/
static int 
wildcard_cmp(char *pattern, char *name, int patternlen, int namelen)
{
	static $DESCRIPTOR(patt_dsc,"");
	static $DESCRIPTOR(name_dsc,"");

	patt_dsc.dsc$a_pointer = pattern;
	patt_dsc.dsc$w_length = patternlen;

	name_dsc.dsc$a_pointer = name;
	name_dsc.dsc$w_length = namelen;

	return str$match_wild(&name_dsc, &patt_dsc);
}

/*
	case blind name comparsion
*/
static int 
case_blind_cmp(char *n1, char *n2, int len)
{
	int c = 0;

	while (len-- > 0)
	{
		c = toupper(*n1) - toupper(*n2);
		if (c != 0)
			break;
		n1++;
		n2++;
	}
	return c;
}

/*
	display_lnmb
*/
static void 
display_lnm(LNMB *lnmb)
{
	static FLAGD lnmb_flags[] = 
	{{LNMB$M_NO_ALIAS,"no_alias"},
	 {LNMB$M_CONFINE,"confine"},
	 {LNMB$M_CRELOG,"crelog"},
	 {LNMB$M_TABLE,"table"},
	 {LNMB$M_NODELETE,"nodelete"},
	 {LNMB$M_CLUSTERWIDE,"clusterwide"}
	};
	static FLAGD lnmx_flags[] = 
	{{LNMX$M_CONCEALED,"concealed"},
	 {LNMX$M_TERMINAL,"terminal"}
	};
	LNMXBUF lnmxbuf;
	LNMX *lnmxptr;
	int ccode;
	char buf[LN_C_MAXVLEN+1], tname[LN_C_MAXLEN];
	char buf2[LN_C_MAXVLEN*3 + 1];
	uint32 u;
	CWLNMX *cwlnmx;

	display_table(lnmb->lnmb$l_table,tname);

	strcpy(buf,"[");
	strcat(buf,mode_names[lnmb->lnmb$l_acmode]);
	if (lnmb->lnmb$l_flags != 0)
	{
		strcat(buf,",");
		display_flags(lnmb->lnmb$l_flags, lnmb_flags, sizeof(lnmb_flags)/sizeof(lnmb_flags[0]),buf);
	}
	strcat(buf,"]");

	sda$ensure(3);
	sda$print("!AF !AZ (!AZ)",lnmb->lnmb$l_namelen, &lnmb->lnmb$t_name,
		buf, tname);

	for (lnmxptr = lnmb->lnmb$l_lnmx; lnmxptr != NULL; lnmxptr = lnmxbuf.lnmx.lnmx$l_next)
	{
		ccode = sda$trymem(lnmxptr, (void *)&lnmxbuf, sizeof(LNMX));
		if (!(ccode & 1))
			break;
		if (lnmxbuf.lnmx.lnmx$l_xlen > LN_C_MAXVLEN)
		{
			putmsg(LN__VTL,lnmxbuf.lnmx.lnmx$l_index,lnmxbuf.lnmx.lnmx$l_xlen);
			break;
		}
		ccode = sda$trymem(lnmxptr, (void *)&lnmxbuf, lnmxbuf.lnmx.lnmx$l_xlen - 1 + sizeof(LNMX));
		if (!(ccode & 1))
			break;
		if (lnmxbuf.lnmx.lnmx$l_flags != 0)
		{
			strcpy(buf,"[");
			display_flags(lnmxbuf.lnmx.lnmx$l_flags, lnmx_flags, sizeof(lnmx_flags)/sizeof(lnmx_flags[0]),buf);
			strcat(buf,"]");
		}
		else
		{
			buf[0] = '\0';	/* no flags to display	*/
		}
		if (lnmxbuf.lnmx.lnmx$l_index >=0 )
		{
			sda$ensure(2);
			sda$print("!_!SL!_!AF!_!AZ",
				lnmxbuf.lnmx.lnmx$l_index, 
				lnmxbuf.lnmx.lnmx$l_xlen, &lnmxbuf.lnmx.lnmx$t_xlation,
				buf);
			display_hex(&lnmxbuf.lnmx.lnmx$t_xlation,lnmxbuf.lnmx.lnmx$l_xlen, buf2);
			sda$print("!_!_!AZ",buf2);
		}
		else
		{
			switch (lnmxbuf.lnmx.lnmx$l_index)
			{
			case LNMX$C_HSHFCN:
				memcpy(&u,&lnmxbuf.lnmx.lnmx$t_xlation,sizeof(u));
				sda$print("!_HSHFCN !8XL!_!AZ",u,buf);
				break;
			case LNMX$C_BACKPTR:
				memcpy(&u,&lnmxbuf.lnmx.lnmx$t_xlation,sizeof(u));
				sda$print("!_Backpointer !8XL",u);
				break;
			case LNMX$C_TABLE:
				sda$print("!_LNMTH address !8XL",
					(char *)lnmxptr + LNMX$S_LNMXDEF - 1);
				break;
			case LNMX$C_IGNORED_INDEX:
				sda$print("!_IGIDX!_!AZ",buf);
				break;
			case LNMX$C_CW_LINKS:
				cwlnmx = (CWLNMX *)&lnmxbuf;
				sda$print("!_Clusterwide name flink !8XL blink !8XL LNMB !8XL",
					cwlnmx->lnmx$l_cw_flink,cwlnmx->lnmx$l_cw_blink,cwlnmx->lnmx$l_cw_lnmb);
				break;
			}
		}
	}
}

/*
	display table name and flags
*/
static void
display_table(LNMTH *th, char *name)
{
	static FLAGD lnmth_flags[] = 
	{{LNMTH$M_SHAREABLE,"shareable"},
	 {LNMTH$M_DIRECTORY,"directory"},
	 {LNMTH$M_GROUP,"group"},
	 {LNMTH$M_SYSTEM,"system"},
	 {LNMTH$M_CLUSTERWIDE,"clusterwide"},
	 {LNMTH$M_REMACTION,"remaction"}
	};
        struct
        {
                LNMB lnmb;
                char name[50];
        } lb;
	LNMTH lnmth;
	int ccode;

	*name = '\0';

	/* get name of table	*/
	ccode = sda$trymem(th, &lnmth, sizeof(lnmth));
	if (!(ccode & 1))
	{
		putmsg(LN__GETTH,th);
		return;
	}
        ccode = sda$trymem(lnmth.lnmth$l_name, &lb, sizeof(lb));
	if (!(ccode & 1))
	{
		putmsg(LN__GETLNMB,lnmth.lnmth$l_name);
		return;
	}
	if (lb.lnmb.lnmb$l_namelen > sizeof(lb.name))
	{
		putmsg(LN__LNTL,lnmth.lnmth$l_name);
		return;
	}
	lb.name[lb.lnmb.lnmb$l_namelen-1] = '\0';

	strcpy(name,&lb.lnmb.lnmb$t_name);

	strcat(name,"\t[");
	display_flags(lnmth.lnmth$l_flags, lnmth_flags, sizeof(lnmth_flags)/sizeof(lnmth_flags[0]),name);
	strcat(name,"]");

}

static int 
get_directory(char *name, LNMTH **dpp)
{
	int ccode;
        LNMB	*lnmbuf, *lnmptr;
	LNMB	lnmb;
	LNMX	*lnmx;
	LNMTH	*lnmth;
	
	ccode = getvalue(name, &lnmptr, sizeof(lnmptr));
	if (!(ccode & 1))
		return ccode;

	ccode = sda$trymem(lnmptr,&lnmb, sizeof(LNMB));
	if (!(ccode & 1))
		return ccode;	
	if (lnmb.lnmb$b_type != DYN$C_LNM)
	{
		putmsg(LN__INVLNMB, lnmptr, lnmb.lnmb$b_type);
		return LN__INVLNMB;
	}

	sda$allocate(lnmb.lnmb$w_size, (void *)&lnmbuf);
	ccode = sda$trymem(lnmptr, lnmbuf, lnmb.lnmb$w_size);

	lnmx = lnmbuf->lnmb$l_lnmx;

	lnmth = (LNMTH *)((char *)lnmx + LNMX$S_LNMXDEF - 1);

	*dpp = lnmth;

	sda$deallocate(lnmbuf, lnmb.lnmb$w_size);

	return ccode;
}

static void 
display_flags(unsigned int flags, FLAGD *fd, int nf, char *outbuf)
{
	int n, i;

	for (i=0,n=0; i < nf; i++)
	{
		if (fd[i].mask & flags)
		{
			if (n > 0)
				strcat(outbuf,",");
			strcat(outbuf,fd[i].desc);
			n++;
		}
	}
}

/*
	dump the logical name table cache blocks
*/
static void 
dump_cache(void)
{
	LNMC *ctl$gq_lnmtblcache[2];
        int ccode;
	LNMC lnmc, *ptr;
	int i;
        struct
        {
                LNMB lnmb;
                char name[50];
        } lb;
	char tname[256];
	uint32 lnmdirseq, sysdirseq;

	ccode = getvalue("CTL$GQ_LNMTBLCACHE",ctl$gq_lnmtblcache,sizeof(ctl$gq_lnmtblcache));
	if (!(ccode & 1))
	{
		putmsg(LN__GETCHDR);
		return;	/* give up on error	*/
	}

	sda$print("Dump of Logical Name Table Cache Blocks");
	sda$format_heading("Dump of Logical Name Table Cache Blocks");
	sda$print(" ");

	/* get process logical name table directory sequence number	*/
	ccode = getvalue("CTL$GL_LNMDIRSEQ",&lnmdirseq,sizeof(lnmdirseq));
	if (!(ccode & 1))
		lnmdirseq = 0;
	
	/*get system logical name table directory sequence number	*/
	ccode = getvalue("LNM$GL_SYSDIRSEQ",&sysdirseq,sizeof(sysdirseq));
	if (!(ccode & 1))
		sysdirseq = 0;

	sda$print("Logical Name Directory Sequence Number - System !UL, Process !UL",
		sysdirseq, lnmdirseq);

	sda$print(" ");

 	for (ptr = ctl$gq_lnmtblcache[0]; ptr != NULL; ptr = lnmc.lnmc$l_flink)
	{
		/* get the cache block		*/
		ccode = sda$trymem(ptr, &lnmc, sizeof(lnmc));
		if (!(ccode & 1))
		{
			putmsg(LN__GETCB,ptr);
			break;
		}
		if (lnmc.lnmc$l_tbladdr != 0)
		{
			if (lnmc.lnmc$b_type != DYN$C_LNMC)
			{
				putmsg(LN__INVCB,lnmc.lnmc$b_type,ptr);
				break;
			}
			/* get the LNMB			*/
			ccode = sda$trymem(lnmc.lnmc$l_tbladdr, &lb, sizeof(lb));
			if (lb.lnmb.lnmb$b_type != DYN$C_LNM)
			{
				putmsg(LN__INVLNMB, ptr, lb.lnmb.lnmb$b_type);
				break;
			}
			lb.name[lb.lnmb.lnmb$l_namelen-1] = '\0';

			/* display the cache block	*/
			sda$print("LNMC @ !XL ",ptr);
			sda$print("Table name address!_: !8XL!_!AZ!_[!AZ]",
				lnmc.lnmc$l_tbladdr,
				&lb.lnmb.lnmb$t_name,
				(lnmc.lnmc$b_mode < 4)?mode_names[lnmc.lnmc$b_mode]:"?");
			sda$print("Current entry number!_: !UL",lnmc.lnmc$l_cacheindx);
			sda$print("Process dir seq.No.!_: !UL",lnmc.lnmc$l_procdirseq);
			sda$print("System dir seq.No.!_: !UL",lnmc.lnmc$l_sysdirseq);
			for (i=0; i < LNMC$K_NUM_ENTRIES; i++)
			{
				if (lnmc.lnmc$l_entry[i] == NULL)
				{
					sda$print("!_entries valid but incomplete");
					break;
				}
				if (lnmc.lnmc$l_entry[i] == (LNMTH *)-1)
				{
					sda$print("!_entries valid and complete");
					break;
				}
				display_table(lnmc.lnmc$l_entry[i], tname);
				sda$print("!_LNMTH : !8XL!_!AZ",lnmc.lnmc$l_entry[i],tname);
			}
		}
		/* if next one is hdr then done	*/
		if (lnmc.lnmc$l_flink == ctl$gq_lnmtblcache[0])
			break;	/* done last one	*/
	}
}

/*
	display_hex - display buffer in hex
*/
static void
display_hex(char *buf, int len, char *outbuf)
{
	static char hex[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	int i = 0;

	while (len--)
	{
		i++;
		if ((i > 0) && (i % 36)==0)
		{
			/* newline	*/
			*outbuf = '\r';	outbuf++;
			*outbuf = '\n';	outbuf++;
			*outbuf = '\t'; outbuf++;
			*outbuf = '\t'; outbuf++;
		}
		*outbuf = hex[0xF & (((unsigned char)*buf) >> 4)];
		outbuf++;
		*outbuf = hex[0xF & *buf];
		outbuf++;
		*outbuf = ' ';
		outbuf++;
		buf++;
	}
	*outbuf = '\0';
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
			ccode = LN__GETVALUE;
		}
	}
	else
	{
		ccode = LN__GETVALUEA;
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
