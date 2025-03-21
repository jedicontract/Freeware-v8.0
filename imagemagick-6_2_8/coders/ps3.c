/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            PPPP   SSSSS  33333                              %
%                            P   P  SS        33                              %
%                            PPPP    SSS    333                               %
%                            P         SS     33                              %
%                            P      SSSSS  33333                              %
%                                                                             %
%                                                                             %
%                     Write Postscript Level III Format.                      %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                              Lars Ruben Skyum                               %
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
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/compress.h"
#include "magick/constitute.h"
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/option.h"
#include "magick/resource_.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/utility.h"
#include "magick/version.h"
#if defined(HasTIFF)
#define CCITTParam  "-1"
#else
#define CCITTParam  "0"
#endif

/*
  Define declarations.
*/
#define PS3_NoCompression "0"
#define PS3_FaxCompression "1"
#define PS3_JPEGCompression "2"
#define PS3_LZWCompression "3"
#define PS3_RLECompression "4"
#define PS3_ZipCompression "5"

#define PS3_RGBColorspace "0"
#define PS3_CMYKColorspace "1"

#define PS3_DirectClass "0"
#define PS3_PseudoClass "1"

/*
  Forward declarations.
*/
static MagickBooleanType
  WritePS3Image(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r P S 3 I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterPS3Image() adds attributes for the PS3 image format to the list of
%  supported formats.  The attributes include the image format tag, a method to
%  read and/or write the format, whether the format supports the saving of more
%  than one frame to the same file or blob, whether the format supports native
%  in-memory I/O, and a brief description of the format.
%
%  The format of the RegisterPS3Image method is:
%
%      RegisterPS3Image(void)
%
*/
ModuleExport void RegisterPS3Image(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("EPS3");
  entry->encoder=(EncoderHandler *) WritePS3Image;
  entry->description=ConstantString("Level III Encapsulated PostScript");
  entry->module=ConstantString("PS3");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PS3");
  entry->encoder=(EncoderHandler *) WritePS3Image;
  entry->description=ConstantString("Level III PostScript");
  entry->module=ConstantString("PS3");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P S 3 I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterPS3Image() removes format registrations made by the PS3 module
%  from the list of supported formats.
%
%  The format of the UnregisterPS3Image method is:
%
%      UnregisterPS3Image(void)
%
*/
ModuleExport void UnregisterPS3Image(void)
{
  (void) UnregisterMagickInfo("EPS3");
  (void) UnregisterMagickInfo("PS3");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P S 3 I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WritePS3Image() translates an image to encapsulated Postscript Level III
%  for printing.  If the supplied geometry is null, the image is centered on
%  the Postscript page.  Otherwise, the image is positioned as specified by the
%  geometry.
%
%  The format of the WritePS3Image method is:
%
%      MagickBooleanType WritePS3Image(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o image: The image.
%
%
*/

static MagickBooleanType SerializeImage(const ImageInfo *image_info,
  Image *image,unsigned char **pixels,size_t *length)
{
  IndexPacket
    *indexes;

  long
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register long
    x;

  register unsigned char
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=MagickTrue;
  *length=(image->colorspace == CMYKColorspace ? 4 : 3)*
    (size_t) image->columns*image->rows;
  *pixels=(unsigned char *) AcquireMagickMemory(*length);
  if (*pixels == (unsigned char *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  q=(*pixels);
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    if (image->colorspace != CMYKColorspace)
      for (x=0; x < (long) image->columns; x++)
      {
        *q++=ScaleQuantumToChar(p->red);
        *q++=ScaleQuantumToChar(p->green);
        *q++=ScaleQuantumToChar(p->blue);
        p++;
      }
    else
      for (x=0; x < (long) image->columns; x++)
      {
        *q++=ScaleQuantumToChar(p->red);
        *q++=ScaleQuantumToChar(p->green);
        *q++=ScaleQuantumToChar(p->blue);
        *q++=ScaleQuantumToChar(indexes[x]);
        p++;
      }
    if (image->previous == (Image *) NULL)
      if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(y,image->rows) != MagickFalse))
        {
          status=image->progress_monitor(SaveImageTag,y,image->rows,
            image->client_data);
          if (status == MagickFalse)
            break;
        }
  }
  if (status == MagickFalse)
    *pixels=(unsigned char *) RelinquishMagickMemory(*pixels);
  return(status);
}

static MagickBooleanType SerializeImageChannel(const ImageInfo *image_info,
  Image *image,unsigned char **pixels,size_t *length)
{
  long
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register long
    x;

  register unsigned char
    *q;

  unsigned char
    code,
    bit;

  unsigned long
    pack,
    padded_columns;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=MagickTrue;
  pack=(unsigned long)
    (IsMonochromeImage(image,&image->exception) != MagickFalse ? 8 : 1);
  padded_columns=((image->columns+pack-1)/pack)*pack;
  *length=(size_t) padded_columns*image->rows/pack;
  *pixels=(unsigned char *) AcquireMagickMemory(*length);
  if (*pixels == (unsigned char *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  q=(*pixels);
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    if (pack == 1)
      for (x=0; x < (long) image->columns; x++)
      {
        *q++=ScaleQuantumToChar(PixelIntensityToQuantum(p));
        p++;
      }
    else
      {
        code='\0';
        for (x=0; x < (long) padded_columns; x++)
        {
          bit=(unsigned char) 0x00;
          if (x < (long) image->columns)
            bit=(unsigned char)
              (PixelIntensityToQuantum(p) == TransparentOpacity ? 0x01 : 0x00);
          code=(code << 1)+bit;
          if (((x+1) % pack) == 0)
            {
              *q++=code;
              code='\0';
            }
          p++;
        }
      }
    if (image->previous == (Image *) NULL)
      if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(y,image->rows) != MagickFalse))
        {
          status=image->progress_monitor(SaveImageTag,y,image->rows,
            image->client_data);
          if (status == MagickFalse)
            break;
        }
  }
  if (status == MagickFalse)
    *pixels=(unsigned char *) RelinquishMagickMemory(*pixels);
  return(status);
}

static MagickBooleanType SerializeImageIndexes(const ImageInfo *image_info,
  Image *image,unsigned char **pixels,size_t *length)
{
  long
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    x;

  register unsigned char
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=MagickTrue;
  *length=(size_t) image->columns*image->rows;
  *pixels=(unsigned char *) AcquireMagickMemory(*length);
  if (*pixels == (unsigned char *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  q=(*pixels);
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
      *q++=(unsigned char) indexes[x];
    if (image->previous == (Image *) NULL)
     if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
         (QuantumTick(y,image->rows) != MagickFalse))
       {
         status=image->progress_monitor(SaveImageTag,y,image->rows,
           image->client_data);
         if (status == MagickFalse)
           break;
       }
  }
  if (status == MagickFalse)
    *pixels=(unsigned char *) RelinquishMagickMemory(*pixels);
  return(status);
}

static MagickBooleanType WritePS3MaskImage(const ImageInfo *image_info,
  Image *image,const CompressionType compression)
{
  char
    buffer[MaxTextExtent];

  Image
    *mask_image;

  MagickBooleanType
    status;

  MagickOffsetType
    start,
    stop;

  register long
    i;

  size_t
    length;

  unsigned char
    *pixels;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->matte != MagickFalse);
  status=MagickTrue;
  /*
    Note BeginData DSC comment for update later.
  */
  start=TellBlob(image);
  (void) FormatMagickString(buffer,MaxTextExtent,
    "%%%%BeginData:%13ld %s Bytes\n",0L,
    compression == NoCompression ? "ASCII" : "BINARY");
  (void) WriteBlobString(image,buffer);
  stop=TellBlob(image);
  /*
    Only lossless compressions for the mask.
  */
  switch (compression)
  {
    case NoCompression:
    {
      (void) FormatMagickString(buffer,MaxTextExtent,
        "currentfile %lu %lu "PS3_NoCompression" ByteStreamDecodeFilter\n",
        image->columns,image->rows);
      break;
    }
    case FaxCompression:
    case Group4Compression:
    default:
    {
      (void) FormatMagickString(buffer,MaxTextExtent,
        "currentfile %lu %lu "PS3_FaxCompression" ByteStreamDecodeFilter\n",
        image->columns, image->rows);
      break;
    }
    case JPEGCompression:
    {
      (void) FormatMagickString(buffer,MaxTextExtent,
        "currentfile %lu %lu "PS3_JPEGCompression" ByteStreamDecodeFilter\n",
        image->columns,image->rows);
      break;
    }
    case LZWCompression:
    {
      (void) FormatMagickString(buffer,MaxTextExtent,
        "currentfile %lu %lu "PS3_LZWCompression" ByteStreamDecodeFilter\n",
        image->columns,image->rows);
      break;
    }
    case RLECompression:
    {
      (void) FormatMagickString(buffer,MaxTextExtent,
        "currentfile %lu %lu "PS3_RLECompression" ByteStreamDecodeFilter\n",
        image->columns,image->rows);
      break;
    }
    case ZipCompression:
    {
      (void) FormatMagickString(buffer,MaxTextExtent,
        "currentfile %lu %lu "PS3_ZipCompression" ByteStreamDecodeFilter\n",
        image->columns,image->rows);
      break;
    }
  }
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,"/ReusableStreamDecode filter\n");
  mask_image=CloneImage(image,0,0,MagickTrue,&image->exception);
  if (mask_image == (Image *) NULL)
    ThrowWriterException(CoderError,image->exception.reason);
  status=SeparateImageChannel(mask_image,OpacityChannel);
  if (status == MagickFalse)
    {
      mask_image=DestroyImage(mask_image);
      return(MagickFalse);
    }
  (void) SetImageType(mask_image,BilevelType);
  mask_image->matte=MagickFalse;
  pixels=(unsigned char *) NULL;
  length=0;
  switch (compression)
  {
    case NoCompression:
    {
      status=SerializeImageChannel(image_info,mask_image,&pixels,&length);
      if (status == MagickFalse)
        break;
      Ascii85Initialize(image);
      for (i=0; i < (long) length; i++)
        Ascii85Encode(image,pixels[i]);
      Ascii85Flush(image);
      pixels=(unsigned char *) RelinquishMagickMemory(pixels);
      break;
    }
    case FaxCompression:
    case Group4Compression:
    default:
    {
      if ((compression == FaxCompression) ||
          (LocaleCompare(CCITTParam,"0") == 0))
        status=HuffmanEncodeImage(image_info,mask_image);
      else
        status=Huffman2DEncodeImage(image_info,mask_image);
      break;
    }
    case JPEGCompression:
    {
      status=JPEGEncodeImage(image_info,mask_image);
      break;
    }
    case LZWCompression:
    {
      status=SerializeImageChannel(image_info,mask_image,&pixels,&length);
      if (status == MagickFalse)
        break;
      status=LZWEncodeImage(image,length,pixels);
      pixels=(unsigned char *) RelinquishMagickMemory(pixels);
      break;
    }
    case RLECompression:
    {
      status=SerializeImageChannel(image_info,mask_image,&pixels,&length);
      if (status == MagickFalse)
        break;
      status=PackbitsEncodeImage(image,length,pixels);
      pixels=(unsigned char *) RelinquishMagickMemory(pixels);
      break;
    }
    case ZipCompression:
    {
      status=SerializeImageChannel(image_info,mask_image,&pixels,&length);
      if (status == MagickFalse)
        break;
      status=ZLIBEncodeImage(image,length,pixels);
      pixels=(unsigned char *) RelinquishMagickMemory(pixels);
      break;
    }
  }
  mask_image=DestroyImage(mask_image);
  (void) WriteBlobByte(image,'\n');
  length=(size_t) (TellBlob(image)-stop);
  stop=TellBlob(image);
  (void) SeekBlob(image,start,SEEK_SET);
  (void) FormatMagickString(buffer,MaxTextExtent,
    "%%%%BeginData:%13ld %s Bytes\n",(long) length,
    compression == NoCompression ? "ASCII" : "BINARY");
  (void) WriteBlobString(image,buffer);
  (void) SeekBlob(image,stop,SEEK_SET);
  (void) WriteBlobString(image,"%%EndData\n");
  (void) WriteBlobString(image, "/mask_stream exch def\n");
  return(status);
}

