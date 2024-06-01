[inherit ('SYS$LIBRARY:STARLET')] module deliver_dispatcher;


(*

;
;++
; Written by Kevin Carosso @ Hughes Aircraft Co., SCG/CTC, January 11, 1985
; Modified by Ned Freed, 16-Nov-1986, to use proper global symbols.
; Modified by Kevin Carosso, 10-MAR-1988, to allow easy DEBUG invocation.
; Modified by Ned Freed, 23-Mar-1989, for CC line and attribute support.
; Modified by Dick Munroe (munroe@dmc.com), 07-Sep-1992, to allow easy access
;       to trace information.
; Converted to Pascal by Wayne Sewell (wayne@tachyon.com), 11-Nov-1994, to 
;	allow easier porting to alpha.
;
;       The problem is that even though we can now debug shareable images,
;       this shareable image is dynamically activated by MAIL.  There is
;       no way for the debugger to get control.  So, we do the following:
;       If the logical name MAILSHR_DEBUG is defined we will signal an
;       SS$_DEBUG the first time into this module.  This will transfer control
;       to DEBUG after this image is mapped in.  Unfortunately, this will
;       not work unless MAIL.EXE or MAIL_SERVER.EXE have the image transfer
;       address array modified to include the traceback entry point and
;       the image flags modified to enable the IHD$V_LNKDEBUG bit (it
;       is bit 0).
;
;       Use PATCH/ABSOLUTE on a private copy of the appropriate image
;       (in VMS V4 use MAIL.EXE, in VMS V5 and later use MAIL.EXE for outbound
;       messages, MAIL_SERVER.EXE for inbound).  
;
;       In OpenVMS VAX, the transfer address array is three longwords 
;       at offset 30 (all numbers in hex).  The image flags are a longword 
;       at offset 20.  Initially, they will look something like:
;
;               +--------------+
;       20:     |   01000028   |        ! Image flags
;               +--------------+
;
;               +--------------+
;       30:     |   00000F18   |        ! Transfer address array
;               +--------------+
;       34:     |   00000000   |
;               +--------------+
;       38:     |   00000000   |
;               +--------------+
;
;
;
;
;       To enable DEBUG, set bit 0 in the flags to be 1.
;       To allow traceback, and hence the SS$_DEBUG signal to work,
;       the first transfer address must be changed to 7FFEDF68 (for
;       VMS V4 or V5 or V6) while the existing address must be moved down to
;       the second longword.  So, our example would become:
;
;               +--------------+
;       20:     |   01000029   |
;               +--------------+
;
;               +--------------+
;       30:     |   7FFEDF68   |
;               +--------------+
;       34:     |   00000F18   |
;               +--------------+
;       38:     |   00000000   |
;               +--------------+
;
;
;
;
;       PATCH does not exist on AXP (as of version 6.1).  However, If you 
;	have a VAX system available, you can copy the image there, patch it,
;       and copy it back to the Alpha.  PATCH won't work in image mode in this
;       case, since it can't recognize an Alpha image (or the machine code,
;       for that matter), but the absolute mode works fine.  With absolute,
;       the image file is just a block of binary data.  If the Alpha is 
;       clustered with VAXen, or the Alpha disk is accessible in some other
;       way (other than DECNET), you can patch the image in place.
;
;
;      In an AXP image, the transfer address array consists of three quadword
;      addresses:
;
;
;               +--------------+
;       50:     |   01000028   |        ! Image flags
;               +--------------+
;
;               +--------------+
;       70:     |   00000F18   |        ! Transfer address array
;               +--------------+
;       74:     |   00000000   |
;               +--------------+
;       78:     |   00000000   |
;               +--------------+
;       7C:     |   00000000   |
;               +--------------+
;       80:     |   00000000   |
;               +--------------+
;       84:     |   00000000   |
;               +--------------+
;
;
;
;       As with the VAX, set bit 0 in the flags to be 1.
;       The transfer address to allow traceback is different for the Alpha.
;       The first transfer address must be changed to FFFFFFFF 00000340 (for
;       AXP VMS V6.1, don't know about 1.5 and earlier) while the existing 
;	address must be moved down to the second quadword.  So, our example 
;	would become:
;
;               +--------------+
;       50:     |   01000029   |
;               +--------------+
;
;               +--------------+
;       70:     |   00000340   |        ! Transfer address array
;               +--------------+
;       74:     |   FFFFFFFF   |
;               +--------------+
;       78:     |   00000F18   |
;               +--------------+
;       7c:     |   00000000   |
;               +--------------+
;       80:     |   00000000   |
;               +--------------+
;       84:     |   00000000   |
;               +--------------+
;
;
;
;       To ensure your private copy is invoked by the MAIL command,
;       define MAIL or MAIL_SERVER to point at your patched copy.
;       Note that MAIL is normally installed with privileges, so you
;       will have to enable those privileges (at least) so that your
;       private copy functions properly.  When MAIL or MAIL_SERVER
;       starts execution, note that DEBUG will start first.  Since we
;       don't care about anything in these images yet, simply tell DEBUG
;       to "GO".
;
;       Defining MAILSHR_DEBUG (the equivalence value does not matter)
;       will cause DEBUG to regain control the first time into the
;       MAILSHR image.  If you compiled and linked with /DEBUG, you
;       should have access to all symbols and full source-code debugging.
;
;
; Modification History
;
;   0.001 Dick Munroe 05-Jun-92
;	Add linkage to image initialization to enable/disable debugging at run
;	time.
;   0.002 Wayne Sewell 11-Nov-94
;	Convert from macro to Pascal for port to AXP.
; 
;
; Written by Kevin Carosso @ Hughes Aircraft Co., SCG/CTC, January 11, 1985
;
;---------------------------------------------------------------------------
; This is invoked by MAIL when it encounters the foreign mail protocol.
; This module really has nothing protocol-specific to it and can be used
; to dispatch to any handler.  The handler should supply the following
; action routines:
;
;       status := MAIL_OUT_CONNECT  (context : unsigned;
;                                    LNK_C_OUT_CONNECT : immediate;
;                                    protocol, node : string_descriptor;
;                                    MAIL$_LOGLINK : immediate;
;                                    file_RAT, file_RFM : immediate;
;                                    MAIL$GL_SYSFLAGS : immediate;
;                                    attached_file : descriptor := immediate 0)
;
;       status := MAIL_OUT_LINE     (context : unsigned;
;                                    [LNK_C_OUT_SENDER | LNK_C_OUT_TO |
;                                     LNK_C_OUT_SUBJ |
;                                     LNK_C_OUT_CC] : immediate;
;                                    node, sender_name : string_descriptor)
;
;       status := MAIL_OUT_CHECK    (context : unsigned;
;                                    [LNK_C_OUT_CKUSER |
;                                     LNK_C_OUT_CKSEND] : immediate;
;                                    node, addressee : string_descriptor;
;                                    procedure MAIL$READ_ERROR_TEXT);
;
;       status := MAIL_OUT_FILE     (context : unsigned;
;                                    LNK_C_OUT_FILE : immediate;
;                                    node : string_descriptor;
;                                    rab : $RAB_TYPE;
;                                    procedure UTIL$REPORT_IO_ERROR);
;
;       status := MAIL_OUT_DEACCESS (context : unsigned;
;                                    LNK_C_OUT_DEACCESS : immediate);
;
;       status := MAIL_OUT_ATTRIBS  (context : unsigned;
;                                    LNK_C_OUT_ATTRIBS : immediate;
;                                    system_flags : immediate;
;                                    idtld : $QUAD);
;
;       status := MAIL_IN_CONNECT   (context : unsigned;
;                                    LNK_C_IN_CONNECT : immediate;
;                                    input_tran : string_descriptor;
;                                    file_RAT, file_RFM : immediate;
;                                    MAIL$GL_SFLAGS : immediate;
;                                    MAIL$Q_PROTOCOL : string_descriptor;
;                                    pflags : immediate);
;
;       status := MAIL_IN_LINE      (context : unsigned;
;                                    [LNK_C_IN_SENDER | LNK_C_IN_CKUSER |
;                                     LNK_C_IN_TO | LNK_C_IN_SUBJ |
;                                     LNK_C_IN_CC] : immediate;
;                                    returned_line : string_descriptor);
;
;       status := MAIL_IN_FILE      (context : unsigned;
;                                    LNK_C_OUT_FILE : immediate;
;                                    0 : immediate;
;                                    rab : $RAB_TYPE;
;                                    procedure UTIL$REPORT_IO_ERROR);
;                                   
;       status := MAIL_IN_ATTRIBS   (context : unsigned;
;                                    LNK_C_IN_ATTRIBS : immediate;
;                                    idtld : $QUAD);
;
;       status := MAIL_IO_READ      (context : unsigned;
;                                    LNK_C_IO_READ : immediate;
;                                    returned_text_line : string_descriptor);
;
;       status := MAIL_IO_WRITE     (context : unsigned;
;                                    LNK_C_IO_WRITE : immediate;
;                                    text_line : string_descriptor);
;
;---------------------------------------------------------------------------


Here we will define the actual routines as external.  Note that everything
is defined as 'var xx : integer'.  This way we don't have to bother defining
things like 'string_descriptor', etc.  The types don't really matter, because
we will be typecasting everything with '%immed iaddress(x)' anyway.  We
basically want to copy all parameters on the stack---immediate values and 
addresses alike---to the routine just as they are.  Doing it this way will work
on both VAX and AXP, while the macro 'callg (ap)' approach will not work on the
Alpha, at least not with some dorking around.  I preferred to just get rid of
the macro code entirely.

*)

