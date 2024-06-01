LISTEN,UTILITIES,listen to LAN and make a list of names and addresses

DESCRIPTION

	Listen to various multicast messages on the ethernet to find out
	what is on the lan. Originally I created a program to listen to
	MOP SYSTEM ID messages.	Then I discovered MOP V4 messages output by
	DECnet/OSI nodes so I added these. Then I found out that various other
	protocols involve multicast messages with would allow me to discover
	more nodes on the LAN.

	LISTEN reads various messages and keeps a table of what it has found. 
	On exit a file is written containing the information from the table. 

	This program uses the famous undocumented NETACP QIO interface to
	attempt to translate DECnet addresses to names. Apart from that no other
	undocumented interfaces are used. Privs are required to run this.

	This program has been used in the following environments

		VAXC V3.2 VMS VAX V5.5-2. 
		VAXC V3.2 VMS VAX V6.2
		DECC V5.0 VMS AXP V6.2	
		DECC V6.5 VMS Alpha V7.3-2
		HP C V7.1 VMS I64 V8.2

	It may work on other versions.

	Any changes or bugs contact me at miller@encompasserve.org

        I have found that I only see ARP messages using promiscious mode 
        which is why LISTEN2 was written. 
        Why ? I don't know.

USAGE

	There are the following programs. 

	a) LISTEN_SYSID which listens for MOP V3 and V4 SYSTEM ID messages.
	b) LISTEN which listens for MOP SYSID messages and other things.
        c) LISTEN2 which listens for MOP SYSID messages and other things
                using promisicous mode.
	c) LIST_LISTEN_TABLE which reads the file produced by the other
		programs and writes a text file.
	d) REPORT_LISTEN_TABLE which is like LIST_LISTEN_TABLE but is
		supposed to make more readable output.
	e) PURGE_LISTEN_TABLE removes from the LISTEN_TABLE file any
		data older than a specified delta time.
	
	LISTEN2 is supplied already built (on VAX/VMS V6.2, Alpha VMS V6.2
	and I64 VMS V8.2). See below for instructions to build the others.

	To use LISTEN2 or the others then specify which LAN interface
	to listen then define the logical ETH.

	e.g DEFINE ETH EWB0:
	If the logical is not defined the programs use a built in table.

	Then run the program. 
	Messages are output about new information discovered.
	Stop the program with control-C. This is trapped and causes the program
	to break out of the read message loop, save the collected data and exit.
        LISTEN2 also saves the data if forced to exit.
	
	If control-C does not work then use CONTROL-Y and EXIT.

        I now generally use LISTEN2 although LISTEN and LISTEN_SYSID should
        still work and update the same file.

	The file is called LISTEN_TABLE.DATA in the default directory. 
	To use another file specification define the logical name 
	LISTEN_TABLE.

	To see whats in LISTEN_TABLE then run LIST_LISTEN_TABLE
	This program prompts for a filename (default output to the terminal).

        REPORT_LISTEN_TABLE is an attempt to make more meaningful reports
        than LIST_LISTEN_TABLE.

	PURGE_LISTEN_TABLE is an attempt to clear out old junk in the
	LISTEN_TABLE file.

	The listening programs various messages as information is detected.
	This is controlled by the variable log_mask. Change it if you want
	more or less logging. See listen_table.h for possible values.

BUILDING

	The programs can be built using the DCL procedures 

		BLISTEN.COM,BLISTEN2.COM,BSYSID.COM,BLIST.COM,BREPORT.COM

	or BUILD.COM to build them all.

	Both assume the current default directory contains the sources. 

	There are a couple of macros defined in LISTEN_SUBS.C than can
	be changed.

	If IP_NAMES is defined non-zero then get_ip_name calls gethostbyaddr
	to get the name correspoding to a IP address. This means this program
	must be linked with the TCP/IP library
		TCP/IP Services for OpenVMS VAX	SYS$LIBRARY:UCX$IPC.OLB
		TCP/IP Services for OpenVMS Alpha SYS$SHARE:UCX$IPC_SHR.EXE

	Define this to be 0 if you have no TCP/IP library. To build using
        MMK with IP NAMES support

        MMK/MACRO=IP_NAMES=1

	If USE_NETACP is defined non-zero then get_nodename will attempt to 
	use the famous undocumented NETACP QIO interface to translate DECnet 
	addresses to names. This requires DECnet/IV and privs (SYSPRV/SYSNAM?).
	This is defined by default. Change USE_NETACP to 0 if you are to
	run on a DECnet/OSI node.

