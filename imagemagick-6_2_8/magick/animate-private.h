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

  MagickCORE private methods to interactively animate an image sequence.
*/
#ifndef _MAGICKCORE_ANIMATE_PRIVATE_H
#define _MAGICKCORE_ANIMATE_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(HasX11)
#include "magick/xwindow-private.h"

extern MagickExport Image
  *XAnimateImages(Display *,XResourceInfo *,char **,const int,Image *);

extern MagickExport void
  XAnimateBackgroundImage(Display *,XResourceInfo *,Image *);
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