function MAIL_OUT_CONNECT (var context : integer;
  var link_flag : integer;
  var protocol, node : integer;
  var log_link_error : integer;
  var file_RAT, file_RFM : integer;
  var MAIL$GL_FLAGS : integer;
  var attached_file : integer) : integer;extern;

function MAIL_OUT_LINE (var context : integer;
  var link_flag : integer;
  var node, line : integer) : integer;extern;

function MAIL_OUT_CHECK (
  var context : integer;
  var link_flag : integer;
  var protocol, addressee : integer;
  var MAIL$READ_ERROR_TEXT : integer) : integer;extern;


function MAIL_OUT_FILE (var context : integer;
  var link_flag : integer;
  var protocol : integer;
  var message_RAB : integer;
  var UTIL$REPORT_ERROR : integer) : integer;extern;

function MAIL_OUT_DEACCESS (var context : integer;
  var link_flag : integer) : integer;extern;

function MAIL_OUT_ATTRIBS (var context : integer;
  var link_flag : integer; var system_flags : integer;
  var idtld : integer) : integer;extern;

function MAIL_IN_CONNECT (var context : integer;
  var link_flag : integer;
  var input_tran : integer;
  var file_RAT, file_RFM : integer;
  var MAIL$GL_SYSFLAGS : integer;
  var MAIL$Q_PROTOCOL : integer;
  var pflags : integer) : integer;extern;

