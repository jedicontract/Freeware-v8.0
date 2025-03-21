/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%           IIIII  DDDD   EEEEE  N   N  TTTTT  IIIII  FFFFF  Y   Y            %
%             I    D   D  E      NN  N    T      I    F       Y Y             %
%             I    D   D  EEE    N N N    T      I    FFF      Y              %
%             I    D   D  E      N  NN    T      I    F        Y              %
%           IIIII  DDDD   EEEEE  N   N    T    IIIII  F        Y              %
%                                                                             %
%                                                                             %
%               Identify an Image Format and Characteristics.                 %
%                                                                             %
%                           Software Design                                   %
%                             John Cristy                                     %
%                            September 1994                                   %
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
%  Identify describes the format and characteristics of one or more image
%  files.  It will also report if an image is incomplete or corrupt.
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/annotate.h"
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/client.h"
#include "magick/coder.h"
#include "magick/color.h"
#include "magick/configure.h"
#include "magick/constitute.h"
#include "magick/decorate.h"
#include "magick/delegate.h"
#include "magick/draw.h"
#include "magick/effect.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/identify.h"
#include "magick/image.h"
#include "magick/image-private.h"
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
#include "magick/prepress.h"
#include "magick/profile.h"
#include "magick/quantize.h"
#include "magick/quantum.h"
#include "magick/random_.h"
#include "magick/resize.h"
#include "magick/resource_.h"
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
%   I d e n t i f y I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IdentifyImage() identifies an image by printing its attributes to the file.
%  Attributes include the image width, height, size, and others.
%
%  The format of the IdentifyImage method is:
%
%      MagickBooleanType IdentifyImage(Image *image,FILE *file,
%        const MagickBooleanType verbose)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o file: The file, typically stdout.
%
%    o verbose: A value other than zero prints more detailed information
%      about the image.
%
%
*/
MagickExport MagickBooleanType IdentifyImage(Image *image,FILE *file,
  const MagickBooleanType verbose)
{
#if (QuantumDepth == 64)
#define IdentifyFormat "    %s:\n      Min: %llu (%g)\n      Max: %llu (%g)\n" \
  "      Mean: %g (%g)\n      Standard deviation: %g (%g)\n"
#else
#define IdentifyFormat "    %s:\n      Min: %lu (%g)\n      Max: %lu (%g)\n" \
  "      Mean: %g (%g)\n      Standard deviation: %g (%g)\n"
#endif

  char
    color[MaxTextExtent],
    format[MaxTextExtent],
    key[MaxTextExtent];

  ChannelStatistics
    *channel_statistics;

  ColorspaceType
    colorspace;

  const ImageAttribute
    *attribute;

  const MagickInfo
    *magick_info;

  double
    elapsed_time,
    user_time;

  Image
    *p;

  long
    y;

  register long
    i,
    x;

  unsigned long
    scale;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (file == (FILE *) NULL)
    file=stdout;
  *format='\0';
  elapsed_time=GetElapsedTime(&image->timer);
  user_time=GetUserTime(&image->timer);
  GetTimerInfo(&image->timer);
  if ((verbose == MagickFalse) || (image->cache == (void *) NULL))
    {
      /*
        Display summary info about the image.
      */
      if (*image->magick_filename != '\0')
        if (LocaleCompare(image->magick_filename,image->filename) != 0)
          (void) fprintf(file,"%s=>",image->magick_filename);
       if ((GetPreviousImageInList(image) == (Image *) NULL) &&
           (GetNextImageInList(image) == (Image *) NULL) && (image->scene == 0))
        (void) fprintf(file,"%s ",image->filename);
      else
        (void) fprintf(file,"%s[%lu] ",image->filename,image->scene);
      (void) fprintf(file,"%s ",image->magick);
      if ((image->magick_columns != 0) || (image->magick_rows != 0))
        if ((image->magick_columns != image->columns) ||
            (image->magick_rows != image->rows))
          (void) fprintf(file,"%lux%lu=>",image->magick_columns,
            image->magick_rows);
      (void) fprintf(file,"%lux%lu ",image->columns,image->rows);
      if ((image->page.width != 0) || (image->page.height != 0) ||
          (image->page.x != 0) || (image->page.y != 0))
        (void) fprintf(file,"%lux%lu%+ld%+ld ",image->page.width,
          image->page.height,image->page.x,image->page.y);
      if (image->storage_class == DirectClass)
        {
          (void) fprintf(file,"DirectClass ");
          if (image->total_colors != 0)
            {
              FormatMagickSize(image->total_colors,format);
              (void) fprintf(file,"%s ",format);
            }
        }
      else
        if (image->total_colors <= image->colors)
          (void) fprintf(file,"PseudoClass %luc ",image->colors);
        else
          {
            (void) fprintf(file,"PseudoClass %lu=>%luc ",image->total_colors,
              image->colors);
          }
      if (image->error.mean_error_per_pixel != 0.0)
        (void) fprintf(file,"%ld/%f/%fdb ",
          (long) (image->error.mean_error_per_pixel+0.5),
          image->error.normalized_mean_error,
          image->error.normalized_maximum_error);
      if (GetBlobSize(image) != 0)
        {
          FormatMagickSize(GetBlobSize(image),format);
          (void) fprintf(file,"%s ",format);
        }
      if (elapsed_time > 0.02)
        (void) fprintf(file,"%0.3fu %ld:%02ld",user_time,
          (long) (elapsed_time/60.0+0.5),(long) ceil(fmod(elapsed_time,60.0)));
      (void) fprintf(file,"\n");
      (void) fflush(file);
      return(ferror(file) != 0 ? MagickFalse : MagickTrue);
    }
  /*
    Display verbose info about the image.
  */
  (void) SignatureImage(image);
  image->total_colors=GetNumberColors(image,(FILE *) NULL,&image->exception);
    (void) fprintf(file,"Image: %s\n",image->filename);
  if (*image->magick_filename != '\0')
    if (LocaleCompare(image->magick_filename,image->filename) != 0)
      (void) fprintf(file,"  Base filename: %s\n",image->magick_filename);
  magick_info=GetMagickInfo(image->magick,&image->exception);
  if ((magick_info == (const MagickInfo *) NULL) ||
      (*GetMagickDescription(magick_info) == '\0'))
    (void) fprintf(file,"  Format: %s\n",image->magick);
  else
    (void) fprintf(file,"  Format: %s (%s)\n",image->magick,
      GetMagickDescription(magick_info));
  (void) fprintf(file,"  Class: %s\n",
    MagickOptionToMnemonic(MagickClassOptions,(long) image->storage_class));
  (void) fprintf(file,"  Geometry: %lux%lu\n",image->columns,image->rows);
  if ((image->magick_columns != 0) || (image->magick_rows != 0))
    if ((image->magick_columns != image->columns) ||
        (image->magick_rows != image->rows))
      (void) fprintf(file,"  Base geometry: %lux%lu\n",image->magick_columns,
        image->magick_rows);
  (void) fprintf(file,"  Type: %s\n",MagickOptionToMnemonic(
    MagickImageOptions,(long) GetImageType(image,&image->exception)));
  (void) fprintf(file,"  Endianess: %s\n",
    MagickOptionToMnemonic(MagickEndianOptions,(long) image->endian));
  /*
    Detail channel depth and extrema.
  */
  colorspace=image->colorspace;
  if (IsGrayImage(image,&image->exception) != MagickFalse)
    colorspace=GRAYColorspace;
  (void) fprintf(file,"  Colorspace: %s\n",
    MagickOptionToMnemonic(MagickColorspaceOptions,(long) colorspace));
  channel_statistics=GetImageChannelStatistics(image,&image->exception);
  (void) fprintf(file,"  Channel depth:\n");
  switch (colorspace)
  {
    case RGBColorspace:
    default:
    {
      (void) fprintf(file,"    Red: %lu-bits\n",
        channel_statistics[RedChannel].depth);
      (void) fprintf(file,"    Green: %lu-bits\n",
        channel_statistics[GreenChannel].depth);
      (void) fprintf(file,"    Blue: %lu-bits\n",
        channel_statistics[BlueChannel].depth);
      if (image->matte != MagickFalse)
        (void) fprintf(file,"    Alpha: %lu-bits\n",
          channel_statistics[OpacityChannel].depth);
      break;
    }
    case CMYKColorspace:
    {
      (void) fprintf(file,"    Cyan: %lu-bits\n",
        channel_statistics[CyanChannel].depth);
      (void) fprintf(file,"    Magenta: %lu-bits\n",
        channel_statistics[MagentaChannel].depth);
      (void) fprintf(file,"    Yellow: %lu-bits\n",
        channel_statistics[YellowChannel].depth);
      (void) fprintf(file,"    Black: %lu-bits\n",
        channel_statistics[BlackChannel].depth);
      if (image->matte != MagickFalse)
        (void) fprintf(file,"    Alpha: %lu-bits\n",
          channel_statistics[OpacityChannel].depth);
      break;
    }
    case GRAYColorspace:
    {
      (void) fprintf(file,"    Gray: %lu-bits\n",
        channel_statistics[GrayChannel].depth);
      if (image->matte != MagickFalse)
        (void) fprintf(file,"    Alpha: %lu-bits\n",
          channel_statistics[OpacityChannel].depth);
      break;
    }
  }
  scale=QuantumRange/(QuantumRange >> (QuantumDepth-
    channel_statistics[AllChannels].depth));
  (void) fprintf(file,"  Channel statistics:\n");
  switch (colorspace)
  {
    case RGBColorspace:
    default:
    {
      (void) fprintf(file,IdentifyFormat,"Red",
        (channel_statistics[RedChannel].minima/scale),
        (double) channel_statistics[RedChannel].minima/(double) QuantumRange,
        (channel_statistics[RedChannel].maxima/scale),
        (double) channel_statistics[RedChannel].maxima/(double) QuantumRange,
        channel_statistics[RedChannel].mean/(double) scale,
        channel_statistics[RedChannel].mean/(double) QuantumRange,
        channel_statistics[RedChannel].standard_deviation/(double) scale,
        channel_statistics[RedChannel].standard_deviation/(double)
        QuantumRange);
      (void) fprintf(file,IdentifyFormat,"Green",
        (channel_statistics[GreenChannel].minima/scale),
        (double) channel_statistics[GreenChannel].minima/(double) QuantumRange,
        (channel_statistics[GreenChannel].maxima/scale),
        (double) channel_statistics[GreenChannel].maxima/(double) QuantumRange,
        channel_statistics[GreenChannel].mean/(double) scale,
        channel_statistics[GreenChannel].mean/(double) QuantumRange,
        channel_statistics[GreenChannel].standard_deviation/(double) scale,
        channel_statistics[GreenChannel].standard_deviation/(double)
        QuantumRange);
      (void) fprintf(file,IdentifyFormat,"Blue",
        (channel_statistics[BlueChannel].minima/scale),
        (double) channel_statistics[BlueChannel].minima/(double) QuantumRange,
        (channel_statistics[BlueChannel].maxima/scale),
        (double) channel_statistics[BlueChannel].maxima/(double) QuantumRange,
        channel_statistics[BlueChannel].mean/(double) scale,
        channel_statistics[BlueChannel].mean/(double) QuantumRange,
        channel_statistics[BlueChannel].standard_deviation/(double) scale,
        channel_statistics[BlueChannel].standard_deviation/(double)
        QuantumRange);
      break;
    }
    case CMYKColorspace:
    {
      (void) fprintf(file,IdentifyFormat,"Cyan",
        (channel_statistics[CyanChannel].minima/scale),
        (double) channel_statistics[CyanChannel].minima/(double) QuantumRange,
        (channel_statistics[CyanChannel].maxima/scale),
        (double) channel_statistics[CyanChannel].maxima/(double) QuantumRange,
        channel_statistics[CyanChannel].mean/(double) scale,
        channel_statistics[CyanChannel].mean/(double) QuantumRange,
        channel_statistics[CyanChannel].standard_deviation/(double) scale,
        channel_statistics[CyanChannel].standard_deviation/(double)
        QuantumRange);
      (void) fprintf(file,IdentifyFormat,"Magenta",
        (channel_statistics[MagentaChannel].minima/scale),
        (double) channel_statistics[MagentaChannel].minima/(double) QuantumRange,
        (channel_statistics[MagentaChannel].maxima/scale),
        (double) channel_statistics[MagentaChannel].maxima/(double) QuantumRange,
        channel_statistics[MagentaChannel].mean/(double) scale,
        channel_statistics[MagentaChannel].mean/(double) QuantumRange,
        channel_statistics[MagentaChannel].standard_deviation/(double) scale,
        channel_statistics[MagentaChannel].standard_deviation/(double)
        QuantumRange);
      (void) fprintf(file,IdentifyFormat,"Yellow",
        (channel_statistics[YellowChannel].minima/scale),
        (double) channel_statistics[YellowChannel].minima/(double) QuantumRange,
        (channel_statistics[YellowChannel].maxima/scale),
        (double) channel_statistics[YellowChannel].maxima/(double) QuantumRange,
        channel_statistics[YellowChannel].mean/(double) scale,
        channel_statistics[YellowChannel].mean/(double) QuantumRange,
        channel_statistics[YellowChannel].standard_deviation/(double) scale,
        channel_statistics[YellowChannel].standard_deviation/(double)
        QuantumRange);
      (void) fprintf(file,IdentifyFormat,"Black",
        (channel_statistics[BlackChannel].minima/scale),
        (double) channel_statistics[BlackChannel].minima/(double) QuantumRange,
        (channel_statistics[BlackChannel].maxima/scale),
        (double) channel_statistics[BlackChannel].maxima/(double) QuantumRange,
        channel_statistics[BlackChannel].mean/(double) scale,
        channel_statistics[BlackChannel].mean/(double) QuantumRange,
        channel_statistics[BlackChannel].standard_deviation/(double) scale,
        channel_statistics[BlackChannel].standard_deviation/(double)
        QuantumRange);
      break;
    }
    case GRAYColorspace:
    {
      (void) fprintf(file,IdentifyFormat,"Gray",
        (channel_statistics[GrayChannel].minima/scale),
        (double) channel_statistics[GrayChannel].minima/(double) QuantumRange,
        (channel_statistics[GrayChannel].maxima/scale),
        (double) channel_statistics[GrayChannel].maxima/(double) QuantumRange,
        channel_statistics[GrayChannel].mean/(double) scale,
        channel_statistics[GrayChannel].mean/(double) QuantumRange,
        channel_statistics[GrayChannel].standard_deviation/(double) scale,
        channel_statistics[GrayChannel].standard_deviation/(double)
        QuantumRange);
      break;
    }
  }
  if (image->matte != MagickFalse)
    (void) fprintf(file,IdentifyFormat,"Alpha",
      ((QuantumRange-channel_statistics[OpacityChannel].minima)/scale),
      (double) (QuantumRange-channel_statistics[OpacityChannel].minima)/(double)
      QuantumRange,((QuantumRange-channel_statistics[OpacityChannel].maxima)/
      scale),(double) (QuantumRange-channel_statistics[OpacityChannel].maxima)/
      (double) QuantumRange,((double) QuantumRange-
      channel_statistics[OpacityChannel].mean)/(double) scale,((double)
      QuantumRange-channel_statistics[OpacityChannel].mean)/(double)
      QuantumRange,channel_statistics[OpacityChannel].standard_deviation/
      (double) scale,channel_statistics[OpacityChannel].standard_deviation/
      (double) QuantumRange);
  channel_statistics=(ChannelStatistics *)
    RelinquishMagickMemory(channel_statistics);
  if (colorspace == CMYKColorspace)
    (void) fprintf(file,"  Total ink density: %.0f%%\n",100.0*
      GetImageTotalInkDensity(image)/(double) QuantumRange);
  x=0;
  p=NewImageList();
  if (image->matte != MagickFalse)
    {
      register const PixelPacket
        *p;

      p=(PixelPacket *) NULL;
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
        if (p == (const PixelPacket *) NULL)
          break;
        for (x=0; x < (long) image->columns; x++)
        {
          if (p->opacity == TransparentOpacity)
            break;
          p++;
        }
        if (x < (long) image->columns)
          break;
      }
      if ((x < (long) image->columns) || (y < (long) image->rows))
        {
          char
            tuple[MaxTextExtent];

          MagickPixelPacket
            pixel;

          GetMagickPixelPacket(image,&pixel);
          pixel.colorspace=RGBColorspace;
          pixel.red=(MagickRealType) p->red;
          pixel.green=(MagickRealType) p->green;
          pixel.blue=(MagickRealType) p->blue;
          pixel.opacity=(MagickRealType) p->opacity;
          GetColorTuple(&pixel,MagickFalse,tuple);
          (void) fprintf(file,"  Alpha: %s\t",tuple);
          GetColorTuple(&pixel,MagickTrue,tuple);
          (void) fprintf(file,"  %s\n",tuple);
        }
    }
  if (image->storage_class == DirectClass)
    (void) fprintf(file,"  Colors: %lu\n",image->total_colors);
  else
    if (image->total_colors <= image->colors)
      (void) fprintf(file,"  Colors: %lu\n",image->colors);
    else
      (void) fprintf(file,"  Colors: %lu=>%lu\n",image->total_colors,
        image->colors);
  if ((image->storage_class == PseudoClass) && (colorspace != GRAYColorspace))
    {
      char
        name[MaxTextExtent];

      register PixelPacket
        *p;

      /*
        Display image colormap.
      */
      *name='\0';
      p=image->colormap;
      for (i=0; i < (long) image->colors; i++)
      {
        char
          tuple[MaxTextExtent];

        MagickPixelPacket
          pixel;

        GetMagickPixelPacket(image,&pixel);
        pixel.colorspace=RGBColorspace;
        pixel.red=(MagickRealType) p->red;
        pixel.green=(MagickRealType) p->green;
        pixel.blue=(MagickRealType) p->blue;
        pixel.opacity=(MagickRealType) p->opacity;
        GetColorTuple(&pixel,MagickFalse,tuple);
        (void) fprintf(file,"  %8ld: %s\t",i,tuple);
        (void) QueryColorname(image,p,SVGCompliance,name,&image->exception);
        (void) fprintf(file,"%s\n",name);
        p++;
      }
    }
  if (image->total_colors < 1024)
    {
      (void) fprintf(file,"  Histogram:\n");
      (void) GetNumberColors(image,file,&image->exception);
    }
  if (image->error.mean_error_per_pixel != 0.0)
    (void) fprintf(file,"  Mean error per pixel: %g\n",
      image->error.mean_error_per_pixel);
  if (image->error.normalized_mean_error != 0.0)
    (void) fprintf(file,"  Normalized mean error: %g\n",
      image->error.normalized_mean_error);
  if (image->error.normalized_maximum_error != 0.0)
    (void) fprintf(file,"  Normalized maximum error: %gn",
      image->error.normalized_maximum_error);
  (void) fprintf(file,"  Rendering intent: %s\n",
    MagickOptionToMnemonic(MagickIntentOptions,(long) image->rendering_intent));
  if (image->gamma != 0.0)
    (void) fprintf(file,"  Gamma: %g\n",image->gamma);
  if ((image->chromaticity.red_primary.x != 0.0) ||
      (image->chromaticity.green_primary.x != 0.0) ||
      (image->chromaticity.blue_primary.x != 0.0) ||
      (image->chromaticity.white_point.x != 0.0))
    {
      /*
        Display image chromaticity.
      */
      (void) fprintf(file,"  Chromaticity:\n");
      (void) fprintf(file,"    red primary: (%g,%g)\n",
        image->chromaticity.red_primary.x,image->chromaticity.red_primary.y);
      (void) fprintf(file,"    green primary: (%g,%g)\n",
        image->chromaticity.green_primary.x,
        image->chromaticity.green_primary.y);
      (void) fprintf(file,"    blue primary: (%g,%g)\n",
        image->chromaticity.blue_primary.x,image->chromaticity.blue_primary.y);
      (void) fprintf(file,"    white point: (%g,%g)\n",
        image->chromaticity.white_point.x,image->chromaticity.white_point.y);
    }
  if ((image->extract_info.width*image->extract_info.height) != 0)
    (void) fprintf(file,"  Tile geometry: %lux%lu%+ld%+ld\n",
      image->extract_info.width,image->extract_info.height,
      image->extract_info.x,image->extract_info.y);
  if ((image->x_resolution != 0.0) && (image->y_resolution != 0.0))
    (void) fprintf(file,"  Resolution: %gx%g\n",image->x_resolution,
      image->y_resolution);
  (void) fprintf(file,"  Units: %s\n",
    MagickOptionToMnemonic(MagickResolutionOptions,(long) image->units));
  FormatMagickSize(GetBlobSize(image),format);
  (void) fprintf(file,"  Filesize: %s\n",format);
  (void) fprintf(file,"  Interlace: %s\n",
    MagickOptionToMnemonic(MagickInterlaceOptions,(long) image->interlace));
  (void) QueryColorname(image,&image->background_color,SVGCompliance,color,
    &image->exception);
  (void) fprintf(file,"  Background color: %s\n",color);
  (void) QueryColorname(image,&image->border_color,SVGCompliance,color,
    &image->exception);
  (void) fprintf(file,"  Border color: %s\n",color);
  (void) QueryColorname(image,&image->matte_color,SVGCompliance,color,
    &image->exception);
  (void) fprintf(file,"  Matte color: %s\n",color);
  if ((image->page.width != 0) || (image->page.height != 0) ||
      (image->page.x != 0) || (image->page.y != 0))
    (void) fprintf(file,"  Page geometry: %lux%lu%+ld%+ld\n",image->page.width,
      image->page.height,image->page.x,image->page.y);
  (void) fprintf(file,"  Dispose: %s\n",
    MagickOptionToMnemonic(MagickDisposeOptions,(long) image->dispose));
  if (image->delay != 0)
    (void) fprintf(file,"  Delay: %lux%lu\n",image->delay,
      image->ticks_per_second);
  if (image->iterations != 1)
    (void) fprintf(file,"  Iterations: %lu\n",image->iterations);
  p=image;
  while (p->previous != (Image *) NULL)
    p=p->previous;
  for (i=1; GetNextImageInList(p) != (Image *) NULL; i++)
    p=GetNextImageInList(p);
  if (i > 1)
    (void) fprintf(file,"  Scene: %lu of %ld\n",image->scene,i);
  else
    if (image->scene != 0)
      (void) fprintf(file,"  Scene: %lu\n",image->scene);
  (void) fprintf(file,"  Compression: %s\n",
    MagickOptionToMnemonic(MagickCompressionOptions,(long) image->compression));
  if (image->quality != UndefinedCompressionQuality)
    (void) fprintf(file,"  Quality: %lu\n",image->quality);
  (void) fprintf(file,"  Orientation: %s\n",
    MagickOptionToMnemonic(MagickOrientationOptions,(long) image->orientation));
  if (image->montage != (char *) NULL)
    (void) fprintf(file,"  Montage: %s\n",image->montage);
  if (image->directory != (char *) NULL)
    {
      Image
        *tile;

      ImageInfo
        *image_info;

      register char
        *p,
        *q;

      WarningHandler
        handler;

      /*
        Display visual image directory.
      */
      image_info=CloneImageInfo((ImageInfo *) NULL);
      (void) CloneString(&image_info->size,"64x64");
      (void) fprintf(file,"  Directory:\n");
      for (p=image->directory; *p != '\0'; p++)
      {
        q=p;
        while ((*q != '\n') && (*q != '\0'))
          q++;
        (void) CopyMagickString(image_info->filename,p,(size_t) (q-p+1));
        p=q;
        (void) fprintf(file,"    %s",image_info->filename);
        handler=SetWarningHandler((WarningHandler) NULL);
        tile=ReadImage(image_info,&image->exception);
        (void) SetWarningHandler(handler);
        if (tile == (Image *) NULL)
          {
            (void) fprintf(file,"\n");
            continue;
          }
        (void) fprintf(file," %lux%lu %s\n",tile->magick_columns,
          tile->magick_rows,tile->magick);
        (void) SignatureImage(tile);
        ResetImageAttributeIterator(tile);
        attribute=GetNextImageAttribute(tile);
        while (attribute != (const ImageAttribute *) NULL)
        {
          if (*attribute->key == '[')
            continue;
          (void) fprintf(file,"  %s:\n",attribute->key);
          (void) fprintf(file,"%s\n",attribute->value);
          attribute=GetNextImageAttribute(tile);
        }
        tile=DestroyImage(tile);
      }
      image_info=DestroyImageInfo(image_info);
    }
  /*
    Display image attributes.
  */
  ResetImageAttributeIterator(image);
  attribute=GetNextImageAttribute(image);
  while (attribute != (const ImageAttribute *) NULL)
  {
    if (*attribute->key != '[')
      {
        (void) fprintf(file,"  %c",toupper(*attribute->key));
        if (strlen(attribute->key) > 1)
          (void) fprintf(file,"%s: ",attribute->key+1);
        if (strlen(attribute->value) > 80)
          (void) fputc('\n',file);
        (void) fprintf(file,"%s\n",attribute->value);
      }
    attribute=GetNextImageAttribute(image);
  }
  (void) FormatMagickString(key,MaxTextExtent,"8BIM:1999,2998:#1");
  attribute=GetImageAttribute(image,key);
  if (attribute != (const ImageAttribute *) NULL)
    {
      (void) fprintf(file,"  Clipping path: ");
      if (strlen(attribute->value) > 80)
        (void) fputc('\n',file);
      (void) fprintf(file,"%s\n",attribute->value);
    }
  if (image->profiles != (void *) NULL)
    {
      const char
        *name;

      const StringInfo
        *profile;

      /*
        Identify image profiles.
      */
      ResetImageProfileIterator(image);
      for (name=GetNextImageProfile(image); name != (char *) NULL; )
      {
        profile=GetImageProfile(image,name);
        if (profile == (StringInfo *) NULL)
          continue;
        (void) fprintf(file,"  Profile-%s: %lu bytes\n",name,
          (unsigned long) profile->length);
        PrintStringInfo(file,name,profile);
        if (LocaleCompare(name,"exif") == 0)
          {
            attribute=GetImageAttribute(image,"EXIF:*");
            if (attribute != (const ImageAttribute *) NULL)
              {
                char
                  **values;

                register char
                  *p;

                values=StringToList(attribute->value);
                if (values != (char **) NULL)
                  {
                    for (x=0; values[x] != (char *) NULL; x++)
                    {
                      (void) fprintf(file,"    ");
                      for (p=values[x]; *p != '\0'; p++)
                      {
                        if ((isupper((int) ((unsigned char) *p)) != 0) &&
                            (islower((int) ((unsigned char) *(p+1))) != 0))
                          if (p > values[x])
                            (void) fprintf(file," ");
                        if (*p == '=')
                          {
                            (void) fprintf(file,": ");
                            for (p++; *p != '\0'; p++)
                              (void) fputc(*p,file);
                            break;
                          }
                        (void) fputc(*p,file);
                      }
                      (void) fputc('\n',file);
                      values[x]=(char *) RelinquishMagickMemory(values[x]);
                    }
                    values=(char **) RelinquishMagickMemory(values);
                  }
              }
          }
        if (LocaleCompare(name,"iptc") == 0)
          {
            char
              *attribute,
              **attribute_list;

            const char
              *tag;

            long
              dataset,
              record,
              sentinel;

            register long
              j;

            size_t
              length;

            /*
              Identify IPTC data.
            */
            for (i=0; i < (long) profile->length; i+=(long) length)
            {
              length=1;
              sentinel= profile->datum[i++];
              if (sentinel != 0x1c)
                continue;
              dataset=profile->datum[i++];
              record=profile->datum[i++];
              switch (record)
              {
                case 5: tag="Image Name"; break;
                case 7: tag="Edit Status"; break;
                case 10: tag="Priority"; break;
                case 15: tag="Category"; break;
                case 20: tag="Supplemental Category"; break;
                case 22: tag="Fixture Identifier"; break;
                case 25: tag="Keyword"; break;
                case 30: tag="Release Date"; break;
                case 35: tag="Release Time"; break;
                case 40: tag="Special Instructions"; break;
                case 45: tag="Reference Service"; break;
                case 47: tag="Reference Date"; break;
                case 50: tag="Reference Number"; break;
                case 55: tag="Created Date"; break;
                case 60: tag="Created Time"; break;
                case 65: tag="Originating Program"; break;
                case 70: tag="Program Version"; break;
                case 75: tag="Object Cycle"; break;
                case 80: tag="Byline"; break;
                case 85: tag="Byline Title"; break;
                case 90: tag="City"; break;
                case 95: tag="Province State"; break;
                case 100: tag="Country Code"; break;
                case 101: tag="Country"; break;
                case 103: tag="Original Transmission Reference"; break;
                case 105: tag="Headline"; break;
                case 110: tag="Credit"; break;
                case 115: tag="Src"; break;
                case 116: tag="Copyright String"; break;
                case 120: tag="Caption"; break;
                case 121: tag="Local Caption"; break;
                case 122: tag="Caption Writer"; break;
                case 200: tag="Custom Field 1"; break;
                case 201: tag="Custom Field 2"; break;
                case 202: tag="Custom Field 3"; break;
                case 203: tag="Custom Field 4"; break;
                case 204: tag="Custom Field 5"; break;
                case 205: tag="Custom Field 6"; break;
                case 206: tag="Custom Field 7"; break;
                case 207: tag="Custom Field 8"; break;
                case 208: tag="Custom Field 9"; break;
                case 209: tag="Custom Field 10"; break;
                case 210: tag="Custom Field 11"; break;
                case 211: tag="Custom Field 12"; break;
                case 212: tag="Custom Field 13"; break;
                case 213: tag="Custom Field 14"; break;
                case 214: tag="Custom Field 15"; break;
                case 215: tag="Custom Field 16"; break;
                case 216: tag="Custom Field 17"; break;
                case 217: tag="Custom Field 18"; break;
                case 218: tag="Custom Field 19"; break;
                case 219: tag="Custom Field 20"; break;
                default: tag="unknown"; break;
              }
              (void) fprintf(file,"    %s[%ld,%ld]: ",tag,dataset,record);
              length=(size_t) (profile->datum[i++] << 8);
              length|=profile->datum[i++];
              attribute=(char *) AcquireMagickMemory((length+MaxTextExtent)*
                sizeof(*attribute));
              if (attribute != (char *) NULL)
                {
                  (void) CopyMagickString(attribute,(char *) profile->datum+i,
                    length+1);
                  attribute_list=StringToList(attribute);
                  if (attribute_list != (char **) NULL)
                    {
                      for (j=0; attribute_list[j] != (char *) NULL; j++)
                      {
                        (void) fputs(attribute_list[j],file);
                        (void) fputs("\n",file);
                        attribute_list[j]=(char *)
                          RelinquishMagickMemory(attribute_list[j]);
                      }
                      attribute_list=(char **) RelinquishMagickMemory(
                        attribute_list);
                    }
                  attribute=(char *) RelinquishMagickMemory(attribute);
                }
            }
          }
        name=GetNextImageProfile(image);
      }
    }
  if (image->taint != MagickFalse)
    (void) fprintf(file,"  Tainted: True\n");
  else
    (void) fprintf(file,"  Tainted: False\n");
  if (elapsed_time > 0.02)
    {
      (void) fprintf(file,"  User time: %0.3fu\n",user_time);
      (void) fprintf(file,"  Elapsed time: %ld:%02ld\n",
      (long) (elapsed_time/60.0+0.5),(long) ceil(fmod(elapsed_time,60.0)));
      FormatMagickSize((MagickSizeType) ((double) image->columns*image->rows/
        elapsed_time),format);
      (void) fprintf(file,"  Pixels per second: %s\n",format);
    }
  (void) fprintf(file,"  Version: %s\n",
    GetMagickVersion((unsigned long *) NULL));
  (void) fflush(file);
  return(ferror(file) != 0 ? MagickFalse : MagickTrue);
}
