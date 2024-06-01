/*
	MBU1	- set/show defaults
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
#pragma module MBU1 "V01-003"

#include <climsgdef.h>
#include <syidef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mbu.h"

static $DESCRIPTOR(bufquo_dsc,"BUFQUO");
static $DESCRIPTOR(count_dsc,"COUNT");
static $DESCRIPTOR(data_size_dsc,"DATA_SIZE");
#ifdef DESC
static $DESCRIPTOR(description_dsc,"DESCRIPTION");
#endif
static $DESCRIPTOR(format_dsc,"FORMAT");
static $DESCRIPTOR(input_dsc,"INPUT");
static $DESCRIPTOR(length_dsc,"LENGTH");
static $DESCRIPTOR(maxmsg_dsc,"MAXMSG");
static $DESCRIPTOR(output_dsc,"OUTPUT");
static $DESCRIPTOR(promsk_dsc,"PROMSK");
static $DESCRIPTOR(width_dsc,"WIDTH");
static $DESCRIPTOR(verify_dsc,"VERIFY");


/*
	set_default_cmd	- set defaults
*/
unsigned long
set_default_cmd(void)
{
	unsigned short len;
	unsigned long ccode;
	$DESCRIPTOR_D(value_dsc);

	def_format = get_format();
	def_data_size = get_data_size();
	def_width = get_width();
#ifdef DESC
	ccode = CLI$PRESENT(&description_dsc);
	if (ccode == CLI$_NEGATED || ccode == CLI$_LOCNEG)
	{
		if (def_description != NULL)
		{
			free(def_description);
			def_description = NULL;
		}
		
	}
	else
	{
		ccode = CLI$GET_VALUE(&description_dsc,&value_dsc,&len);
		if ((ccode & 1) != 0)
		{
			if (def_description != NULL)
				free(def_description);
			def_description = malloc(len + 1);
			strncpy(def_description,value_dsc.dsc$a_pointer,len);
			def_description[len] = '\0';
			LIB$SFREE1_DD(&value_dsc);
		}
	}
#endif
	ccode = CLI$GET_VALUE(&output_dsc,&value_dsc,&len);
	if ((ccode & 1) != 0)
	{
		if (def_output != NULL)
			free(def_output);
		def_output = malloc(len + 1);
		strncpy(def_output,value_dsc.dsc$a_pointer,len);
		def_output[len] = '\0';
		LIB$SFREE1_DD(&value_dsc);
	}
	ccode = CLI$GET_VALUE(&input_dsc,&value_dsc,&len);
	if ((ccode & 1) != 0)
	{
		if (def_input != NULL)
			free(def_input);
		def_input = malloc(len + 1);
		strncpy(def_input,value_dsc.dsc$a_pointer,len);
		def_input[len] = '\0';
		LIB$SFREE1_DD(&value_dsc);
	}
	ccode = CLI$GET_VALUE(&maxmsg_dsc,&value_dsc,&len);
	if ((ccode & 1) != 0)
	{
		def_maxmsg = atoi(value_dsc.dsc$a_pointer);
		LIB$SFREE1_DD(&value_dsc);
	}
	ccode = CLI$GET_VALUE(&bufquo_dsc,&value_dsc,&len);
	if ((ccode & 1) != 0)
	{
		def_bufquo = atoi(value_dsc.dsc$a_pointer);
		LIB$SFREE1_DD(&value_dsc);
	}
	ccode = CLI$GET_VALUE(&length_dsc,&value_dsc,&len);
	if ((ccode & 1) != 0)
	{
		def_length = atoi(value_dsc.dsc$a_pointer);
		LIB$SFREE1_DD(&value_dsc);
	}
	ccode = CLI$GET_VALUE(&count_dsc,&value_dsc,&len);
	if ((ccode & 1) != 0)
	{
		def_count = atoi(value_dsc.dsc$a_pointer);
		LIB$SFREE1_DD(&value_dsc);
	}
	ccode = CLI$GET_VALUE(&promsk_dsc,&value_dsc,&len);
	if ((ccode & 1) != 0)
	{
		ccode = parse_prot(value_dsc.dsc$a_pointer,
				   value_dsc.dsc$w_length,
				   0xFFFF,
				   &def_promsk);
   		if ((ccode & 1) == 0)
   			ccode = MBU__SYNTAX;
		LIB$SFREE1_DD(&value_dsc);
	}
   	else
   		ccode = MBU__NORMAL;

	LIB$SFREE1_DD(&value_dsc);

	return(ccode);
}

/*
	get_format	- get data format 
*/
FORMAT
get_format(void)
{
	$DESCRIPTOR_D(value_dsc);
	unsigned short len;
	unsigned long ccode;
	FORMAT format;

	ccode = CLI$GET_VALUE(&format_dsc,&value_dsc,&len);
	if ((ccode & 1) != 0)
	{
		if (*value_dsc.dsc$a_pointer == 'H')
			format = HEX;
		else if (*value_dsc.dsc$a_pointer == 'D')
			format = DECIMAL;
		else if (*value_dsc.dsc$a_pointer == 'T')
			format = TEXT;
		else
		{
			putmsg(MBU__BADFORMAT);
			format = def_format;
		}
	}
	else
		format = def_format;

	LIB$SFREE1_DD(&value_dsc);
	return(format);
}

