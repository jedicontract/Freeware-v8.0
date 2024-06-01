#define MYNAMEIS "ALPHA_IMPLVER_AMASK_FEATURES.C V1.3 (EV4 to EV7)\n"
/*
//
//  Copyright © 2002 Hewlett-Packard Company
//
//
//
//  This tool retrieves the Alpha microprocessor family and features
//
//  Author: Stephen Hoffman, OpenVMS Engineering, Hewlett-Packard Company
//
//  V1.0: 16-Jan-1997
//  V1.1: 16-Jan-1997
//  V1.2: 15-Feb-2000
//  V1.3: 29-May-2002
//
//  Operates and tested under the hp C compiler V5 and later under 
//  hp OpenVMS Alpha.  This example should also operate under the
//  hp C compiler on hp Tru64 UNIX.
//
//  See the Alpha Architecture Reference Manual for the current
//  microprocessor family codes, as well as the instruction set
//  extension definition codes.
//
//  As of this writing, the author is not aware of official acronyms
//  for amask bits 9 and 12.  Acronyms for these have been proposed.
//
//
//  Warning: Past product naming practices and assigned constant 
//  values do not necessarily predict future practices nor values.
//
//
*/
#include <c_asm.h>
#include <stdio.h>
#include <stdlib.h>
#define AMASK_BIX    0
#define AMASK_FIX    1
#define AMASK_CIX    2
#define AMASK_MVI    8
#define AMASK_9      9
#define AMASK_12     12
#define IMPLVER_EV4  0
#define IMPLVER_EV5  1
#define IMPLVER_EV6  2
#define IF_AMASK_BIT( x ) definedbits |= 1L << x; if ( ~amask & 1L << x )
main()
    {
    int implver = asm("implver %r0;");
    int amask = asm("amask %a0,%r0;", -1 );
    int definedbits = 0;

    printf(MYNAMEIS);
    printf("\n");
    printf("Alpha Microprocessor Family:\n");
    /*
    //  The "microprocessor family" includes the named microprocessor,
    //  and various microprocessors (if any) derived from it.
    */
    printf("  Alpha 21%1.1d64 or variant.\n", implver );
    printf("  EV%1.1d microprocessor core.\n", implver + 4 );
    if (( ~amask & 1L << AMASK_BIX ) && (implver == IMPLVER_EV5 ))
      printf("    21164A/EV56 or variant\n");
    if (( ~amask & 1L << AMASK_CIX ) && (implver == IMPLVER_EV6 ))
      printf("    21264A/EV67 or variant\n");

    /*
    //  The amask bit is *clear* if the capability is *present*...
    */
    printf("\n");
    printf("Alpha extensions available:\n");
    if ( !~amask ) printf("  none.\n");

    IF_AMASK_BIT( AMASK_BIX ) 
	printf("  byte-word, et al.\n");
    IF_AMASK_BIT( AMASK_FIX ) 
	printf("  square root, et al.\n");
    IF_AMASK_BIT( AMASK_CIX ) 
	printf("  count extensions, et al.\n");
    IF_AMASK_BIT( AMASK_MVI ) 
	printf("  audio-video, et al.\n");
    IF_AMASK_BIT( AMASK_9   )
	printf("  precise floating-point exceptions.\n");
    IF_AMASK_BIT( AMASK_12  ) 
	printf("  support for prefetch with modify.\n");

    if ( ~amask & ~definedbits )
	{
	printf("\n");
	printf("  Other AMASK capability bit(s) are present.\n");
	printf("    Undecoded bits in the AMASK bitmask: 0x0%08.x\n", ~amask );
	}

    printf("\n");
    printf("Please see the Alpha Architecture documentation and the\n");
    printf("Alpha Microprocessor documentation for details of AMASK\n");
    printf("and IMPLVER, and the meanings of the AMASK bitmask bits.\n");
    printf("(Pointers to this documentation are in the OpenVMS FAQ.)\n");
    printf("\n");

    return EXIT_SUCCESS;
    }
