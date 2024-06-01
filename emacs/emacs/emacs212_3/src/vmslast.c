/*
 * File VMSLAST.C
 * OpenVMS and I64, only
 * Part of the mapping implementation
 *
 * In the linker options file,
 * this module must be the last object module for the EMACS cluster.
 *
 * This module only contains markers for the begin and end of the
 * compiler generated .sbbs and .sdata short data sections.
 *
 * For -map ALL data must be dumped or loaded. On I64 ALL data is distributed
 * between "normal" sections like $DATA$ and short data sections, like .sdata.
 * Therefore the short data must be dumped and loaded. However, short data
 * can not be collected or can not be split into several image segments.
 *
 * With the current version, short data looks like this:
 *
 * contributions: !_ NEWDISP _!_ ... _!_ VM-LIMIT _!_ NEWDISP _!_ ... _!_VM-LIMIT _!_ <more contris> _!
 * sections:      [ .sbss                         ][ .sdata                                           ]
 * addresses:     A                                B                                C
 *
 * What currently needs to be mapped is A (incl.) to C (excl.), where A is aligned at a page, but C is not.
 * Mapping the whole thing would work, too. But there is no way to determine where it ends.
 *
 * The easiest approach is to have marker at the start of  short data and around C and map from the start up to C.
 * It goes like this: This module VMSLAST supplies a $sbss and a .sdata section. VMSLAST is the last module on the EMACS
 * cluster this gives an different layout:
 *
 * contributions: !_ VMSLAST _!_ NEWDISP _!_ ... _!_ VM-LIMIT _!_ NEWDISP _!_ ... _!_VM-LIMIT _!_ VMSLAST _!_ <more contris> _!
 * sections:      [ $sbss    ][ .sbss                         ][ .sdata                                                       ]
 * addresses:     A'          A                                B                                C    C'
 *
 * The linker sorts the sections: $sbss comes before .sbss. This gives us a start marker with address value A'.
 * The linker adds contributions as specified in the cluster=EMACS option. This gives an end marker with address value C.
 * VMSLAST's $sbss is just a pointer pointing to it's own address.
 * VMSLAST's .sdata is a pointer pointing to it's own address plus enough data (8KB) to make sure that the next contribution
 * is on the next page. The next page starts at C'.
 * Then A' to C' can be determined at run time and can be mapped.
 *
 * Please note, due to code changes, there might be more .sbss data after VM-LIMIT, which is not necessary to save.
 * In case someone wants to exactly save and map the necessary data, a similar approach to determine the end of necessary
 * .sbss data and the start of necessary .sdata can be used.
 */

/*
 * The compiler (V7.2) doesn't support the PSECT attribut SHORT, if/when it does, SHORT can be added, here and removed from
 * the linker options file.
 */
#pragma extern_model strict_refdef "$sbss"
void *sbss_start = &sbss_start;

#pragma extern_model strict_refdef ".sdata"
void *sdata_end = &sdata_end;
char sdata_filler [8192-8] = {0};
