VOIT, Programming, VMS Objects and Images Tools for Alpha and I64: shiml and xpd.

These are the HP OpenVMS Engineering tools shiml and xpd.

SHIML, SHareable IMage List, lists all the shareable images an image depends on.
XPD, eXternal Procedures and Data, lists all the external procedures and data an
image references in shareable images.

Files

FREEWARE_README.TXT - This text file
[.ALPHA]SHIML.EXE
[.ALPHA]XPD.EXE
[.I64]SHIML.EXE
[.I64]XPD.EXE

SHIML usage

$ MC your_disk:[your_dir]SHIML [-u] full_image_file_spec
  -u: unique, no duplicates in shareable image names

Examples

$ mc sys$disk:[]shiml miniref.exe
recursive SHareable IMage dependency List (Alpha), version 1.0
 [ -> Translated Logical Name ] [ - Required Match: ID [ / Actual Match: ID ]]
MINISHR - MATLEQ: 1,2 / MATLEQ: 1,3
    SYS$PUBLIC_VECTORS
SYS$PUBLIC_VECTORS
$

$ mc sys$disk:[]shiml -u miniref.exe
SHareable IMage dependency List (Alpha), version 1.0
MINISHR
SYS$PUBLIC_VECTORS
$

$ define minishr sys$disk:[]minishr.exe;17
$ mc sys$disk:[]shiml miniref.exe
recursive SHareable IMage dependency List (Alpha), version 1.0
 [ -> Translated Logical Name ] [ - Required Match: ID [ / Actual Match: ID ]]
MINISHR -> SYS$DISK:[]MINISHR.EXE;17 - MATLEQ: 1,2
-e-slt, sublist terminated, SYS$DISK:[]MINISHR.EXE;17 not found or other error
SYS$PUBLIC_VECTORS
$

XPD usage

$ MC your_disk:[your_dir]XPD full_image_file_spec

Example

$ mc sys$disk:[]xpd miniref.exe
eXternal Procedure and Data list (I64), version 1.0
MINISHR -> SYS$DISK:[]MINISHR.EXE:
index 3 maps to EXPORTED_CONST, type is absolute data
index 2 maps to EXPORTED_DATA, type is data
index 0 maps to EXPORTED_PSECT, type is section
index 1 maps to EXPORTED_PROCEDURE, type is procedure
$

