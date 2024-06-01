$! Fac.Com
$! Copyright 1998  Digital Equipment Corporation  All Rights Reserved
$! Stephen Hoffman, OpenVMS Engineering, Nashua NH
$! An example of working with an OpenVMS common; with shared memory.
$ write sys$output "OpenVMS Alpha C COMMON Example"
$ copy sys$input: faccommon.h
/*
// faccommon.h
// Copyright 1998  Digital Equipment Corporation  All Rights Reserved
// This is an OpenVMS Alpha C Common example; created by fac.com...
*/
#define ARRAYSTEP 10
#define ARRAYSIZE 100
#pragma extern_model save
#pragma extern_model common_block shr
struct FacCommonStruct
  {
  int ArraySize;
  int Array[ARRAYSIZE];
  };
extern struct FacCommonStruct FacCommon;
#pragma extern_model restore
$ if f$search("FacCommon.h.-1") .nes. "" then purge FacCommon.h
$ cc FacCommon.h
$ link/share=FacCommonWrtShr FacCommon,Sys$Input/Opt
Symbol_Vector=(FacCommon=PSECT)
PSECT_ATTR=FacCommon,shr
$ Install Replace sys$disk:[]FacCommonWrtShr /write/head/share/open
$ define/job/nolog FacCommonWrtShr sys$disk:[]FacCommonWrtShr.exe
$ if f$search("FacCommon.Obj.-1") .nes. "" then purge FacCommon.Obj
$ wait 00:00:02
$ if f$search("FacCommonWrtShr.Exe.-1") .nes. "" then purge FacCommonWrtShr.Exe
$
$ cc sys$input/obj=facmain.obj
/*
// facmain.c
// Copyright 1998  Digital Equipment Corporation  All Rights Reserved
// This is an OpenVMS Alpha C Common example; created by fac.com...
*/
#include <ssdef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sys$disk:[]faccommon.h"

main()
  {
  int i;

  printf("FacMain is running...\n");
  printf("FacMain is examining the contents of the common...\n");
  printf("  FacCommon.ArraySize = %d\n", FacCommon.ArraySize );
  FacCommon.ArraySize = ARRAYSIZE;
  for ( i = 0; i < ARRAYSIZE; i++ )
    if ( !(i % ARRAYSTEP) )
      printf("  FacCommon.Array[%d] = %d\n", i, FacCommon.Array[i] );
  printf("FacMain is updating the contents of the common...\n");
  for ( i = 0; i < ARRAYSIZE; i++ )
    FacCommon.Array[i] += 1;
  printf("FacMain is examining the contents of the common...\n");
  for ( i = 0; i < ARRAYSIZE; i++ )
    if ( !(i % ARRAYSTEP) )
      printf("  FacCommon.Array[%d] = %d\n", i, FacCommon.Array[i] );
  printf("Run again to update the common again...\n");
  printf("FacMain is done...\n");

  return SS$_NORMAL;
  }
$ link facmain,Sys$Input/Option
sys$disk:[]FacCommonWrtShr/share
$ if f$search("FacMain.Obj.-1") .nes. "" then purge FacMain.Obj
$ if f$search("FacMain.Exe.-1") .nes. "" then purge FacMain.Exe
$ write sys$output "Rebuild completed.  It's now time to RUN FACMAIN"
$ Exit