static MagickBooleanType WritePS3Image(const ImageInfo *image_info,Image *image)
{
  static const char
    *PostscriptProlog[]=
    {
      "/ByteStreamDecodeFilter",
      "{",
      "  /z exch def",
      "  /r exch def",
      "  /c exch def",
      "  z "PS3_NoCompression" eq { /ASCII85Decode filter } if",
      "  z "PS3_FaxCompression" eq",
      "  {",
      "    <<",
      "      /K "CCITTParam,
      "      /Columns c",
      "      /Rows r",
      "    >>",
      "    /CCITTFaxDecode filter",
      "  } if",
      "  z "PS3_JPEGCompression" eq { /DCTDecode filter } if",
      "  z "PS3_LZWCompression" eq { /LZWDecode filter } if",
      "  z "PS3_RLECompression" eq { /RunLengthDecode filter } if",
      "  z "PS3_ZipCompression" eq { /FlateDecode filter } if",
      "} bind def",
      "",
      "/DirectClassImageDict",
      "{",
      "  colorspace "PS3_RGBColorspace" eq",
      "  {",
      "    /DeviceRGB setcolorspace",
      "    <<",
      "      /ImageType 1",
      "      /Width columns",
      "      /Height rows",
      "      /BitsPerComponent 8",
      "      /DataSource pixel_stream",
      "      /MultipleDataSources false",
      "      /ImageMatrix [columns 0 0 rows neg 0 rows]",
      "      /Decode [0 1 0 1 0 1]",
      "    >>",
      "  }",
      "  {",
      "    /DeviceCMYK setcolorspace",
      "    <<",
      "      /ImageType 1",
      "      /Width columns",
      "      /Height rows",
      "      /BitsPerComponent 8",
      "      /DataSource pixel_stream",
      "      /MultipleDataSources false",
      "      /ImageMatrix [columns 0 0 rows neg 0 rows]",
      "      /Decode",
      "        compression "PS3_JPEGCompression" eq",
      "        { [1 0 1 0 1 0 1 0] }",
      "        { [0 1 0 1 0 1 0 1] }",
      "        ifelse",
      "    >>",
      "  }",
      "  ifelse",
      "} bind def",
      "",
      "/PseudoClassImageDict",
      "{",
      "  % Colors in colormap image.",
      "  currentfile buffer readline pop",
      "  token pop /colors exch def pop",
      "  colors 0 eq",
      "  {",
      "    % Depth of grayscale image.",
      "    currentfile buffer readline pop",
      "    token pop /bits exch def pop",
      "    /DeviceGray setcolorspace",
      "    <<",
      "      /ImageType 1",
      "      /Width columns",
      "      /Height rows",
      "      /BitsPerComponent bits",
      "      /Decode [0 1]",
      "      /ImageMatrix [columns 0 0 rows neg 0 rows]",
      "      /DataSource pixel_stream",
      "    >>",
      "  }",
      "  {",
      "    % RGB colormap.",
      "    /colormap colors 3 mul string def",
      "    compression "PS3_NoCompression" eq",
      "    { currentfile /ASCII85Decode filter colormap readstring pop pop }",
      "    { currentfile colormap readstring pop pop }",
      "    ifelse",
      "    [ /Indexed /DeviceRGB colors 1 sub colormap ] setcolorspace",
      "    <<",
      "      /ImageType 1",
      "      /Width columns",
      "      /Height rows",
      "      /BitsPerComponent 8",
      "      /Decode [0 255]",
      "      /ImageMatrix [columns 0 0 rows neg 0 rows]",
      "      /DataSource pixel_stream",
      "    >>",
      "  }",
      "  ifelse",
      "} bind def",
      "",
      "/NonMaskedImageDict",
      "{",
      "  class "PS3_PseudoClass" eq",
      "  { PseudoClassImageDict }",
      "  { DirectClassImageDict }",
      "  ifelse",
      "} bind def",
      "",
      "/MaskedImageDict",
      "{",
      "  <<",
      "    /ImageType 3",
      "    /InterleaveType 3",
      "    /DataDict NonMaskedImageDict",
      "    /MaskDict",
      "    <<",
      "      /ImageType 1",
      "      /Width columns",
      "      /Height rows",
      "      /BitsPerComponent 1",
      "      /DataSource mask_stream",
      "      /MultipleDataSources false",
      "      /ImageMatrix [ columns 0 0 rows neg 0 rows]",
      "      /Decode [ 0 1 ]",
      "    >>",
      "  >>",
      "} bind def",
      "",
      "/ClipImage",
      "{} def",
      "",
      "/DisplayImage",
      "{",
      "  /buffer 512 string def",
      "  % Translation.",
      "  currentfile buffer readline pop",
      "  token pop /x exch def",
      "  token pop /y exch def pop",
      "  x y translate",
      "  % Image size and font size.",
      "  currentfile buffer readline pop",
      "  token pop /x exch def",
      "  token pop /y exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /pointsize exch def pop",
      (char *) NULL
    },
    *PostscriptEpilog[]=
    {
      "  x y scale",
      "  % Clipping path.",
      "  currentfile buffer readline pop",
      "  token pop /clipped exch def pop",
      "  % Showpage.",
      "  currentfile buffer readline pop",
      "  token pop /sp exch def pop",
      "  % Image pixel size.",
      "  currentfile buffer readline pop",
      "  token pop /columns exch def",
      "  token pop /rows exch def pop",
      "  % Colorspace (RGB/CMYK).",
      "  currentfile buffer readline pop",
      "  token pop /colorspace exch def pop",
      "  % Transparency.",
      "  currentfile buffer readline pop",
      "  token pop /alpha exch def pop",
      "  % Stencil mask?",
      "  currentfile buffer readline pop",
      "  token pop /stencil exch def pop",
      "  % Image class (direct/pseudo).",
      "  currentfile buffer readline pop",
      "  token pop /class exch def pop",
      "  % Compression type.",
      "  currentfile buffer readline pop",
      "  token pop /compression exch def pop",
      "  % Clip and render.",
      "  /pixel_stream currentfile columns rows compression",
      "    ByteStreamDecodeFilter def",
      "  clipped { ClipImage } if",
      "  alpha stencil not and",
      "  { MaskedImageDict mask_stream resetfile }",
      "  { NonMaskedImageDict }",
      "  ifelse",
      "  stencil { 0 setgray imagemask } { image } ifelse",
      "  grestore",
      "  sp { showpage } if",
      "} bind def",
      (char *) NULL
    };

  char
    buffer[MaxTextExtent],
    date[MaxTextExtent],
    **labels,
    page_geometry[MaxTextExtent];

  const char
    *option,
    **q;

  CompressionType
    compression;

  const ImageAttribute
    *attribute;

  GeometryInfo
    geometry_info;

  long
    j;

  MagickBooleanType
    status;

  MagickOffsetType
    scene,
    start,
    stop;

  MagickStatusType
    flags;

  PointInfo
    delta,
    resolution,
    scale;

  RectangleInfo
    geometry,
    media_info,
    page_info;

  register long
    i;

  SegmentInfo
    bounds;

  size_t
    length;

  time_t
    timer;

  unsigned char
    *pixels;

  unsigned long
    page,
    pixel,
    text_size;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFalse)
    return(MagickFalse);
  compression=image->compression;
  switch (compression)
  {
    case FaxCompression:
    case Group4Compression:
    { 
      if ((IsMonochromeImage(image,&image->exception) == MagickFalse) ||
          (image->matte != MagickFalse))
        compression=RLECompression;
      break;
    }
#if !defined(HasJPEG)
    case JPEGCompression:
    {
      compression=RLECompression;
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        MissingDelegateError,"JPEGLibraryIsNotAvailable","`%s'",
        image->filename);
      break;
    }