function MAIL_IN_LINE (var context : integer;
  var link_flag : integer;
  var line : integer) : integer;extern;


function MAIL_IN_FILE (var context : integer;
  var link_flag : integer;
  var scratch : integer;
  var RAB : integer;
  var UTIL$REPORT_IO_ERROR : integer) : integer;extern;

function MAIL_IN_ATTRIBS (var context : integer;
  var link_flag : integer; var idtld : integer) : integer;extern;

function MAIL_IO_WRITE (var context : integer;
  var link_flag : integer;
  line : integer) : integer;extern;

function MAIL_IO_READ (var context : integer;
  var link_flag : integer;
  var returned_line : integer) : integer;extern;




(*



Define major and minor protocol identifiers.  MAIL requires that these
be 1.  The shareable image MUST be linked with the options file MAILSHR.OPT
that promotes these symbols to UNIVERSAL symbols so they will end up
in the shareable image's symbol table.  There is no UNIVERSAL linker option
in Alpha VMS, so SYMBOL_VECTOR is used instead.

*)

var

mail$c_prot_major  : [global,value] integer := 2;
mail$c_prot_minor  : [global,value] integer := 1;


(*

 Constants for dispatcher, taken from MAIL.SDL listing

*)

const

lnk_c_out_connect  = 0;
lnk_c_out_sender   = 1;
lnk_c_out_ckuser   = 2;
lnk_c_out_to       = 3;
lnk_c_out_subj     = 4;
lnk_c_out_file     = 5;
lnk_c_out_cksend   = 6;
lnk_c_out_deaccess = 7;

lnk_c_in_connect   = 8;
lnk_c_in_sender    = 9;
lnk_c_in_ckuser    = 10;
lnk_c_in_to        = 11;
lnk_c_in_subj      = 12;
lnk_c_in_file      = 13;

lnk_c_io_read      = 14;
lnk_c_io_write     = 15;

lnk_c_in_cc        = 16;
lnk_c_out_cc       = 17;

