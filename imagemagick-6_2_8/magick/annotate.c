/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%           AAA   N   N  N   N   OOO   TTTTT   AAA   TTTTT  EEEEE             %
%          A   A  NN  N  NN  N  O   O    T    A   A    T    E                 %
%          AAAAA  N N N  N N N  O   O    T    AAAAA    T    EEE               %
%          A   A  N  NN  N  NN  O   O    T    A   A    T    E                 %
%          A   A  N   N  N   N   OOO     T    A   A    T    EEEEE             %
%                                                                             %
%                                                                             %
%                  ImageMagick Image Annotation Methods                       %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%  Copyright 1999-2006 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Digital Applications (www.digapp.com) contributed the stroked text algorithm.
% It was written by Leonard Rosenthol.
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/annotate.h"
#include "magick/attribute.h"
#include "magick/cache-view.h"
#include "magick/client.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/composite.h"
#include "magick/composite-private.h"
#include "magick/constitute.h"
#include "magick/draw.h"
#include "magick/draw-private.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/image-private.h"
#include "magick/log.h"
#include "magick/quantum.h"
#include "magick/resource_.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/transform.h"
#include "magick/type.h"
#include "magick/utility.h"
#include "magick/xwindow-private.h"
#if defined(HasFREETYPE)
#if defined(__MINGW32__)
#  undef interface
#endif
#if defined(HAVE_FT2BUILD_H)
#  include <ft2build.h>
#endif
#if defined(FT_FREETYPE_H)
#  include FT_FREETYPE_H
#else
#  include <freetype/freetype.h>
#endif
#if defined(FT_GLYPH_H)
#  include FT_GLYPH_H
#else
#  include <freetype/ftglyph.h>
#endif
#if defined(FT_OUTLINE_H)
#  include FT_OUTLINE_H
#else
#  include <freetype/ftoutln.h>
#endif
#if defined(FT_BBOX_H)
#  include FT_BBOX_H
#else
#  include <freetype/ftbbox.h>
#endif /* defined(FT_BBOX_H) */
#endif

