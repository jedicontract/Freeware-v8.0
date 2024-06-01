PWAIT$SDA is a SDA extension (using the API first documented for VMS V7.2) 
which displays information about a process waiting and what it is waiting for.
PWAIT$SDA can be used to look at a crash dump or at a running system.

PWAIT$SDA is intended to help investigating why a process appears hung.

PWAIT$SDA has been built on OpenVMS Alpha (V7.2->V8.2) and OpenVMS Itanium.

INSTALLATION
------------
Copy the supplied PWAIT$SDA.EXE for your version of VMS to SYS$SHARE: or define
the logical name PWAIT$SDA to point to the file.

e.g for OpenVMS Alpha V7.3-2

$ DEFINE PWAIT$SDA dev:[dir.subdir]PWAIT$SDA.EXE_ALP_V732

TO USE
------
When analyzing a crash dump or the current running system using SDA enter the 
command PWAIT (at the SDA> prompt) and information will be displayed about the
currently selected process. 

The currently selected process is the process most recently displayed with
SHOW PROCESS or the current process in a crash or the process selected with
SET PROCESS.

To look at process with pid 321
PWAIT /ID=321

To look at the process named SERVER
PWAIT SERVER

The information displayed varies depending on the process state.
For example :-
(the following list is not intended to list everything that PWAIT displays).
LEF/CEF
- the number of the event flag(s)
- the local event flags and common event flags (if relevent)
- the active I/O channels and related I/O requests
- the timer requests for this process
JIBWAIT i.e. waiting a pooled quota such as BYTLM
- name of quota and initial and current values
RWAST
- name of quota (if BIOLM/DIOLM appears to be 0)
- pc at which process is waiting
- address of the caller (which can indicate the possible reason for the RWAST)
- active channels (in some cases)
RWMBX
- name of mailbox that process is waiting for space in (on Alpha only)
MUXTEX
- address of mutex inc related symbolic name if known

Examples

! looking at crash
$ ANALYZE/CRASH SYS$SYSTEM:SYSDUMP.DMP
SDA> CLUE CRASH		! get the basic info
SDA> PWAIT		! look at the current process

$ ANALYZE/SYSTEM
SDA> SHOW SUMMARY	! get list of processes to find pid 
SDA> PWAIT/IN=10C	! look at process with id 10c 

AUTHOR
------
This program was written by Ian Miller.

Bug reports and comments to

	miller@encompasserve.org


CHANGES
-------
  V0.1	Initial version.
  V0.2  more info
  V0.3  For RWMBX try and workout which mailbox
  V0.4	Look at Kernel Thread Blocks instead of PCB, various tidy up
  V0.5  Display active channels. Allow specification of process.
  V0.6	Display some IRPs
  V0.7	Display Timer Queue and how long process has been waiting
  V0.8	Fix build on VMS V7.2-1
  V0.9	Add changes suggested by Richard Whalen of Process Software for
        building pwait$sda on Itanium VMS. A couple of other small changes.
  V0.A	Fix bug in handling of processing with more than one kernel thread.
	Fix bug in display of some mwait states.
	If process is waiting for a know static mutex display its name.
	Display active channels and TQE list for HIB processes.
	Some cosmetic changes to displays.
  V0.B	Show the deaccess irp if there is one.
	Display some suggestions about other resource wait states.
  V0.C	Display the inner mode semphore for kthreads waiting for it.
  V0.D	Better analysis including looking at the return address.
	Fixed some more bugs. Analyze context wait (used by Fast I/O et al).
	When displaying TQE display only wakeup ones for HIB process and
	for LEF process match waiting EF.
  V0.E	Display I/O's for mailbox and disk. Various other improvements.
  V0.F	Display how long process has been waiting (thanks to Volker Halle).
  V0.G  Analyze FPG. Analyze locks. Various bug fixes. (thanks to Volker Halle).
  V0.H	If process waiting for sub-process to die then display names of sub-process.
	If process in mutex wait then display names of processes that own mutexes.
  V1.0	Lots of improvements to analysis, lock tracing etc.


  TO BUILD
  --------
  A C compiler is required. I usually use DEC C V7.1

  @B_PWAIT$SDA.COM


  COPYRIGHT NOTICE
  ----------------
  This software is COPYRIGHT © 2004, Ian Miller. ALL RIGHTS RESERVED.

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