lnk_c_in_attribs   = 18;
lnk_c_out_attribs  = 19;


(*
;
;
; Here's the main routine that is called by MAIL.  Note that we don't really
; do any work here, just dispatch the call to the appropriate handler.  The
; reason I do it this way is that I am not interested in writing the handlers
; in MACRO, and I cannot easily deal with different numbers of arguments in
; the same procedure in other languages.
;

;
;
; Own storage for DEBUG context.  If MAILSHR_DEBUG translates to
; anything, then we signal SS$_DEBUG the first time in here.  This
; is because we are a dynamically activated image and there is no
; way to set a break-point here.  Once we've signalled the first time,
; we can ignore it since DEBUG will know all about us.
;


*)

var 

debug_checked :  integer := 0;   (* zero means we haven't checked *)


deliver__unkfunc : [external,value] integer;



(* Routine to signal errors *)

procedure LIB$SIGNAL (%IMMED stat : [list, unsafe] integer); extern;



[global] function mail$protocol ( var p1 : integer;
			 var p2 : integer;
			 var p3 : integer;
			 var p4 : integer;
			 var p5 : integer;
			 var p6 : integer;
			 var p7 : integer;
			 var p8 : integer;
			 var p9 : integer) : integer;


var the_function : integer;
     sts : integer;

begin                                   

if debug_checked = 0 then begin 
   debug_checked := 1;
   sts := $trnlnm (tabnam := 'LNM$FILE_DEV',lognam := 'MAILSHR_DEBUG') ;
   if sts = ss$_normal then lib$signal (ss$_debug);
   end;

the_function := iaddress(p2);

case the_function of 

   lnk_c_out_connect:
      sts := mail_out_connect(%immed iaddress(p1),%immed iaddress(p2),
			%immed iaddress(p3),%immed iaddress(p4),
			%immed iaddress(p5),%immed iaddress(p6),
			%immed iaddress(p7),%immed iaddress(p8),
			%immed iaddress(p9));

   lnk_c_out_to, lnk_c_out_cc, lnk_c_out_subj, lnk_c_out_sender:
      sts := mail_out_line(%immed iaddress(p1),%immed iaddress(p2),
			%immed iaddress(p3),%immed iaddress(p4));

   lnk_c_out_cksend, lnk_c_out_ckuser:
      sts := mail_out_check(%immed iaddress(p1),%immed iaddress(p2),
			%immed iaddress(p3),%immed iaddress(p4),
			%immed iaddress(p5));

   lnk_c_out_file:
      sts := mail_out_file(%immed iaddress(p1),%immed iaddress(p2),
			%immed iaddress(p3),%immed iaddress(p4),
			%immed iaddress(p5));

   lnk_c_out_deaccess:
      sts := mail_out_deaccess(%immed iaddress(p1),%immed iaddress(p2));

   lnk_c_in_connect:
      sts := mail_in_connect(%immed iaddress(p1),%immed iaddress(p2),
			%immed iaddress(p3),%immed iaddress(p4),
			%immed iaddress(p5),%immed iaddress(p6),
			%immed iaddress(p7),%immed iaddress(p8));

   lnk_c_in_sender, lnk_c_in_ckuser, lnk_c_in_to, lnk_c_in_subj, lnk_c_in_cc:
      sts := mail_in_line(%immed iaddress(p1),%immed iaddress(p2),
			%immed iaddress(p3));

   lnk_c_in_file:
      sts := mail_in_file(%immed iaddress(p1),%immed iaddress(p2),
			%immed iaddress(p3),%immed iaddress(p4),
			%immed iaddress(p5));


   lnk_c_io_read:
      sts := mail_io_read(%immed iaddress(p1),%immed iaddress(p2),
			%immed iaddress(p3));

   lnk_c_io_write:
      sts := mail_io_write(%immed iaddress(p1),%immed iaddress(p2),
			%immed iaddress(p3));

   lnk_c_in_attribs:
      sts := mail_in_attribs(%immed iaddress(p1),%immed iaddress(p2),
			%immed iaddress(p3));

   lnk_c_out_attribs:
      sts := mail_out_attribs(%immed iaddress(p1),%immed iaddress(p2),
			%immed iaddress(p3),%immed iaddress(p4));

   otherwise 
      sts := deliver__unkfunc;

   end;



mail$protocol := sts;

end;

end.
