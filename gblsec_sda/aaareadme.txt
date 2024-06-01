GBLSEC$SDA,UTILITIES, a SDA extension to display info about a global section

GBLSEC$SDA is a SDA extension (using the API first documented for VMS V7.2) 
which displays information about a global section including the name of the
file associated with it and the processes mapped to it.

GBLSEC$SDA can be used to look at a crash dump or at a running system.

INSTALLATION
------------
Either copy GBLSEC$SDA.EXE to SYS$SHARE or define a logical name GBLSEC$SDA
as the current location.

$ DEFINE GBLSEC$SDA DISK:[DIR.SUBDIR]GBLSEC$SDA.EXE

TO USE
------
When analyzing a crash dump or the current running system using SDA enter the 
command GBLSEC (at the SDA> prompt) followed by the global section name and 
information will be displayed about the global section.

Global section names containing lower case letters have to be specified in 
quotes e.g "DnsClerkCache".

To look at a system global section

	GBLSEC section-name

To look at a group global section

	GBLSEC section-name /GROUP=n

where n is the group number in octal. If the group number is not 
specified the first group global section with the specified
name is displayed.

To look at delete pending global sections

	GBLSEC section-name /DELETE_PENDING

If GBLSEC there is a file associated with the global section
e.g the global section is the result of installing a file then
GBLSEC obtains the device name and file id from the internal 
data structures and translates this to a filename using
LIB$FID_TO_NAME. There are two issues with this

1. you could be looking at a crash dump from another system
2. you could be looking at the live system and a file system
   lock could block the lookup of the file name.

Therefore there is a /FID_ONLY qualifier which, if specified
on the command line, prevents GBLSEC from looking up the 
filename. In this case the device name and FID is displayed.

Specify /NOGSD to supress display of the global section descriptor.
Specify /NOGSTE to supress display of the Global Section Table Entry.
Specify /NOUSERS to not look for processes using this global
section.

GBLSEC searches for all global sections with the specified name 
(there may be more than one. They may have different idents)
To stop after finding the first one specify /NOALL.

GBLSEC creates some symbols in the SDA symbol table to aid 
futher investigations which contain the addresses of various 
data structures. These symbols are GSD, GSTE, FCB, WCB, UCB, GPTE 
and NGPTE (number of NGPTE);

To find the processes using a particular installed file then
use the INSTALL LIST filespec/GLOBAL command to find the names
of the global sections created by installing the file. Then
use GBLSEC/NOGSD/NOGPTE/USE for each global section.

Example

! looking at crash
$ ANALYZE/CRASH SYS$SYSTEM:SYSDUMP.DMP
SDA> GBLSEC  SMG$TERMTABLE

GBLSEC V1.0 (c) 2005, Ian Miller (miller@encompasserve.org) built on VMS V7.3-2

GSD: Global Section Descriptor
GSName: SMG$TERMTABLE
PCBUIC: [1,4]
FILUIC: [1,4]
ORB:    81402788
GSTX:   00000127
PROT:   0000FFFF
IDENT:  00000000
IPID/RGSTX: 00000000
FLAGS:  0082C3C1
        WRTMOD = USER, AMOD = USER, GBL PERM SYS

File:   DISK$ALPHASYS:[VMS$COMMON.SYSEXE]TERMTABLE.EXE;1

GSTE: Global Section Table Entry
GSD:    81F35270
PFC:           0.
WINDOW: 8144D640
VBN:           1
REFCNT:       11.
UNITCNT:     170.
VPX:        3376.
FLAGS:  0082C3C1
        WRTMOD = USER, AMOD = USER, GBL PERM SYS

searching for processes mapped to this global section
PID 000003B9 _FTA9:
1 processes found to be mapped to this global section

AUTHOR
------
This program was written by Ian Miller.

Bug reports and comments to

	miller@encompasserve.org

CHANGES
-------
  V0.1	Initial version looked at system global sections only
  V0.2	Add support for group and delete pending sections and
	improve displays. Implement FID lookup.
  V0.3	Find all global sections matching the name. Add more qualifers.
  V0.4	Add creation of useful symbols.
  V0.5  Various small improvments. Fix build on I64 and VMS V7.2. Message file.
  V1.0  First public release
  V1.1	Search dynamic regions. Various bug fixes.
  V1.2	Bug fixes to allow repeated use in same SDA session.
 	Handle gaps in P1 region.

TO BUILD
--------
A C compiler is required. I usually use DEC C V7.1 on OpenVMS Alpha V7.3-2
GBLSEC does build on OpenVMS Alpha V7.2 to V8.2 and OpenVMS I64 V8.2 to V8.2-1

  @B_GBLSEC$SDA.COM

COPYRIGHT NOTICE
----------------
This software is COPYRIGHT © 2005, Ian Miller. ALL RIGHTS RESERVED.

This software is released under the licence defined below.

LICENSE
-------
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
