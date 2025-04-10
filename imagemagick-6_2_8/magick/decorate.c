/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%            DDDD   EEEEE   CCCC   OOO   RRRR    AAA   TTTTT  EEEEE           %
%            D   D  E      C      O   O  R   R  A   A    T    E               %
%            D   D  EEE    C      O   O  RRRR   AAAAA    T    EEE             %
%            D   D  E      C      O   O  R R    A   A    T    E               %
%            DDDD   EEEEE   CCCC   OOO   R  R   A   A    T    EEEEE           %
%                                                                             %
%                                                                             %
%                     ImageMagick Image Decoration Methods                    %
%                                                                             %
%                                Software Design                              %
%                                  John Cristy                                %
%                                   July 1992                                 %
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
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/color-private.h"
#include "magick/colorspace-private.h"
#include "magick/composite.h"
#include "magick/decorate.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/pixel-private.h"
#include "magick/quantum.h"

/*
  Define declarations.
*/
#define AccentuateModulate  ScaleCharToQuantum(80)
#define HighlightModulate  ScaleCharToQuantum(125)
#define ShadowModulate  ScaleCharToQuantum(135)
#define DepthModulate  ScaleCharToQuantum(185)
#define TroughModulate  ScaleCharToQuantum(110)

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B o r d e r I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BorderImage() surrounds the image with a border of the color defined by
%  the bordercolor member of the image structure.  The width and height
%  of the border are defined by the corresponding members of the border_info
%  structure.
%
%  The format of the BorderImage method is:
%
%      Image *BorderImage(const Image *image,const RectangleInfo *border_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o border_info:  Define the width and height of the border.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *BorderImage(const Image *image,
  const RectangleInfo *border_info,ExceptionInfo *exception)
{
  Image
    *border_image,
    *clone_image;

  FrameInfo
    frame_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(border_info != (RectangleInfo *) NULL);
  frame_info.width=image->columns+(border_info->width << 1);
  frame_info.height=image->rows+(border_info->height << 1);
  frame_info.x=(long) border_info->width;
  frame_info.y=(long) border_info->height;
  frame_info.inner_bevel=0;
  frame_info.outer_bevel=0;
  clone_image=CloneImage(image,0,0,MagickTrue,exception);
  if (clone_image == (Image *) NULL)
    return((Image *) NULL);
  clone_image->matte_color=image->border_color;
  border_image=FrameImage(clone_image,&frame_info,exception);
  clone_image=DestroyImage(clone_image);
  if (border_image != (Image *) NULL)
    border_image->matte_color=image->matte_color;
  return(border_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F r a m e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FrameImage() adds a simulated three-dimensional border around the image.
%  The color of the border is defined by the matte_color member of image.
%  Members width and height of frame_info specify the border width of the
%  vertical and horizontal sides of the frame.  Members inner and outer
%  indicate the width of the inner and outer shadows of the frame.
%
%  The format of the FrameImage method is:
%
%      Image *FrameImage(const Image *image,const FrameInfo *frame_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o frame_info: Define the width and height of the frame and its bevels.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *FrameImage(const Image *image,const FrameInfo *frame_info,
  ExceptionInfo *exception)
{
#define FrameImageTag  "Frame/Image"

  Image
    *frame_image;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    accentuate,
    border,
    highlight,
    interior,
    matte,
    shadow,
    trough;

  register IndexPacket
    *frame_indexes;

  register long
    x;

  register PixelPacket
    *q;

  unsigned long
    bevel_width,
    height,
    width;

  /*
    Check frame geometry.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(frame_info != (FrameInfo *) NULL);
  bevel_width=(unsigned long) (frame_info->outer_bevel+frame_info->inner_bevel);
  width=frame_info->width-frame_info->x-bevel_width;
  height=frame_info->height-frame_info->y-bevel_width;
  if ((width < image->columns) || (height < image->rows))
    ThrowImageException(OptionError,"FrameIsLessThanImageSize");
  /*
    Initialize framed image attributes.
  */
  frame_image=CloneImage(image,frame_info->width,frame_info->height,MagickTrue,
    exception);
  if (frame_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(frame_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&frame_image->exception);
		  frame_image=DestroyImage(frame_image);
      return((Image *) NULL);
    }
  if (frame_image->matte_color.opacity != OpaqueOpacity)
    frame_image->matte=MagickTrue;
  frame_image->page=image->page;
  if ((image->page.width != 0) && (image->page.height != 0))
    {
      frame_image->page.width+=frame_image->columns-image->columns;
      frame_image->page.height+=frame_image->rows-image->rows;
    }
  /*
    Initialize 3D effects color.
  */
  GetMagickPixelPacket(frame_image,&interior);
  SetMagickPixelPacket(&image->border_color,(IndexPacket *) NULL,&interior);
  GetMagickPixelPacket(frame_image,&matte);
  matte.colorspace=RGBColorspace;
  SetMagickPixelPacket(&image->matte_color,(IndexPacket *) NULL,&matte);
  GetMagickPixelPacket(frame_image,&border);
  border.colorspace=RGBColorspace;
  SetMagickPixelPacket(&image->border_color,(IndexPacket *) NULL,&border);
  GetMagickPixelPacket(frame_image,&accentuate);
  accentuate.red=QuantumScale*((QuantumRange-AccentuateModulate)*matte.red+
    (QuantumRange*AccentuateModulate));
  accentuate.green=QuantumScale*((QuantumRange-AccentuateModulate)*matte.green+
    (QuantumRange*AccentuateModulate));
  accentuate.blue=QuantumScale*((QuantumRange-AccentuateModulate)*matte.blue+
    (QuantumRange*AccentuateModulate));
  accentuate.opacity=matte.opacity;
  GetMagickPixelPacket(frame_image,&highlight);
  highlight.red=QuantumScale*((QuantumRange-HighlightModulate)*matte.red+
    (QuantumRange*HighlightModulate));
  highlight.green=QuantumScale*((QuantumRange-HighlightModulate)*matte.green+
    (QuantumRange*HighlightModulate));
  highlight.blue=QuantumScale*((QuantumRange-HighlightModulate)*matte.blue+
    (QuantumRange*HighlightModulate));
  highlight.opacity=matte.opacity;
  GetMagickPixelPacket(frame_image,&shadow);
  shadow.red=QuantumScale*matte.red*ShadowModulate;
  shadow.green=QuantumScale*matte.green*ShadowModulate;
  shadow.blue=QuantumScale*matte.blue*ShadowModulate;
  shadow.opacity=matte.opacity;
  GetMagickPixelPacket(frame_image,&trough);
  trough.red=QuantumScale*matte.red*TroughModulate;
  trough.green=QuantumScale*matte.green*TroughModulate;
  trough.blue=QuantumScale*matte.blue*TroughModulate;
  trough.opacity=matte.opacity;
  if (image->colorspace == CMYKColorspace)
    {
      RGBtoCMYK(&interior);
      RGBtoCMYK(&matte);
      RGBtoCMYK(&border);
      RGBtoCMYK(&accentuate);
      RGBtoCMYK(&highlight);
      RGBtoCMYK(&shadow);
      RGBtoCMYK(&trough);
    }
  height=(unsigned long) (frame_info->outer_bevel+(frame_info->y-bevel_width)+
    frame_info->inner_bevel);
  q=SetImagePixels(frame_image,0,0,frame_image->columns,height);
  frame_indexes=GetIndexes(frame_image);
  if (q != (PixelPacket *) NULL)
    {
      /*
        Draw top of ornamental border.
      */
      for (y=0; y < (long) frame_info->outer_bevel; y++)
      {
        for (x=0; x < (long) (frame_image->columns-y); x++)
        {
          if (x < y)
            SetPixelPacket(&highlight,q,frame_indexes);
          else
            SetPixelPacket(&accentuate,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        for ( ; x < (long) frame_image->columns; x++)
        {
          SetPixelPacket(&shadow,q,frame_indexes);
          q++;
          frame_indexes++;
        }
      }
      for (y=0; y < (long) (frame_info->y-bevel_width); y++)
      {
        for (x=0; x < (long) frame_info->outer_bevel; x++)
        {
          SetPixelPacket(&highlight,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        width=frame_image->columns-2*frame_info->outer_bevel;
        for (x=0; x < (long) width; x++)
        {
          SetPixelPacket(&matte,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        for (x=0; x < (long) frame_info->outer_bevel; x++)
        {
          SetPixelPacket(&shadow,q,frame_indexes);
          q++;
          frame_indexes++;
        }
      }
      for (y=0; y < (long) frame_info->inner_bevel; y++)
      {
        for (x=0; x < (long) frame_info->outer_bevel; x++)
        {
          SetPixelPacket(&highlight,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        for (x=0; x < (long) (frame_info->x-bevel_width); x++)
        {
          SetPixelPacket(&matte,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        width=image->columns+((unsigned long) frame_info->inner_bevel << 1)-y;
        for (x=0; x < (long) width; x++)
        {
          if (x < y)
            SetPixelPacket(&shadow,q,frame_indexes);
          else
            SetPixelPacket(&trough,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        for ( ; x < (long) (image->columns+2*frame_info->inner_bevel); x++)
        {
          SetPixelPacket(&highlight,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        width=frame_info->width-frame_info->x-image->columns-bevel_width;
        for (x=0; x < (long) width; x++)
        {
          SetPixelPacket(&matte,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        for (x=0; x < (long) frame_info->outer_bevel; x++)
        {
          SetPixelPacket(&shadow,q,frame_indexes);
          q++;
          frame_indexes++;
        }
      }
      (void) SyncImagePixels(frame_image);
    }
  /*
    Draw sides of ornamental border.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    /*
      Initialize scanline with matte color.
    */
    q=SetImagePixels(frame_image,0,frame_info->y+y,frame_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    frame_indexes=GetIndexes(frame_image);
    for (x=0; x < (long) frame_info->outer_bevel; x++)
    {
      SetPixelPacket(&highlight,q,frame_indexes);
      q++;
      frame_indexes++;
    }
    for (x=0; x < (long) (frame_info->x-bevel_width); x++)
    {
      SetPixelPacket(&matte,q,frame_indexes);
      q++;
      frame_indexes++;
    }
    for (x=0; x < (long) frame_info->inner_bevel; x++)
    {
      SetPixelPacket(&shadow,q,frame_indexes);
      q++;
      frame_indexes++;
    }
    /*
      Set frame interior to interior color.
    */
    for (x=0; x < (long) image->columns; x++)
    {
      SetPixelPacket(&interior,q,frame_indexes);
      q++;
      frame_indexes++;
    }
    for (x=0; x < (long) frame_info->inner_bevel; x++)
    {
      SetPixelPacket(&highlight,q,frame_indexes);
      q++;
      frame_indexes++;
    }
    width=frame_info->width-frame_info->x-image->columns-bevel_width;
    for (x=0; x < (long) width; x++)
    {
      SetPixelPacket(&matte,q,frame_indexes);
      q++;
      frame_indexes++;
    }
    for (x=0; x < (long) frame_info->outer_bevel; x++)
    {
      SetPixelPacket(&shadow,q,frame_indexes);
      q++;
      frame_indexes++;
    }
    if (SyncImagePixels(frame_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(FrameImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  height=(unsigned long) (frame_info->inner_bevel+frame_info->height-
    frame_info->y-image->rows-bevel_width+frame_info->outer_bevel);
  q=SetImagePixels(frame_image,0,(long) (frame_image->rows-height),
    frame_image->columns,height);
  if (q != (PixelPacket *) NULL)
    {
      /*
        Draw bottom of ornamental border.
      */
      frame_indexes=GetIndexes(frame_image);
      for (y=frame_info->inner_bevel-1; y >= 0; y--)
      {
        for (x=0; x < (long) frame_info->outer_bevel; x++)
        {
          SetPixelPacket(&highlight,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        for (x=0; x < (long) (frame_info->x-bevel_width); x++)
        {
          SetPixelPacket(&matte,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        for (x=0; x < y; x++)
        {
          SetPixelPacket(&shadow,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        for ( ; x < (long) (image->columns+2*frame_info->inner_bevel); x++)
        {
          if (x >= (long) (image->columns+2*frame_info->inner_bevel-y))
            SetPixelPacket(&highlight,q,frame_indexes);
          else
            SetPixelPacket(&accentuate,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        width=frame_info->width-frame_info->x-image->columns-bevel_width;
        for (x=0; x < (long) width; x++)
        {
          SetPixelPacket(&matte,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        for (x=0; x < (long) frame_info->outer_bevel; x++)
        {
          SetPixelPacket(&shadow,q,frame_indexes);
          q++;
          frame_indexes++;
        }
      }
      height=frame_info->height-frame_info->y-image->rows-bevel_width;
      for (y=0; y < (long) height; y++)
      {
        for (x=0; x < (long) frame_info->outer_bevel; x++)
        {
          SetPixelPacket(&highlight,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        width=frame_image->columns-2*frame_info->outer_bevel;
        for (x=0; x < (long) width; x++)
        {
          SetPixelPacket(&matte,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        for (x=0; x < (long) frame_info->outer_bevel; x++)
        {
          SetPixelPacket(&shadow,q,frame_indexes);
          q++;
          frame_indexes++;
        }
      }
      for (y=frame_info->outer_bevel-1; y >= 0; y--)
      {
        for (x=0; x < y; x++)
        {
          SetPixelPacket(&highlight,q,frame_indexes);
          q++;
          frame_indexes++;
        }
        for ( ; x < (long) frame_image->columns; x++)
        {
          if (x >= (long) (frame_image->columns-y))
            SetPixelPacket(&shadow,q,frame_indexes);
          else
            SetPixelPacket(&trough,q,frame_indexes);
          q++;
          frame_indexes++;
        }
      }
      (void) SyncImagePixels(frame_image);
    }
  x=(long) (frame_info->outer_bevel+(frame_info->x-bevel_width)+
    frame_info->inner_bevel);
  y=(long) (frame_info->outer_bevel+(frame_info->y-bevel_width)+
    frame_info->inner_bevel);
  (void) CompositeImage(frame_image,image->compose,image,x,y);
  return(frame_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R a i s e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RaiseImage() creates a simulated three-dimensional button-like effect
%  by lightening and darkening the edges of the image.  Members width and
%  height of raise_info define the width of the vertical and horizontal
%  edge of the effect.
%
%  The format of the RaiseImage method is:
%
%      MagickBooleanType RaiseImage(const Image *image,
%        const RectangleInfo *raise_info,const MagickBooleanType raise)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o raise_info: Define the width and height of the raise area.
%
%    o raise: A value other than zero creates a 3-D raise effect,
%      otherwise it has a lowered effect.
%
%
*/
MagickExport MagickBooleanType RaiseImage(Image *image,
  const RectangleInfo *raise_info,const MagickBooleanType raise)
{
#define AccentuateFactor  ScaleCharToQuantum(135)
#define HighlightFactor  ScaleCharToQuantum(190)
#define ShadowFactor  ScaleCharToQuantum(190)
#define RaiseImageTag  "Raise/Image"
#define TroughFactor  ScaleCharToQuantum(135)

  MagickBooleanType
    status;

  Quantum
    foreground,
    background;

  long
    y;

  register long
    x;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(raise_info != (RectangleInfo *) NULL);
  if ((image->columns <= (raise_info->width << 1)) ||
      (image->rows <= (raise_info->height << 1)))
    ThrowBinaryException(OptionError,"ImageSizeMustExceedBevelWidth",
      image->filename);
  foreground=QuantumRange;
  background=0;
  if (raise == MagickFalse)
    {
      foreground=0;
      background=QuantumRange;
    }
  if (SetImageStorageClass(image,DirectClass) == MagickFalse)
    return(MagickFalse);
  for (y=0; y < (long) raise_info->height; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < y; x++)
    {
      q->red=(Quantum) (QuantumScale*((MagickRealType) q->red*
        HighlightFactor+(MagickRealType) foreground*(QuantumRange-
        HighlightFactor))+0.5);
      q->green=(Quantum) (QuantumScale*((MagickRealType) q->green*
        HighlightFactor+(MagickRealType) foreground*(QuantumRange-
        HighlightFactor))+0.5);
      q->blue=(Quantum) (QuantumScale*((MagickRealType) q->blue*
        HighlightFactor+(MagickRealType) foreground*(QuantumRange-
        HighlightFactor))+0.5);
      q++;
    }
    for ( ; x < (long) (image->columns-y); x++)
    {
      q->red=(Quantum) (QuantumScale*((MagickRealType) q->red*AccentuateFactor+
        (MagickRealType) foreground*(QuantumRange-AccentuateFactor))+0.5);
      q->green=(Quantum) (QuantumScale*((MagickRealType) q->green*
        AccentuateFactor+(MagickRealType) foreground*(QuantumRange-
        AccentuateFactor))+0.5);
      q->blue=(Quantum) (QuantumScale*((MagickRealType) q->blue*
        AccentuateFactor+(MagickRealType) foreground*(QuantumRange-
        AccentuateFactor))+0.5);
      q++;
    }
    for ( ; x < (long) image->columns; x++)
    {
      q->red=(Quantum) (QuantumScale*((MagickRealType) q->red*ShadowFactor+
        (MagickRealType) background*(QuantumRange-ShadowFactor))+0.5);
      q->green=(Quantum) (QuantumScale*((MagickRealType) q->green*ShadowFactor+
        (MagickRealType) background*(QuantumRange-ShadowFactor))+0.5);
      q->blue=(Quantum) (QuantumScale*((MagickRealType) q->blue*ShadowFactor+
        (MagickRealType) background*(QuantumRange-ShadowFactor))+0.5);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(RaiseImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  for ( ; y < (long) (image->rows-raise_info->height); y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) raise_info->width; x++)
    {
      q->red=(Quantum) (QuantumScale*((MagickRealType) q->red*
        HighlightFactor+(MagickRealType) foreground*(QuantumRange-
        HighlightFactor))+0.5);
      q->green=(Quantum) (QuantumScale*((MagickRealType) q->green*
        HighlightFactor+(MagickRealType) foreground*(QuantumRange-
        HighlightFactor))+0.5);
      q->blue=(Quantum) (QuantumScale*((MagickRealType) q->blue*
        HighlightFactor+(MagickRealType) foreground*(QuantumRange-
        HighlightFactor))+0.5);
      q++;
    }
    for ( ; x < (long) (image->columns-raise_info->width); x++)
      q++;
    for ( ; x < (long) image->columns; x++)
    {
      q->red=(Quantum) (QuantumScale*((MagickRealType) q->red*ShadowFactor+
        (MagickRealType) background*(QuantumRange-ShadowFactor))+0.5);
      q->green=(Quantum) (QuantumScale*((MagickRealType) q->green*ShadowFactor+
        (MagickRealType) background*(QuantumRange-ShadowFactor))+0.5);
      q->blue=(Quantum) (QuantumScale*((MagickRealType) q->blue*ShadowFactor+
        (MagickRealType) background*(QuantumRange-ShadowFactor))+0.5);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(RaiseImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  for ( ; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) (image->rows-y); x++)
    {
      q->red=(Quantum) (QuantumScale*((MagickRealType) q->red*
        HighlightFactor+(MagickRealType) foreground*(QuantumRange-
        HighlightFactor))+0.5);
      q->green=(Quantum) (QuantumScale*((MagickRealType) q->green*
        HighlightFactor+(MagickRealType) foreground*(QuantumRange-
        HighlightFactor))+0.5);
      q->blue=(Quantum) (QuantumScale*((MagickRealType) q->blue*
        HighlightFactor+(MagickRealType) foreground*(QuantumRange-
        HighlightFactor))+0.5);
      q++;
    }
    for ( ; x < (long) (image->columns-(image->rows-y)); x++)
    {
      q->red=(Quantum) (QuantumScale*((MagickRealType) q->red*TroughFactor+
        (MagickRealType) background*(QuantumRange-TroughFactor))+0.5);
      q->green=(Quantum) (QuantumScale*((MagickRealType) q->green*TroughFactor+
        (MagickRealType) background*(QuantumRange-TroughFactor))+0.5);
      q->blue=(Quantum) (QuantumScale*((MagickRealType) q->blue*TroughFactor+
        (MagickRealType) background*(QuantumRange-TroughFactor))+0.5);
      q++;
    }
    for ( ; x < (long) image->columns; x++)
    {
      q->red=(Quantum) (QuantumScale*((MagickRealType) q->red*ShadowFactor+
        (MagickRealType) background*(QuantumRange-ShadowFactor))+0.5);
      q->green=(Quantum) (QuantumScale*((MagickRealType) q->green*ShadowFactor+
        (MagickRealType) background*(QuantumRange-ShadowFactor))+0.5);
      q->blue=(Quantum) (QuantumScale*((MagickRealType) q->blue*ShadowFactor+
        (MagickRealType) background*(QuantumRange-ShadowFactor))+0.5);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(RaiseImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(MagickTrue);
}
