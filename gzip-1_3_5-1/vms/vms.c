/* vms.c -- target dependent functions for VMS
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License, see the file COPYING.
 *
 * This file was written by Karl-Jose Filler <pla_jfi@pki-nbg.philips.de>
 * and updated by Jean-loup Gailly.
 *
 * Additional updates on 5-Nov-2001 and 29-Jul-2005 by Stephen Hoffman, 
 * OpenVMS Engineering.
 *
 */

#include <descrip.h>
#include <lib$routines.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char **vms_argv = NULL;

static int max_files = 10000;

typedef struct dsc$descriptor DscT;
extern void *xmalloc( unsigned size );

int find_file_c();

vms_expand_args(old_argc, argv)
    int *old_argc;
    char **argv[];
{
    int	    i;
    int	    new_argc = 0;
    int	    context, status;
    char    buf[255], *p;
    
    vms_argv = (char**)xmalloc((max_files+1)*sizeof(char*));

    vms_argv[new_argc++] = **argv;

    for (i=1; i < *old_argc; i++) {
	if (*argv[0][i] == '-') {   /* switches */
	    if (new_argc < max_files) {
		vms_argv[new_argc++] = argv[0][i];
	    }
	} else {		    /* Files */
	    context = 0;
	    if (find_file_c(argv[0][i], buf, sizeof(buf), &context) & 1 != 1) {
		/* 
	         * Wrong file ?
		 * forward it to gzip
		 */
		if (new_argc < max_files) {
		    vms_argv[new_argc++] = argv[0][i];
		}
	    } else {
		p = (char*)xmalloc((int)strlen(buf)+1);
		strcpy(p, buf);
		if (new_argc < max_files) {
		    vms_argv[new_argc++] = p;
		}
		while (find_file_c(argv[0][i], buf, 
		       sizeof(buf), &context) & 1 == 1) {
		    p = (char*)xmalloc((int)strlen(buf)+1);
		    strcpy(p, buf);
		    if (new_argc < max_files) {
			vms_argv[new_argc++] = p;
		    }
		}
	    }
	}
    }
    if (new_argc <= max_files) {
	*old_argc = new_argc;
	vms_argv[new_argc] = NULL;
	*argv = vms_argv;
    } else {
	free(vms_argv); /* the expanded file names should also be freed ... */
	vms_argv = NULL;
	max_files = new_argc + 1;
	vms_expand_args(old_argc, argv);
    }
}

int find_file_c(in,out,out_len,context)
    char *in;
    char *out;
    int   out_len;
    int  *context;
{
    DscT	in_desc = { 0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL };
    DscT	out_desc = { 0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL };
    int		status;
    char	*p;
  
    in_desc.dsc$a_pointer = in;
    in_desc.dsc$w_length = strlen(in);
  
    out_desc.dsc$a_pointer = out;
    out_desc.dsc$w_length = out_len;
  
    status = lib$find_file((void*) &in_desc,(void*) &out_desc,context);

    p   = out_desc.dsc$a_pointer;
    while(*p != ' ') {
	p++;
    }
    *p = 0;
  
    return status;
}
