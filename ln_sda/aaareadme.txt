LN$SDA,UTILITIES, a SDA extension to display logical names

LN$SDA is a SDA extension (using the API first documented for VMS V7.2) 
which displays logical names. Logical names can be specified by name or
by value and optionally using wildcards (normal VMS wildcards * and %).
The logical name tables of the currently selected process in SDA and/or
shared tables are searched.

LN$SDA can be used to look at a crash dump or at a running system.

INSTALLATION
------------
Either copy LN$SDA.EXE to SYS$SHARE or define a logical name LN$SDA
as the current location.

$ DEFINE LN$SDA DISK:[DIR.SUBDIR]LN$SDA.EXE

TO USE
------
When analyzing a crash dump or the current running system using SDA enter the 
command LN (at the SDA> prompt) as follows


	LN	name	[table] 

 Specify table to search a specific logical name table or use the qualifiers 
 /SYSTEM to search LNM$SYSTEM only
 /GROUP to search LNM$GROUP only
 /PROCESS to search LNM$PROCESS only
 /ALL to search this process and all shared tables

 Only one of /SYSTEM,  /GROUP, /JOB, /PROCESS and /ALL can be specified.

 If no table is specified default is to search the tables specified by the 
 LNM$DCL_LOGICAL search list.

 Logical names containing lower case letters have to be specified in 
 quotes e.g "DnsClerkCache".

 Use
 /CASE_BLIND for case blind matching of name
 /WILDCARD if name contains wildcards

 The qualifier /VALUE specifies that the name is the value of the logical name
 to search for.
 /WILDCARD and /CASE_BLIND can be used with /VALUE.


 The qualifiers /USER, /SUPERVISOR, /EXECUTIVE, and /KERNEL restrict the
 search to only logical names that match the mode specifed. 
 The default is to match any mode.

 The /DUMP_CACHE to request that the process logical name table
 translation cache is displayed.

 The output is formatted assuming a 132 character line length.

 Due to the simplified table name resolution algorithm implemented if the
 specified table name resolves to more than 30 actual tables then only the
 the first 30 are searched. This should not be a problem usually.
 The usual value of LNM$DCL_LOGICAL resolves to six tables.

AUTHOR
------
This program was written by Ian Miller.

Bug reports and comments to

	miller@encompasserve.org

CHANGES
-------
  V1.0	First public release.

TO BUILD
--------
A C compiler is required. I usually use DEC C V7.1 on OpenVMS Alpha V7.3-2
LN does build on OpenVMS Alpha V7.2 to V8.2 and OpenVMS I64 V8.2 to V8.2-1

  @B_LN$SDA.COM

COPYRIGHT NOTICE
----------------
This software is COPYRIGHT � 2006, Ian Miller. ALL RIGHTS RESERVED.

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
