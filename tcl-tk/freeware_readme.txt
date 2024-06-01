Tcl/Tk, Programming, Tcl/Tk 8.0.5 for OpenVMS

The Tcl8_0_5.zip and Tk8_0_5.zip are ports of the Tcl8.0.5 and Tk8.0.5 sources
onto OpenVMS V7.3-2.   For the OpenVMS build, this submission requires MMS from
the DECset product (or a compatible makefile tool), and the HP C compiler (and
preferably C V6.5 or above).

Unzip the files with the commands:

            $ unzip Tcl8_0_5.zip
            $ unzip Tk8_0_5.zip

on any ODS-2 or ODS-5 disk. Then enter the following commands in order to build
from the sources:

            $ set def [.tcl8_0_5.vms]
	    $ mms all
	    $ set def [-.-.tk8_0_5.vms]
            $ mms all


More information on Tcl and Tk software can be found at:

    http://sourceforge.net/docman/display_doc.php?docid=958&group_id=10894

Porting work done by Philippe Vouters (HP France), using OpenVMS Alpha V7.3-2.

