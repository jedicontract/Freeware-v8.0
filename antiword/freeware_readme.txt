ANTIWORD,Miscellaneous, a MS Word document file reader/converter

Antiword 0.37 for VMS
Antiword is a MS Word document file reader, it extracts the plain text,
or converts it into a Postscript file.
Original Author: Adri van Os. Homepage: http://www.winfield.demon.nl/
Ported to VMS by Joseph Huber,Homepage: http://wwwvms.mppmu.mpg.de/~huber/pds/

Build instructions for VMS:
 Set default [.SRC]
 With GNU make (gmake) under DCL: Execute "(g)make -f [-]makefile.vms" .
 With GNV bash shell: make -f ../makefile.vms_bash .
 With MMS or MMK : MMx/descr=[-]DESCRIP.MMS .
 Without make or MMS, compile everything, except main_r, 
  then link main_u  and all other object files.
  (all commands are in [-]vms_make.com).

Installation:
   define a foreign command pointing to antiword.exe
 or
   copy antiword.exe into dcl$path:
 Setup/font files:
 System-wide: copy the [.resources] files into the directory defined by 
  /usr/share/antiword
 or leave the [.resources] subdirectory in place, and let the user
 (or sys$sylogin) execute this procedure: antiword_setup.com
 Private: create directory [.ANTIWORD] in sys$login:
          copy the [.resources] files into the newly created directory.

 Eventually add [.Docs]antiword.hlp to a helpfile of Your choice:
  $ LIBRARIAN sys$help:site/help [.Docs]antiword

 Usage:
 Produce a postscript file from a word-document:
   pipe antiword -p a4 file.doc >file.ps

 A test document is in [.Docs]testdoc.doc .

A DCL commandfile in DCL$PATH directory does the postscript conversion 
and X11 display with Ghostscript in one go: wordviewer.com .
Use it in a commandline like this: wordviewer testdoc

Further info see antiword_vms.html.

Joseph.Huber at mppmu.mpg.de
