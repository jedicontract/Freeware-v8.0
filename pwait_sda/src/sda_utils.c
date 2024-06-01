/*
	SDA_UTILS - SDA extension utility routines

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
#pragma module SDAU "V1.0"

#define __NEW_STARLET 1
#include <arch_defs.h>
#include <sda_routines.h>
#include <varargs.h>
#include <descrip.h>
#include <ssdef.h>
#include <rmsdef.h>
#include <starlet.h>
#include <stsdef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <gen64def.h>
#include <ints.h>


#include "sda_utils.h"

/*
	sext - sign extend an address to 64 bits

	ap		- ap ptr to address 
*/
void
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
int 
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
