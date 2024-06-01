MOSAIC, WEB_TOOLS, The Mosaic Web Browser

VMS Mosaic V4.0, 4-JUN-2006

VMS Mosaic is a GUI web browser.  It supports HTML V4.0 (including
tables and frames), animated GIFs, cookies, secure connections, etc.
It does not support Java, JavaScript or style sheets.  A C compiler
is required to build it, if none of the pre-built executables in
the [.EXE] directory are useable.

VMS Mosaic is supported on VAXes running OpenVMS 5.4-3 thru 7.3, on
Alpha systems using OpenVMS V1.5 thru 8.2, and on IA64 systems running
OpenVMS V8.1 thru 8.2-1.  Mosaic will work with UCX (TCP/IP Services),
CMU, MultiNet, Pathway, TCPware or SOCKETSHR with NETLIB.  CMU TCP/IP
is supported via LIBCMUII or SOCKETSHR.  The Mosaic has been compiled
with VAX C, DEC C and GNU C (VAX version 2.7.1 only).  Versions 1.1
thru 1.5 of DECwindows Motif are supported.  Both HP SSL and OpenSSL
are supported for secure connections.

The file MOSAIC4_0.ZIP contains the sources, documentation and build
procedure.  Note that the file is a ZIP archive, so the UNZIP program
is needed to unpack it.  Once it is unpacked, the file README.VMS-4_0
contains the installation instructions.

See http://vaxa.wvnet.edu/vmswww/vms_mosaic.html for updates and
bug fixes beyond this release.  In particular, see the file
ftp://vaxa.wvnet.edu/mosaic/readme.bugs.

Changes since the previous freeware release (V3.8-1):

     From 3.8-1 to 3.9
     -----------------

 o Window title is now updated as soon as <TITLE> is processed
 o Added verification of SSL server certificates
 o Added preference and menu option for SSL certificate verification
 o Added support for HTTP refresh headers
 o Added detection of UTF-8 encoded pages
 o Added support for UTF-8 encoded ASCII characters
 o Added partial support for UTF-8 encoded left/right arrows, dagger,
   em/en dashes, bullet, ellipsis, euro, trademark and quote marks
 o Added native color quantization with Floyd-Steinberg dithering
   for PNG and BMP RGB images
 o Added support for Version 4 and 5 Windows BMP images
 o Added support for transparent BMP images
 o Added support for inline frames in tables
 o Added preference settings for the hotlist menu's height and width
 o Changed default hotlist menu height to 502
 o Changed the Document Links scroll window height to 200
 o Added support for "about:blank" for use by Google Groups
 o Added partial support for &Dagger, &euro, &hellip, &mdash and &trade
 o Improved support for &bull, &larr and &rarr
 o Added workaround for bug in the ProFTPD ftp server
 o Added cookieTrace trace option
 o Changed alpha channel transparancy processing to use 24 bit
   color quantization with Floyd-Steinberg dithering
 o Changed to escape spaces and control characters in URLs
 o Increased the default ftp filename display length to 26
 o Fixed crash caused by background images with very large dimensions
 o Fixed printing of pages with inline frames
 o Fixed problem with username/password prompts on secure pages
 o Fixed problem with form button background color in tables
 o Rewrote and improved performance of ParseMarkType routine
 o Overhauled the ftp support code including many bug fixes
 o Reworked XPM image support using LibXpm version 3.4k code
 o Upgraded PNG library to version 1.0.18
 o Upgraded ZLIB library to version 1.2.3
 o Colorized the FTP toolbar icons
 o Colorized the Error and Question dialog pixmaps
 o Optimized preallocation of memory for markup and formatting elements
 o Added preference settings for markup and element preallocation
 o Restored support for Motif 1.1
 o Cleaned up history file processing
 o Cleaned up mailto code
 o Various bug, memory leak and socket leak fixes
 o Various source reformating and cleanups

     From VMS 3.9 to VMS 4.0
     -----------------------

 o Added support for <FIELDSET>, <LEGEND>, <LABEL>, <OPTGROUP>,
   <COL>, <COLGROUP>, <DEL> and <INS> tags
 o Added support for <BUTTON> tags (text content only)
 o Added support for LABEL in <OPTION> tags
 o Added support for FRAME and RULES in <TABLE> tags
 o Added detection of UTF-8 character set specified in <META> tags
 o Added support and preference setting for MenuBar tearoff menus
 o Added tooltip (aka balloon) help for the toolbar, cookie jar manager
   and various menus
 o Added tooltip support for TITLE in span, image, anchor and form tags
 o Added preference and menu option to enable/disable tooltip help
 o Added preference settings for tooltip foreground and background
   colors, font face, shape and popup/popdown delay times
 o Added Postscript printing of form text content and button labels
 o Added Postscript printing support for the Symbol font
 o Added Postscript printing support for SIZE and NOSHADE in <HR> tags
 o Added Postscript printing of square and block bullets
 o Added support and preference setting for duplex Postscript printing
 o Added support for HTML tags as text in form text areas
 o Added full support for &hellip, &mdash, &trade, &harr, &larr,
   &rarr, &uarr, &darr, &lozenge, &prime, &Prime, &fnof, &clubs,
   &diams, &hearts and &spades
 o Added full support for UTF-8 encoded left/right arrows, em dash,
   ellipsis and trademark
 o Added feedback messages for images which are too large
 o Added 32x32 and 75x75 pixmaps for the desktop icon
 o Added detection of missing MOSAIC.DAT
 o Added support for the XHTML <BR/> tag with missing space
 o Added the LIBLITECLUE library
 o Fixed slow drag scrolling of pages with lots of form widgets
 o Fixed DCL qualifiers which didn't work with preferences enabled
 o Fixed major GC (Graphics Context) leak and reduced GC allocations
 o Fixed hangs caused by broken https servers doing keepalive
   on redirects
 o Eliminated progress meter flickering
 o Changed to limit Mosaic's size preferences to the screen size
 o Automatically rescales large images to the maximum pixmap dimension
   preferences if they are set non-zero
 o Fixed failure of frame loading to abort
 o Fixed failure of animations in inline frames to abort
 o Fixed size calculation problems with inline frames in tables
 o Improved support for NOSHADE in <HR> tags
 o Improved search text positioning in source view window
 o Eliminated underlining and overstriking of leading spaces
 o Reduced space between captions and tables
 o Changed to display large option menus as scrolled lists
 o Changed to not rewrite unmodified hotlist at exit
 o Changed to allow form elements outside of forms
 o Changed NOTRACE builds to use separate work directories
 o Changed to ignore non-standard <NOINDEX> tags
 o Colorized the Information and Warning dialog pixmaps
 o Made minor changes to the Menubar to make it more consistant with
   the Motif 1.2 Style Guide
 o Replaced Topica with Yahoo Groups on the navigation menu
 o Disabled mouse button 2 "drag and drop" on icons and form buttons
 o Registers the "default" language procedure with XtSetLanguageProc
 o Progress meter now works with Motif 1.1
 o Eliminated use of Motif gadgets in the MenuBar
 o Reduced by ten the number of types sent in http Accept: headers
 o Eliminated all use of the caddr_t C type
 o Various bug and memory leak fixes
 o Various source reformating and cleanups


George Cook, West Virginia Network (WVNET)
Email address: cook@wvnet.edu
