/* Copyright (C) 2006 Alexey Chupahin  elvis_75@mail.ru
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*  -*- c-file-style: "linux" -*-
 *
 * Copyright (C) 1996-2000 by Andrew Tridgell
 * Copyright (C) Paul Mackerras 1996
 * Copyright (C) 2001, 2002 by Martin Pool <mbp@samba.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/**
 * This is like socketpair but uses tcp. It is used by the Samba
 * regression test code.
 *
 * The function guarantees that nobody else can attach to the socket,
 * or if they do that this function fails and the socket gets closed
 * returns 0 on success, -1 on failure the resulting file descriptors
 * are symmetrical.
 **/


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <string.h>
  #include <sys/types.h>
  #include <netinet/in.h>
  #include <netdb.h>
  #include <unistd.h>
#include <config.h>

#ifdef O_NONBLOCK
# define NONBLOCK_FLAG O_NONBLOCK
#elif defined SYSV
# define NONBLOCK_FLAG O_NDELAY
#else
# define NONBLOCK_FLAG FNDELAY
#endif

void set_blocking(int fd)
{
        int val;

        if ((val = fcntl(fd, F_GETFL, 0)) == -1)
                return;
        if (val & NONBLOCK_FLAG) {
                val &= ~NONBLOCK_FLAG;
                fcntl(fd, F_SETFL, val);
        }
}

void set_nonblocking(int fd)
{
        int val;

        if ((val = fcntl(fd, F_GETFL, 0)) == -1)
                return;
        if (!(val & NONBLOCK_FLAG)) {
                val |= NONBLOCK_FLAG;
                fcntl(fd, F_SETFL, val);
        }
}

int socketpair(int domain, int type, int protocol, int *fd)
{
        int listener;
        struct sockaddr_in sock;
        struct sockaddr_in sock2;
        size_t socklen = sizeof sock;
        int connect_done = 0;

        fd[0] = fd[1] = listener = -1;

        memset(&sock, 0, sizeof sock);

        if ((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1)
                goto failed;

        memset(&sock2, 0, sizeof sock2);
#ifdef HAVE_SOCKADDR_IN_LEN
        sock2.sin_len = sizeof sock2;
#endif
        sock2.sin_family = PF_INET;

        bind(listener, (struct sockaddr *)&sock2, sizeof sock2);

        if (listen(listener, 1) != 0)
                goto failed;

        if (getsockname(listener, (struct sockaddr *)&sock, &socklen) != 0)
                goto failed;

        if ((fd[1] = socket(PF_INET, SOCK_STREAM, 0)) == -1)
                goto failed;

        set_nonblocking(fd[1]);

        sock.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (connect(fd[1], (struct sockaddr *)&sock, sizeof sock) == -1) {
                if (errno != EINPROGRESS)
                        goto failed;
        } else
                connect_done = 1;

        if ((fd[0] = accept(listener, (struct sockaddr *)&sock, &socklen)) == -1)
                goto failed;

        close(listener);
        listener = -1;

        set_blocking(fd[1]);

        if (connect_done == 0) {
                if (connect(fd[1], (struct sockaddr *)&sock, sizeof sock) != 0
                    && errno != EISCONN)
                        goto failed;
        }

        /* all OK! */
        return 0;

 failed:
        if (fd[0] != -1)
                close(fd[0]);
        if (fd[1] != -1)
                close(fd[1]);
        if (listener != -1)
                close(listener);
        return -1;
}

int fork()
{
 return (  (int)vfork() );
}

int mkfifo(const char *path, unsigned short mode)
{
return (creat(path, 0777));
}
/* This simple routine translate full OpenVMS path DISK:[PATH1...PATHn] or DISK:[PATH1...PATHn]file or [.PATH1...PATHn]file
** 
only for use only with clamav. But anybody can patch this and use for anything 
(c) Alexey Chupahin 17-MAR-2006  under GPL
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *vms2unix(char *src)
{
char *dst;
int  i,j;
int DEV=0; // flag is device present in the src path
int FIL=1;
/* length of unix-style path is less or equal VMS-style path */

if(src==NULL) return NULL;

dst= (char *)malloc(strlen(src));

if ( (!strchr(src,':')) && (!strchr(src,'[')) )  j=0;  /* if there is no "disk:[ "  only simple file name */
  else j=1;

for (i=0; i<strlen(src); i++)
{
 switch (src[i])
 {
   case ':' :
	if (i==0) { return NULL; }
	DEV=1;
	dst[j++]='/';
        break;
   case '[' : 
   	FIL=0;
	if ( (!DEV) && (src[i+1]!='.') ) { return NULL; } /* incorrect path like ddd[PATH1...PATHn] */
	if ( DEV && (src[i+1]!='.') ) 
		{ dst[0]='/'; dst[j++]='/'; break; }
	if ( (!DEV) && (src[i+1] == '.') && (i==0) ) { dst[0]='.'; dst[1]='/'; j=2; break;}
		else { return NULL; } /* incorrect path like dddd[PATH1...PATHn] */

	break;
    case '.' : if ( (i>0) && (!FIL) ) {    if (src[i-1] != '[') { dst[j++]='/'; }    }
	       if (FIL) { dst[j++] = src[i]; } 
	  break;
    case ']' :  dst[j++]='/'; FIL=1;
          break;
    default : dst[j++] = src[i];
  }	// switch
 } // for	      

dst[j]=(char) 0;
return (dst);
}

char * vms_deldots( char *name )
{
char *i, *name_dup=strdup(name);
for (i=strchr(name_dup, (int)'.');i=strchr(name_dup, (int)'.');) { *i='_'; }
return name_dup; 
}

/*
main(int argc, char **argv)
{
printf("%s\n",vms2unix(argv[1]));
}
*/
