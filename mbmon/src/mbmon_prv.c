/*
	MBMON_PRV	- MBMON priviledged library (system services)

	This component of MBMON is the interface between MBMONCP and
	the executive image.

 COPYRIGHT NOTICE

 This software is COPYRIGHT (c) 2005, Ian Miller. ALL RIGHTS RESERVED.

 Released under licence described in freeware_readme.txt 

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
#ifndef OLD_STARLET
#define __NEW_STARLET 1
#endif
#include <builtins.h>
#include <plvdef.h>
#include <prdef.h>
#include <ssdef.h>
#include <ssdescrdef.h>
#include <descrip.h>
#include <string.h>
#include <vms_macros.h>
#include <ccbdef.h>
#include <pcbdef.h>
#include <ucbdef.h>
#include <prvdef.h>
#include <dcdef.h>
#if OLD_EFN
#define EFN 21
#else
#include <efndef.h>
#define EFN EFN$C_ENF
#endif

#ifdef OLD_STARLET
typedef union prvdef PRVDEF;
#endif

#include "mbmon.h"
#include "mbmon_prv.h"
#include "ldrdef.h"

int rundown_handler();

/* fudge around typing of routine argument types	*/
#pragma environment save
#pragma message disable PTRMISMATCH
/* Kernel system service table */
int (*(kernel_table[]))() = 
{
	mbmon_load,
	mbmon_unload,
        mbmon_setup,
        mbmon_add,
        mbmon_remove
};
#pragma environment restore

int kernel_flags [] = {0,0,0};

#define KERNEL_ROUTINE_COUNT sizeof(kernel_table)/sizeof(int *)

/*
	the PLV
*/
#ifdef __ALPHA
#pragma extern_model save
#pragma extern_model strict_refdef "USER_SERVICES"
#endif
extern const PLV user_services = 
{
        PLV$C_TYP_CMOD,         /* type */
        0,                      /* version */
        {
	        {
			KERNEL_ROUTINE_COUNT,	/* # of kernel routines */
	        	0,			/* # of exec routines */
		        kernel_table,           /* kernel routine list */
		        0,             		/* exec routine list */
		        rundown_handler,        /* kernel rundown handler */
		        0,        		/* exec rundown handler */
		        0,                      /* no RMS dispatcher */
		        kernel_flags,           /* kernel routine flags */
		        0			/* exec routine flags */
		}
        }
};
#ifdef __ALPHA
#pragma extern_model restore
#endif

static int rundown_done = 0;
static int execlet_mapped = 0;
static MBMON_VECTOR mbmon_vec;
static MBMON_UNIT mbmon[MBMON_MAX_UNIT];
static LDR_REFHDL refhdl;
static MBMON_VECTOR *mbvp;
static $DESCRIPTOR(execlet_dsc,MBMON_EXECLET_NAME);

/*
	mbmon_map	- map the execlet
*/
static unsigned long
mbmon_map(void)
{
	unsigned long ccode;
	MBMON_VECTOR *vp;

	if (execlet_mapped)
	{
		ccode = SS$_NORMAL;
	}
	else
	{
#if TRACE
		tr_print(("mbmon_prv_map - start"));
#endif
		/* locate the loadable execlet	*/
		ccode = ldr$ref_info(&execlet_dsc,&refhdl);
		if (ccode & 1)
		{
#if TRACE
			tr_print(("mbmon_prv_map - found execlet 0x%x",refhdl.lid));
			tr_print(("mbmon_prv_map - refcnt %d",refhdl.lid->ldrimg$l_refcnt));
#endif
			/* use ldrimg$l_nonpag_w_base to find the vector	*/
			vp = refhdl.lid->ldrimg$l_nonpag_w_base;	
#if TRACE
			tr_print(("mbmon_prv_map - mbmon vector addresss 0x%x",vp));
#endif
			/* copy the vector to local storage			*/
			memcpy(&mbmon_vec,vp,sizeof(MBMON_VECTOR));
		
			/* validate vector			*/
			if ((mbmon_vec.version != MBMON_VECTOR_VERSION)
			||  (mbmon_vec.count != MBMON_VECTOR_COUNT))
			{
				ccode = SS$_IDMISMATCH;
			}
		}
		if (ccode & 1)
		{
			/* one more reason not to unload executive image */
			ccode = sys$make_ref(&refhdl);
#if TRACE
			tr_print(("mbmon_prv_map - refcnt %d",refhdl.lid->ldrimg$l_refcnt));
#endif
			execlet_mapped = 1;

			/* clear local monitored mailbox table		*/
			memset(mbmon,'\0',sizeof(mbmon));
		}
#if TRACE
		tr_print(("mbmon_prv_map - end - status 0x%x",ccode));
#endif
	}
	return ccode;
}

