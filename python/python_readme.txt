  Python V2.3.5

Full documentation and some useful links are available from the Python
for VMS site <http://vmspython.dyndns.org/>.


        Software Requirements

   1. CPQ SSL V1.0-B or V1.1(available as a free download
      <http://h71000.www7.hp.com/openvms/products/ssl/ssl_terms.html>
      from HP)
   2. JFP ZLIB 1.1-4 or ZLIB 1.2.1  /(See above)/ <#zlib>
   3. JFP LibBZ2 1.0-2  /(See above)/ <#libbz2>

It is *strongly* recommended that you install Python on an ODS-5 volume.

By default, the installation procedure installs Python in the
|SYS$COMMON:[PYTHON]| directory. To install it in another directory,
dev:[dir], use the |/DESTINATION| parameter of the |PRODUCT| command. In
this case, Python will be installed in the |dev:[dir.PYTHON]| directory.


        Installation

   1. Make the directory which holds the ZIP file your default directory
   2. Extract the PCSI kit from the ZIP archive.

$ UNZIP "-V" python235-v0150-0-1-AXP
           OR
  UNZIP "-V" python235-v0150-0-1-I64

   3. Install Python to your chosen destination.

$ PRODUCT INSTALL python  /(default)/

or ...

$ PRODUCT INSTALL python /DESTINATION=dev:[dir]

   4. Finally, run the DCL procedure |STARTUP.COM|.

(You might want to add this line to your |SYSTARTUP_VMS.COM| file.)

$ @dev:[dir]STARTUP

      *Shared Images*
      ------------------------------------------------------------------------
      The |STARTUP.COM| procedure runs two other DCL procedures:
      |LOGICALS.COM|, to set up the correct Python logicals; and
      |INSTALL_DYNAMICS_MODULES.COM| which uses the VMS INTALL command
      to load Python modules as shared images (requires CMKRNL
      privileges). By default, no modules are INSTALL'ed as shared images.

   5. /(Optional)/: Post-Installation Module Compilation

$ SET DEFAULT PYTHON_VMS
$ @SETUP
$ PYTHON COMPILE_PYTHON_MODULES.PY
$ PYTHON -"OO" COMPILE_PYTHON_MODULES.PY

   6. /(Optional)/: Post-Installation Module Testing

$ SET DEFAULT PYTHON_VMS
$ @SETUP
$ @ALLTESTS

*Module Testing Results*
------------------------------------------------------------------------

Currently, 1 test /should/ fail:
/test_time/

Remember, all PCSI kits listed below are also available from the North
American mirror site <http://erebus.homeip.net/anonymous/kits/> and the
Australian mirror site <http://ftp.vsm.com.au/jfp/kits/>