FILES

        FORMAT_SYSID.C  - routines to format a MOP V3 or V4 SYSTEM ID message.
        LISTEN.C        - LISTEN program main
        LISTEN2.C       - LISTEN2 program main
        LISTEN_SUBS.C   - routines used by LISTEN and LISTEN_SYSID
        LISTEN_SUBS.H   - defintions of the routines in LISTEN_SUBS.C
        LISTEN_SYSID.C  - program that listens to MOP SYSID messages only.
        LISTEN_PROCESS.C - message processing routines for LISTEN and LISTEN2
        LISTEN_PROCESS.H - defintions of routines in LISTEN_PROCESS.C
        LISTEN_TABLE.C  - LISTEN table handling.
        LISTEN_TABLE.H  - LISTEN table defintions.
        LIST_LISTEN_TABLE.C - program to display LISTEN_TABLE.DATA
        LISTEN_TABLE.DATA - table of info saved by LISTEN or LISTEN_SYSID
        NMADEF.H        - defintions for LAN driver (from STARLET.MLB)
        SYSID.H         - defintions of MOP SYSTEM ID messages.
        ETHERNET.H      - defintions of ethernet frame headers
        MESSAGES.H      - defintions of various messages seen on the lan.
        LISTEN_VAX.OPT  - link options file used on VAX/VMS
        LISTEN_ALPHA.OPT - link options file used on ALPHA/VMS
        REPORT_LISTEN_TABLE.C - program to generate reports from LISTEN_TABLE
	PURGE_LISTEN_TABLE.C - program to remove old data from LISTEN_TABLE

NOTES
	The file format is intended to be extensible so later versions of the
	program may save other information.

	The file can be sorted so the node records are in order of ethernet
	address simply by

		SORT LISTEN_TABLE.DATA LISTEN_TABLE.DATA

	I also intend to write something to allow some changes to 
	LISTEN_TABLE.DATA (remove erroneous names etc, add names not 
		obtainable by listening).

        At present LISTEN2 only looks at messages sent to multicast destination
        addresses to reduce the number of messages it has to look at.
	(all IP and ARP messages are looked at).

        Later versions may look at all traffic.

BUGS
        Probably lots but here is some I know about.

        Analysis of packet headers to determine frame format in
        LISTEN2.C\analyse_hdr sometimes gets the wrong result.

        Some nodes are marked as LAST servers but are not.

	Processing control-C as a signal does not appear to work with
	DECC V6.5 on VMS V7.3-2. Use control-Y then EXIT to exit.

LICENSE

The Artistic License

Preamble

The intent of this document is to state the conditions under which a Package
may be copied, such that the Copyright Holder maintains some semblance of
artistic control over the development of the package, while giving the users of
the package the right to use and distribute the Package in a more-or-less
customary fashion, plus the right to make reasonable modifications.

Definitions:

    * "Package" refers to the collection of files distributed by the Copyright
Holder, and derivatives of that collection of files created through textual
modification.
    * "Standard Version" refers to such a Package if it has not been modified,
or has been modified in accordance with the wishes of the Copyright Holder.
    * "Copyright Holder" is whoever is named in the copyright or copyrights for
the package.
    * "You" is you, if you're thinking about copying or distributing this
Package.
    * "Reasonable copying fee" is whatever you can justify on the basis of
media cost, duplication charges, time of people involved, and so on. (You will
not be required to justify it to the Copyright Holder, but only to the
computing community at large as a market that must bear the fee.)
    * "Freely Available" means that no fee is charged for the item itself,
though there may be fees involved in handling the item. It also means that
recipients of the item may redistribute it under the same conditions they
received it.

1. You may make and give away verbatim copies of the source form of the
Standard Version of this Package without restriction, provided that you
duplicate all of the original copyright notices and associated disclaimers.

2. You may apply bug fixes, portability fixes and other modifications derived
from the Public Domain or from the Copyright Holder. A Package modified in such
a way shall still be considered the Standard Version.

3. You may otherwise modify your copy of this Package in any way, provided that
you insert a prominent notice in each changed file stating how and when you
changed that file, and provided that you do at least ONE of the following:

    a) place your modifications in the Public Domain or otherwise make them
Freely Available, such as by posting said modifications to Usenet or an
equivalent medium, or placing the modifications on a major archive site such as
ftp.uu.net, or by allowing the Copyright Holder to include your modifications
in the Standard Version of the Package.

    b) use the modified Package only within your corporation or organization.

    c) rename any non-standard executables so the names do not conflict with
standard executables, which must also be provided, and provide a separate
manual page for each non-standard executable that clearly documents how it
differs from the Standard Version.

    d) make other distribution arrangements with the Copyright Holder.

4. You may distribute the programs of this Package in object code or executable
form, provided that you do at least ONE of the following:

    a) distribute a Standard Version of the executables and library files,
together with instructions (in the manual page or equivalent) on where to get
the Standard Version.

    b) accompany the distribution with the machine-readable source of the
Package with your modifications.

    c) accompany any non-standard executables with their corresponding Standard
Version executables, giving the non-standard executables non-standard names,
and clearly documenting the differences in manual pages (or equivalent),
together with instructions on where to get the Standard Version.

    d) make other distribution arrangements with the Copyright Holder.

5. You may charge a reasonable copying fee for any distribution of this
Package. You may charge any fee you choose for support of this Package. You may
not charge a fee for this Package itself. However, you may distribute this
Package in aggregate with other (possibly commercial) programs as part of a
larger (possibly commercial) software distribution provided that you do not
advertise this Package as a product of your own.

6. The scripts and library files supplied as input to or produced as output
from the programs of this Package do not automatically fall under the copyright
of this Package, but belong to whomever generated them, and may be sold
commercially, and may be aggregated with this Package.

7. C or perl subroutines supplied by you and linked into this Package shall not
be considered part of this Package.

8. The name of the Copyright Holder may not be used to endorse or promote
products derived from this software without specific prior written permission.

9. THIS PACKAGE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The End.
