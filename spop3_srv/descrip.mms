cflags = /reentrancy=multithread/include=[]/lis/nowar
!/debu/noopt
linkflags = /trace/threads=(multiple_kernel_threads,upcalls)
!/debu/trace

target	= spop3_msg.h, spop3def.h, spop3_srv.exe

!*********************************************************************
objs	=	spop3_srv.obj,spop3_msg.obj,spop3_vmsmail.obj,spop3_netio.obj
incl	= 	spop3_msg.h, spop3def.h

!
all		:$(target)

$(objs)		: $(incl)

spop3_srv.exe	: $(objs)
	link  $(linkflags) $(objs)

!*********************************************************************

spop3def.h		:spop3def.sdl
	sdl/alpha/language=(cc)/vms /c_dev spop3def.sdl

spop3_msg.h		:spop3_msg.msg
	message/sdl	spop3_msg.msg
	sdl/alpha/language=(cc) spop3_msg.sdl

spop3_msg.obj		:spop3_msg.msg
	message/obj	spop3_msg.msg



!*********************************************************************
clean	:
  @ delete/log *.obj;*,*.lis;*,*.exe;*
!*********************************************************************

kit	:
	zip "-V" SPOP3_SRV	spop*.c spop*.h *.mms spop*.msg *.sdl
	zip "-V" SPOP3_SRV	README.TXT spop*.com

prod	:
	copy	spop3*.com,spop3*.exe	disk$htlogs:[spop3]
	purge	disk$htlogs:[spop3]	/before="-7-" /log
