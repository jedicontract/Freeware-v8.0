SQuirrel_SQL_Client, Utilities, JDBC-based graphical SQL client 

SQuirreL SQL Client is a graphical Java program that will allow
you to view the structure of a JDBC compliant database, browse the
data in tables, issue SQL commands, etc.

The SQuirreL SQL Client project is hosted at

     http://squirrel-sql.sourceforge.net/

SQuirrel SQL Client is free software, and is distributed under the
terms of the GNU Lesser General Public License.

For license details, see

    http://www.gnu.org/copyleft/lesser.html

This OpenVMS Freeware package contains the full, unmodified contents
of the standard SQuirreL SQL Client distribution, and in addition it
includes a DCL command procedure to facilitate running the package on
OpenVMS.  It also includes the jTDS JDBC driver for accessing Sybase 
and Microsoft databases.  These additions are also released under the
LGPL.  JDBC drivers for most other databases are available free to the 
end user, but they generally have licenses that are more restrictive
than the LGPL and thus cannot be redistributed with this package.

This package requires Java 1.4 or later and all of its extensive 
and aggressive tweaking of default system and process quotas.  Do
not ignore these requirements if you want to run SQuirreL SQL 
successfully.  The package must be installed on an ODS-5 disk, and 
it must be run from an account that has a SYS$LOGIN directory residing 
on an ODS-5 disk.

To install SQuirreL SQL  Client, issue the following command in the
same directory as the product kit:

    $ PRODUCT INSTALL SQUIRRELSQL

After installing SQuirreL SQL Client, you may run it from the command 
line as follows (replace the device and directory names if you have
chosen a non-default installation directory):

    $ @SYS$COMMON:[SQuirreL_SQL_Client]squirrel-sql.com

You may wish to add that commmand to your DECWindows desktop using:

   Application Manager 
      Desktop Apps 
          Create Action
  
If you do add the command to your desktop, you may also wish to reference
the acorn icon supplied with the package; to do so, you must first copy 
the following file from the installation directory:

    [SQuirreL_SQL_Client.icons]acorn.xpm

to the following location under your SYS$LOGIN directory:

    [.DT.ICONS]acorn.m_pm

To provide access to your database, acquire a JDBC driver from your
database supplier and drop its .jar file into the [.lib] directory
under the main installation directory.  Be sure to set read and 
execute permission on the file.
