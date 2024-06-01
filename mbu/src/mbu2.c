/*
	MBU2	- create and delete commands
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
#pragma module MBU2 "V01-003"

#include <stdio.h>
#include <stdlib.h>
#include <climsgdef.h>

#include "mbu.h"

/* local data	*/
static $DESCRIPTOR(name_dsc,"NAME");


/*
	create_cmd	- execute create command
*/
unsigned long 
create_cmd(void)
{
	extern unsigned long SYS$CREMBX();
	unsigned long ccode, maxmsg, bufquo, promsk, prmflg;
	static $DESCRIPTOR(bufquo_dsc,"BUFQUO");
	static $DESCRIPTOR(maxmsg_dsc,"MAXMSG");
	static $DESCRIPTOR(promsk_dsc,"PROMSK");
	static $DESCRIPTOR(temp_dsc,"TEMPORARY");
	static $DESCRIPTOR(perm_dsc,"PERMANENT");
	$DESCRIPTOR_D(value_dsc);
	unsigned short chan;

	/* set defaults		*/
	maxmsg = def_maxmsg;
	bufquo = def_bufquo;
	promsk = def_promsk;
	prmflg = 1;	/* permanent is default	*/

	/* get values given	*/
	ccode = CLI$GET_VALUE(&bufquo_dsc,&value_dsc);
	if ((ccode & 1) != 0)
	{
		bufquo = atoi(value_dsc.dsc$a_pointer);
		LIB$SFREE1_DD(&value_dsc);
	}
	ccode = CLI$GET_VALUE(&maxmsg_dsc,&value_dsc);
	if ((ccode & 1) != 0)
	{
		maxmsg = atoi(value_dsc.dsc$a_pointer);
		LIB$SFREE1_DD(&value_dsc);
	}
	ccode = CLI$GET_VALUE(&promsk_dsc,&value_dsc);
	if ((ccode & 1) != 0)
	{
		ccode = parse_prot(value_dsc.dsc$a_pointer,
				   value_dsc.dsc$w_length,def_promsk,
				   &promsk);
		LIB$SFREE1_DD(&value_dsc);
   		if ((ccode & 1) == 0)
   			return(MBU__SYNTAX);
	}
	ccode = CLI$PRESENT(&temp_dsc);
	if (ccode == CLI$_PRESENT || ccode == CLI$_LOCPRES 
	||  ccode == CLI$_DEFAULTED)
		prmflg = 0;
	ccode = CLI$PRESENT(&perm_dsc);
	if (ccode == CLI$_PRESENT || ccode == CLI$_LOCPRES
	||  ccode == CLI$_DEFAULTED)
		prmflg = 1;
	ccode = CLI$GET_VALUE(&name_dsc,&value_dsc);
	if (!(ccode))
		return ccode;	

	/* create the mailbox	*/
	ccode = SYS$CREMBX(prmflg,&chan,maxmsg,bufquo,promsk,0,&value_dsc);

	LIB$SFREE1_DD(&value_dsc);
	return(ccode);
}

/*
	delete_cmd	- execute delete command
*/
unsigned long
delete_cmd(void)
{
	extern unsigned long SYS$ASSIGN(), SYS$DELMBX(), SYS$DASSGN();
	$DESCRIPTOR_D(value_dsc);
	unsigned long ccode;
	unsigned short chan;

	/* get the mailbox name	*/
	CLI$GET_VALUE(&name_dsc,&value_dsc);

	ccode = SYS$ASSIGN(&value_dsc,&chan,0,0);

	if ((ccode & 1) != 0)
	{
		ccode = SYS$DELMBX(chan);
		SYS$DASSGN(chan);
	}


	LIB$SFREE1_DD(&name_dsc);
	return(ccode);
}