/*
  Forward declarations.
*/
static MagickBooleanType
  RenderType(Image *,const DrawInfo *,const PointInfo *,TypeMetric *),
  RenderPostscript(Image *,const DrawInfo *,const PointInfo *,TypeMetric *),
  RenderFreetype(Image *,const DrawInfo *,const char *,const PointInfo *,
    TypeMetric *),
  RenderX11(Image *,const DrawInfo *,const PointInfo *,TypeMetric *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A n n o t a t e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AnnotateImage() annotates an image with text.  Optionally you can include
%  any of the following bits of information about the image by embedding
%  the appropriate special characters:
%
%    %b   file size in bytes.
%    %c   comment.
%    %d   directory in which the image resides.
%    %e   extension of the image file.
%    %f   original filename of the image.
%    %h   height of image.
%    %i   filename of the image.
%    %k   number of unique colors.
%    %l   image label.
%    %m   image file format.
%    %n   number of images in a image sequence.
%    %o   output image filename.
%    %p   page number of the image.
%    %q   image depth (8 or 16).
%    %p   page number of the image.
%    %q   image depth (8 or 16).
%    %s   image scene number.
%    %t   image filename without any extension.
%    %u   a unique temporary filename.
%    %w   image width.
%    %x   x resolution of the image.
%    %y   y resolution of the image.
%
%  The format of the AnnotateImage method is:
%
%      MagickBooleanType AnnotateImage(Image *image,DrawInfo *draw_info)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%
*/
MagickExport MagickBooleanType AnnotateImage(Image *image,
  const DrawInfo *draw_info)
{
  char
    primitive[MaxTextExtent],
    **textlist;

  DrawInfo
    *annotate,
    *annotate_info;

  GeometryInfo
    geometry_info;

  MagickBooleanType
    status;

  PointInfo
    offset;

  RectangleInfo
    geometry;

  register long
    i;

  size_t
    length;

  TypeMetric
    metrics;

  unsigned long
    height,
    number_lines;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->signature == MagickSignature);
  if (draw_info->text == (char *) NULL)
    return(MagickFalse);
  if (*draw_info->text == '\0')
    return(MagickTrue);
  textlist=StringToList(draw_info->text);
  if (textlist == (char **) NULL)
    return(MagickFalse);
  length=strlen(textlist[0]);
  for (i=1; textlist[i] != (char *) NULL; i++)
    if (strlen(textlist[i]) > length)
      length=strlen(textlist[i]);
  number_lines=(unsigned long) i;
  annotate=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  SetGeometry(image,&geometry);
  SetGeometryInfo(&geometry_info);
  if (annotate_info->geometry != (char *) NULL)
    {
      (void) ParsePageGeometry(image,annotate_info->geometry,&geometry);
      (void) ParseGeometry(annotate_info->geometry,&geometry_info);
    }
  if (SetImageStorageClass(image,DirectClass) == MagickFalse)
    return(MagickFalse);
  status=MagickTrue;
  for (i=0; textlist[i] != (char *) NULL; i++)
  {
    /*
      Position text relative to image.
    */
    annotate_info->affine.tx=geometry_info.xi-image->page.x;
    annotate_info->affine.ty=geometry_info.psi-image->page.y;
    (void) CloneString(&annotate->text,textlist[i]);
    (void) GetTypeMetrics(image,annotate,&metrics);
    height=(unsigned long) (metrics.ascent-metrics.descent+0.5);
    switch (annotate->gravity)
    {
      case UndefinedGravity:
      default:
      {
        offset.x=annotate_info->affine.tx+i*annotate_info->affine.ry*height;
        offset.y=annotate_info->affine.ty+i*annotate_info->affine.sy*height;
        break;
      }
      case NorthWestGravity:
      {
        offset.x=(geometry.width == 0 ? -1.0 : 1.0)*annotate_info->affine.tx+i*
          annotate_info->affine.ry*height+annotate_info->affine.ry*
          (metrics.ascent+metrics.descent);
        offset.y=(geometry.height == 0 ? -1.0 : 1.0)*annotate_info->affine.ty+i*
          annotate_info->affine.sy*height+annotate_info->affine.sy*
          metrics.ascent;
        break;
      }
      case NorthGravity:
      {
        offset.x=(geometry.width == 0 ? -1.0 : 1.0)*annotate_info->affine.tx+
          geometry.width/2.0+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width/2+annotate_info->affine.ry*
          (metrics.ascent+metrics.descent);
        offset.y=(geometry.height == 0 ? -1.0 : 1.0)*annotate_info->affine.ty+i*
          annotate_info->affine.sy*height+annotate_info->affine.sy*
          metrics.ascent-annotate_info->affine.rx*metrics.width/2.0;
        break;
      }
      case NorthEastGravity:
      {
        offset.x=(geometry.width == 0 ? 1.0 : -1.0)*annotate_info->affine.tx+
          geometry.width+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width+annotate_info->affine.ry*
          (metrics.ascent+metrics.descent);
        offset.y=(geometry.height == 0 ? -1.0 : 1.0)*annotate_info->affine.ty+i*
          annotate_info->affine.sy*height+annotate_info->affine.sy*
          metrics.ascent-annotate_info->affine.rx*metrics.width;
        break;
      }
      case WestGravity:
      {
        offset.x=(geometry.width == 0 ? -1.0 : 1.0)*annotate_info->affine.tx+i*
          annotate_info->affine.ry*height+annotate_info->affine.ry*
          (metrics.ascent+metrics.descent-(number_lines-1.0)*height)/2.0;
        offset.y=(geometry.height == 0 ? -1.0 : 1.0)*annotate_info->affine.ty+
          geometry.height/2.0+i*annotate_info->affine.sy*height+
          annotate_info->affine.sy*(metrics.ascent+metrics.descent-
          (number_lines-1.0)*height)/2.0;
        break;
      }
      case StaticGravity:
      case CenterGravity:
      {
        offset.x=(geometry.width == 0 ? -1.0 : 1.0)*annotate_info->affine.tx+
          geometry.width/2.0+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width/2.0+annotate_info->affine.ry*
          (metrics.ascent+metrics.descent-(number_lines-1)*height)/2.0;
        offset.y=(geometry.height == 0 ? -1.0 : 1.0)*annotate_info->affine.ty+
          geometry.height/2.0+i*annotate_info->affine.sy*height-
          annotate_info->affine.rx*metrics.width/2.0+annotate_info->affine.sy*
          (metrics.ascent+metrics.descent-(number_lines-1.0)*height)/2.0;
        break;
      }
      case EastGravity:
      {
        offset.x=(geometry.width == 0 ? 1.0 : -1.0)*annotate_info->affine.tx+
          geometry.width+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width+annotate_info->affine.ry*
          (metrics.ascent+metrics.descent-(number_lines-1.0)*height)/2.0;
        offset.y=(geometry.height == 0 ? -1.0 : 1.0)*annotate_info->affine.ty+
          geometry.height/2.0+i*annotate_info->affine.sy*height-
          annotate_info->affine.rx*metrics.width+annotate_info->affine.sy*
          (metrics.ascent+metrics.descent-(number_lines-1.0)*height)/2.0;
        break;
      }
      case SouthWestGravity:
      {
        offset.x=(geometry.width == 0 ? -1.0 : 1.0)*annotate_info->affine.tx+i*
          annotate_info->affine.ry*height-annotate_info->affine.ry*
          (number_lines-1.0)*height;
        offset.y=(geometry.height == 0 ? 1.0 : -1.0)*annotate_info->affine.ty+
          geometry.height+i*annotate_info->affine.sy*height-
          annotate_info->affine.sy*(number_lines-1.0)*height+metrics.descent;
        break;
      }
      case SouthGravity:
      {
        offset.x=(geometry.width == 0 ? -1.0 : 1.0)*annotate_info->affine.tx+
          geometry.width/2.0+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width/2.0-annotate_info->affine.ry*
          (number_lines-1.0)*height/2.0;
        offset.y=(geometry.height == 0 ? 1.0 : -1.0)*annotate_info->affine.ty+
          geometry.height+i*annotate_info->affine.sy*height-
          annotate_info->affine.rx*metrics.width/2.0-annotate_info->affine.sy*
          (number_lines-1.0)*height+metrics.descent;
        break;
      }
      case SouthEastGravity:
      {
        offset.x=(geometry.width == 0 ? 1.0 : -1.0)*annotate_info->affine.tx+
          geometry.width+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width-annotate_info->affine.ry*
          (number_lines-1.0)*height;
        offset.y=(geometry.height == 0 ? 1.0 : -1.0)*annotate_info->affine.ty+
          geometry.height+i*annotate_info->affine.sy*height-
          annotate_info->affine.rx*metrics.width-annotate_info->affine.sy*
          (number_lines-1.0)*height+metrics.descent;
        break;
      }
    }
    switch (annotate->align)
    {
      case LeftAlign:
      {
        offset.x=annotate_info->affine.tx+i*annotate_info->affine.ry*height;
        offset.y=annotate_info->affine.ty+i*annotate_info->affine.sy*height;
        break;
      }
      case CenterAlign:
      {
        offset.x=annotate_info->affine.tx+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width/2.0;
        offset.y=annotate_info->affine.ty+i*annotate_info->affine.sy*height-
          annotate_info->affine.rx*metrics.width/2.0;
        break;
      }
      case RightAlign:
      {
        offset.x=annotate_info->affine.tx+i*annotate_info->affine.ry*height-
          annotate_info->affine.sx*metrics.width;
        offset.y=annotate_info->affine.ty+i*annotate_info->affine.sy*height-
          annotate_info->affine.rx*metrics.width;
        break;
      }
      default:
        break;
    }
    if (draw_info->undercolor.opacity != TransparentOpacity)
      {
        DrawInfo
          *undercolor_info;

        /*
          Text box.
        */
        undercolor_info=CloneDrawInfo((ImageInfo *) NULL,(DrawInfo *) NULL);
        undercolor_info->fill=draw_info->undercolor;
        undercolor_info->affine=draw_info->affine;
        undercolor_info->affine.tx=offset.x-draw_info->affine.ry*metrics.ascent;
        undercolor_info->affine.ty=offset.y-draw_info->affine.sy*metrics.ascent;
        (void) FormatMagickString(primitive,MaxTextExtent,
          "rectangle 0,0 %g,%ld",metrics.width,height);
        (void) CloneString(&undercolor_info->primitive,primitive);
        (void) DrawImage(image,undercolor_info);
        (void) DestroyDrawInfo(undercolor_info);
      }
    annotate_info->affine.tx=offset.x;
    annotate_info->affine.ty=offset.y;
    (void) FormatMagickString(primitive,MaxTextExtent,
      "stroke-width %g line 0,0 %g,0",metrics.underline_thickness,
      metrics.width);
    if (annotate->decorate == OverlineDecoration)
      {
        annotate_info->affine.ty-=(draw_info->affine.sy*
          (metrics.ascent+metrics.descent-metrics.underline_position+2));
        (void) CloneString(&annotate_info->primitive,primitive);
        (void) DrawImage(image,annotate_info);
      }
    else
      if (annotate->decorate == UnderlineDecoration)
        {
          annotate_info->affine.ty-=
            (draw_info->affine.sy*metrics.underline_position+2);
          (void) CloneString(&annotate_info->primitive,primitive);
          (void) DrawImage(image,annotate_info);
        }
    /*
      Annotate image with text.
    */
    status=RenderType(image,annotate,&offset,&metrics);
    if (status == MagickFalse)
      break;
    if (annotate->decorate == LineThroughDecoration)
      {
        annotate_info->affine.ty-=(draw_info->affine.sy*(height+
          metrics.underline_position+metrics.descent)/2.0);
        (void) CloneString(&annotate_info->primitive,primitive);
        (void) DrawImage(image,annotate_info);
      }
  }
  /*
    Free resources.
  */
  annotate_info=DestroyDrawInfo(annotate_info);
  annotate=DestroyDrawInfo(annotate);
  for (i=0; textlist[i] != (char *) NULL; i++)
    textlist[i]=(char *) RelinquishMagickMemory(textlist[i]);
  textlist=(char **) RelinquishMagickMemory(textlist);
  return(status);
}

#if defined(HasFREETYPE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   E n c o d e S J I S                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EncodeSJIS() converts an ASCII text string to 2-bytes per character code
%  (like UCS-2).  Returns the translated codes and the character count.
%  Characters under 0x7f are just copied, characters over 0x80 are tied with
%  the next character.
%
%  Katsutoshi Shibuya contributed this method.
%
%  The format of the EncodeSJIS function is:
%
%      encoding=EncodeSJIS(const Image *image,const char *text,size_t count)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o text: The text.
%
%    o count: return the number of characters generated by the encoding.
%
%
*/

static int GetOneCharacter(const unsigned char *text,size_t *length)
{
  int
    c;

  if (*length < 1)
    return(-1);
  c=text[0];
  if ((c & 0x80) == 0)
    {
      *length=1;
      return((int) c);
    }
  if (*length < 2)
    {
      *length=0;
      return(-1);
    }
  *length=2;
  c=((int) (text[0]) << 8);
  c|=text[1];
  return((int) c);
}

