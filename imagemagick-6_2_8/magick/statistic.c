/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        SSSSS  TTTTT   AAA   TTTTT  IIIII  SSSSS  TTTTT  IIIII   CCCC        %
%        SS       T    A   A    T      I    SS       T      I    C            %
%         SSS     T    AAAAA    T      I     SSS     T      I    C            %
%           SS    T    A   A    T      I       SS    T      I    C            %
%        SSSSS    T    A   A    T    IIIII  SSSSS    T    IIIII   CCCC        %
%                                                                             %
%                                                                             %
%                          ImageMagick Image Methods                          %
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
#include "magick/attribute.h"
#include "magick/animate.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/cache.h"
#include "magick/cache-private.h"
#include "magick/client.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/colorspace-private.h"
#include "magick/composite.h"
#include "magick/composite-private.h"
#include "magick/compress.h"
#include "magick/constitute.h"
#include "magick/deprecate.h"
#include "magick/display.h"
#include "magick/draw.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/list.h"
#include "magick/image-private.h"
#include "magick/magic.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/module.h"
#include "magick/monitor.h"
#include "magick/option.h"
#include "magick/paint.h"
#include "magick/pixel-private.h"
#include "magick/profile.h"
#include "magick/quantize.h"
#include "magick/random_.h"
#include "magick/segment.h"
#include "magick/semaphore.h"
#include "magick/signature.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/timer.h"
#include "magick/utility.h"
#include "magick/version.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t I m a g e B o u n d i n g B o x                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageBoundingBox() returns the bounding box of an image canvas.
%
%  The format of the GetImageBoundingBox method is:
%
%      RectangleInfo GetImageBoundingBox(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o bounds: Method GetImageBoundingBox returns the bounding box of an
%      image canvas.
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport RectangleInfo GetImageBoundingBox(const Image *image,
  ExceptionInfo *exception)
{
  long
    y;

  MagickPixelPacket
    target[3],
    pixel;

  RectangleInfo
    bounds;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    x;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  bounds.width=0;
  bounds.height=0;
  bounds.x=(long) image->columns;
  bounds.y=(long) image->rows;
  GetMagickPixelPacket(image,&target[0]);
  p=AcquireImagePixels(image,0,0,1,1,exception);
  if (p == (const PixelPacket *) NULL)
    return(bounds);
  SetMagickPixelPacket(p,GetIndexes(image),&target[0]);
  GetMagickPixelPacket(image,&target[1]);
  p=AcquireImagePixels(image,(long) image->columns-1,0,1,1,exception);
  SetMagickPixelPacket(p,GetIndexes(image),&target[1]);
  GetMagickPixelPacket(image,&target[2]);
  p=AcquireImagePixels(image,0,(long) image->rows-1,1,1,exception);
  SetMagickPixelPacket(p,GetIndexes(image),&target[2]);
  GetMagickPixelPacket(image,&pixel);
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    if (p == (const PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      SetMagickPixelPacket(p,indexes+x,&pixel);
      if ((x < bounds.x) &&
          (IsMagickColorSimilar(&pixel,&target[0]) == MagickFalse))
        bounds.x=x;
      if ((x > (long) bounds.width) &&
          (IsMagickColorSimilar(&pixel,&target[1]) == MagickFalse))
        bounds.width=(unsigned long) x;
      if ((y < bounds.y) &&
          (IsMagickColorSimilar(&pixel,&target[0]) == MagickFalse))
        bounds.y=y;
      if ((y > (long) bounds.height) &&
          (IsMagickColorSimilar(&pixel,&target[2]) == MagickFalse))
        bounds.height=(unsigned long) y;
      p++;
    }
  }
  if ((bounds.width != 0) || (bounds.height != 0))
    {
      bounds.width-=(bounds.x-1);
      bounds.height-=(bounds.y-1);
    }
  return(bounds);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e C h a n n e l D e p t h                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageChannelDepth() returns the depth of a particular image channel.
%
%  The format of the GetImageChannelDepth method is:
%
%      unsigned long GetImageChannelDepth(const Image *image,
%        const ChannelType channel,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

MagickExport unsigned long GetImageDepth(const Image *image,
  ExceptionInfo *exception)
{
  return(GetImageChannelDepth(image,AllChannels,exception));
}

MagickExport unsigned long GetImageChannelDepth(const Image *image,
  const ChannelType channel,ExceptionInfo *exception)
{
  long
    y;

  MagickStatusType
    status;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    x;

  unsigned long
    depth,
    scale;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  depth=1;
  scale=QuantumRange/(QuantumRange >> (QuantumDepth-depth));
  if ((image->storage_class == PseudoClass) && (image->matte == MagickFalse))
    {
      p=image->colormap;
      for (x=0; x < (long) image->colors; )
      {
        status=MagickFalse;
        if ((channel & RedChannel) != 0)
          status|=((unsigned long) p->red != (scale*(p->red/scale)));
        if ((channel & GreenChannel) != 0)
          status|=((unsigned long) p->green != (scale*(p->green/scale)));
        if ((channel & BlueChannel) != 0)
          status|=((unsigned long) p->blue != (scale*(p->blue/scale)));
        if (status != MagickFalse)
          {
            depth++;
            if (depth == QuantumDepth)
              return(depth);
            scale=QuantumRange/(QuantumRange >> (QuantumDepth-depth));
            continue;
          }
        x++;
        p++;
      }
    }
  else
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) image->columns; )
      {
        status=MagickFalse;
        if ((channel & RedChannel) != 0)
          status|=((unsigned long) p->red != (scale*(p->red/scale)));
        if ((channel & GreenChannel) != 0)
          status|=((unsigned long) p->green != (scale*(p->green/scale)));
        if ((channel & BlueChannel) != 0)
          status|=((unsigned long) p->blue != (scale*(p->blue/scale)));
        if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
          status|=((unsigned long) p->opacity != (scale*(p->opacity/scale)));
        if (((channel & IndexChannel) != 0) &&
            (image->colorspace == CMYKColorspace))
          status|=((unsigned long) indexes[x] != (scale*(indexes[x]/scale)));
        if (status != MagickFalse)
          {
            depth++;
            if (depth == QuantumDepth)
              return(depth);
            scale=QuantumRange/(QuantumRange >> (QuantumDepth-depth));
            continue;
          }
        x++;
        p++;
      }
    }
  return(depth);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e C h a n n e l E x t r e m a                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageChannelExtrema() returns the extrema of one or more image channels.
%
%  The format of the GetImageChannelExtrema method is:
%
%      MagickBooleanType GetImageChannelExtrema(const Image *image,
%        const ChannelType channel,unsigned long *minima,unsigned long *maxima,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o minima: The minimum value in the channel.
%
%    o maxima: The maximum value in the channel.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

MagickExport MagickBooleanType GetImageExtrema(const Image *image,
  unsigned long *minima,unsigned long *maxima,ExceptionInfo *exception)
{
  return(GetImageChannelExtrema(image,AllChannels,minima,maxima,exception));
}

MagickExport MagickBooleanType GetImageChannelExtrema(const Image *image,
  const ChannelType channel,unsigned long *minima,unsigned long *maxima,
  ExceptionInfo *exception)
{
  long
    y;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    x;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  *maxima=0;
  *minima=(unsigned long) QuantumRange;
  y=(long) image->rows;
  if ((image->storage_class == PseudoClass) && (image->matte == MagickFalse))
    {
      p=image->colormap;
      for (x=0; x < (long) image->colors; )
      {
        if ((channel & RedChannel) != 0)
          {
            if ((unsigned long) p->red < *minima)
              *minima=p->red;
            if ((unsigned long) p->red > *maxima)
              *maxima=p->red;
          }
        if ((channel & GreenChannel) != 0)
          {
            if ((unsigned long) p->green < *minima)
              *minima=p->green;
            if ((unsigned long) p->green > *maxima)
              *maxima=p->green;
          }
        if ((channel & BlueChannel) != 0)
          {
            if ((unsigned long) p->blue < *minima)
              *minima=p->blue;
            if ((unsigned long) p->blue > *maxima)
              *maxima=p->blue;
          }
        x++;
        p++;
      }
    }
  else
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) image->columns; )
      {
        if ((channel & RedChannel) != 0)
          {
            if ((unsigned long) p->red < *minima)
              *minima=p->red;
            if ((unsigned long) p->red > *maxima)
              *maxima=p->red;
          }
        if ((channel & GreenChannel) != 0)
          {
            if ((unsigned long) p->green < *minima)
              *minima=p->green;
            if ((unsigned long) p->green > *maxima)
              *maxima=p->green;
          }
        if ((channel & BlueChannel) != 0)
          {
            if ((unsigned long) p->blue < *minima)
              *minima=p->blue;
            if ((unsigned long) p->blue > *maxima)
              *maxima=p->blue;
          }
        if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
          {
            if ((unsigned long) p->opacity < *minima)
              *minima=p->opacity;
            if ((unsigned long) p->opacity > *maxima)
              *maxima=p->opacity;
          }
        if (((channel & IndexChannel) != 0) &&
            (image->colorspace == CMYKColorspace))
          {
            if ((unsigned long) indexes[x] < *minima)
              *minima=indexes[x];
            if ((unsigned long) indexes[x] > *maxima)
              *maxima=indexes[x];
          }
        x++;
        p++;
      }
    }
  return(y == (long) image->rows ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e C h a n n e l M e a n                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageChannelMean() returns the mean and standard deviation of one or more
%  image channels.
%
%  The format of the GetImageChannelMean method is:
%
%      MagickBooleanType GetImageChannelMean(const Image *image,
%        const ChannelType channel,double *mean,double *standard_deviation,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o mean: The average value in the channel.
%
%    o standard_deviation: The standard deviation of the channel.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

MagickExport MagickBooleanType GetImageMean(const Image *image,double *mean,
  double *standard_deviation,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=GetImageChannelMean(image,AllChannels,mean,standard_deviation,
    exception);
  return(status);
}

MagickExport MagickBooleanType GetImageChannelMean(const Image *image,
  const ChannelType channel,double *mean,double *standard_deviation,
  ExceptionInfo *exception)
{
#define PixelSquared(x)  ((x)*(x))

  double
    area;

  long
    y;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    x;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  *mean=0.0;
  *standard_deviation=0.0;
  area=0.0;
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    if (p == (const PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        {
          *mean+=p->red;
          *standard_deviation+=(double) p->red*p->red;
          area++;
        }
      if ((channel & GreenChannel) != 0)
        {
          *mean+=p->green;
          *standard_deviation+=(double) p->green*p->green;
          area++;
        }
      if ((channel & BlueChannel) != 0)
        {
          *mean+=p->blue;
          *standard_deviation+=(double) p->blue*p->blue;
          area++;
        }
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        {
          *mean+=p->opacity;
          *standard_deviation+=(double) p->opacity*p->opacity;
          area++;
        }
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        {
          *mean+=indexes[x];
          *standard_deviation+=(double) indexes[x]*indexes[x];
          area++;
        }
      p++;
    }
  }
  if (y < (long) image->rows)
    return(MagickFalse);
  if (area != 0)
    {
      *mean/=area;
      *standard_deviation/=area;
    }
  *standard_deviation=sqrt(*standard_deviation-(*mean*(*mean)));
  return(y == (long) image->rows ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e C h a n n e l S t a t i s t i c s                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageChannelStatistics() returns statistics for each channel in the
%  image.  The statistics incude the channel depth, its minima and
%  maxima, the mean, and the standard deviation.  You can access the red
%  channel mean, for example, like this:
%
%      channel_statistics=GetImageChannelStatistics(image,excepton);
%      red_mean=channel_statistics[RedChannel].mean;
%
%  Use MagickRelinquishMemory() to free the statistics buffer.
%
%  The format of the GetImageChannelStatistics method is:
%
%      ChannelStatistics *GetImageChannelStatistics(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport ChannelStatistics *GetImageChannelStatistics(const Image *image,
  ExceptionInfo *exception)
{
  ChannelStatistics
    *channel_statistics;

  double
    area;

  long
    y;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  size_t
    length;

  unsigned long
    channels,
    scale;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  length=(AllChannels+1)*sizeof(*channel_statistics);
  channel_statistics=(ChannelStatistics *) AcquireMagickMemory(length);
  if (channel_statistics == (ChannelStatistics *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      image->filename);
  (void) ResetMagickMemory(channel_statistics,0,length);
  for (i=0; i <= AllChannels; i++)
  {
    channel_statistics[i].depth=1;
    channel_statistics[i].scale=(unsigned long)
      QuantumRange/(QuantumRange >> (QuantumDepth-1));
    channel_statistics[i].maxima=0;
    channel_statistics[i].minima=QuantumRange;
    channel_statistics[i].mean=0.0;
    channel_statistics[i].standard_deviation=0.0;
  }
  y=(long) image->rows;
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    if (p == (const PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; )
    {
      if (channel_statistics[RedChannel].depth != QuantumDepth)
        {
          scale=channel_statistics[RedChannel].scale;
          if ((unsigned long) p->red != (scale*(p->red/scale)))
            {
              channel_statistics[RedChannel].depth++;
              channel_statistics[RedChannel].scale=QuantumRange/
                (QuantumRange >> (QuantumDepth-
                channel_statistics[RedChannel].depth));
              continue;
            }
        }
      if (channel_statistics[GreenChannel].depth != QuantumDepth)
        {
          scale=channel_statistics[GreenChannel].scale;
          if ((unsigned long) p->green != (scale*(p->green/scale)))
            {
              channel_statistics[GreenChannel].depth++;
              channel_statistics[GreenChannel].scale=QuantumRange/
                (QuantumRange >> (QuantumDepth-
                channel_statistics[GreenChannel].depth));
              continue;
            }
        }
      if (channel_statistics[BlueChannel].depth != QuantumDepth)
        {
          scale=channel_statistics[BlueChannel].scale;
          if ((unsigned long) p->blue != (scale*(p->blue/scale)))
            {
              channel_statistics[BlueChannel].depth++;
              channel_statistics[BlueChannel].scale=QuantumRange/
                (QuantumRange >> (QuantumDepth-
                channel_statistics[BlueChannel].depth));
              continue;
            }
        }
      if (image->matte != MagickFalse)
        {
          if (channel_statistics[OpacityChannel].depth != QuantumDepth)
            {
              scale=channel_statistics[OpacityChannel].scale;
              if ((unsigned long) p->opacity != (scale*(p->opacity/scale)))
                {
                  channel_statistics[OpacityChannel].depth++;
                  channel_statistics[OpacityChannel].scale=QuantumRange/
                    (QuantumRange >> (QuantumDepth-
                    channel_statistics[OpacityChannel].depth));
                  continue;
                }
            }
        }
      if (image->colorspace == CMYKColorspace)
        {
          if (channel_statistics[BlackChannel].depth != QuantumDepth)
            {
              scale=channel_statistics[BlackChannel].scale;
              if ((unsigned long) indexes[x] != (scale*(indexes[x]/scale)))
                {
                  channel_statistics[BlackChannel].depth++;
                  channel_statistics[BlackChannel].scale=QuantumRange/
                    (QuantumRange >> (QuantumDepth-
                    channel_statistics[BlackChannel].depth));
                  continue;
                }
            }
        }
      if (p->red < channel_statistics[RedChannel].minima)
        channel_statistics[RedChannel].minima=p->red;
      if (p->red > channel_statistics[RedChannel].maxima)
        channel_statistics[RedChannel].maxima=p->red;
      channel_statistics[RedChannel].mean+=p->red;
      channel_statistics[RedChannel].standard_deviation+=
        (double) p->red*p->red;
      if (p->green < channel_statistics[GreenChannel].minima)
        channel_statistics[GreenChannel].minima=p->green;
      if (p->green > channel_statistics[GreenChannel].maxima)
        channel_statistics[GreenChannel].maxima=p->green;
      channel_statistics[GreenChannel].mean+=p->green;
      channel_statistics[GreenChannel].standard_deviation+=
        (double) p->green*p->green;
      if (p->blue < channel_statistics[BlueChannel].minima)
        channel_statistics[BlueChannel].minima=p->blue;
      if (p->blue > channel_statistics[BlueChannel].maxima)
        channel_statistics[BlueChannel].maxima=p->blue;
      channel_statistics[BlueChannel].mean+=p->blue;
      channel_statistics[BlueChannel].standard_deviation+=
        (double) p->blue*p->blue;
      if (image->matte != MagickFalse)
        {
          if (p->opacity < channel_statistics[OpacityChannel].minima)
            channel_statistics[OpacityChannel].minima=p->opacity;
          if (p->opacity > channel_statistics[OpacityChannel].maxima)
            channel_statistics[OpacityChannel].maxima=p->opacity;
          channel_statistics[OpacityChannel].mean+=p->opacity;
          channel_statistics[OpacityChannel].standard_deviation+=
            (double) p->opacity*p->opacity;
        }
      if (image->colorspace == CMYKColorspace)
        {
          if (indexes[x] < channel_statistics[BlackChannel].minima)
            channel_statistics[BlackChannel].minima=indexes[x];
          if (indexes[x] > channel_statistics[BlackChannel].maxima)
            channel_statistics[BlackChannel].maxima=indexes[x];
          channel_statistics[BlackChannel].mean+=indexes[x];
          channel_statistics[BlackChannel].standard_deviation+=
            (double) indexes[x]*indexes[x];
        }
      x++;
      p++;
    }
  }
  area=(double) image->columns*image->rows;
  for (i=0; i < AllChannels; i++)
  {
    channel_statistics[i].mean/=area;
    channel_statistics[i].standard_deviation/=area;
  }
  for (i=0; i < AllChannels; i++)
  {
    channel_statistics[AllChannels].depth=Max(
      channel_statistics[AllChannels].depth,channel_statistics[i].depth);
    channel_statistics[AllChannels].minima=Min(
      channel_statistics[AllChannels].minima,channel_statistics[i].minima);
    channel_statistics[AllChannels].maxima=Max(
      channel_statistics[AllChannels].maxima,channel_statistics[i].maxima);
    channel_statistics[AllChannels].mean+=channel_statistics[i].mean;
    channel_statistics[AllChannels].standard_deviation+=
      channel_statistics[i].standard_deviation;
  }
  channels=3;
  if (image->matte != MagickFalse)
    channels++;
  if (image->colorspace == CMYKColorspace)
    channels++;
  channel_statistics[AllChannels].mean/=channels;
  channel_statistics[AllChannels].standard_deviation/=channels;
  for (i=0; i <= AllChannels; i++)
    channel_statistics[i].standard_deviation=sqrt(
      channel_statistics[i].standard_deviation-
       (channel_statistics[i].mean*channel_statistics[i].mean));
  return(channel_statistics);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e Q u a n t u m D e p t h                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageQuantumDepth() returns the depth of the image rounded to a legal
%  quantum depth: 8, 16, or 32.
%
%  The format of the GetImageQuantumDepth method is:
%
%      unsigned long GetImageQuantumDepth(const Image *image,
%        const MagickBooleanType constrain)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o constrain: A value other than MagickFalse, constrains the depth to
%      a maximum of QuantumDepth.
%
*/
MagickExport unsigned long GetImageQuantumDepth(const Image *image,
  const MagickBooleanType constrain)
{
  unsigned long
    depth;

  depth=image->depth;
  if (depth <= 8)
    depth=8;
  else
    if (depth <= 16)
      depth=16;
  if (constrain != MagickFalse)
    depth=Min(depth,QuantumDepth);
  return(depth);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e C h a n n e l D e p t h                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageChannelDepth() sets the depth of the image.
%
%  The format of the SetImageChannelDepth method is:
%
%      MagickBooleanType SetImageChannelDepth(Image *image,
%        const ChannelType channel,const unsigned long depth)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o depth: The image depth.
%
%
*/

MagickExport MagickBooleanType SetImageDepth(Image *image,
  const unsigned long depth)
{
  return(SetImageChannelDepth(image,AllChannels,depth));
}

MagickExport MagickBooleanType SetImageChannelDepth(Image *image,
  const ChannelType channel,const unsigned long depth)
{
  long
    y;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  unsigned long
    scale;

  assert(image != (Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  y=(long) image->rows;
  if (GetImageDepth(image,&image->exception) > Min(depth,QuantumDepth))
    {
      /*
        Scale pixels to desired depth.
      */
      scale=QuantumRange/(QuantumRange >>
       (QuantumDepth-Min(depth,QuantumDepth)));
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) image->columns; x++)
        {
          if ((channel & RedChannel) != 0)
            q->red=(Quantum) (scale*(q->red/scale));
          if ((channel & GreenChannel) != 0)
            q->green=(Quantum) (scale*(q->green/scale));
          if ((channel & BlueChannel) != 0)
            q->blue=(Quantum) (scale*(q->blue/scale));
          if (((channel & OpacityChannel) != 0) &&
              (image->matte != MagickFalse))
            q->opacity=(Quantum) (scale*(q->opacity/scale));
          if (((channel & IndexChannel) != 0) &&
              (image->colorspace == CMYKColorspace))
            indexes[x]=(Quantum) (scale*(indexes[x]/scale));
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      if (image->storage_class == PseudoClass)
        {
          register long
            i;

          q=image->colormap;
          for (i=0; i < (long) image->colors; i++)
          {
            if ((channel & RedChannel) != 0)
              q->red=(Quantum) (scale*(q->red/scale));
            if ((channel & GreenChannel) != 0)
              q->green=(Quantum) (scale*(q->green/scale));
            if ((channel & BlueChannel) != 0)
              q->blue=(Quantum) (scale*(q->blue/scale));
            if (((channel & OpacityChannel) != 0) &&
                (image->matte != MagickFalse))
              q->opacity=(Quantum) (scale*(q->opacity/scale));
            q++;
        }
      }
    }
  image->depth=Min(depth,QuantumDepth);
  return(y == (long) image->rows ? MagickTrue : MagickFalse);
}
