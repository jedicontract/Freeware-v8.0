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

  MagickCore Application Programming Interface declarations.
*/

#ifndef _MAGICKCORE_CORE_H
#define _MAGICKCORE_CORE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if !defined(_MAGICKCORE_CONFIG_H)
# define _MAGICKCORE_CONFIG_H
# if !defined(vms) && !defined(macintosh)
#  include "magick/magick-config.h"
# else
#  include "magick-config.h"
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
# if defined(_MT) && defined(_DLL) && !defined(_MAGICKDLL_) && !defined(_LIB)
#  define _MAGICKDLL_
# endif
# if defined(_MAGICKDLL_)
#  if defined(_VISUALC_)
#   pragma warning( disable: 4273 )  /* Disable the dll linkage warnings */
#  endif
#  if !defined(_MAGICKLIB_)
#   define MagickExport  __declspec(dllimport)
#   if defined(_VISUALC_)
#    pragma message( "Magick lib DLL import interface" )
#   endif
#  else
#   define MagickExport  __declspec(dllexport)
#   if defined(_VISUALC_)
#    pragma message( "Magick lib DLL export interface" )
#   endif
#  endif
# else
#  define MagickExport
#  if defined(_VISUALC_)
#   pragma message( "Magick lib static interface" )
#  endif
# endif

# if defined(_DLL) && !defined(_LIB)
#  define ModuleExport  __declspec(dllexport)
#  if defined(_VISUALC_)
#   pragma message( "Magick module DLL export interface" )
#  endif
# else
#  define ModuleExport
#  if defined(_VISUALC_)
#   pragma message( "Magick module static interface" )
#  endif

# endif
# define MagickGlobal __declspec(thread)
# if defined(_VISUALC_)
#  pragma warning(disable : 4018)
#  pragma warning(disable : 4244)
#  pragma warning(disable : 4142)
#  pragma warning(disable : 4800)
#  pragma warning(disable : 4786)
# endif
#else
# define MagickExport
# define ModuleExport
# define MagickGlobal
#endif

#define MaxTextExtent  4096
#define MagickSignature  0xabacadabUL

#if !defined(magick_attribute)
#  if !defined(__GNUC__)
#    define magick_attribute(x)  /* nothing */
#  else
#    define magick_attribute  __attribute__
#  endif
#endif

#if defined(MagickMethodPrefix)
# include "magick/methods.h"
#endif
#include "magick/magick-type.h"
#include "magick/animate.h"
#include "magick/annotate.h"
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/cache.h"
#include "magick/cache-view.h"
#include "magick/coder.h"
#include "magick/client.h"
#include "magick/color.h"
#include "magick/colorspace.h"
#include "magick/compare.h"
#include "magick/composite.h"
#include "magick/compress.h"
#include "magick/configure.h"
#include "magick/constitute.h"
#include "magick/decorate.h"
#include "magick/delegate.h"
#include "magick/deprecate.h"
#include "magick/display.h"
#include "magick/draw.h"
#include "magick/effect.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/fx.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/hashmap.h"
#include "magick/identify.h"
#include "magick/image.h"
#include "magick/layer.h"
#include "magick/list.h"
#include "magick/locale_.h"
#include "magick/log.h"
#include "magick/magic.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/module.h"
#include "magick/monitor.h"
#include "magick/montage.h"
#include "magick/option.h"
#include "magick/paint.h"
#include "magick/pixel.h"
#include "magick/prepress.h"
#include "magick/profile.h"
#include "magick/quantize.h"
#include "magick/quantum.h"
#include "magick/registry.h"
#include "magick/random_.h"
#include "magick/resize.h"
#include "magick/resource_.h"
#include "magick/segment.h"
#include "magick/shear.h"
#include "magick/signature.h"
#include "magick/splay-tree.h"
#include "magick/stream.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/timer.h"
#include "magick/token.h"
#include "magick/transform.h"
#include "magick/type.h"
#include "magick/utility.h"
#include "magick/version.h"
#include "magick/xml-tree.h"
#include "magick/xwindow.h"

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