/*
	mbmon_unmap	- unmap the execlet
*/
static unsigned long
mbmon_unmap(void)
{
	unsigned long ccode;

	if (execlet_mapped)
	{
#if TRACE
		tr_print(("mbmon_prv_unmap - start"));
#endif
		/* one less reason not to unload executive image */
		ccode = sys$remove_ref(&refhdl);
		execlet_mapped = 0;
#if TRACE
		tr_print(("mbmon_prv_unmap - end - status 0x%x",ccode));
		tr_print(("mbmon_prv_unmap - refcnt %d",refhdl.lid->ldrimg$l_refcnt));
#endif
	}
	else
	{
		ccode = SS$_NORMAL;
	}
	return ccode;
}

/*
	mbmon_load	- load mbmon execlet
*/
int
mbmon_load(void)
{
	unsigned long ccode;
	MBMON_VECTOR *vp;
	PRVDEF check_priv = {0};
	extern unsigned long sys$check_privilege();

#if TRACE
	tr_print(("mbmon_prv_load - start"));
#endif

	/* check user has CMKRNL	*/
	check_priv.prv$v_cmkrnl = 1;
	ccode = sys$check_privilege(EFN,
		(struct _generic_64 *)&check_priv,NULL,0,NULL,0,NULL,0);
	if (!(ccode & 1))
	{
#if TRACE
		tr_print(("mbmon_prv_load - user is a luser"));
#endif
	}

	if (ccode & 1)
	{
		ccode = ldr$load_image(&execlet_dsc,LDR$M_UNL|LDR$M_USER_BUF,
			&refhdl,&mbvp);
	}
#if TRACE
	tr_print(("mbmon_prv_load - load image status 0x%x",ccode));
#endif
	if (ccode & 1)
	{
		/* use ldrimg$l_nonpag_w_base to find the vector	*/
		vp = refhdl.lid->ldrimg$l_nonpag_w_base;	

#if TRACE
		tr_print(("mbmon_prv_load - lid 0x%x",refhdl.lid));
		tr_print(("mbmon_prv_load - refcnt %d",refhdl.lid->ldrimg$l_refcnt));
		tr_print(("mbmon_prv_load - mbmon vector addresss 0x%x",vp));
#endif
		/* copy the vector to local storage			*/
		memcpy(&mbmon_vec,vp,sizeof(MBMON_VECTOR));
		
		/* validate vector					*/
		if ((mbmon_vec.version != MBMON_VECTOR_VERSION)
		||  (mbmon_vec.count != MBMON_VECTOR_COUNT))
		{
			ccode = SS$_IDMISMATCH;
		}
	}
#if TRACE
	tr_print(("mbmon_prv_load - end - status 0x%x",ccode));
#endif
	return ccode;
}

/*
	mbmon_unload	- unload mbmon execlet
*/
int
mbmon_unload(void)
{
	unsigned long ccode = SS$_NORMAL;
	PRVDEF check_priv = {0};
	extern unsigned long sys$check_privilege();

#if TRACE
	tr_print(("mbmon_prv_unload - start"));
#endif

	/* check user has CMKRNL	*/
	check_priv.prv$v_cmkrnl = 1;
	ccode = sys$check_privilege(EFN,
		(struct _generic_64 *)&check_priv,NULL,0,NULL,0,NULL,0);
	if (!(ccode & 1))
	{
#if TRACE
		tr_print(("mbmon_prv_unload - user is a luser"));
#endif
	}

	/* locate the loadable execlet	*/
	ccode = ldr$ref_info(&execlet_dsc,&refhdl);

	if (ccode & 1)
	{
#if TRACE
		tr_print(("mbmon_prv_unload - lid 0x%x",refhdl.lid));
		tr_print(("mbmon_prv_unload - refcnt %d",refhdl.lid->ldrimg$l_refcnt));
#endif
		ccode = ldr$unload_image(&execlet_dsc,&refhdl);
	}
	execlet_mapped = 0;

#if TRACE
	tr_print(("mbmon_prv_unload - end - status 0x%x",ccode));
#endif
	return ccode;
}

