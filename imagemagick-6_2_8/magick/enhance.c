/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%              EEEEE  N   N  H   H   AAA   N   N   CCCC  EEEEE                %
%              E      NN  N  H   H  A   A  NN  N  C      E                    %
%              EEE    N N N  HHHHH  AAAAA  N N N  C      EEE                  %
%              E      N  NN  H   H  A   A  N  NN  C      E                    %
%              EEEEE  N   N  H   H  A   A  N   N   CCCC  EEEEE                %
%                                                                             %
%                                                                             %
%                    ImageMagick Image Enhancement Methods                    %
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
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/quantum.h"
#include "magick/string_.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o n t r a s t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ContrastImage() enhances the intensity differences between the lighter and
%  darker elements of the image.  Set sharpen to a MagickTrue to increase the
%  image contrast otherwise the contrast is reduced.
%
%  The format of the ContrastImage method is:
%
%      MagickBooleanType ContrastImage(Image *image,
%        const MagickBooleanType sharpen)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o sharpen: Increase or decrease image contrast.
%
%
*/
MagickExport MagickBooleanType ContrastImage(Image *image,
  const MagickBooleanType sharpen)
{
#define DullContrastImageTag  "DullContrast/Image"
#define SharpenContrastImageTag  "SharpenContrast/Image"

  int
    sign;

  long
    y;

  MagickBooleanType
    status;

  register long
    i,
    x;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  sign=sharpen != MagickFalse ? 1 : -1;
  if (image->storage_class == PseudoClass)
    {
      /*
        Contrast enhance colormap.
      */
      for (i=0; i < (long) image->colors; i++)
        Contrast(sign,&image->colormap[i].red,&image->colormap[i].green,
          &image->colormap[i].blue);
    }
  /*
    Contrast enhance image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      Contrast(sign,&q->red,&q->green,&q->blue);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(sharpen != MagickFalse ?
          SharpenContrastImageTag : DullContrastImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o n t r a s t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  The ContrastStretchImage() is a simple image enhancement technique that
%  attempts to improve the contrast in an image by `stretching' the range of
%  intensity values it contains to span a desired range of values. It differs
%  from the more sophisticated histogram equalization in that it can only
%  apply %  a linear scaling function to the image pixel values.  As a result
%  the `enhancement' is less harsh.
%
%  The format of the ContrastStretchImage method is:
%
%      MagickBooleanType ContrastStretchImage(Image *image,
%        const char *levels)
%      MagickBooleanType ContrastStretchImageChannel(Image *image,
%        const unsigned long channel,const double black_point,
%        const double white_point)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o black_point: The black point.
%
%    o white_point: The white point.
%
%    o levels: Specify the levels where the black and white points have the
%      range of 0 to number-of-pixels (e.g. 1%, 10x90%, etc.).
%
*/

MagickExport MagickBooleanType ContrastStretchImage(Image *image,
  const char *levels)
{
  double
    black_point,
    white_point;

  GeometryInfo
    geometry_info;

  MagickBooleanType
    status;

  MagickStatusType
    flags;

  /*
    Parse levels.
  */
  if (levels == (char *) NULL)
    return(MagickFalse);
  flags=ParseGeometry(levels,&geometry_info);
  black_point=geometry_info.rho;
  white_point=(double) image->columns*image->rows;
  if ((flags & SigmaValue) != 0)
    white_point=geometry_info.sigma;
  if ((flags & PercentValue) != 0)
    {
      black_point*=(double) QuantumRange/100.0;
      white_point*=(double) QuantumRange/100.0;
    }
  if ((flags & SigmaValue) == 0)
    white_point=(double) image->columns*image->rows-black_point;
  status=ContrastStretchImageChannel(image,DefaultChannels,black_point,
    white_point);
  return(status);
}

MagickExport MagickBooleanType ContrastStretchImageChannel(Image *image,
  const ChannelType channel,const double black_point,const double white_point)
{
#define MaxRange(color)  ((MagickRealType) ScaleQuantumToMap(color))
#define NormalizeImageTag  "Normalize/Image"

  double
    intensity;

  ExceptionInfo
    *exception;

  long
    y;

  IndexPacket
    *indexes;

  MagickBooleanType
    status;

  MagickPixelPacket
    black,
    *histogram,
    *normalize_map,
    white;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  register PixelPacket
    *q;

  /*
    Allocate histogram and normalize map.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  histogram=(MagickPixelPacket *)
    AcquireMagickMemory((MaxMap+1)*sizeof(*histogram));
  normalize_map=(MagickPixelPacket *)
    AcquireMagickMemory((MaxMap+1)*sizeof(*normalize_map));
  if ((histogram == (MagickPixelPacket *) NULL) ||
      (normalize_map == (MagickPixelPacket *) NULL))
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  /*
    Form histogram.
  */
  (void) ResetMagickMemory(histogram,0,(MaxMap+1)*sizeof(*histogram));
  if (channel == DefaultChannels)
    {
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
        if (p == (const PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          histogram[ScaleQuantumToMap(PixelIntensityToQuantum(p))].index++;
          p++;
        }
      }
      for (x=0; x <= (long) MaxMap; x++)
      {
        histogram[x].red=histogram[x].index;
        histogram[x].green=histogram[x].index;
        histogram[x].blue=histogram[x].index;
        histogram[x].opacity=histogram[x].index;
      }
    }
  else
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) image->columns; x++)
      {
        if ((channel & RedChannel) != 0)
          histogram[ScaleQuantumToMap(p->red)].red++;
        if ((channel & GreenChannel) != 0)
          histogram[ScaleQuantumToMap(p->green)].green++;
        if ((channel & BlueChannel) != 0)
          histogram[ScaleQuantumToMap(p->blue)].blue++;
        if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
          histogram[ScaleQuantumToMap(p->opacity)].opacity++;
        if (((channel & IndexChannel) != 0) &&
            (image->colorspace == CMYKColorspace))
          histogram[ScaleQuantumToMap(indexes[x])].index++;
        p++;
      }
    }
  /*
    Find the histogram boundaries by locating the black/white levels.
  */
  black.red=0.0;
  white.red=MaxRange(QuantumRange)-1.0;
  if ((channel & RedChannel) != 0)
    {
      intensity=0.0;
      for (x=0; x <= (long) MaxMap; x++)
      {
        intensity+=histogram[x].red;
        if (intensity > black_point)
          break;
      }
      black.red=(MagickRealType) x;
      intensity=0.0;
      for (x=(long) MaxMap; x != 0; x--)
      {
        intensity+=histogram[x].red;
        if (intensity > ((double) image->columns*image->rows-white_point))
          break;
      }
      white.red=(MagickRealType) x;
    }
  black.green=0.0;
  white.green=MaxRange(QuantumRange)-1.0;
  if ((channel & GreenChannel) != 0)
    {
      intensity=0.0;
      for (x=0; x <= (long) MaxMap; x++)
      {
        intensity+=histogram[x].green;
        if (intensity > black_point)
          break;
      }
      black.green=(MagickRealType) x;
      intensity=0.0;
      for (x=(long) MaxMap; x != 0; x--)
      {
        intensity+=histogram[x].green;
        if (intensity > ((double) image->columns*image->rows-white_point))
          break;
      }
      white.green=(MagickRealType) x;
    }
  black.blue=0.0;
  white.blue=MaxRange(QuantumRange)-1.0;
  if ((channel & BlueChannel) != 0)
    {
      intensity=0.0;
      for (x=0; x <= (long) MaxMap; x++)
      {
        intensity+=histogram[x].blue;
        if (intensity > black_point)
          break;
      }
      black.blue=(MagickRealType) x;
      intensity=0.0;
      for (x=(long) MaxMap; x != 0; x--)
      {
        intensity+=histogram[x].blue;
        if (intensity > ((double) image->columns*image->rows-white_point))
          break;
      }
      white.blue=(MagickRealType) x;
    }
  black.opacity=0.0;
  white.opacity=MaxRange(QuantumRange)-1.0;
  if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
    {
      intensity=0.0;
      for (x=0; x <= (long) MaxMap; x++)
      {
        intensity+=histogram[x].opacity;
        if (intensity > black_point)
          break;
      }
      black.opacity=(MagickRealType) x;
      intensity=0.0;
      for (x=(long) MaxMap; x != 0; x--)
      {
        intensity+=histogram[x].opacity;
        if (intensity > ((double) image->columns*image->rows-white_point))
          break;
      }
      white.opacity=(MagickRealType) x;
    }
  black.index=0.0;
  white.index=MaxRange(QuantumRange)-1.0;
  if (((channel & IndexChannel) != 0) && (image->colorspace == CMYKColorspace))
    {
      intensity=0.0;
      for (x=0; x <= (long) MaxMap; x++)
      {
        intensity+=histogram[x].index;
        if (intensity > black_point)
          break;
      }
      black.index=(MagickRealType) x;
      intensity=0.0;
      for (x=(long) MaxMap; x != 0; x--)
      {
        intensity+=histogram[x].index;
        if (intensity > ((double) image->columns*image->rows-white_point))
          break;
      }
      white.index=(MagickRealType) x;
    }
  histogram=(MagickPixelPacket *) RelinquishMagickMemory(histogram);
  /*
    Stretch the histogram to create the normalized image mapping.
  */
  (void) ResetMagickMemory(normalize_map,0,(MaxMap+1)*sizeof(*normalize_map));
  for (i=0; i <= (long) MaxMap; i++)
  {
    if ((channel & RedChannel) != 0)
      {
        if (i < (long) black.red)
          normalize_map[i].red=0.0;
        else
          if (i > (long) white.red)
            normalize_map[i].red=(MagickRealType) QuantumRange;
          else
            if (black.red != white.red)
              normalize_map[i].red=(MagickRealType) ScaleMapToQuantum(
                (unsigned long) (MaxMap*(i-black.red)/(white.red-black.red)));
          }
    if ((channel & GreenChannel) != 0)
      {
        if (i < (long) black.green)
          normalize_map[i].green=0.0;
        else
          if (i > (long) white.green)
            normalize_map[i].green=(MagickRealType) QuantumRange;
          else
            if (black.green != white.green)
              normalize_map[i].green=(MagickRealType) ScaleMapToQuantum(
                (unsigned long) (MaxMap*(i-black.green)/(white.green-
                black.green)));
      }
    if ((channel & BlueChannel) != 0)
      {
        if (i < (long) black.blue)
          normalize_map[i].blue=0.0;
        else
          if (i > (long) white.blue)
            normalize_map[i].blue=(MagickRealType) QuantumRange;
          else
            if (black.blue != white.blue)
              normalize_map[i].blue=(MagickRealType) ScaleMapToQuantum(
                (unsigned long) (MaxMap*(i-black.blue)/(white.blue-
                black.blue)));
      }
    if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
      {
        if (i < (long) black.opacity)
          normalize_map[i].opacity=0.0;
        else
          if (i > (long) white.opacity)
            normalize_map[i].opacity=(MagickRealType) QuantumRange;
          else
            if (black.opacity != white.opacity)
              normalize_map[i].opacity=(MagickRealType) ScaleMapToQuantum(
                (unsigned long) (MaxMap*(i-black.opacity)/(white.opacity-
                black.opacity)));
      }
    if (((channel & IndexChannel) != 0) &&
        (image->colorspace == CMYKColorspace))
      {
        if (i < (long) black.index)
          normalize_map[i].index=0.0;
        else
          if (i > (long) white.index)
            normalize_map[i].index=(MagickRealType) QuantumRange;
          else
            if (black.index != white.index)
              normalize_map[i].index=(MagickRealType) ScaleMapToQuantum(
                (unsigned long) (MaxMap*(i-black.index)/(white.index-
                black.index)));
      }
  }
  /*
    Normalize the image.
  */
  if ((((channel & OpacityChannel) != 0) && (image->matte != MagickFalse)) ||
      (((channel & IndexChannel) != 0) &&
        (image->colorspace == CMYKColorspace)))
    image->storage_class=DirectClass;
  if (image->storage_class == PseudoClass)
    {
      /*
        Normalize colormap.
      */
      for (i=0; i < (long) image->colors; i++)
      {
        if ((channel & RedChannel) != 0)
          {
            if (black.red != white.red)
              image->colormap[i].red=RoundToQuantum(normalize_map[
                ScaleQuantumToMap(image->colormap[i].red)].red);
          }
        if ((channel & GreenChannel) != 0)
          {
            if (black.green != white.green)
              image->colormap[i].green=RoundToQuantum(normalize_map[
                ScaleQuantumToMap(image->colormap[i].green)].green);
          }
        if ((channel & BlueChannel) != 0)
          {
            if (black.blue != white.blue)
              image->colormap[i].blue=RoundToQuantum(normalize_map[
                ScaleQuantumToMap(image->colormap[i].blue)].blue);
          }
      }
    }
  /*
    Normalize image.
  */
  exception=(&image->exception);
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=(long) image->columns-1; x >= 0; x--)
    {
      if ((channel & RedChannel) != 0)
        {
          if (black.red != white.red)
            q->red=RoundToQuantum(normalize_map[ScaleQuantumToMap(q->red)].red);
        }
      if ((channel & GreenChannel) != 0)
        {
          if (black.green != white.green)
            q->green=RoundToQuantum(
              normalize_map[ScaleQuantumToMap(q->green)].green);
        }
      if ((channel & BlueChannel) != 0)
        {
          if (black.blue != white.blue)
            q->blue=RoundToQuantum(
              normalize_map[ScaleQuantumToMap(q->blue)].blue);
        }
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        {
          if (black.opacity != white.opacity)
            q->opacity=RoundToQuantum(
              normalize_map[ScaleQuantumToMap(q->opacity)].opacity);
        }
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        {
          if (black.index != white.index)
            indexes[x]=(IndexPacket) RoundToQuantum(
              normalize_map[ScaleQuantumToMap(indexes[x])].index);
        }
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(NormalizeImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  normalize_map=(MagickPixelPacket *) RelinquishMagickMemory(normalize_map);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     E n h a n c e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EnhanceImage() applies a digital filter that improves the quality of a
%  noisy image.
%
%  The format of the EnhanceImage method is:
%
%      Image *EnhanceImage(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *EnhanceImage(const Image *image,ExceptionInfo *exception)
{
#define Enhance(weight) \
  mean=((MagickRealType) r->red+pixel.red)/2; \
  distance=(MagickRealType) r->red-(MagickRealType) pixel.red; \
  distance_squared=QuantumScale*(2.0*((MagickRealType) QuantumRange+1.0)+ \
     mean)*distance*distance; \
  mean=((MagickRealType) r->green+pixel.green)/2; \
  distance=(MagickRealType) r->green-(MagickRealType) pixel.green; \
  distance_squared+=4.0*distance*distance; \
  mean=((MagickRealType) r->blue+pixel.blue)/2; \
  distance=(MagickRealType) r->blue-(MagickRealType) pixel.blue; \
  distance_squared+=QuantumScale*(3.0*((MagickRealType) \
    QuantumRange+1.0)-1.0-mean)*distance*distance; \
  mean=((MagickRealType) r->opacity+pixel.opacity)/2; \
  distance=(MagickRealType) r->opacity-(MagickRealType) pixel.opacity; \
  distance_squared+=QuantumScale*(3.0*((MagickRealType) \
    QuantumRange+1.0)-1.0-mean)*distance*distance; \
  if (distance_squared < ((MagickRealType) QuantumRange*QuantumRange/25.0)) \
    { \
      aggregate.red+=(weight)*r->red; \
      aggregate.green+=(weight)*r->green; \
      aggregate.blue+=(weight)*r->blue; \
      aggregate.opacity+=(weight)*r->opacity; \
      total_weight+=(weight); \
    } \
  r++;
#define EnhanceImageTag  "Enhance/Image"

  Image
    *enhance_image;

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    distance,
    distance_squared,
    mean,
    total_weight;

  PixelPacket
    pixel;

  MagickPixelPacket
    aggregate,
    zero;

  register const PixelPacket
    *p,
    *r;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize enhanced image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if ((image->columns < 5) || (image->rows < 5))
    return((Image *) NULL);
  enhance_image=CloneImage(image,0,0,MagickTrue,exception);
  if (enhance_image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(enhance_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&enhance_image->exception);
      enhance_image=DestroyImage(enhance_image);
      return((Image *) NULL);
    }
  /*
    Enhance image.
  */
  (void) ResetMagickMemory(&zero,0,sizeof(zero));
  (void) ResetMagickMemory(&aggregate,0,sizeof(aggregate));
  for (y=0; y < (long) image->rows; y++)
  {
    /*
      Read another scan line.
    */
    p=AcquireImagePixels(image,-2,y-2,image->columns+4,5,exception);
    q=GetImagePixels(enhance_image,0,y,enhance_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    /*
      Transfer first 2 pixels of the scanline.
    */
    for (x=0; x < (long) image->columns; x++)
    {
      /*
        Compute weighted average of target pixel color components.
      */
      aggregate=zero;
      total_weight=0.0;
      r=p+2*(image->columns+4)+2;
      pixel=(*r);
      r=p;
      Enhance(5.0);  Enhance(8.0);  Enhance(10.0); Enhance(8.0);  Enhance(5.0);
      r=p+(image->columns+4);
      Enhance(8.0);  Enhance(20.0); Enhance(40.0); Enhance(20.0); Enhance(8.0);
      r=p+2*(image->columns+4);
      Enhance(10.0); Enhance(40.0); Enhance(80.0); Enhance(40.0); Enhance(10.0);
      r=p+3*(image->columns+4);
      Enhance(8.0);  Enhance(20.0); Enhance(40.0); Enhance(20.0); Enhance(8.0);
      r=p+4*(image->columns+4);
      Enhance(5.0);  Enhance(8.0);  Enhance(10.0); Enhance(8.0);  Enhance(5.0);
      q->red=(Quantum) ((aggregate.red+(total_weight/2)-1)/total_weight);
      q->green=(Quantum) ((aggregate.green+(total_weight/2)-1)/total_weight);
      q->blue=(Quantum) ((aggregate.blue+(total_weight/2)-1)/total_weight);
      q->opacity=(Quantum)
        ((aggregate.opacity+(total_weight/2)-1)/total_weight);
      p++;
      q++;
    }
    if (SyncImagePixels(enhance_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(EnhanceImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(enhance_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     E q u a l i z e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EqualizeImage() applies a histogram equalization to the image.
%
%  The format of the EqualizeImage method is:
%
%      MagickBooleanType EqualizeImage(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport MagickBooleanType EqualizeImage(Image *image)
{
#define EqualizeImageTag  "Equalize/Image"

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    black,
    *histogram,
    intensity,
    *map,
    white;

  PixelPacket
    *equalize_map;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  register PixelPacket
    *q;

  /*
    Allocate and initialize histogram arrays.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  histogram=(MagickPixelPacket *)
    AcquireMagickMemory((MaxMap+1)*sizeof(*histogram));
  map=(MagickPixelPacket *)
    AcquireMagickMemory((MaxMap+1)*sizeof(*map));
  equalize_map=(PixelPacket *)
    AcquireMagickMemory((MaxMap+1)*sizeof(*equalize_map));
  if ((histogram == (MagickPixelPacket *) NULL) ||
      (map == (MagickPixelPacket *) NULL) ||
      (equalize_map == (PixelPacket *) NULL))
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  /*
    Form histogram.
  */
  (void) ResetMagickMemory(histogram,0,(MaxMap+1)*sizeof(*histogram));
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      histogram[ScaleQuantumToMap(p->red)].red++;
      histogram[ScaleQuantumToMap(p->green)].green++;
      histogram[ScaleQuantumToMap(p->blue)].blue++;
      histogram[ScaleQuantumToMap(p->opacity)].opacity++;
      p++;
    }
  }
  /*
    Integrate the histogram to get the equalization map.
  */
  (void) ResetMagickMemory(&intensity,0,sizeof(intensity));
  for (i=0; i <= (long) MaxMap; i++)
  {
    intensity.red+=histogram[i].red;
    intensity.green+=histogram[i].green;
    intensity.blue+=histogram[i].blue;
    intensity.opacity+=histogram[i].opacity;
    map[i]=intensity;
  }
  black=map[0];
  white=map[MaxMap];
  (void) ResetMagickMemory(equalize_map,0,(MaxMap+1)*sizeof(*equalize_map));
  for (i=0; i <= (long) MaxMap; i++)
  {
    if (white.red != black.red)
      equalize_map[i].red=ScaleMapToQuantum((unsigned long) (
        (MaxMap*(map[i].red-black.red))/(white.red-black.red)));
    if (white.green != black.green)
      equalize_map[i].green=ScaleMapToQuantum((unsigned long) (
        (MaxMap*(map[i].green-black.green))/(white.green-black.green)));
    if (white.blue != black.blue)
      equalize_map[i].blue=ScaleMapToQuantum((unsigned long) (
        (MaxMap*(map[i].blue-black.blue))/(white.blue-black.blue)));
    if (white.opacity != black.opacity)
      equalize_map[i].opacity=ScaleMapToQuantum((unsigned long) (
        (MaxMap*(map[i].opacity-black.opacity))/(white.opacity-black.opacity)));
  }
  histogram=(MagickPixelPacket *) RelinquishMagickMemory(histogram);
  map=(MagickPixelPacket *) RelinquishMagickMemory(map);
  if (image->storage_class == PseudoClass)
    {
      /*
        Equalize colormap.
      */
      for (i=0; i < (long) image->colors; i++)
      {
        if (black.red != white.red)
          image->colormap[i].red=
            equalize_map[ScaleQuantumToMap(image->colormap[i].red)].red;
        if (black.green != white.green)
          image->colormap[i].green=
            equalize_map[ScaleQuantumToMap(image->colormap[i].green)].green;
        if (black.blue != white.blue)
          image->colormap[i].blue=
            equalize_map[ScaleQuantumToMap(image->colormap[i].blue)].blue;
      }
    }
  /*
    Equalize image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      if (black.red != white.red)
        q->red=equalize_map[ScaleQuantumToMap(q->red)].red;
      if (black.green != white.green)
        q->green=equalize_map[ScaleQuantumToMap(q->green)].green;
      if (black.blue != white.blue)
        q->blue=equalize_map[ScaleQuantumToMap(q->blue)].blue;
      if (black.opacity != white.opacity)
        q->opacity=equalize_map[ScaleQuantumToMap(q->opacity)].opacity;
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(EqualizeImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  equalize_map=(PixelPacket *) RelinquishMagickMemory(equalize_map);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     G a m m a I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GammaImage() gamma-corrects a particular image channel.  The same
%  image viewed on different devices will have perceptual differences in the
%  way the image's intensities are represented on the screen.  Specify
%  individual gamma levels for the red, green, and blue channels, or adjust
%  all three with the gamma parameter.  Values typically range from 0.8 to 2.3.
%
%  You can also reduce the influence of a particular channel with a gamma
%  value of 0.
%
%  The format of the GammaImage method is:
%
%      MagickBooleanType GammaImage(Image *image,const double gamma)
%      MagickBooleanType GammaImageChannel(Image *image,
%        const ChannelType channel,const double gamma)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o gamma: The image gamma.
%
*/

MagickExport MagickBooleanType GammaImage(Image *image,const char *level)
{
  GeometryInfo
    geometry_info;

  MagickPixelPacket
    gamma;

  MagickStatusType
    flags,
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (level == (char *) NULL)
    return(MagickFalse);
  flags=ParseGeometry(level,&geometry_info);
  gamma.red=geometry_info.rho;
  gamma.green=geometry_info.sigma;
  if ((flags & SigmaValue) == 0)
    gamma.green=gamma.red;
  gamma.blue=geometry_info.xi;
  if ((flags & XiValue) == 0)
    gamma.blue=gamma.red;
  if ((gamma.red == 1.0) && (gamma.green == 1.0) && (gamma.blue == 1.0))
    return(MagickTrue);
  status=GammaImageChannel(image,RedChannel,(double) gamma.red);
  status|=GammaImageChannel(image,GreenChannel,(double) gamma.green);
  status|=GammaImageChannel(image,BlueChannel,(double) gamma.blue);
  return(status != 0 ? MagickTrue : MagickFalse);
}

MagickExport MagickBooleanType GammaImageChannel(Image *image,
  const ChannelType channel,const double gamma)
{
#define GammaCorrectImageTag  "GammaCorrect/Image"

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    *gamma_map;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  /*
    Allocate and initialize gamma maps.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (gamma == 1.0)
    return(MagickTrue);
  gamma_map=(MagickRealType *)
    AcquireMagickMemory((MaxMap+1)*sizeof(*gamma_map));
  if (gamma_map == (MagickRealType *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  (void) ResetMagickMemory(gamma_map,0,(MaxMap+1)*sizeof(*gamma_map));
  if (gamma != 0.0)
    for (i=0; i <= (long) MaxMap; i++)
      gamma_map[i]=(MagickRealType) ScaleMapToQuantum((unsigned long)
        (MaxMap*pow((double) i/MaxMap,1.0/gamma)+0.5));
  if (image->storage_class == PseudoClass)
    {
      /*
        Gamma-correct colormap.
      */
      for (i=0; i < (long) image->colors; i++)
      {
        if ((channel & RedChannel) != 0)
          image->colormap[i].red=RoundToQuantum
            (gamma_map[ScaleQuantumToMap(image->colormap[i].red)]);
        if ((channel & GreenChannel) != 0)
          image->colormap[i].green=RoundToQuantum
            (gamma_map[ScaleQuantumToMap(image->colormap[i].green)]);
        if ((channel & BlueChannel) != 0)
          image->colormap[i].blue=RoundToQuantum
            (gamma_map[ScaleQuantumToMap(image->colormap[i].blue)]);
        if ((channel & OpacityChannel) != 0)
          image->colormap[i].opacity=RoundToQuantum
            (gamma_map[ScaleQuantumToMap(image->colormap[i].opacity)]);
      }
    }
  /*
    Gamma-correct image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        q->red=RoundToQuantum(gamma_map[ScaleQuantumToMap(q->red)]);
      if ((channel & GreenChannel) != 0)
        q->green=RoundToQuantum(gamma_map[ScaleQuantumToMap(q->green)]);
      if ((channel & BlueChannel) != 0)
        q->blue=RoundToQuantum(gamma_map[ScaleQuantumToMap(q->blue)]);
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        q->opacity=RoundToQuantum(gamma_map[ScaleQuantumToMap(q->opacity)]);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        indexes[x]=RoundToQuantum(gamma_map[ScaleQuantumToMap(indexes[x])]);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(GammaCorrectImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  if (image->gamma != 0.0)
    image->gamma*=gamma;
  gamma_map=(MagickRealType *) RelinquishMagickMemory(gamma_map);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     L e v e l I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LevelImage() adjusts the levels of a particular image channel by
%  scaling the colors falling between specified white and black points to
%  the full available quantum range. The parameters provided represent the
%  black, mid, and white points.  The black point specifies the darkest
%  color in the image. Colors darker than the black point are set to
%  zero. Gamma specifies a gamma correction to apply to the image.
%  White point specifies the lightest color in the image.  Colors brighter
%  than the white point are set to the maximum quantum value.
%
%  The format of the LevelImage method is:
%
%      MagickBooleanType LevelImage(Image *image,const char *levels)
%      MagickBooleanType LevelImageChannel(Image *image,
%        const ChannelType channel,const char *levels)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o levels: Specify the levels where the black and white points have the
%      range of 0-QuantumRange, and gamma has the range 0-10 (e.g. 10x90%+2).
%
*/

MagickExport MagickBooleanType LevelImage(Image *image,const char *levels)
{
  double
    black_point,
    gamma,
    white_point;

  GeometryInfo
    geometry_info;

  MagickBooleanType
    status;

  MagickStatusType
    flags;

  /*
    Parse levels.
  */
  if (levels == (char *) NULL)
    return(MagickFalse);
  flags=ParseGeometry(levels,&geometry_info);
  black_point=geometry_info.rho;
  white_point=(double) QuantumRange;
  if ((flags & SigmaValue) != 0)
    white_point=geometry_info.sigma;
  gamma=1.0;
  if ((flags & XiValue) != 0)
    gamma=geometry_info.xi;
  if ((AbsoluteValue(white_point) <= 10.0) && (AbsoluteValue(gamma) > 10.0))
    {
      double
        swap;

      swap=gamma;
      gamma=white_point;
      white_point=swap;
    }
  if ((flags & PercentValue) != 0)
    {
      black_point*=(double) image->columns*image->rows/100.0;
      white_point*=(double) image->columns*image->rows/100.0;
    }
  if ((flags & SigmaValue) == 0)
    white_point=(double) QuantumRange-black_point;
  status=LevelImageChannel(image,DefaultChannels,black_point,white_point,gamma);
  return(status);
}

MagickExport MagickBooleanType LevelImageChannel(Image *image,
  const ChannelType channel,const double black_point,const double white_point,
  const double gamma)
{
#define LevelImageTag  "Level/Image"

  IndexPacket
    *indexes;

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    *levels_map;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned long
    black,
    white;

  /*
    Allocate and initialize levels map.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  levels_map=(MagickRealType *) AcquireMagickMemory((MaxMap+1)*
    sizeof(*levels_map));
  if (levels_map == (MagickRealType *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  black=ScaleQuantumToMap((Quantum) (black_point+0.5));
  white=ScaleQuantumToMap((Quantum) (white_point+0.5));
  for (i=0; i < (long) black; i++)
    levels_map[i]=0.0;
  if (gamma == 1.0)
    {
      for ( ; i < (long) white; i++)
        levels_map[i]=(MagickRealType) (MaxMap*(((MagickRealType) i-black)/
          (white-black)));
    }
  else
    for ( ; i < (long) white; i++)
      levels_map[i]=(MagickRealType) (MaxMap*(pow(((double) i-black)/
        (white-black),1.0/gamma)));
  for ( ; i <= (long) MaxMap; i++)
    levels_map[i]=(MagickRealType) MaxMap;
  if (image->storage_class == PseudoClass)
    {
      /*
        Level colormap.
      */
      for (i=0; i < (long) image->colors; i++)
      {
        if ((channel & RedChannel) != 0)
          image->colormap[i].red=ScaleMapToQuantum((unsigned long)
            levels_map[ScaleQuantumToMap(image->colormap[i].red)]);
        if ((channel & GreenChannel) != 0)
          image->colormap[i].green=ScaleMapToQuantum((unsigned long)
            levels_map[ScaleQuantumToMap(image->colormap[i].green)]);
        if ((channel & BlueChannel) != 0)
          image->colormap[i].blue=ScaleMapToQuantum((unsigned long)
            levels_map[ScaleQuantumToMap(image->colormap[i].blue)]);
      }
    }
  /*
    Level image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        q->red=ScaleMapToQuantum((unsigned long)
          levels_map[ScaleQuantumToMap(q->red)]);
      if ((channel & GreenChannel) != 0)
        q->green=ScaleMapToQuantum((unsigned long)
          levels_map[ScaleQuantumToMap(q->green)]);
      if ((channel & BlueChannel) != 0)
        q->blue=ScaleMapToQuantum((unsigned long)
          levels_map[ScaleQuantumToMap(q->blue)]);
      if (((channel & OpacityChannel) != 0) & (image->matte != MagickFalse))
        q->opacity=ScaleMapToQuantum((unsigned long)
          levels_map[ScaleQuantumToMap(q->opacity)]);
      if (((channel & IndexChannel) != 0) &&
          ((image->storage_class == PseudoClass) ||
           (image->colorspace == CMYKColorspace)))
        indexes[x]=ScaleMapToQuantum((unsigned long)
          levels_map[ScaleQuantumToMap(indexes[x])]);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(GammaCorrectImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  levels_map=(MagickRealType *) RelinquishMagickMemory(levels_map);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     M o d u l a t e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ModulateImage() lets you control the brightness, saturation, and hue
%  of an image.  Modulate represents the brightness, saturation, and hue
%  as one parameter (e.g. 90,150,100).  If the image colorspace is HSL, the
%  modulation is luminosity, saturation, and hue.  And if the colorspace is
%  HWB, use blackness, whiteness, and hue.
%
%  The format of the ModulateImage method is:
%
%      MagickBooleanType ModulateImage(Image *image,const char *modulate)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o modulate: Define the percent change in brightness, saturation, and
%      hue.
%
%
*/
MagickExport MagickBooleanType ModulateImage(Image *image,const char *modulate)
{
#define ModulateImageTag  "Modulate/Image"

  double
    percent_brightness,
    percent_hue,
    percent_saturation;

  GeometryInfo
    geometry_info;

  long
    y;

  MagickBooleanType
    status;

  MagickStatusType
    flags;

  register long
    i,
    x;

  register PixelPacket
    *q;

  /*
    Initialize gamma table.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (modulate == (char *) NULL)
    return(MagickFalse);
  flags=ParseGeometry(modulate,&geometry_info);
  percent_brightness=geometry_info.rho;
  percent_saturation=geometry_info.sigma;
  if ((flags & SigmaValue) == 0)
    percent_saturation=100.0;
  percent_hue=geometry_info.xi;
  if ((flags & XiValue) == 0)
    percent_hue=100.0;
  (void) SetImageColorspace(image,RGBColorspace);
  if (image->storage_class == PseudoClass)
    {
      /*
        Modulate colormap.
      */
      for (i=0; i < (long) image->colors; i++)
        switch (image->colorspace)
        {
          case HSBColorspace:
          default:
          {
            ModulateHSB(percent_hue,percent_saturation,percent_brightness,
              &image->colormap[i].red,&image->colormap[i].green,
              &image->colormap[i].blue);
            break;
          }
          case HSLColorspace:
          {
            ModulateHSL(percent_hue,percent_saturation,percent_brightness,
              &image->colormap[i].red,&image->colormap[i].green,
              &image->colormap[i].blue);
            break;
          }
          case HWBColorspace:
          {
            ModulateHWB(percent_hue,percent_saturation,percent_brightness,
              &image->colormap[i].red,&image->colormap[i].green,
              &image->colormap[i].blue);
            break;
          }
        }
    }
  /*
    Modulate image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      switch (image->colorspace)
      {
        case HSBColorspace:
        default:
        {
          ModulateHSB(percent_hue,percent_saturation,percent_brightness,
            &q->red,&q->green,&q->blue);
          break;
        }
        case HSLColorspace:
        {
          ModulateHSL(percent_hue,percent_saturation,percent_brightness,
            &q->red,&q->green,&q->blue);
          break;
        }
        case HWBColorspace:
        {
          ModulateHWB(percent_hue,percent_saturation,percent_brightness,
            &q->red,&q->green,&q->blue);
          break;
        }
      }
      q++;
    }
    if (SyncImagePixels(image) == 0)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ModulateImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     N e g a t e I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NegateImage() negates the colors in the reference image.  The grayscale
%  option means that only grayscale values within the image are negated.
%
%  The format of the NegateImageChannel method is:
%
%      MagickBooleanType NegateImage(Image *image,
%        const MagickBooleanType grayscale)
%      MagickBooleanType NegateImageChannel(Image *image,
%        const ChannelType channel,const MagickBooleanType grayscale)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o grayscale: If MagickTrue, only negate grayscale pixels within the image.
%
*/

MagickExport MagickBooleanType NegateImage(Image *image,
  const MagickBooleanType grayscale)
{
  MagickBooleanType
    status;

  status=NegateImageChannel(image,DefaultChannels,grayscale);
  return(status);
}

MagickExport MagickBooleanType NegateImageChannel(Image *image,
  const ChannelType channel,const MagickBooleanType grayscale)
{
#define NegateImageTag  "Negate/Image"

  IndexPacket
    *indexes;

  long
    y;

  MagickBooleanType
    status;

  register long
    x;

  register PixelPacket
    *q;

  register long
    i;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->storage_class == PseudoClass)
    {
      /*
        Negate colormap.
      */
      for (i=0; i < (long) image->colors; i++)
      {
        if (grayscale != MagickFalse)
          if ((image->colormap[i].red != image->colormap[i].green) ||
              (image->colormap[i].green != image->colormap[i].blue))
            continue;
        if ((channel & RedChannel) != 0)
          image->colormap[i].red=(~image->colormap[i].red);
        if ((channel & GreenChannel) != 0)
          image->colormap[i].green=(~image->colormap[i].green);
        if ((channel & BlueChannel) != 0)
          image->colormap[i].blue=(~image->colormap[i].blue);
      }
    }
  /*
    Negate image.
  */
  if (grayscale != MagickFalse)
    {
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) image->columns; x++)
        {
          if ((q->red != q->green) || (q->green != q->blue))
            {
              q++;
              continue;
            }
          if ((channel & RedChannel) != 0)
            q->red=(~q->red);
          if ((channel & GreenChannel) != 0)
            q->green=(~q->green);
          if ((channel & BlueChannel) != 0)
            q->blue=(~q->blue);
          if (((channel & OpacityChannel) != 0) &&
              (image->matte != MagickFalse))
            q->opacity=(~q->opacity);
          if (((channel & IndexChannel) != 0) &&
              (image->colorspace == CMYKColorspace))
            indexes[x]=(~indexes[x]);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(NegateImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      return(MagickTrue);
    }
  /*
    Negate image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        q->red=(~q->red);
      if ((channel & GreenChannel) != 0)
        q->green=(~q->green);
      if ((channel & BlueChannel) != 0)
        q->blue=(~q->blue);
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        q->opacity=(~q->opacity);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        indexes[x]=(~indexes[x]);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(NegateImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     N o r m a l i z e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  The NormalizeImage() method enhances the contrast of a color image by
%  mapping the darkest 2 percent of all pixel to black and the brightest
%  1 percent to white.
%
%  The format of the NormalizeImage method is:
%
%      MagickBooleanType NormalizeImage(Image *image,
%        const unsigned long channel)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
*/

MagickExport MagickBooleanType NormalizeImage(Image *image)
{
  MagickBooleanType
    status;

  status=NormalizeImageChannel(image,DefaultChannels);
  return(status);
}

MagickExport MagickBooleanType NormalizeImageChannel(Image *image,
  const ChannelType channel)
{
  double
    black_point,
    white_point;

  black_point=(double) image->columns*image->rows*0.02;
  white_point=(double) image->columns*image->rows*0.99;
  return(ContrastStretchImageChannel(image,channel,black_point,white_point));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     S i g m o i d a l C o n t r a s t I m a g e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SigmoidalContrastImage() adjusts the contrast of an image with a non-linear
%  sigmoidal contrast algorithm.  Increase the contrast of the image using a
%  sigmoidal transfer function without saturating highlights or shadows.
%  Contrast indicates how much to increase the contrast (0 is none; 3 is
%  typical; 20 is pushing it); mid-point indicates where midtones fall in the
%  resultant image (0 is white; 50% is middle-gray; 100% is black).  Set
%  sharpen to MagickTrue to increase the image contrast otherwise the contrast
%  is reduced.
%
%  The format of the SigmoidalContrastImage method is:
%
%      MagickBooleanType SigmoidalContrastImage(Image *image,
%        const MagickBooleanType sharpen,const char *levels)
%      MagickBooleanType SigmoidalContrastImageChannel(Image *image,
%        const ChannelType channel,const MagickBooleanType sharpen,
%        const double contrast,const double midpoint)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o sharpen: Increase or decrease image contrast.
%
%    o contrast: control the "shoulder" of the contast curve.
%
%    o midpoint: control the "toe" of the contast curve.
%
*/

MagickExport MagickBooleanType SigmoidalContrastImage(Image *image,
  const MagickBooleanType sharpen,const char *levels)
{
  GeometryInfo
    geometry_info;

  MagickBooleanType
    status;

  MagickStatusType
    flags;

  flags=ParseGeometry(levels,&geometry_info);
  if ((flags & SigmaValue) == 0)
    geometry_info.sigma=QuantumRange/2.0;
  if ((flags & PercentValue) != 0)
    geometry_info.sigma=QuantumRange*geometry_info.sigma/100.0;
  status=SigmoidalContrastImageChannel(image,DefaultChannels,sharpen,
    geometry_info.rho,geometry_info.sigma);
  return(status);
}

MagickExport MagickBooleanType SigmoidalContrastImageChannel(Image *image,
  const ChannelType channel,const MagickBooleanType sharpen,
  const double contrast,const double midpoint)
{
#define SigmoidalContrastImageTag  "SigmoidalContrast/Image"

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    *sigmoidal_map;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  /*
    Allocate and initialize sigmoidal maps.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  sigmoidal_map=(MagickRealType *)
    AcquireMagickMemory((MaxMap+1)*sizeof(*sigmoidal_map));
  if (sigmoidal_map == (MagickRealType *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  (void) ResetMagickMemory(sigmoidal_map,0,(MaxMap+1)*sizeof(*sigmoidal_map));
  for (i=0; i <= (long) MaxMap; i++)
  {
    if (sharpen != MagickFalse)
      {
        sigmoidal_map[i]=(MagickRealType) ScaleMapToQuantum((unsigned long)
          (MaxMap*((1.0/(1.0+exp(contrast*(midpoint/(double) QuantumRange-
          (double) i/MaxMap))))-(1.0/(1.0+exp(contrast*(midpoint/
          (double) QuantumRange)))))/((1.0/(1.0+exp(contrast*(midpoint/
          (double) QuantumRange-1.0))))-(1.0/(1.0+exp(contrast*(midpoint/
          (double) QuantumRange)))))+0.5));
        continue;
      }
    sigmoidal_map[i]=(MagickRealType) ScaleMapToQuantum((unsigned long)
      (MaxMap*(QuantumScale*midpoint-log((1.0-(1.0/(1.0+exp(midpoint/
      (double) QuantumRange*contrast))+((double) i/MaxMap)*((1.0/
      (1.0+exp(contrast*(midpoint/(double) QuantumRange-1.0))))-(1.0/
      (1.0+exp(midpoint/(double) QuantumRange*contrast))))))/
      (1.0/(1.0+exp(midpoint/(double) QuantumRange*contrast))+
      ((double) i/MaxMap)*((1.0/(1.0+exp(contrast*(midpoint/
      (double) QuantumRange-1.0))))-(1.0/(1.0+exp(midpoint/
      (double) QuantumRange*contrast))))))/contrast)));
  }
  if (image->storage_class == PseudoClass)
    {
      /*
        Sigmoidal-contrast enhance colormap.
      */
      for (i=0; i < (long) image->colors; i++)
      {
        if ((channel & RedChannel) != 0)
          image->colormap[i].red=(Quantum)
            (sigmoidal_map[ScaleQuantumToMap(image->colormap[i].red)]+0.5);
        if ((channel & GreenChannel) != 0)
          image->colormap[i].green=(Quantum)
            (sigmoidal_map[ScaleQuantumToMap(image->colormap[i].green)]+0.5);
        if ((channel & BlueChannel) != 0)
          image->colormap[i].blue=(Quantum)
            (sigmoidal_map[ScaleQuantumToMap(image->colormap[i].blue)]+0.5);
        if ((channel & OpacityChannel) != 0)
          image->colormap[i].opacity=(Quantum)
            (sigmoidal_map[ScaleQuantumToMap(image->colormap[i].opacity)]+0.5);
      }
    }
  /*
    Sigmoidal-contrast enhance image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        q->red=(Quantum) (sigmoidal_map[ScaleQuantumToMap(q->red)]+0.5);
      if ((channel & GreenChannel) != 0)
        q->green=(Quantum) (sigmoidal_map[ScaleQuantumToMap(q->green)]+0.5);
      if ((channel & BlueChannel) != 0)
        q->blue=(Quantum) (sigmoidal_map[ScaleQuantumToMap(q->blue)]+0.5);
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        q->opacity=(Quantum) (sigmoidal_map[ScaleQuantumToMap(q->opacity)]+0.5);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        indexes[x]=(IndexPacket)
          (sigmoidal_map[ScaleQuantumToMap(indexes[x])]+0.5);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SigmoidalContrastImageTag,y,
          image->rows,image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  sigmoidal_map=(MagickRealType *) RelinquishMagickMemory(sigmoidal_map);
  return(MagickTrue);
}
