Python_JFP, Languages, Python Language for OpenVMS

Python is an interpreted programming language created by Guido van Rossum 
in 1990. Python is fully dynamically typed and uses automatic memory 
management; it is thus similar to Perl, Ruby, Scheme, Smalltalk, and Tcl. 
Python is developed as an open source project, managed by the non-profit 
Python Software Foundation.

	--

Python V2.3.5
http://www.vsm.com.au/ftp/jfp/kits/
© Tous droits réservés 2003 Jean-François PIÉRONNE

Full documentation and some useful links are available from the Python for VMS site.
Software Requirements

   1. JFP ZLIB 1.2.3
   2. JFP LibBZ2 1.0-2

Python must be installed on an ODS-5 volume.

By default, the installation procedure installs Python in the SYS$COMMON:[PYTHON235]
directory. To install it in another directory, dev:[dir], use the /DESTINATION parameter 
of the PRODUCT command. In this case, Python will be installed in the dev:[dir.PYTHON235] 
directory.

Installation

   1. Make the directory which holds the ZIP file your default directory
   2. Extract the PCSI kit from the ZIP archive.

$ UNZIP "-V" PYTHON235-V0138-0-1

   3. Install Python to your chosen destination.

$ PRODUCT INSTALL python235  (default)

or ... 

$ PRODUCT INSTALL python235 /DESTINATION=dev:[dir]

   4. Finally, run the DCL procedure STARTUP.COM.

(You might want to add this line to your SYSTARTUP_VMS.COM file.)

$ @dev:[dir]STARTUP

      Shared Images
      The STARTUP.COM procedure runs two other DCL procedures: LOGICALS.COM, 
      to set up the correct Python logicals; and INSTALL_DYNAMICS_MODULES.COM 
      which uses the VMS INTALL command to load Python modules as shared images 
      (requires CMKRNL privileges). By default, no modules are INSTALL'ed as 
      shared images.
   5. (Optional): Post-Installation Module Testing

$ SET DEFAULT PYTHON_VMS
$ @SETUP
$ @ALLTESTS

Module Testing Results

Currently, 1 test should (partially) fail:
test_time 