/*
	mbmon_setup - setup for monitoring mailboxes	
*/
int mbmon_setup(void)
{
	unsigned long ccode;
	extern PCB *CTL$GL_PCB;
	extern void sch_std$iolockr(PCB*);
	extern void sch_std$iounlock(PCB*);
	PRVDEF check_priv = {0};
	extern unsigned long sys$check_privilege();

#if TRACE
	tr_print(("mbmon_prv_setup - start"));
#endif

	/* check for SYSPRV and SYSNAM	*/
	check_priv.prv$v_sysprv = 1;
	check_priv.prv$v_sysnam = 1;
	ccode = sys$check_privilege(EFN, (struct _generic_64 *) &check_priv,NULL, 0, 
		NULL, 0, NULL,0);

	if (!(ccode & 1))
	{
#if TRACE
		tr_print(("mbmon_prv_setup - user is a luser"));
#endif
	}
	ccode = mbmon_map();
	if (ccode & 1)
	{
		/* lock the I/O database for read access	*/
		sch_std$iolockr(CTL$GL_PCB);

		/* setup for monitoring				*/
		ccode = mbmon_vec.setup();

		/* unlock the I/O database			*/
		sch_std$iounlock(CTL$GL_PCB);
	}
#if TRACE
	tr_print(("mbmon_prv_setup - end - status 0x%x",ccode));
#endif
    return ccode;
}

/*
	mbmon_add - add a mailbox to the list to be monitored

	chan to mailbox to be monitored
	chan to mailbox to receive messages written to mb1
*/
int mbmon_add(unsigned long chan1, unsigned long chan2)
{
	register int ccode, i;
	CCB *ccb1, *ccb2;
	MB_UCB *ucb1, *ucb2;
	extern unsigned long ioc$verify_chan(unsigned long chan, CCB **ccb);
	extern PCB *CTL$GL_PCB;
	extern void sch_std$iolockr(PCB*);
	extern void sch_std$iounlock(PCB*);

#if TRACE
	tr_print(("mbmon_prv_add - start"));
#endif

	ccode = mbmon_map();
	if (!(ccode & 1))
		return ccode;

	/* get the CCB addresses	*/
#if TRACE
	tr_print(("mbmon_prv_add - chan1 %x chan2 %x",chan1,chan2));
#endif
	ccode = ioc$verify_chan(chan1,&ccb1);
	if (!(ccode & 1))
	{
#if TRACE
		tr_print(("mbmon_prv_add - chan1 bad status 0x%x",ccode));
#endif
		return ccode;
	}
#if TRACE
	tr_print(("mbmon_prv_add - CCB1 %x",ccb1));
#endif
	ccode = ioc$verify_chan(chan2,&ccb2);
	if (!(ccode & 1))
	{
#if TRACE
		tr_print(("mbmon_prv_add - chan2 bad status 0x%x",ccode));
#endif
		return ccode;
	}
#if TRACE
	tr_print(("mbmon_prv_add - CCB2 %x",ccb2));
#endif

	/* lock the I/O database for read access	*/
	sch_std$iolockr(CTL$GL_PCB);

	/* get ucb addresses from CCBs			*/
	ucb1 = (MB_UCB *)ccb1->ccb$l_ucb;
	ucb2 = (MB_UCB *)ccb2->ccb$l_ucb;
#if TRACE
	tr_print(("mbmon_prv_add - UCB1 %x, UCB2 %x",ucb1,ucb2));
#endif

	if ((ucb1->ucb$r_ucb.ucb$b_devclass == DC$_MAILBOX)
	&&  (ucb2->ucb$r_ucb.ucb$b_devclass == DC$_MAILBOX))
	{
		/* add for monitoring				*/
		ccode = mbmon_vec.add(ucb1,ucb2,CTL$GL_PCB->pcb$l_epid);
	}
	else
	{
		/* not a mailbox	*/
		ccode = SS$_BADPARAM;
	}
	/* unlock the I/O database			*/
	sch_std$iounlock(CTL$GL_PCB);

	/* if all ok add the mailbox to the local table	*/
	if (ccode & 1)
	{
		for (i=0; i < MBMON_MAX_UNIT; i++)
		{
			if (mbmon[i].mb_unit1 == 0)
			{
				/* found a free entry so fill it in	*/
				mbmon[i].mb_unit1 = ucb1->ucb$r_ucb.ucb$w_unit;
				mbmon[i].mb_unit2 = ucb2->ucb$r_ucb.ucb$w_unit;
				mbmon[i].epid = CTL$GL_PCB->pcb$l_epid;
				mbmon[i].mb_ucb1 = ucb1;
				mbmon[i].mb_ucb2 = ucb2;
				break;
			}
		}
	}
#if TRACE
	tr_print(("mbmon_prv_add - end - status 0x%x",ccode));
#endif
	return ccode;
}