#endif
#if !defined(HasZLIB)
    case ZipCompression:
    {
      compression=RLECompression;
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        MissingDelegateError,"ZipLibraryIsNotAvailable","`%s'",image->filename);
      break;
    }
#endif
    default:
      break;
  }
  (void) ResetMagickMemory(&bounds,0,sizeof(bounds));
  page=0;
  scene=0;
  do
  {
    /*
      Scale relative to dots-per-inch.
    */
    delta.x=DefaultResolution;
    delta.y=DefaultResolution;
    flags=ParseGeometry(PSDensityGeometry,&geometry_info);
    resolution.x=geometry_info.rho;
    resolution.y=geometry_info.sigma;
    if ((flags & SigmaValue) == 0)
      resolution.y=resolution.x;
    if (image->x_resolution != 0.0)
      resolution.x=image->x_resolution;
    if (image->y_resolution != 0.0)
      resolution.y=image->y_resolution;
    if (image_info->density != (char *) NULL)
      {
        flags=ParseGeometry(image_info->density,&geometry_info);
        resolution.x=geometry_info.rho;
        resolution.y=geometry_info.sigma;
        if ((flags & SigmaValue) == 0)
          resolution.y=resolution.x;
      }
    if (image->units == PixelsPerCentimeterResolution)
      {
        resolution.x*=2.54;
        resolution.y*=2.54;
      }
    SetGeometry(image,&geometry);
    (void) FormatMagickString(page_geometry,MaxTextExtent,"%lux%lu",
      image->columns,image->rows);
    if (image_info->page != (char *) NULL)
      (void) CopyMagickString(page_geometry,image_info->page,MaxTextExtent);
    else
      if ((image->page.width != 0) && (image->page.height != 0))
        (void) FormatMagickString(page_geometry,MaxTextExtent,"%lux%lu%+ld%+ld",
          image->page.width,image->page.height,image->page.x,image->page.y);
      else
        if ((image->gravity != UndefinedGravity) &&
            (LocaleCompare(image_info->magick,"PS") == 0))
          (void) CopyMagickString(page_geometry,PSPageGeometry,MaxTextExtent);
    (void) ParseMetaGeometry(page_geometry,&geometry.x,&geometry.y,
      &geometry.width,&geometry.height);
    scale.x=(double) (geometry.width*delta.x)/resolution.x;
    geometry.width=(unsigned long) (scale.x+0.5);
    scale.y=(double) (geometry.height*delta.y)/resolution.y;
    geometry.height=(unsigned long) (scale.y+0.5);
    (void) ParseAbsoluteGeometry(page_geometry,&media_info);
    (void) ParseGravityGeometry(image,page_geometry,&page_info);
    if (image->gravity != UndefinedGravity)
      {
        geometry.x=(-page_info.x);
        geometry.y=(long) (media_info.height+page_info.y-image->rows);
      }
    text_size=0;
    attribute=GetImageAttribute(image,"Label");
    if (attribute != (const ImageAttribute *) NULL)
      text_size=(unsigned long)
        (MultilineCensus(attribute->value)*image_info->pointsize+12);
    page++;
    if (page == 1)
      {
        /*
          Postscript header on the first page.
        */
        if (LocaleCompare(image_info->magick,"PS3") == 0)
          (void) CopyMagickString(buffer,"%!PS-Adobe-3.0\n",MaxTextExtent);
        else
          (void) CopyMagickString(buffer,"%!PS-Adobe-3.0 EPSF-3.0\n",
            MaxTextExtent);
        (void) WriteBlobString(image,buffer);
        (void) FormatMagickString(buffer,MaxTextExtent,
          "%%%%Creator: ImageMagick %s\n",MagickLibVersionText);
        (void) WriteBlobString(image,buffer);
        (void) FormatMagickString(buffer,MaxTextExtent,"%%%%Title: %s\n",
          image->filename);
        (void) WriteBlobString(image,buffer);
        timer=time((time_t *) NULL);
        (void) localtime(&timer);
        (void) CopyMagickString(date,ctime(&timer),MaxTextExtent);
        date[strlen(date)-1]='\0';
        (void) FormatMagickString(buffer,MaxTextExtent,
          "%%%%CreationDate: %s\n",date);
        (void) WriteBlobString(image,buffer);
        bounds.x1=(double) geometry.x;
        bounds.y1=(double) geometry.y;
        bounds.x2=(double) geometry.x+scale.x;
        bounds.y2=(double) geometry.y+scale.y+text_size;
        if ((image_info->adjoin != MagickFalse) &&
            (GetNextImageInList(image) != (Image *) NULL))
          {
            (void) WriteBlobString(image,"%%BoundingBox: (atend)\n");
            (void) WriteBlobString(image,"%%HiResBoundingBox: (atend)\n");
          }
        else
          {
            (void) FormatMagickString(buffer,MaxTextExtent,
              "%%%%BoundingBox: %g %g %g %g\n",floor(bounds.x1+0.5),
              floor(bounds.y1+0.5),ceil(bounds.x2-0.5),ceil(bounds.y2-0.5));
            (void) WriteBlobString(image,buffer);
            (void) FormatMagickString(buffer,MaxTextExtent,
              "%%%%HiResBoundingBox: %g %g %g %g\n",bounds.x1,bounds.y1,
              bounds.x2,bounds.y2);
            (void) WriteBlobString(image,buffer);
            if (image->colorspace == CMYKColorspace)
              (void) WriteBlobString(image,
                "%%DocumentProcessColors: Cyan Magenta Yellow Black\n");
            else
              if (IsGrayImage(image,&image->exception) != MagickFalse)
                (void) WriteBlobString(image,
                  "%%DocumentProcessColors: Black\n");
          }
        /*
          Font resources
        */
        attribute=GetImageAttribute(image,"Label");
        if (attribute != (const ImageAttribute *) NULL)
          (void) WriteBlobString(image,
            "%%DocumentNeededResources: font Helvetica\n");
        (void) WriteBlobString(image,"%%LanguageLevel: 3\n");
        /*
          Pages, orientation and order.
        */
        if (LocaleCompare(image_info->magick,"PS3") != 0)
          (void) WriteBlobString(image,"%%Pages: 1\n");
        else
          {
            (void) WriteBlobString(image,"%%Orientation: Portrait\n");
            (void) WriteBlobString(image,"%%PageOrder: Ascend\n");
            if (image_info->adjoin == MagickFalse)
              (void) CopyMagickString(buffer,"%%Pages: 1\n",MaxTextExtent);
            else
              (void) FormatMagickString(buffer,MaxTextExtent,"%%%%Pages: %lu\n",
                (unsigned long) GetImageListLength(image));
            (void) WriteBlobString(image,buffer);
          }
        (void) WriteBlobString(image,"%%EndComments\n");
        /*
          The static postscript procedures prolog.
        */
        (void)WriteBlobString(image,"%%BeginProlog\n");
        for (q=PostscriptProlog; *q; q++)
        {
          (void) WriteBlobString(image,*q);
          (void) WriteBlobByte(image,'\n');
        }
        /*
          One label line for each line in label string.
        */
        attribute=GetImageAttribute(image,"Label");
        if (attribute != (const ImageAttribute *) NULL)
        {
          (void) WriteBlobString(image,"\n  % Labels.\n  /Helvetica findfont "
            "pointsize scalefont setfont\n");
          for (i=(long) MultilineCensus(attribute->value)-1; i >= 0; i--)
          {
            (void) WriteBlobString(image,
              "  currentfile buffer readline pop token pop\n");
            (void) FormatMagickString(buffer,MaxTextExtent,
              "  0 y %g add moveto show pop\n",i*image_info->pointsize+12);
            (void) WriteBlobString(image,buffer);
          }
        }
        /*
          The static postscript procedures epilog.
        */
        for (q=PostscriptEpilog; *q; q++)
        {
          (void) WriteBlobString(image,*q);
          (void) WriteBlobByte(image,'\n');
        }
        (void)WriteBlobString(image,"%%EndProlog\n");
      }
    (void) FormatMagickString(buffer,MaxTextExtent,"%%%%Page: 1 %lu\n",page);
    (void) WriteBlobString(image,buffer);
    /*
      Page bounding box.
    */
    (void) FormatMagickString(buffer,MaxTextExtent,
      "%%%%PageBoundingBox: %ld %ld %ld %ld\n",geometry.x,geometry.y,geometry.x+
      (long) geometry.width,geometry.y+(long) (geometry.height+text_size));
    (void) WriteBlobString(image,buffer);
    /*
      Page process colors if not RGB.
    */
    if (image->colorspace == CMYKColorspace)
      (void) WriteBlobString(image,
        "%%PageProcessColors: Cyan Magenta Yellow Black\n");
    else
      if (IsGrayImage(image,&image->exception) != MagickFalse)
        (void) WriteBlobString(image,"%%PageProcessColors: Black\n");
    /*
      Adjust document bounding box to bound page bounding box.
    */
    if ((double) geometry.x < bounds.x1)
      bounds.x1=(double) geometry.x;
    if ((double) geometry.y < bounds.y1)
      bounds.y1=(double) geometry.y;
    if ((double) (geometry.x+scale.x) > bounds.x2)
      bounds.x2=(double) geometry.x+scale.x;
    if ((double) (geometry.y+scale.y+text_size) > bounds.y2)
      bounds.y2=(double) geometry.y+scale.y+text_size;
    /*
      Page font resource if there's a label.
    */
    attribute=GetImageAttribute(image,"Label");
    if (attribute != (const ImageAttribute *) NULL)
      (void) WriteBlobString(image,"%%PageResources: font Helvetica\n");
    /*
      PS clipping path from Photoshop clipping path.
    */
    if ((image->clip_mask == (Image *) NULL) ||
        (LocaleNCompare("8BIM:",image->clip_mask->magick_filename,5) != 0))
      (void) WriteBlobString(image,"/ClipImage {} def\n");
    else
      {
        const ImageAttribute
          *attribute;

        attribute=GetImageAttribute(image,image->clip_mask->magick_filename);
        if (attribute == (const ImageAttribute *) NULL)
          return(MagickFalse);
        (void) WriteBlobString(image,attribute->value);
        (void) WriteBlobByte(image,'\n');
      }
    /*
      Push a dictionary for our own def's if this is an EPS.
    */
    if (LocaleCompare(image_info->magick,"PS3") != 0)
      (void) WriteBlobString(image,"userdict begin\n");
    /*
      Image mask.
    */
    if ((image->matte != MagickFalse) &&
        (WritePS3MaskImage(image_info,image,compression) == MagickFalse))
      {
        CloseBlob(image);
        return(MagickFalse);
      }
    /*
      Remember position of BeginData comment so we can update it.
    */
    start=TellBlob(image);
    (void) FormatMagickString(buffer,MaxTextExtent,
      "%%%%BeginData:%13ld %s Bytes\n",0L,
      compression == NoCompression ? "ASCII" : "BINARY");
    (void) WriteBlobString(image,buffer);
    stop=TellBlob(image);
    (void) WriteBlobString(image,"DisplayImage\n");
    /*
      Translate, scale, and font point size.
    */
    (void) FormatMagickString(buffer,MaxTextExtent,"%ld %ld\n%g %g\n%f\n",
      geometry.x,geometry.y,scale.x,scale.y,image_info->pointsize);
    (void) WriteBlobString(image,buffer);
    /*
      Output labels.
    */
    labels=(char **) NULL;
    attribute=GetImageAttribute(image,"Label");
    if (attribute != (const ImageAttribute *) NULL)
      labels=StringToList(attribute->value);
    if (labels != (char **) NULL)
      {
        for (i=0; labels[i] != (char *) NULL; i++)
        {
          if (compression != NoCompression)
            {
              for (j=0; labels[i][j] != '\0'; j++)
                (void) WriteBlobByte(image,(unsigned char) labels[i][j]);
              (void) WriteBlobByte(image,'\n');
            }
          else
            {
              (void) WriteBlobString(image,"<~");
              Ascii85Initialize(image);
              for (j=0; labels[i][j] != '\0'; j++)
                Ascii85Encode(image,(unsigned char) labels[i][j]);
              Ascii85Flush(image);
            }
          labels[i]=(char *) RelinquishMagickMemory(labels[i]);
        }
        labels=(char **) RelinquishMagickMemory(labels);
      }
    /*
      Photoshop clipping path active?
    */
    if ((image->clip_mask != (Image *) NULL) &&
        (LocaleNCompare("8BIM:",image->clip_mask->magick_filename,5) == 0))
        (void) WriteBlobString(image,"true\n");
      else
        (void) WriteBlobString(image,"false\n");
    /*
      Showpage for non-EPS.
    */
    (void) WriteBlobString(image, LocaleCompare(image_info->magick,"PS3") == 0 ?
      "true\n" : "false\n");
    /*
      Image columns, rows, and color space.
    */
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu %lu\n%s\n",
      image->columns,image->rows,image->colorspace == CMYKColorspace ?
      PS3_CMYKColorspace : PS3_RGBColorspace);
    (void) WriteBlobString(image,buffer);
    /*
      Masked image?
    */
    (void) WriteBlobString(image,image->matte != MagickFalse ?
      "true\n" : "false\n");
    /*
      Render with imagemask operator?
    */
    option=GetImageOption(image_info,"ps3:imagemask");
    (void) WriteBlobString(image,((option != (const char *) NULL) &&
      (IsMonochromeImage(image,&image->exception) != MagickFalse)) ?
      "true\n" : "false\n");
    /*
      Output pixel data.
    */
    pixels=(unsigned char *) NULL;
    length=0;
    if ((image_info->type != TrueColorType) &&
        (image_info->type != TrueColorMatteType) &&
        (image_info->type != ColorSeparationType) &&
        (image_info->type != ColorSeparationMatteType) &&
        (image->colorspace != CMYKColorspace) &&
        ((IsGrayImage(image,&image->exception) != MagickFalse) ||
         (IsMonochromeImage(image,&image->exception) != MagickFalse)))
      {
        /*
          Gray images.
        */
        (void) WriteBlobString(image,PS3_PseudoClass"\n");
        switch (compression)
        {
          case NoCompression:
          default:
          {
            (void) WriteBlobString(image,PS3_NoCompression"\n");
            break;
          }
          case FaxCompression:
          case Group4Compression:
          {
            (void) WriteBlobString(image,PS3_FaxCompression"\n");
            break;
          }
          case JPEGCompression:
          {
            (void) WriteBlobString(image,PS3_JPEGCompression"\n");
            break;
          }
          case LZWCompression:
          {
            (void) WriteBlobString(image,PS3_LZWCompression"\n");
            break;
          }
          case RLECompression:
          {
            (void) WriteBlobString(image,PS3_RLECompression"\n");
            break;
          }
          case ZipCompression:
          {
            (void) WriteBlobString(image,PS3_ZipCompression"\n");
            break;
          }
        }
        /*
          Number of colors -- 0 for single component non-color mapped data.
        */
        (void) WriteBlobString(image,"0\n");
        /*
          1 bit or 8 bit components?
        */
        (void) FormatMagickString(buffer,MaxTextExtent,"%d\n",
          IsMonochromeImage(image,&image->exception) != MagickFalse ? 1 : 8);
        (void) WriteBlobString(image,buffer);
        /*
          Image data.
        */
        if (compression == JPEGCompression)
          status=JPEGEncodeImage(image_info,image);
        else
          if ((compression == FaxCompression) ||
              (compression == Group4Compression))
           {
             if (LocaleCompare(CCITTParam,"0") == 0)
               status=HuffmanEncodeImage(image_info,image);
             else
               status=Huffman2DEncodeImage(image_info,image);
           }
        else
          {
            status=SerializeImageChannel(image_info,image,&pixels,&length);
            if (status == MagickFalse)
              {
                CloseBlob(image);
                return(MagickFalse);
              }
            switch (compression)
            {
              case NoCompression:
              default:
              {
                Ascii85Initialize(image);
                for (i=0; i < (long) length; i++)
                  Ascii85Encode(image,pixels[i]);
                Ascii85Flush(image);
                status=MagickTrue;
                break;
              }
              case LZWCompression:
              {
                status=LZWEncodeImage(image,length,pixels);
                break;
              }
              case RLECompression:
              {
                status=PackbitsEncodeImage(image,length,pixels);
                break;
              }
              case ZipCompression:
              {
                status=ZLIBEncodeImage(image,length,pixels);
                break;
              }
            }
            pixels=(unsigned char *) RelinquishMagickMemory(pixels);
          }
      }
    else
      if ((image->storage_class == DirectClass) || (image->colors > 256) ||
          (compression == JPEGCompression))
        {
          /*
            Truecolor image.
          */
          (void) WriteBlobString(image,PS3_DirectClass"\n");
          switch (compression)
          {
            case NoCompression:
            default:
            {
              (void) WriteBlobString(image,PS3_NoCompression"\n");
              break;
            }
            case RLECompression:
            {
              (void) WriteBlobString(image,PS3_RLECompression"\n");
              break;
            }
            case JPEGCompression:
            {
              (void) WriteBlobString(image,PS3_JPEGCompression"\n");
              break;
            }
            case LZWCompression:
            {
              (void) WriteBlobString(image,PS3_LZWCompression"\n");
              break;
            }
            case ZipCompression:
            {
              (void) WriteBlobString(image,PS3_ZipCompression"\n");
              break;
            }
          }
          /*
            Image data.
          */
          if (compression == JPEGCompression)
            status=JPEGEncodeImage(image_info,image);
          else
            {
              /*
                Stream based compressions.
              */
              status=SerializeImage(image_info,image,&pixels,&length);
              if (status == MagickFalse)
                {
                  CloseBlob(image);
                  return(MagickFalse);
                }
              switch (compression)
              {
                case NoCompression:
                default:
                {
                  Ascii85Initialize(image);
                  for (i=0; i < (long) length; i++)
                    Ascii85Encode(image,pixels[i]);
                  Ascii85Flush(image);
                  status=MagickTrue;
                  break;
                }
                case RLECompression:
                {
                  status=PackbitsEncodeImage(image,length,pixels);
                  break;
                }
                case LZWCompression:
                {
                  status=LZWEncodeImage(image,length,pixels);
                  break;
                }
                case ZipCompression:
                {
                  status=ZLIBEncodeImage(image,length,pixels);
                  break;
                }
              }
              pixels=(unsigned char *) RelinquishMagickMemory(pixels);
            }
          }
        else
          {
            /*
              Colormapped images.
            */
            (void) WriteBlobString(image,PS3_PseudoClass"\n");
            switch (compression)
            {
              case NoCompression:
              default:
              {
                (void) WriteBlobString(image,PS3_NoCompression"\n");
                break;
              }
              case JPEGCompression:
              {
                (void) WriteBlobString(image,PS3_JPEGCompression"\n");
                break;
              }
              case RLECompression:
              {
                (void) WriteBlobString(image,PS3_RLECompression"\n");
                break;
              }
              case LZWCompression:
              {
                (void) WriteBlobString(image,PS3_LZWCompression"\n");
                break;
              }
              case ZipCompression:
              {
                (void) WriteBlobString(image,PS3_ZipCompression"\n");
                break;
              }
            }
            /*
              Number of colors in color map.
            */
            (void) FormatMagickString(buffer,MaxTextExtent,"%lu\n",
              image->colors);
            (void) WriteBlobString(image,buffer);
            /*
              Color map - uncompressed.
            */
            if (compression != NoCompression)
              {
                for (i=0; i < (long) image->colors; i++)
                {
                  pixel=ScaleQuantumToChar(image->colormap[i].red);
                  (void) WriteBlobByte(image,(unsigned char) pixel);
                  pixel=ScaleQuantumToChar(image->colormap[i].green);
                  (void) WriteBlobByte(image,(unsigned char) pixel);
                  pixel=ScaleQuantumToChar(image->colormap[i].blue);
                  (void) WriteBlobByte(image,(unsigned char) pixel);
                }
              }
            else
              {
                Ascii85Initialize(image);
                for (i=0; i < (long) image->colors; i++)
                {
                  pixel=ScaleQuantumToChar(image->colormap[i].red);
                  Ascii85Encode(image,(unsigned char) pixel);
                  pixel=ScaleQuantumToChar(image->colormap[i].green);
                  Ascii85Encode(image,(unsigned char) pixel);
                  pixel=ScaleQuantumToChar(image->colormap[i].blue);
                  Ascii85Encode(image,(unsigned char) pixel);
                }
                Ascii85Flush(image);
              }
            status=SerializeImageIndexes(image_info,image,&pixels,&length);
            if (status == MagickFalse)
              {
                CloseBlob(image);
                return(MagickFalse);
              }
            switch (compression)
            {
              case NoCompression:
              default:
              {
                Ascii85Initialize(image);
                for (i=0; i < (long) length; i++)
                  Ascii85Encode(image,pixels[i]);
                Ascii85Flush(image);
                status=MagickTrue;
                break;
              }
              case JPEGCompression:
              {
                status=JPEGEncodeImage(image_info,image);
                break;
              }
              case RLECompression:
              {
                status=PackbitsEncodeImage(image,length,pixels);
                break;
              }
              case LZWCompression:
              {
                status=LZWEncodeImage(image,length,pixels);
                break;
              }
              case ZipCompression:
              {
                status=ZLIBEncodeImage(image,length,pixels);
                break;
              }
            }
            pixels=(unsigned char *) RelinquishMagickMemory(pixels);
          }
    (void) WriteBlobByte(image,'\n');
    if (status == MagickFalse)
      {
        CloseBlob(image);
        return(MagickFalse);
      }
    /*
      Update BeginData now that we know the data size.
    */
    length=(size_t) (TellBlob(image)-stop);
    stop=TellBlob(image);
    (void) SeekBlob(image,start,SEEK_SET);
    (void) FormatMagickString(buffer,MaxTextExtent,
      "%%%%BeginData:%13ld %s Bytes\n",(long) length,
      compression == NoCompression ? "ASCII" : "BINARY");
    (void) WriteBlobString(image,buffer);
    (void) SeekBlob(image,stop,SEEK_SET);
    (void) WriteBlobString(image,"%%EndData\n");
    /*
      End private dictionary if this is an EPS.
    */
    if (LocaleCompare(image_info->magick,"PS3") != 0)
      (void) WriteBlobString(image,"end\n");
    (void) WriteBlobString(image,"%%PageTrailer\n");
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        status=image->progress_monitor(SaveImagesTag,scene,
          GetImageListLength(image),image->client_data);
        if (status == MagickFalse)
          break;
      }
    scene++;
  } while (image_info->adjoin != MagickFalse);
  (void) WriteBlobString(image,"%%Trailer\n");
  if (page > 1)
    {
      (void) FormatMagickString(buffer,MaxTextExtent,
        "%%%%BoundingBox: %g %g %g %g\n",floor(bounds.x1+0.5),
        floor(bounds.y1+0.5),ceil(bounds.x2-0.5),ceil(bounds.y2-0.5));
      (void) WriteBlobString(image,buffer);
      (void) FormatMagickString(buffer,MaxTextExtent,
        "%%%%HiResBoundingBox: %g %g %g %g\n",bounds.x1,bounds.y1,
        bounds.x2,bounds.y2);
      (void) WriteBlobString(image,buffer);
    }
  (void) WriteBlobString(image,"%%EOF\n");
  CloseBlob(image);
  return(MagickTrue);
}