static unsigned short *EncodeSJIS(const Image *image,const char *text,
  size_t *count)
{
  int
    c;

  register const char
    *p;

  register unsigned short
    *q;

  size_t
    length;

  unsigned short
    *encoding;

  *count=0;
  if ((text == (char *) NULL) || (*text == '\0'))
    return((unsigned short *) NULL);
  length=strlen(text)+MaxTextExtent;
  encoding=(unsigned short *) AcquireMagickMemory(length*sizeof(*encoding));
  if (encoding == (unsigned short *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      image->filename);
  q=encoding;
  for (p=text; *p != '\0'; p+=length)
  {
    length=strlen(p);
    c=GetOneCharacter((const unsigned char *) p,&length);
    if (c < 0)
      {
        q=encoding;
        for (p=text; *p != '\0'; p++)
          *q++=(unsigned char) *p;
        break;
      }
    *q=(unsigned short) c;
    q++;
  }
  *count=q-encoding;
  return(encoding);
}
#endif

#if defined(HasFREETYPE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   E n c o d e T e x t                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EncodeText() converts an ASCII text string to wide text and returns the
%  translation and the character count.
%
%  The format of the EncodeText function is:
%
%      encoding=EncodeText(const Image *image,const char *text,size_t count)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o text: The text.
%
%    o count: return the number of characters generated by the encoding.
%
%
*/
static unsigned short *EncodeText(const Image *image,const char *text,
  size_t *count)
{
  register const char
    *p;

  register unsigned short
    *q;

  unsigned short
    *encoding;

  *count=0;
  if ((text == (char *) NULL) || (*text == '\0'))
    return((unsigned short *) NULL);
  encoding=(unsigned short *)
    AcquireMagickMemory((strlen(text)+MaxTextExtent)*sizeof(*encoding));
  if (encoding == (unsigned short *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      image->filename);
  q=encoding;
  for (p=text; *p != '\0'; p++)
    *q++=(unsigned char) *p;
  *count=q-encoding;
  return(encoding);
}
#endif

#if defined(HasFREETYPE)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   E n c o d e U n i c o d e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EncodeUnicode() converts an ASCII text string to Unicode and returns the
%  Unicode translation and the character count.  Characters under 0x7f are
%  just copied, characters over 0x80 are tied with the next character.
%
%  The format of the EncodeUnicode function is:
%
%      unsigned short *EncodeUnicode(const Image *image,
%        const unsigned char *text,size_t count)
%
%  A description of each parameter follows:
%
%    o unicode:  EncodeUnicode() returns a pointer to an unsigned short array
%      representing the encoded version of the ASCII string.
%
%    o text: The text.
%
%    o count: return the number of characters generated by the encoding.
%
*/

static long GetUnicodeCharacter(const unsigned char *text,size_t *length)
{
  unsigned long
    c;

  if (*length < 1)
    return(-1);
  c=text[0];
  if ((c & 0x80) == 0)
    {
      *length=1;
      return((long) c);
    }
  if ((*length < 2) || ((text[1] & 0xc0) != 0x80))
    {
      *length=0;
      return(-1);
    }
  if ((c & 0xe0) != 0xe0)
    {
      *length=2;
      c=(text[0] & 0x1f) << 6;
      c|=text[1] & 0x3f;
      return((long) c);
    }
  if ((*length < 3) || ((text[2] & 0xc0) != 0x80))
    {
      *length=0;
      return(-1);
    }
  if ((c & 0xf0) != 0xf0)
    {
      *length=3;
      c=(text[0] & 0xf) << 12;
      c|=(text[1] & 0x3f) << 6;
      c|=text[2] & 0x3f;
      return((long) c);
    }
  if ((*length < 4) || ((c & 0xf8) != 0xf0) || ((text[3] & 0xc0) != 0x80))
    {
      *length=0;
      return(-1);
    }
  *length=4;
  c=(text[0] & 0x7) << 18;
  c|=(text[1] & 0x3f) << 12;
  c|=(text[2] & 0x3f) << 6;
  c|=text[3] & 0x3f;
  return((long) c);
}

static unsigned short *EncodeUnicode(const Image *image,const char *text,
  size_t *count)
{
  long
    c;

  register const char
    *p;

  register unsigned short
    *q;

  size_t
    length;

  unsigned short
    *unicode;

  *count=0;
  if ((text == (char *) NULL) || (*text == '\0'))
    return((unsigned short *) NULL);
  length=strlen(text)+MaxTextExtent;
  unicode=(unsigned short *) AcquireMagickMemory(length*sizeof(*unicode));
  if (unicode == (unsigned short *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      image->filename);
  q=unicode;
  for (p=text; *p != '\0'; p+=length)
  {
    length=strlen(p);
    c=GetUnicodeCharacter((const unsigned char *) p,&length);
    if (c < 0)
      {
        q=unicode;
        for (p=text; *p != '\0'; p++)
          *q++=(unsigned char) *p;
        break;
      }
    *q=(unsigned short) c;
    q++;
  }
  *count=q-unicode;
  return(unicode);
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M u l t i l i n e T y p e M e t r i c s                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMultilineTypeMetrics() returns the following information for the
%  specified font and text:
%
%    character width
%    character height
%    ascent
%    descent
%    text width
%    text height
%    maximum horizontal advance
%    underline position
%    underline thickness
%
%  This method is like GetTypeMetrics() but it returns the maximum text width
%  and height for multiple lines of text.
%
%  The format of the GetMultilineTypeMetrics method is:
%
%      MagickBooleanType GetMultilineTypeMetrics(Image *image,
%        const DrawInfo *draw_info,TypeMetric *metrics)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%    o metrics: Return the font metrics in this structure.
%
*/
MagickExport MagickBooleanType GetMultilineTypeMetrics(Image *image,
  const DrawInfo *draw_info,TypeMetric *metrics)
{
  char
    **textlist;

  double
    max_width;

  DrawInfo
    *annotate_info;

  MagickBooleanType
    status;

  PointInfo
    offset;

  register long
    i;

  unsigned long
    number_lines;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->text != (char *) NULL);
  assert(draw_info->signature == MagickSignature);
  if (*draw_info->text == '\0')
    return(MagickFalse);
  annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  annotate_info->text=(char *) RelinquishMagickMemory(annotate_info->text);
  /*
    Convert newlines to multiple lines of text.
  */
  textlist=StringToList(draw_info->text);
  if (textlist == (char **) NULL)
    return(MagickFalse);
  annotate_info->render=MagickFalse;
  (void) ResetMagickMemory(metrics,0,sizeof(*metrics));
  offset.x=0.0;
  offset.y=0.0;
  /*
    Find the widest of the text lines.
  */
  annotate_info->text = textlist[0];
  status=RenderType(image,annotate_info,&offset,metrics);
  max_width = metrics->width;
  for (i=1; textlist[i] != (char *) NULL; i++)
  {
    annotate_info->text = textlist[i];
    status=RenderType(image,annotate_info,&offset,metrics);
    if (metrics->width > max_width)
      max_width=metrics->width;
  }
  number_lines=(unsigned long) i;
  metrics->width=max_width;
  metrics->height=(double) number_lines*(long) (metrics->ascent-
    metrics->descent+0.5)+1;
  /*
    Free resources.
  */
  annotate_info->text=(char *) NULL;
  annotate_info=DestroyDrawInfo(annotate_info);
  for (i=0; textlist[i] != (char *) NULL; i++)
    textlist[i]=(char *) RelinquishMagickMemory(textlist[i]);
  textlist=(char **) RelinquishMagickMemory(textlist);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t T y p e M e t r i c s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetTypeMetrics() returns the following information for the specified font
%  and text:
%
%    character width
%    character height
%    ascent
%    descent
%    text width
%    text height
%    maximum horizontal advance
%    underline position
%    underline thickness
%
%  The format of the GetTypeMetrics method is:
%
%      MagickBooleanType GetTypeMetrics(Image *image,const DrawInfo *draw_info,
%        TypeMetric *metrics)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%    o metrics: Return the font metrics in this structure.
%
%
*/
MagickExport MagickBooleanType GetTypeMetrics(Image *image,
  const DrawInfo *draw_info,TypeMetric *metrics)
{
  DrawInfo
    *annotate_info;

  MagickBooleanType
    status;

  PointInfo
    offset;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(draw_info != (DrawInfo *) NULL);
  assert(draw_info->text != (char *) NULL);
  assert(draw_info->signature == MagickSignature);
  annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  annotate_info->render=MagickFalse;
  (void) ResetMagickMemory(metrics,0,sizeof(*metrics));
  offset.x=0.0;
  offset.y=0.0;
  status=RenderType(image,annotate_info,&offset,metrics);
  annotate_info=DestroyDrawInfo(annotate_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e n d e r T y p e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RenderType() renders text on the image.  It also returns the bounding box of
%  the text relative to the image.
%
%  The format of the RenderType method is:
%
%      MagickBooleanType RenderType(Image *image,DrawInfo *draw_info,
%        const PointInfo *offset,TypeMetric *metrics)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%
*/
static MagickBooleanType RenderType(Image *image,const DrawInfo *draw_info,
  const PointInfo *offset,TypeMetric *metrics)
{
  const TypeInfo
    *type_info;

  DrawInfo
    *annotate_info;

  MagickBooleanType
    status;

  type_info=(const TypeInfo *) NULL;
  if (draw_info->font != (char *) NULL)
    {
      if (*draw_info->font == '@')
        return(RenderFreetype(image,draw_info,(char *) NULL,offset,metrics));
      if (*draw_info->font == '-')
        return(RenderX11(image,draw_info,offset,metrics));
      if (IsAccessible(draw_info->font) != MagickFalse)
        return(RenderFreetype(image,draw_info,(char *) NULL,offset,metrics));
      type_info=GetTypeInfo(draw_info->font,&image->exception);
      if (type_info == (const TypeInfo *) NULL)
        (void) ThrowMagickException(&image->exception,GetMagickModule(),
          TypeWarning,"UnableToReadFont","`%s'",draw_info->font);
    }
  if ((type_info == (const TypeInfo *) NULL) &&
      (draw_info->family != (const char *) NULL))
    {
      type_info=GetTypeInfoByFamily(draw_info->family,draw_info->style,
        draw_info->stretch,draw_info->weight,&image->exception);
      if (type_info == (const TypeInfo *) NULL)
        (void) ThrowMagickException(&image->exception,GetMagickModule(),
          TypeWarning,"UnableToReadFont","`%s'",draw_info->family);
    }
  if (type_info == (const TypeInfo *) NULL)
    type_info=GetTypeInfoByFamily("arial",draw_info->style,
      draw_info->stretch,draw_info->weight,&image->exception);
  if (type_info == (const TypeInfo *) NULL)
    type_info=GetTypeInfoByFamily("helvetica",draw_info->style,
      draw_info->stretch,draw_info->weight,&image->exception);
  if (type_info == (const TypeInfo *) NULL)
    type_info=GetTypeInfoByFamily((const char *) NULL,draw_info->style,
      draw_info->stretch,draw_info->weight,&image->exception);
  if (type_info == (const TypeInfo *) NULL)
    return(RenderPostscript(image,draw_info,offset,metrics));
  annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  annotate_info->face=type_info->face;
  if (type_info->metrics != (char *) NULL)
    (void) CloneString(&annotate_info->metrics,type_info->metrics);
  if (type_info->glyphs != (char *) NULL)
    (void) CloneString(&annotate_info->font,type_info->glyphs);
  status=RenderFreetype(image,annotate_info,type_info->encoding,offset,metrics);
  annotate_info=DestroyDrawInfo(annotate_info);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e n d e r F r e e t y p e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RenderFreetype() renders text on the image with a Truetype font.  It also
%  returns the bounding box of the text relative to the image.
%
%  The format of the RenderFreetype method is:
%
%      MagickBooleanType RenderFreetype(Image *image,DrawInfo *draw_info,
%        const char *encoding,const PointInfo *offset,TypeMetric *metrics)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%    o encoding: The font encoding.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%
*/

#if defined(HasFREETYPE)
static int TraceCubicBezier(FT_Vector *p,FT_Vector *q,FT_Vector *to,
  DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MaxTextExtent];

  affine=draw_info->affine;
  (void) FormatMagickString(path,MaxTextExtent,"C%g,%g %g,%g %g,%g",
    affine.tx+p->x/64.0,affine.ty-p->y/64.0,affine.tx+q->x/64.0,
    affine.ty-q->y/64.0,affine.tx+to->x/64.0,affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static int TraceLineTo(FT_Vector *to,DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MaxTextExtent];

  affine=draw_info->affine;
  (void) FormatMagickString(path,MaxTextExtent,"L%g,%g",affine.tx+to->x/64.0,
    affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static int TraceMoveTo(FT_Vector *to,DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MaxTextExtent];

  affine=draw_info->affine;
  (void) FormatMagickString(path,MaxTextExtent,"M%g,%g",affine.tx+to->x/64.0,
    affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static int TraceQuadraticBezier(FT_Vector *control,FT_Vector *to,
  DrawInfo *draw_info)
{
  AffineMatrix
    affine;

  char
    path[MaxTextExtent];

  affine=draw_info->affine;
  (void) FormatMagickString(path,MaxTextExtent,"Q%g,%g %g,%g",
    affine.tx+control->x/64.0,affine.ty-control->y/64.0,affine.tx+to->x/64.0,
    affine.ty-to->y/64.0);
  (void) ConcatenateString(&draw_info->primitive,path);
  return(0);
}

static MagickBooleanType RenderFreetype(Image *image,const DrawInfo *draw_info,
  const char *encoding,const PointInfo *offset,TypeMetric *metrics)
{
#if !defined(FT_OPEN_PATHNAME)
#define FT_OPEN_PATHNAME  ft_open_pathname
#endif

  typedef struct _GlyphInfo
  {
    FT_UInt
      id;

    FT_Vector
      origin;

    FT_Glyph
      image;
  } GlyphInfo;

  const ImageAttribute
    *attribute;

  DrawInfo
    *annotate_info;

  FT_BBox
    bounds;

  FT_BitmapGlyph
    bitmap;

  FT_Encoding
    encoding_type;

  FT_Error
    status;

  FT_Face
    face;

  FT_Int32
    flags;

  FT_Library
    library;

  FT_Matrix
    affine;

  FT_Open_Args
    args;

  FT_Vector
    origin;

  GlyphInfo
    glyph,
    last_glyph;

  long
    y;

  MagickBooleanType
    active;

  MagickRealType
    fill_opacity;

  PixelPacket
    fill_color;

  PointInfo
    point,
    resolution;

  register long
    i,
    x;

  register PixelPacket
    *q;

  register unsigned char
    *p;

  size_t
    length;

  static FT_Outline_Funcs
    OutlineMethods =
    {
      (FT_Outline_MoveTo_Func) TraceMoveTo,
      (FT_Outline_LineTo_Func) TraceLineTo,
      (FT_Outline_ConicTo_Func) TraceQuadraticBezier,
      (FT_Outline_CubicTo_Func) TraceCubicBezier,
      0, 0
    };

  unsigned short
    *text;

  /*
    Initialize Truetype library.
  */
  status=FT_Init_FreeType(&library);
  if (status != 0)
    ThrowBinaryException(TypeError,"UnableToInitializeFreetypeLibrary",
      image->filename);
  args.flags=FT_OPEN_PATHNAME;
  if (*draw_info->font != '@')
    args.pathname=draw_info->font;
  else
    args.pathname=draw_info->font+1;
  face=(FT_Face) NULL;
  status=FT_Open_Face(library,&args,draw_info->face,&face);
  if (status != 0)
    {
      (void) FT_Done_FreeType(library);
      ThrowBinaryException(TypeError,"UnableToReadFont",draw_info->font);
    }
  if ((draw_info->metrics != (char *) NULL) &&
      (IsAccessible(draw_info->metrics) != MagickFalse))
    (void) FT_Attach_File(face,draw_info->metrics);
  encoding_type=ft_encoding_unicode;
  status=FT_Select_Charmap(face,encoding_type);
  if ((status != 0) && (face->num_charmaps != 0))
    status=FT_Set_Charmap(face,face->charmaps[0]);
  if (encoding != (char *) NULL)
    {
      if (LocaleCompare(encoding,"AdobeCustom") == 0)
        encoding_type=ft_encoding_adobe_custom;
      if (LocaleCompare(encoding,"AdobeExpert") == 0)
        encoding_type=ft_encoding_adobe_expert;
      if (LocaleCompare(encoding,"AdobeStandard") == 0)
        encoding_type=ft_encoding_adobe_standard;
      if (LocaleCompare(encoding,"AppleRoman") == 0)
        encoding_type=ft_encoding_apple_roman;
      if (LocaleCompare(encoding,"BIG5") == 0)
        encoding_type=ft_encoding_big5;
      if (LocaleCompare(encoding,"GB2312") == 0)
        encoding_type=ft_encoding_gb2312;
      if (LocaleCompare(encoding,"Johab") == 0)
        encoding_type=ft_encoding_johab;
#if defined(ft_encoding_latin_1)
      if (LocaleCompare(encoding,"Latin-1") == 0)
        encoding_type=ft_encoding_latin_1;
#endif
      if (LocaleCompare(encoding,"Latin-2") == 0)
        encoding_type=ft_encoding_latin_2;
      if (LocaleCompare(encoding,"None") == 0)
        encoding_type=ft_encoding_none;
      if (LocaleCompare(encoding,"SJIScode") == 0)
        encoding_type=ft_encoding_sjis;
      if (LocaleCompare(encoding,"Symbol") == 0)
        encoding_type=ft_encoding_symbol;
      if (LocaleCompare(encoding,"Unicode") == 0)
        encoding_type=ft_encoding_unicode;
      if (LocaleCompare(encoding,"Wansung") == 0)
        encoding_type=ft_encoding_wansung;
      status=FT_Select_Charmap(face,encoding_type);
      if (status != 0)
        ThrowBinaryException(TypeError,"UnrecognizedFontEncoding",encoding);
    }
  /*
    Set text size.
  */
  resolution.x=DefaultResolution;
  resolution.y=DefaultResolution;
  if (draw_info->density != (char *) NULL)
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      flags=ParseGeometry(draw_info->density,&geometry_info);
      resolution.x=geometry_info.rho;
      resolution.y=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        resolution.y=resolution.x;
    }
  (void) FT_Set_Char_Size(face,(FT_F26Dot6) (64.0*draw_info->pointsize),
    (FT_F26Dot6) (64.0*draw_info->pointsize),(FT_UInt) resolution.x,
    (FT_UInt) resolution.y);
  metrics->pixels_per_em.x=face->size->metrics.x_ppem;
  metrics->pixels_per_em.y=face->size->metrics.y_ppem;
  metrics->ascent=(double) face->size->metrics.ascender/64.0;
  metrics->descent=(double) face->size->metrics.descender/64.0;
  metrics->width=0;
  metrics->height=(double) face->size->metrics.height/64.0;
  metrics->max_advance=(double) face->size->metrics.max_advance/64.0;
  metrics->bounds.x1=0.0;
  metrics->bounds.y1=metrics->descent;
  metrics->bounds.x2=metrics->ascent+metrics->descent;
  metrics->bounds.y2=metrics->ascent+metrics->descent;
  metrics->underline_position=face->underline_position/64.0;
  metrics->underline_thickness=face->underline_thickness/64.0;
  if (*draw_info->text == '\0')
    {
      (void) FT_Done_Face(face);
      (void) FT_Done_FreeType(library);
      return(MagickTrue);
    }
  /*
    Convert text to 2-byte format as prescribed by the encoding.
  */
  switch (encoding_type)
  {
    case ft_encoding_sjis:
    {
      text=EncodeSJIS(image,draw_info->text,&length);
      break;
    }
    case ft_encoding_unicode:
    {
      text=EncodeUnicode(image,draw_info->text,&length);
      break;
    }
    default:
    {
      if (draw_info->encoding != (char *) NULL)
        {
          if (LocaleCompare(draw_info->encoding,"SJIS") == 0)
            {
              text=EncodeSJIS(image,draw_info->text,&length);
              break;
            }
          if ((LocaleCompare(draw_info->encoding,"UTF-8") == 0) ||
              (encoding_type != ft_encoding_none))
            {
              text=EncodeUnicode(image,draw_info->text,&length);
              break;
            }
        }
      if (encoding_type == ft_encoding_unicode)
        {
          text=EncodeUnicode(image,draw_info->text,&length);
          break;
        }
      text=EncodeText(image,draw_info->text,&length);
      break;
    }
  }
  if (text == (unsigned short *) NULL)
    {
      (void) FT_Done_Face(face);
      (void) FT_Done_FreeType(library);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  /*
    Compute bounding box.
  */
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),"Font %s; "
      "font-encoding %s; text-encoding %s; pointsize %g",
      draw_info->font != (char *) NULL ? draw_info->font : "none",
      encoding != (char *) NULL ? encoding : "none",
      draw_info->encoding != (char *) NULL ? draw_info->encoding : "none",
      draw_info->pointsize);
  flags=FT_LOAD_NO_BITMAP;
  attribute=GetImageAttribute(image,"hinting");
  if ((attribute != (const ImageAttribute *) NULL) &&
      (LocaleCompare(attribute->value,"off") == 0))
    flags|=FT_LOAD_NO_HINTING;
  glyph.id=0;
  glyph.image=NULL;
  last_glyph.id=0;
  last_glyph.image=NULL;
  origin.x=0;
  origin.y=0;
  affine.xx=(FT_Fixed) (65536L*draw_info->affine.sx+0.5);
  affine.yx=(FT_Fixed) (-65536L*draw_info->affine.rx+0.5);
  affine.xy=(FT_Fixed) (-65536L*draw_info->affine.ry+0.5);
  affine.yy=(FT_Fixed) (65536L*draw_info->affine.sy+0.5);
  annotate_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  (void) QueryColorDatabase("#000000ff",&annotate_info->fill,&image->exception);
  (void) CloneString(&annotate_info->primitive,"path '");
  if ((draw_info->render != MagickFalse) && (image->matte == MagickFalse))
    SetImageOpacity(image,OpaqueOpacity);
  for (i=0; i < (long) length; i++)
  {
    glyph.id=FT_Get_Char_Index(face,text[i]);
    if (glyph.id == 0)
      glyph.id=text[i];
    if ((glyph.id != 0) && (last_glyph.id != 0) && FT_HAS_KERNING(face))
      {
        FT_Vector
          kerning;

        (void) FT_Get_Kerning(face,last_glyph.id,glyph.id,ft_kerning_default,
          &kerning);
        origin.x+=kerning.x;
      }
    glyph.origin=origin;
    status=FT_Load_Glyph(face,glyph.id,FT_LOAD_NO_BITMAP);
    if (status != 0)
      continue;
    status=FT_Get_Glyph(face->glyph,&glyph.image);
    if (status != 0)
      continue;
    status=FT_Outline_Get_BBox(&((FT_OutlineGlyph) glyph.image)->outline,
      &bounds);
    if (status != 0)
      continue;
    if ((i == 0) || (bounds.xMin < metrics->bounds.x1))
      metrics->bounds.x1=bounds.xMin;
    if ((i == 0) || (bounds.yMin < metrics->bounds.y1))
      metrics->bounds.y1=bounds.yMin;
    if ((i == 0) || (bounds.xMax > metrics->bounds.x2))
      metrics->bounds.x2=bounds.xMax;
    if ((i == 0) || (bounds.yMax > metrics->bounds.y2))
      metrics->bounds.y2=bounds.yMax;
    if (draw_info->render != MagickFalse)
      if ((draw_info->stroke.opacity != TransparentOpacity) ||
          (draw_info->stroke_pattern != (Image *) NULL))
        {
          /*
            Trace the glyph.
          */
          annotate_info->affine.tx=glyph.origin.x/64.0;
          annotate_info->affine.ty=glyph.origin.y/64.0;
          (void) FT_Outline_Decompose(&((FT_OutlineGlyph) glyph.image)->outline,
            &OutlineMethods,annotate_info);
        }
    FT_Vector_Transform(&glyph.origin,&affine);
    (void) FT_Glyph_Transform(glyph.image,&affine,&glyph.origin);
    if (draw_info->render != MagickFalse)
      {
        long
          x_offset,
          y_offset;

        ViewInfo
          *image_view;

        /*
          Rasterize the glyph.
        */
        status=FT_Glyph_To_Bitmap(&glyph.image,ft_render_mode_normal,
          (FT_Vector *) NULL,MagickTrue);
        if (status != 0)
          continue;
        bitmap=(FT_BitmapGlyph) glyph.image;
        if (SetImageStorageClass(image,DirectClass) == MagickFalse)
          return(MagickFalse);
        point.x=offset->x+bitmap->left;
        point.y=offset->y-bitmap->top;
        p=bitmap->bitmap.buffer;
        image_view=OpenCacheView(image);
        for (y=0; y < (long) bitmap->bitmap.rows; y++)
        {
          x_offset=(long) (point.x+0.5);
          y_offset=(long) (point.y+y+0.5);
          if ((y_offset < 0) || (y_offset >= (long) image->rows))
            {
              p+=bitmap->bitmap.width;
              continue;
            }
          q=GetCacheView(image_view,x_offset,y_offset,bitmap->bitmap.width,1);
          active=q != (PixelPacket *) NULL ? MagickTrue : MagickFalse;
          for (x=0; x < (long) bitmap->bitmap.width; x++)
          {
            x_offset=(long) (point.x+x+0.5);
            if ((*p == 0) || (x_offset < 0) ||
                (x_offset >= (long) image->columns))
              {
                p++;
                q++;
                continue;
              }
            fill_opacity=(MagickRealType) (*p)/255.0;
            if (draw_info->text_antialias == MagickFalse)
              fill_opacity=fill_opacity > 0.5 ? 1.0 : 0.0;
            if (active == MagickFalse)
              q=GetCacheView(image_view,x_offset,y_offset,1,1);
            if (q == (PixelPacket *) NULL)
              {
                p++;
                q++;
                continue;
              }
            fill_color=GetFillColor(draw_info,x_offset,y_offset);
            fill_opacity=QuantumRange-fill_opacity*
              (QuantumRange-fill_color.opacity);
            MagickCompositeOver(&fill_color,fill_opacity,q,q->opacity,q);
            if (active == MagickFalse)
              (void) SyncCacheView(image_view);
            p++;
            q++;
          }
          if (SyncCacheView(image_view) == MagickFalse)
            break;
        }
        image_view=CloseCacheView(image_view);
      }
    origin.x+=face->glyph->advance.x;
    if (origin.x > metrics->width)
      metrics->width=origin.x;
    if (last_glyph.id != 0)
      FT_Done_Glyph(last_glyph.image);
    last_glyph=glyph;
  }
  metrics->width/=64.0;
  metrics->bounds.x1/=64.0;
  metrics->bounds.y1/=64.0;
  metrics->bounds.x2/=64.0;
  metrics->bounds.y2/=64.0;
  if ((draw_info->stroke.opacity != TransparentOpacity) ||
      (draw_info->stroke_pattern != (Image *) NULL))
    {
      metrics->bounds.x1-=draw_info->stroke_width/2.0;
      metrics->bounds.y1-=draw_info->stroke_width/2.0;
      metrics->bounds.x2+=draw_info->stroke_width/2.0;
      metrics->bounds.y2+=draw_info->stroke_width/2.0;
      if (draw_info->render != MagickFalse)
        {
          /*
            Draw text stroke.
          */
          annotate_info->affine.tx=offset->x;
          annotate_info->affine.ty=offset->y;
          (void) ConcatenateString(&annotate_info->primitive,"'");
          (void) DrawImage(image,annotate_info);
        }
      }
  if (glyph.id != 0)
    FT_Done_Glyph(glyph.image);
  /*
    Free resources.
  */
  text=(unsigned short *) RelinquishMagickMemory(text);
  annotate_info=DestroyDrawInfo(annotate_info);
  (void) FT_Done_Face(face);
  (void) FT_Done_FreeType(library);
  return(MagickTrue);
}
#else
static MagickBooleanType RenderFreetype(Image *image,
  const DrawInfo *magick_unused(draw_info),const char *magick_unused(encoding),
  const PointInfo *magick_unused(offset),TypeMetric *magick_unused(metrics))
{
  ThrowBinaryException(MissingDelegateError,"FreeTypeLibraryIsNotAvailable",
    image->filename)
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e n d e r P o s t s c r i p t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RenderPostscript() renders text on the image with a Postscript font.  It
%  also returns the bounding box of the text relative to the image.
%
%  The format of the RenderPostscript method is:
%
%      MagickBooleanType RenderPostscript(Image *image,DrawInfo *draw_info,
%        const PointInfo *offset,TypeMetric *metrics)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%
*/

static char *EscapeParenthesis(const char *text)
{
  char
    *buffer;

  register char
    *p;

  register long
    i;

  unsigned long
    escapes;

  escapes=0;
  buffer=AcquireString(text);
  p=buffer;
  for (i=0; i < (long) Min(strlen(text),MaxTextExtent-escapes-1); i++)
  {
    if ((text[i] == '(') || (text[i] == ')'))
      {
        *p++='\\';
        escapes++;
      }
    *p++=text[i];
  }
  *p='\0';
  return(buffer);
}

static MagickBooleanType RenderPostscript(Image *image,
  const DrawInfo *draw_info,const PointInfo *offset,TypeMetric *metrics)
{
  char
    filename[MaxTextExtent],
    geometry[MaxTextExtent],
    *text;

  FILE
    *file;

  Image
    *annotate_image,
    *pattern;

  ImageInfo
    *annotate_info;

  int
    unique_file;

  long
    y;

  MagickBooleanType
    identity;

  PointInfo
    extent,
    point,
    resolution;

  register long
    i,
    x;

  register PixelPacket
    *q;

  /*
    Render label with a Postscript font.
  */
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),
      "Font %s; pointsize %g",draw_info->font != (char *) NULL ?
      draw_info->font : "none",draw_info->pointsize);
  file=(FILE *) NULL;
  unique_file=AcquireUniqueFileResource(filename);
  if (unique_file != -1)
    file=fdopen(unique_file,"wb");
  if ((unique_file == -1) || (file == (FILE *) NULL))
    {
      ThrowFileException(&image->exception,FileOpenError,"UnableToOpenFile",
        filename);
      return(MagickFalse);
    }
  (void) fprintf(file,"%%!PS-Adobe-3.0\n");
  (void) fprintf(file,"/ReencodeType\n");
  (void) fprintf(file,"{\n");
  (void) fprintf(file,"  findfont dup length\n");
  (void) fprintf(file,
    "  dict begin { 1 index /FID ne {def} {pop pop} ifelse } forall\n");
  (void) fprintf(file,
    "  /Encoding ISOLatin1Encoding def currentdict end definefont pop\n");
  (void) fprintf(file,"} bind def\n");
  /*
    Sample to compute bounding box.
  */
  identity=(draw_info->affine.sx == draw_info->affine.sy) &&
    (draw_info->affine.rx == 0.0) && (draw_info->affine.ry == 0.0) ?
    MagickTrue : MagickFalse;
  extent.x=0.0;
  extent.y=0.0;
  for (i=0; i <= (long) (strlen(draw_info->text)+2); i++)
  {
    point.x=fabs(draw_info->affine.sx*i*draw_info->pointsize+
      draw_info->affine.ry*2.0*draw_info->pointsize);
    point.y=fabs(draw_info->affine.rx*i*draw_info->pointsize+
      draw_info->affine.sy*2.0*draw_info->pointsize);
    if (point.x > extent.x)
      extent.x=point.x;
    if (point.y > extent.y)
      extent.y=point.y;
  }
  (void) fprintf(file,"%g %g moveto\n",identity  != MagickFalse ? 0.0 :
    extent.x/2.0,extent.y/2.0);
  (void) fprintf(file,"%g %g scale\n",draw_info->pointsize,
    draw_info->pointsize);
  if ((draw_info->font == (char *) NULL) || (*draw_info->font == '\0'))
    (void) fprintf(file,
      "/Times-Roman-ISO dup /Times-Roman ReencodeType findfont setfont\n");
  else
    (void) fprintf(file,
      "/%s-ISO dup /%s ReencodeType findfont setfont\n",
      draw_info->font,draw_info->font);
  (void) fprintf(file,"[%g %g %g %g 0 0] concat\n",draw_info->affine.sx,
    -draw_info->affine.rx,-draw_info->affine.ry,draw_info->affine.sy);
  text=EscapeParenthesis(draw_info->text);
  if (identity == MagickFalse)
    (void) fprintf(file,"(%s) stringwidth pop -0.5 mul -0.5 rmoveto\n",
      text);
  (void) fprintf(file,"(%s) show\n",text);
  text=(char *) RelinquishMagickMemory(text);
  (void) fprintf(file,"showpage\n");
  (void) fclose(file);
  (void) FormatMagickString(geometry,MaxTextExtent,"%ldx%ld+0+0!",
    (long) (extent.x+0.5),(long) (extent.y+0.5));
  annotate_info=CloneImageInfo((ImageInfo *) NULL);
  (void) FormatMagickString(annotate_info->filename,MaxTextExtent,
    "ps:%s",filename);
  (void) CloneString(&annotate_info->page,geometry);
  if (draw_info->density != (char *) NULL)
    (void) CloneString(&annotate_info->density,draw_info->density);
  annotate_info->antialias=draw_info->text_antialias;
  annotate_image=ReadImage(annotate_info,&image->exception);
  CatchException(&image->exception);
  annotate_info=DestroyImageInfo(annotate_info);
  (void) RelinquishUniqueFileResource(filename);
  if (annotate_image == (Image *) NULL)
    return(MagickFalse);
  resolution.x=DefaultResolution;
  resolution.y=DefaultResolution;
  if (draw_info->density != (char *) NULL)
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      flags=ParseGeometry(draw_info->density,&geometry_info);
      resolution.x=geometry_info.rho;
      resolution.y=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        resolution.y=resolution.x;
    }
  if (identity == MagickFalse)
    (void) TransformImage(&annotate_image,"0x0",(char *) NULL);
  else
    {
      RectangleInfo
        crop_info;

      crop_info=GetImageBoundingBox(annotate_image,&annotate_image->exception);
      crop_info.height=(unsigned long) ((resolution.y/DefaultResolution)*
        ExpandAffine(&draw_info->affine)*draw_info->pointsize+0.5);
      crop_info.y=(long) ((resolution.y/DefaultResolution)*extent.y/8.0+0.5);
      (void) FormatMagickString(geometry,MaxTextExtent,"%lux%lu%+ld%+ld",
        crop_info.width,crop_info.height,crop_info.x,crop_info.y);
      (void) TransformImage(&annotate_image,geometry,(char *) NULL);
    }
  metrics->pixels_per_em.x=(resolution.y/DefaultResolution)*
    ExpandAffine(&draw_info->affine)*draw_info->pointsize;
  metrics->pixels_per_em.y=metrics->pixels_per_em.x;
  metrics->ascent=metrics->pixels_per_em.x;
  metrics->descent=metrics->pixels_per_em.y/-5.0;
  metrics->width=(double) annotate_image->columns/
    ExpandAffine(&draw_info->affine);
  metrics->height=1.152*metrics->pixels_per_em.x;
  metrics->max_advance=metrics->pixels_per_em.x;
  metrics->bounds.x1=0.0;
  metrics->bounds.y1=metrics->descent;
  metrics->bounds.x2=metrics->ascent+metrics->descent;
  metrics->bounds.y2=metrics->ascent+metrics->descent;
  metrics->underline_position=(-2.0);
  metrics->underline_thickness=1.0;
  if (draw_info->render == MagickFalse)
    {
      annotate_image=DestroyImage(annotate_image);
      return(MagickTrue);
    }
  if (draw_info->fill.opacity != TransparentOpacity)
    {
      PixelPacket
        fill_color;

      ViewInfo
        *annotate_view;

      /*
        Render fill color.
      */
      if (image->matte == MagickFalse)
        SetImageOpacity(image,OpaqueOpacity);
      if (annotate_image->matte == MagickFalse)
        SetImageOpacity(annotate_image,OpaqueOpacity);
      fill_color=draw_info->fill;
      pattern=draw_info->fill_pattern;
      annotate_view=OpenCacheView(annotate_image);
      for (y=0; y < (long) annotate_image->rows; y++)
      {
        q=GetCacheView(annotate_view,0,y,annotate_image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (long) annotate_image->columns; x++)
        {
          if (pattern != (Image *) NULL)
            fill_color=AcquireOnePixel(pattern,
              (long) (x-pattern->extract_info.x) % pattern->columns,
              (long) (y-pattern->extract_info.y) % pattern->rows,
              &image->exception);
          q->opacity=(Quantum) (QuantumRange-(((QuantumRange-(MagickRealType)
            PixelIntensityToQuantum(q))*(QuantumRange-fill_color.opacity))/
            QuantumRange)+0.5);
          q->red=fill_color.red;
          q->green=fill_color.green;
          q->blue=fill_color.blue;
          q++;
        }
        if (SyncCacheView(annotate_view) == MagickFalse)
          break;
      }
      annotate_view=CloseCacheView(annotate_view);
      (void) CompositeImage(image,OverCompositeOp,annotate_image,
        (long) (offset->x+0.5),(long) (offset->y-(metrics->ascent+
        metrics->descent)+0.5));
    }
  annotate_image=DestroyImage(annotate_image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e n d e r X 1 1                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RenderX11() renders text on the image with an X11 font.  It also returns the
%  bounding box of the text relative to the image.
%
%  The format of the RenderX11 method is:
%
%      MagickBooleanType RenderX11(Image *image,DrawInfo *draw_info,
%        const PointInfo *offset,TypeMetric *metrics)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o draw_info: The draw info.
%
%    o offset: (x,y) location of text relative to image.
%
%    o metrics: bounding box of text.
%
%
*/
#if defined(HasX11)
static MagickBooleanType RenderX11(Image *image,const DrawInfo *draw_info,
  const PointInfo *offset,TypeMetric *metrics)
{
  MagickBooleanType
    status;

  static DrawInfo
    cache_info;

  static Display
    *display = (Display *) NULL;

  static XAnnotateInfo
    annotate_info;

  static XFontStruct
    *font_info;

  static XPixelInfo
    pixel;

  static XResourceInfo
    resource_info;

  static XrmDatabase
    resource_database;

  static XStandardColormap
    *map_info;

  static XVisualInfo
    *visual_info;

  unsigned long
    height,
    width;

  if (display == (Display *) NULL)
    {
      /*
        Open X server connection.
      */
      display=XOpenDisplay(draw_info->server_name);
      if (display == (Display *) NULL)
        ThrowBinaryException(XServerError,"UnableToOpenXServer",
          draw_info->server_name);
      /*
        Get user defaults from X resource database.
      */
      (void) XSetErrorHandler(XError);
      resource_database=XGetResourceDatabase(display,GetClientName());
      XGetResourceInfo(resource_database,GetClientName(),&resource_info);
      resource_info.close_server=MagickFalse;
      resource_info.colormap=PrivateColormap;
      resource_info.font=AcquireString(draw_info->font);
      resource_info.background_color=AcquireString("#ffffffffffff");
      resource_info.foreground_color=AcquireString("#000000000000");
      map_info=XAllocStandardColormap();
      if (map_info == (XStandardColormap *) NULL)
        ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
          image->filename);
      /*
        Initialize visual info.
      */
      visual_info=XBestVisualInfo(display,map_info,&resource_info);
      if (visual_info == (XVisualInfo *) NULL)
        ThrowBinaryException(XServerError,"UnableToGetVisual",
          image->filename);
      map_info->colormap=(Colormap) NULL;
      pixel.pixels=(unsigned long *) NULL;
      /*
        Initialize Standard Colormap info.
      */
      XGetMapInfo(visual_info,XDefaultColormap(display,visual_info->screen),
        map_info);
      XGetPixelPacket(display,visual_info,map_info,&resource_info,
        (Image *) NULL,&pixel);
      pixel.annotate_context=XDefaultGC(display,visual_info->screen);
      /*
        Initialize font info.
      */
      font_info=XBestFont(display,&resource_info,MagickFalse);
      if (font_info == (XFontStruct *) NULL)
        ThrowBinaryException(XServerError,"UnableToLoadFont",draw_info->font);
      if ((map_info == (XStandardColormap *) NULL) ||
          (visual_info == (XVisualInfo *) NULL) ||
          (font_info == (XFontStruct *) NULL))
        {
          XFreeResources(display,visual_info,map_info,&pixel,font_info,
            &resource_info,(XWindowInfo *) NULL);
          ThrowBinaryException(XServerError,"UnableToLoadFont",image->filename);
        }
      cache_info=(*draw_info);
    }
  /*
    Initialize annotate info.
  */
  XGetAnnotateInfo(&annotate_info);
  annotate_info.stencil=ForegroundStencil;
  if (cache_info.font != draw_info->font)
    {
      /*
        Type name has changed.
      */
      (void) XFreeFont(display,font_info);
      (void) CloneString(&resource_info.font,draw_info->font);
      font_info=XBestFont(display,&resource_info,MagickFalse);
      if (font_info == (XFontStruct *) NULL)
        ThrowBinaryException(XServerError,"UnableToLoadFont",draw_info->font);
    }
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(AnnotateEvent,GetMagickModule(),
      "Font %s; pointsize %g",draw_info->font != (char *) NULL ?
      draw_info->font : "none",draw_info->pointsize);
  cache_info=(*draw_info);
  annotate_info.font_info=font_info;
  annotate_info.text=(char *) draw_info->text;
  annotate_info.width=(unsigned int) XTextWidth(font_info,draw_info->text,
    (int) strlen(draw_info->text));
  annotate_info.height=(unsigned int) font_info->ascent+font_info->descent;
  metrics->pixels_per_em.x=(double) font_info->max_bounds.width;
  metrics->pixels_per_em.y=(double) font_info->max_bounds.width;
  metrics->ascent=(double) font_info->ascent;
  metrics->descent=(double) (-font_info->descent);
  metrics->width=annotate_info.width/ExpandAffine(&draw_info->affine);
  metrics->height=metrics->pixels_per_em.x+4;
  metrics->max_advance=(double) font_info->max_bounds.width;
  metrics->bounds.x1=0.0;
  metrics->bounds.y1=metrics->descent;
  metrics->bounds.x2=metrics->ascent+metrics->descent;
  metrics->bounds.y2=metrics->ascent+metrics->descent;
  metrics->underline_position=(-2.0);
  metrics->underline_thickness=1.0;
  if (draw_info->render == MagickFalse)
    return(MagickTrue);
  if (draw_info->fill.opacity == TransparentOpacity)
    return(MagickTrue);
  /*
    Render fill color.
  */
  width=annotate_info.width;
  height=annotate_info.height;
  if ((draw_info->affine.rx != 0.0) || (draw_info->affine.ry != 0.0))
    {
      if (((draw_info->affine.sx-draw_info->affine.sy) == 0.0) &&
          ((draw_info->affine.rx+draw_info->affine.ry) == 0.0))
        annotate_info.degrees=(180.0/MagickPI)*
          atan2(draw_info->affine.rx,draw_info->affine.sx);
    }
  (void) FormatMagickString(annotate_info.geometry,MaxTextExtent,
    "%lux%lu+%ld+%ld",width,height,(long) (offset->x+0.5),
    (long) (offset->y-metrics->ascent-metrics->descent+0.5));
  pixel.pen_color.red=ScaleQuantumToShort(draw_info->fill.red);
  pixel.pen_color.green=ScaleQuantumToShort(draw_info->fill.green);
  pixel.pen_color.blue=ScaleQuantumToShort(draw_info->fill.blue);
  status=XAnnotateImage(display,&pixel,&annotate_info,image);
  if (status == 0)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  return(MagickTrue);
}
#else
static MagickBooleanType RenderX11(Image *image,const DrawInfo *draw_info,
  const PointInfo *offset,TypeMetric *metrics)
{
  ThrowBinaryException(MissingDelegateError,"XWindowLibraryIsNotAvailable",
    draw_info->font);
}
#endif