/*
	get_data_size	- get data size
*/
DATA_SIZE 
get_data_size(void)
{
	$DESCRIPTOR_D(value_dsc);
	unsigned short len;
	unsigned long ccode;
	DATA_SIZE data_size;

	ccode = CLI$GET_VALUE(&data_size_dsc,&value_dsc,&len);
	if ((ccode & 1) != 0)
	{
		if (*value_dsc.dsc$a_pointer == 'B')
			data_size = BYTE;
		else if (*value_dsc.dsc$a_pointer == 'W')
			data_size = WORD;
		else if (*value_dsc.dsc$a_pointer == 'L')
			data_size = LONG;
		else
		{
			putmsg(MBU__BADSIZE);
			data_size = def_data_size;
		}
	}
	else
		data_size = def_data_size;

	LIB$SFREE1_DD(&value_dsc);
	return(data_size);
}

/*
	get_width
*/
unsigned long 
get_width(void)
{
	$DESCRIPTOR_D(value_dsc);
	unsigned short len;
	unsigned long ccode, width;

	ccode = CLI$GET_VALUE(&width_dsc,&value_dsc,&len);
	if ((ccode & 1) != 0)
	{
		width = atoi(value_dsc.dsc$a_pointer);
	}
	else
		width = def_width;

	if (width < MIN_WIDTH)
	{
		putmsg(MBU__INVVAL);
		width = MIN_WIDTH;
	}

	if (width > MAX_WIDTH)
	{
		putmsg(MBU__INVVAL);
		width = MAX_WIDTH;
	}

	LIB$SFREE1_DD(&value_dsc);
	return(width);
}

/*
	show_default_cmd - show defaults
*/
unsigned long 
show_default_cmd(void)
{
	putmsg(MBU__DEFAULTS);
	printf("Format is ");
	switch (def_format)
	{
	case HEX:
		puts("HEX");
		break;
	case DECIMAL:
		puts("DECIMAL");
		break;
	case TEXT:
		puts("TEXT");
		break;
	default:
		putmsg(MBU__BADFORMAT);
	}
	printf("Data size is ");
	switch (def_data_size)
	{
	case BYTE:
		puts("BYTE");
		break;
	case WORD:
		puts("WORD");
		break;
	case LONG:
		puts("LONG");
		break;
	default:
		putmsg(MBU__BADSIZE);
	}
#ifdef DESC
	if (def_description != NULL)
		printf("Description is %s\n",def_description);
	else
		puts("No default description");
#endif
	printf("Mailbox buffer quota is %u\n",def_bufquo);
	printf("Mailbox maximum message size is %u\n",def_maxmsg);
	printf("Default display width is %u\n",def_width);
        printf("Default protection is %s\n",display_prot(def_promsk));
	if (def_length != 0)
		printf("Limit on length of message displayed is %u\n",
			def_length);
	else
		puts("Length of message displayed is not limited by default");
	if (def_count != 0)
		printf("Limit on number of messages displayed is %u\n",
			def_count);
	else
		puts("Number of messages displayed is not limited by default");
	if (def_input != NULL)
		printf("Input file is %s\n",def_input);
	else
		puts("Input file is SYS$INPUT");
	if (def_output != NULL)
		printf("Output file is %s\n",def_output);
	else
		puts("Output file is SYS$OUTPUT");

	printf("Command verify is %s\n",verify_flag?"on":"off");

	return(MBU__NORMAL);
}

/*
   	display_prot	- display protection mask
*/
char *
display_prot(unsigned long promsk)
{
	static char buf[80];
	static char category[4] = {'S','O','G','W'};
	static char access[4] = {'R','W','P','L'};
	unsigned long mask, i, j;

	/* i is the bit position 0-15, j is the index into output buffer */
	for (mask=1,i=0,j=0; i < 16;  i++, mask = mask << 1)
	{
		if ((i % 4) == 0)	/* if start of category */
		{
			if (i != 0)	/* if not first category*/
				buf[j++] = ',';	/* comma seperator	*/
			buf[j++] = category[i/4];
			buf[j++] = ':';
		}
		if ((promsk & mask) == 0)
			buf[j++] = access[i%4];
	}
	buf[j] = '\0';
	return(buf);
}

/*
	ini_defaults	- initialise command defaults
*/
void
ini_defaults(void)
{
	unsigned long ccode;
	extern unsigned long LIB$GETSYI();

	def_format = HEX;
	def_data_size = BYTE;
#ifdef DESC
	def_description = NULL; /* no description	*/
#endif
	ccode = LIB$GETSYI(&SYI$_DEFMBXBUFQUO,&def_bufquo);
	ccode = LIB$GETSYI(&SYI$_DEFMBXMXMSG,&def_maxmsg);
	def_length = 0;	   	/* no limit		*/
	def_count=0;		/* no limit		*/
	def_output = NULL;	/* no output file	*/
	def_input = NULL;	/* no input file	*/
	def_width = 80;		/* NB - SHOULD REALLY BE TERMINAL WIDTH */
   	def_promsk = 0;		/* all access allowed	*/
	verify_flag = FALSE;	/* no verify output	*/
}

/*
	set_verify_cmd
*/
unsigned long 
set_verify_cmd(void)
{
	unsigned long ccode;

	ccode = CLI$PRESENT(&verify_dsc);
	if (ccode == CLI$_NEGATED || ccode == CLI$_LOCNEG)
		verify_flag = FALSE;
	else
		verify_flag = TRUE;
	return MBU__NORMAL;
}
