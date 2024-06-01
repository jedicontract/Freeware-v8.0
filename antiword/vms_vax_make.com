$! these are changes for VAX, reported by Peter Weaver.
$! in fact they are for VMS prior to 7.3 and/or DECC/CRTL prior to V 6.
$! with time I have to make up the correct conditions,
$! see also the local MISC.C for defined(__VAX), which in fact must also
$! be conditioned on DECC and CRTL version ...
$ defines=p1
$ arch_name = "''f$getsyi("ARCH_NAME")'"
$ if defines .eqs. "" then defines="(NDEBUG,__STDC_ISO_10646__)"
$ if p2.eqs."LINK" then goto dolink
$ if arch_name .eqs. "VAX" then CC:= CC/decc
$ if arch_name .eqs. "VAX" then assign sys$library sys
$ CC /DEFINE='defines' main_u
$ CC /DEFINE='defines' asc85enc
$ CC /DEFINE='defines' blocklist
$ CC /DEFINE='defines' chartrans
$ CC /DEFINE='defines' datalist
$ CC /DEFINE='defines' depot
$ CC /DEFINE='defines' dib2eps
$ CC /DEFINE='defines' doclist
$ CC /DEFINE='defines' fail
$ CC /DEFINE='defines' finddata
$ CC /DEFINE='defines' findtext
$ CC /DEFINE='defines' fmt_text
$ CC /DEFINE='defines' fontlist
$ CC /DEFINE='defines' fonts
$ CC /DEFINE='defines' fonts_u
$ CC /DEFINE='defines' hdrftrlist
$ CC /DEFINE='defines' imgexam
$ CC /DEFINE='defines' imgtrans
$ CC /DEFINE='defines' jpeg2eps
$ CC /DEFINE='defines' listlist
$ CC /DEFINE='defines' misc
$ CC /DEFINE='defines' notes
$ CC /DEFINE='defines' options
$ CC /DEFINE='defines' out2window
$ CC /DEFINE='defines' output
$ CC /DEFINE='defines' pdf
$ CC /DEFINE='defines' pictlist
$ CC /DEFINE='defines' png2eps
$ CC /DEFINE='defines' postscript
$ CC /DEFINE='defines' prop0
$ CC /DEFINE='defines' prop2
$ CC /DEFINE='defines' prop6
$ CC /DEFINE='defines' prop8
$ CC /DEFINE='defines' properties
$ CC /DEFINE='defines' propmod
$ CC /DEFINE='defines' rowlist
$ CC /DEFINE='defines' sectlist
$ CC /DEFINE='defines' stylelist
$ CC /DEFINE='defines' stylesheet
$ CC /DEFINE='defines' summary
$ CC /DEFINE='defines' tabstop
$ CC /DEFINE='defines' text
$ CC /DEFINE='defines' unix
$ CC /DEFINE='defines' utf8
$ CC /DEFINE='defines' word2text
$ CC /DEFINE='defines' worddos
$ CC /DEFINE='defines' wordlib
$ CC /DEFINE='defines' wordmac
$ CC /DEFINE='defines' wordole
$ CC /DEFINE='defines' wordwin
$ CC /DEFINE='defines' xmalloc
$ CC /DEFINE='defines' xml
$!
$ DELETE/NOLOG antiword.olb;*
$ Library antiword.olb/create/object *.obj
$dolink:
$ if arch_name .eqs. "VAX"
$ 	then
$ 		LINK/EXEC=ANTIWORD antiword/libr/include=main_u,antiword/library,sys$input:/options
		sys$share:decc$crtl/shareable
$	else
$ 		LINK/EXEC=ANTIWORD antiword/libr/include=main_u,antiword/library
$	endif
$!
