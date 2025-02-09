/*
  Copyright 1999-2006 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.
  
  You may not use this file except in compliance with the License.
  obtain a copy of the License at
  
    http://www.imagemagick.org/script/license.php
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickWand Application Programming Interface declarations.
*/

#ifndef _MAGICK_WAND_H
#define _MAGICK_WAND_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if !defined(_MAGICKWAND_CONFIG_H)
# define _MAGICKWAND_CONFIG_H
# if !defined(vms) && !defined(macintosh)
#  include "wand/wand-config.h"
# else
#  include "wand-config.h"
# endif
# if defined(__cplusplus) || defined(c_plusplus)
#  undef inline
# endif
#endif

#include <stdio.h>
#include <sys/types.h>

#if defined(__CYGWIN32__) && !defined(__CYGWIN__)
# define __CYGWIN__ __CYGWIN32__
#endif
#if defined(__CYGWIN__)
# if defined(__WINDOWS__)
#   undef __WINDOWS__
# endif
#elif defined(_WIN32)
# define __WINDOWS__ _WIN32
#elif defined(WIN32)
# define __WINDOWS__ WIN32
#endif
#if defined(__CYGWIN__) && defined(__WINDOWS__)
# undef __WINDOWS__
#endif

#if defined(__WINDOWS__)
# if defined(_MT) && defined(_DLL) && !defined(_WANDDLL_) && !defined(_LIB)
#  define _WANDDLL_
# endif
# if defined(_WANDDLL_)
#  if defined(_VISUALC_)
#   pragma warning( disable: 4273 )  /* Disable the dll linkage warnings */
#  endif
#  if !defined(_WANDLIB_)
#   define WandExport  __declspec(dllimport)
#   if defined(_VISUALC_)
#    pragma message( "MagickWand lib DLL import interface" )
#   endif
#  else
#   define WandExport  __declspec(dllexport)
#   if defined(_VISUALC_)
#    pragma message( "MagickWand lib DLL export interface" )
#   endif
#  endif
# else
#  define WandExport
#  if defined(_VISUALC_)
#   pragma message( "MagickWand lib static interface" )
#  endif
# endif

# if defined(_DLL) && !defined(_LIB)
#  define ModuleExport  __declspec(dllexport)
#  if defined(_VISUALC_)
#   pragma message( "MagickWand module DLL export interface" )
#  endif
# else
#  define ModuleExport
#  if defined(_VISUALC_)
#   pragma message( "MagickWand module static interface" )
#  endif

# endif
# define WandGlobal __declspec(thread)
# if defined(_VISUALC_)
#  pragma warning(disable : 4018)
#  pragma warning(disable : 4244)
#  pragma warning(disable : 4142)
#  pragma warning(disable : 4800)
#  pragma warning(disable : 4786)
# endif
#else
# define WandExport
# define ModuleExport
# define WandGlobal
#endif

#define MaxTextExtent  4096
#define WandSignature  0xabacadabUL

#if !defined(wand_attribute)
#  if !defined(__GNUC__)
#    define wand_attribute(x)  /* nothing */
#  else
#    define wand_attribute  __attribute__
#  endif
#endif

typedef struct _MagickWand
  MagickWand;

#include "magick/MagickCore.h"
#include "wand/animate.h"
#include "wand/compare.h"
#include "wand/composite.h"
#include "wand/conjure.h"
#include "wand/convert.h"
#include "wand/display.h"
#include "wand/drawing-wand.h"
#include "wand/identify.h"
#include "wand/import.h"
#include "wand/magick-attribute.h"
#include "wand/magick-image.h"
#include "wand/mogrify.h"
#include "wand/montage.h"
#include "wand/pixel-iterator.h"
#include "wand/pixel-wand.h"

extern WandExport long
  MagickGetIteratorIndex(MagickWand *);

extern WandExport MagickBooleanType
  IsMagickWand(const MagickWand *),
  MagickClearException(MagickWand *),
  MagickSetIteratorIndex(MagickWand *,const long);

extern WandExport MagickWand
  *CloneMagickWand(const MagickWand *),
  *DestroyMagickWand(MagickWand *),
  *NewMagickWand(void);

extern WandExport void
  ClearMagickWand(MagickWand *),
  MagickWandGenesis(void),
  MagickWandTerminus(void),
  *MagickRelinquishMemory(void *),
  MagickResetIterator(MagickWand *),
  MagickSetFirstIterator(MagickWand *),
  MagickSetLastIterator(MagickWand *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