int mbmon_remove(unsigned long chan1)
{
	register int ccode, i;
	CCB *ccb1;
	MB_UCB *ucb1;
	extern unsigned long ioc$verify_chan(unsigned long chan, CCB **ccb);
	extern PCB *CTL$GL_PCB;
	extern void sch_std$iolockr(PCB*);
	extern void sch_std$iounlock(PCB*);

#if TRACE
	tr_print(("mbmon_prv_remove - start"));
#endif
	ccode = mbmon_map();
	if (!(ccode & 1))
		return ccode;

	ccode = ioc$verify_chan(chan1,&ccb1);
	if (!(ccode & 1))
	{
#if TRACE
		tr_print(("mbmon_prv_remove - bad chan status 0x%x",ccode));
#endif
		return ccode;
	}
#if TRACE
	tr_print(("mbmon_prv_remove - CCB1 %x",ccb1));
#endif

	/* lock the I/O database for read access	*/
	sch_std$iolockr(CTL$GL_PCB);

	/* get ucb addresses from CCBs			*/
	ucb1 = (MB_UCB *)ccb1->ccb$l_ucb;
#if TRACE
	tr_print(("mbmon_prv_remove - UCB1 %x",ucb1));
#endif
	if (ucb1->ucb$r_ucb.ucb$b_devclass == DC$_MAILBOX)
	{
		/* remove from monitoring			*/
		ccode = mbmon_vec.remove(ucb1,CTL$GL_PCB->pcb$l_epid);
	}
	else
	{
		/* not a mailbox	*/
		ccode = SS$_BADPARAM;
	}

	/* unlock the I/O database			*/
	sch_std$iounlock(CTL$GL_PCB);

	/* remove the mailbox to the local table	*/
	for (i=0; i < MBMON_MAX_UNIT; i++)
	{
		if (mbmon[i].mb_unit1 == ucb1->ucb$r_ucb.ucb$w_unit)
		{
			mbmon[i].mb_unit1 = 0;	/* mark as free	*/
		}
	}
#if TRACE
	tr_print(("mbmon_prv_remove - end - status 0x%x", ccode));
#endif
	return ccode;
}

/*
	image rundown handler
*/
int rundown_handler()
{
	unsigned long ccode;
	int i;
	extern PCB *CTL$GL_PCB;
	extern void sch_std$iolockr(PCB*);
	extern void sch_std$iounlock(PCB*);

#if TRACE
	tr_print(("mbmon_prv_rundown - start")); 
#endif 

	if (rundown_done)
	{
#if TRACE
		tr_print(("mbmon_prv_rundown - already been called")); 
		ccode = SS$_NORMAL;
#endif 
	}
	else
	{
		ccode = mbmon_map();

		/* for each mailbox in the local table call mbmon_xt_remove*/
		for (i=0; i < MBMON_MAX_UNIT; i++)
		{
			if (mbmon[i].mb_unit1 != 0)
			{
				/* lock the I/O database for read access	*/
				sch_std$iolockr(CTL$GL_PCB);
				mbmon_vec.remove(mbmon[i].mb_ucb1,CTL$GL_PCB->pcb$l_epid);

				/* unlock the I/O database			*/
				sch_std$iounlock(CTL$GL_PCB);

				/* mark table entry as free	*/
				mbmon[i].mb_unit1 = 0;
			}
		}

		mbmon_unmap();

		rundown_done = 1;
	}
#if TRACE
	tr_print(("mbmon_prv_rundown - end - status 0x%x",ccode)); 
#endif 

	return ccode;
}
