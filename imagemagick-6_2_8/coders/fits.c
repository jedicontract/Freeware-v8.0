/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        FFFFF  IIIII  TTTTT  SSSSS                           %
%                        F        I      T    SS                              %
%                        FFF      I      T     SSS                            %
%                        F        I      T       SS                           %
%                        F      IIIII    T    SSSSS                           %
%                                                                             %
%                                                                             %
%            Read/Write Flexible Image Transport System Images.               %
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
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/quantum.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/statistic.h"
#include "magick/string_.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteFITSImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s F I T S                                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsFITS() returns MagickTrue if the image format type, identified by the
%  magick string, is FITS.
%
%  The format of the IsFITS method is:
%
%      MagickBooleanType IsFITS(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
*/
static MagickBooleanType IsFITS(const unsigned char *magick,const size_t length)
{
  if (length < 6)
    return(MagickFalse);
  if (LocaleNCompare((char *) magick,"IT0",3) == 0)
    return(MagickTrue);
  if (LocaleNCompare((char *) magick,"SIMPLE",6) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d F I T S I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadFITSImage() reads a FITS image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadFITSImage method is:
%
%      Image *ReadFITSImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: Method ReadFITSImage returns a pointer to the image after
%      reading.  A null image is returned if there is a memory shortage or if
%      the image cannot be read.
%
%    o filename: Specifies the name of the image to read.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *ReadFITSImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  typedef struct _FITSInfo
  {
    int
      simple,
      bits_per_pixel,
      columns,
      rows,
      number_axes,
      number_scenes;

    double
      min_data,
      max_data,
      zero,
      scale;
  } FITSInfo;

  char
    keyword[MaxTextExtent],
    value[MaxTextExtent];

  double
    exponential[2048],
    pixel,
    scale,
    scaled_pixel;

  FITSInfo
    fits_info;

  Image
    *image;

  IndexPacket
    index;

  int
    c,
    quantum;

  long
    exponent,
    j,
    k,
    l,
    scene,
    y;

  MagickBooleanType
    status,
    value_expected;

  MagickSizeType
    number_pixels;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  register long
    i;

  register unsigned char
    *p;

  ssize_t
    count;

  size_t
    packet_size;

  unsigned char
    *fits_pixels,
    long_quantum[8];

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Initialize image header.
  */
  fits_info.simple=MagickFalse;
  fits_info.bits_per_pixel=8;
  fits_info.columns=1;
  fits_info.rows=1;
  fits_info.rows=1;
  fits_info.number_scenes=1;
  fits_info.number_axes=0;
  fits_info.min_data=0.0;
  fits_info.max_data=0.0;
  fits_info.zero=0.0;
  fits_info.scale=1.0;
  /*
    Decode image header.
  */
  c=ReadBlobByte(image);
  if (c == EOF)
    {
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  for ( ; c != EOF; )
  {
    if (isalnum((int) ((unsigned char) c)) == 0)
      c=ReadBlobByte(image);
    else
      {
        register char
          *p;

        /*
          Determine a keyword and its value.
        */
        p=keyword;
        do
        {
          if ((size_t) (p-keyword) < (MaxTextExtent/2))
            *p++=(char) c;
          c=ReadBlobByte(image);
        } while ((isalnum((int) ((unsigned char) c)) != MagickFalse) ||
                 ((int) c == '_'));
        *p='\0';
        if (LocaleCompare(keyword,"END") == 0)
          break;
        value_expected=MagickFalse;
        while ((isspace((int) ((unsigned char) c)) != 0) || ((int) c == '='))
        {
          if ((int) c == '=')
            value_expected=MagickTrue;
          c=ReadBlobByte(image);
        }
        if (value_expected == MagickFalse)
          continue;
        p=value;
        while (isalnum(c) || (c == '-') || (c == '+') || (c == '.'))
        {
          if ((size_t) (p-value) < (MaxTextExtent/2))
            *p++=c;
          c=ReadBlobByte(image);
        }
        *p='\0';
        /*
          Assign a value to the specified keyword.
        */
        if (LocaleCompare(keyword,"SIMPLE") == 0)
          fits_info.simple=(int) ((*value == 'T') || (*value == 't'));
        if (LocaleCompare(keyword,"BITPIX") == 0)
          fits_info.bits_per_pixel=atoi(value);
        if (LocaleCompare(keyword,"NAXIS") == 0)
          fits_info.number_axes=atoi(value);
        if (LocaleCompare(keyword,"NAXIS1") == 0)
          fits_info.columns=atoi(value);
        if (LocaleCompare(keyword,"NAXIS2") == 0)
          fits_info.rows=atoi(value);
        if (LocaleCompare(keyword,"NAXIS3") == 0)
          fits_info.number_scenes=atoi(value);
        if (LocaleCompare(keyword,"DATAMAX") == 0)
          fits_info.max_data=atof(value);
        if (LocaleCompare(keyword,"DATAMIN") == 0)
          fits_info.min_data=atof(value);
        if (LocaleCompare(keyword,"BZERO") == 0)
          fits_info.zero=atof(value);
        if (LocaleCompare(keyword,"BSCALE") == 0)
          fits_info.scale=atof(value);
        (void) SetImageAttribute(image,keyword,value);
      }
    while (((TellBlob(image) % 80) != 0) && (c != EOF))
      c=ReadBlobByte(image);
    c=ReadBlobByte(image);
  }
  while (((TellBlob(image) % 2880) != 0) && (c != EOF))
    c=ReadBlobByte(image);
  /*
    Verify that required image information is defined.
  */
  number_pixels=(MagickSizeType) fits_info.columns*fits_info.rows;
  if ((fits_info.simple == 0) || (fits_info.number_axes < 1) ||
      (fits_info.number_axes > 4) || (number_pixels == 0))
    ThrowReaderException(CorruptImageError,"ImageTypeNotSupported");
  if (fits_info.bits_per_pixel == -32)
    {
      exponential[150]=1.0;
      for (i=151; i < 256; i++)
        exponential[i]=2.0*exponential[i-1];
      for (i=149; i >= 0; i--)
        exponential[i]=exponential[i+1]/2.0;
    }
  if (fits_info.bits_per_pixel == -64)
    {
      exponential[1075]=1.0;
      for (i=1076; i < 2048; i++)
        exponential[i]=2.0*exponential[i-1];
      for (i=1074; i >= 0; i--)
        exponential[i]=exponential[i+1]/2.0;
    }
  for (scene=0; scene < (long) fits_info.number_scenes; scene++)
  {
    /*
      Create linear colormap.
    */
    image->columns=(unsigned long) fits_info.columns;
    image->rows=(unsigned long) fits_info.rows;
    image->depth=fits_info.bits_per_pixel <= 8 ? 8UL : Min(QuantumDepth,16);
    image->storage_class=PseudoClass;
    image->scene=(unsigned long) scene;
    if (AllocateImageColormap(image,1UL << image->depth) == MagickFalse)
      ThrowReaderException(ResourceLimitError,"UnableToAllocateColormap");
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    /*
      Initialize image structure.
    */
    packet_size=(size_t) fits_info.bits_per_pixel/8;
    if (fits_info.bits_per_pixel < 0)
      packet_size=(size_t) (-fits_info.bits_per_pixel/8);
    number_pixels=(MagickSizeType) image->columns*image->rows;
    if ((packet_size*number_pixels) !=
        (MagickSizeType) ((size_t) (packet_size*number_pixels)))
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    fits_pixels=(unsigned char *)
      AcquireMagickMemory(packet_size*image->columns*image->rows);
    if (fits_pixels == (unsigned char *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    /*
      Convert FITS pixels to pixel packets.
    */
    count=ReadBlob(image,packet_size*image->columns*image->rows,fits_pixels);
    if (count != (ssize_t) (packet_size*image->columns*image->rows))
      ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
    if ((fits_info.bits_per_pixel > 0) &&
        (fits_info.min_data == 0.0) && (fits_info.max_data == 0.0))
      fits_info.max_data=(1UL << fits_info.bits_per_pixel)-1;
    else
      {
        /*
          Determine minimum and maximum intensity.
        */
        p=fits_pixels;
        long_quantum[0]=(*p);
        quantum=(*p++);
        for (j=1; j <= (long) (packet_size-1); j++)
        {
          long_quantum[j]=(*p);
          quantum=(quantum << 8) | (*p++);
        }
        pixel=(double) quantum;
        if (fits_info.bits_per_pixel == -32)
          {
            j=((long) long_quantum[1] << 16) | ((long) long_quantum[2] << 8) |
               (long) long_quantum[3];
            k=(int) *long_quantum;
            exponent=((k & 127) << 1) | (j >> 23);
            *(float *) long_quantum=
              exponential[exponent]*(float) (j | 0x800000);
            if ((exponent | j) == 0)
              *(float *) long_quantum=0.0;
            if (k & 128)
              *(float *) long_quantum=(-(*(float *) long_quantum));
            pixel=(double) (*((float *) long_quantum));
          }
        if (fits_info.bits_per_pixel == -64)
          {
            j=((long) long_quantum[1] << 24) |
              ((long) long_quantum[2] << 16) |
              ((long) long_quantum[3] << 8) |
               (long) long_quantum[4];
            k=(int) *long_quantum;
            l=((int) long_quantum[5] << 16) | ((int) long_quantum[6] << 8) |
               (int) long_quantum[7];
            exponent=((k & 127) << 4) | (j >> 28);
            *(double *) long_quantum=exponential[exponent]*(16777216.0*
              (double) ((j & 0x0FFFFFFF) | 0x10000000)+(double) l);
            if ((exponent | j | l) == 0)
              *(double *) long_quantum=0.0;
            if (k & 128)
              *(double *)long_quantum=(-(*(double *) long_quantum));
            pixel=(double) (*((double *) long_quantum));
          }
        fits_info.min_data=pixel*fits_info.scale-fits_info.zero;
        fits_info.max_data=pixel*fits_info.scale-fits_info.zero;
        for (i=1; i < (long) number_pixels; i++)
        {
          long_quantum[0]=(*p);
          quantum=(*p++);
          for (j=1; j <= (long) (packet_size-1); j++)
          {
            long_quantum[j]=(*p);
            quantum=(quantum << 8) | (*p++);
          }
          pixel=(double) quantum;
          if (fits_info.bits_per_pixel == -32)
            {
              j=((long) long_quantum[1] << 16) | ((long) long_quantum[2] << 8) |
                 (long) long_quantum[3];
              k=(int) *long_quantum;
              exponent=((k & 127) << 1) | (j >> 23);
              *(float *) long_quantum=
                exponential[exponent]*(float) (j | 0x800000);
              if ((exponent | j) == 0)
                *(float *) long_quantum=0.0;
              if (k & 128)
                *(float *) long_quantum=(-(*(float *) long_quantum));
              pixel=(double) (*((float *) long_quantum));
            }
          if (fits_info.bits_per_pixel == -64)
            {
              j=((long) long_quantum[1] << 24) |
                ((long) long_quantum[2] << 16) |
                ((long) long_quantum[3] << 8) |
                 (long) long_quantum[4];
              k=(int) *long_quantum;
              l=((int) long_quantum[5] << 16) | ((int) long_quantum[6] << 8) |
                 (int) long_quantum[7];
              exponent=((k & 127) << 4) | (j >> 28);
              *(double *) long_quantum=exponential[exponent]*(16777216.0*
                (double) ((j & 0x0FFFFFFF) | 0x10000000)+(double) l);
              if ((exponent | j | l) == 0)
                *(double *) long_quantum=0.0;
              if (k & 128)
                *(double *)long_quantum=(-(*(double *) long_quantum));
              pixel=(double) (*((double *) long_quantum));
            }
          scaled_pixel=pixel*fits_info.scale-fits_info.zero;
          if (scaled_pixel < fits_info.min_data)
            fits_info.min_data=scaled_pixel;
          if (scaled_pixel > fits_info.max_data)
            fits_info.max_data=scaled_pixel;
        }
        if (fits_info.bits_per_pixel == 16)
          if (pixel > 32767)
            pixel-=65536L;
      }
    /*
      Convert FITS pixels to pixel packets.
    */
    scale=1.0;
    if ((fits_info.bits_per_pixel < 0) || ((fits_info.max_data-
        fits_info.min_data) > (double) ((1UL << image->depth)-1)))
      scale=(double) ((1UL << image->depth)-1)/
        (fits_info.max_data-fits_info.min_data);
    p=fits_pixels;
    for (y=(long) image->rows-1; y >= 0; y--)
    {
      q=SetImagePixels(image,0,y,image->columns,1);
      if (q == (PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) image->columns; x++)
      {
        long_quantum[0]=(*p);
        quantum=(*p++);
        for (j=1; j <= (long) (packet_size-1); j++)
        {
          long_quantum[j]=(*p);
          quantum=(quantum << 8) | (*p++);
        }
        pixel=(double) quantum;
        if (fits_info.bits_per_pixel == -32)
          {
            j=((long) long_quantum[1] << 16) | ((long) long_quantum[2] << 8) |
               (long) long_quantum[3];
            k=(int) *long_quantum;
            exponent=((k & 127) << 1) | (j >> 23);
            *(float *) long_quantum=
              exponential[exponent]*(float) (j | 0x800000);
            if ((exponent | j) == 0)
              *(float *) long_quantum=0.0;
            if (k & 128)
              *(float *) long_quantum=(-(*(float *) long_quantum));
            pixel=(double) (*((float *) long_quantum));
          }
        if (fits_info.bits_per_pixel == -64)
          {
            j=((long) long_quantum[1] << 24) |
              ((long) long_quantum[2] << 16) |
              ((long) long_quantum[3] << 8) |
               (long) long_quantum[4];
            k=(long) *long_quantum;
            l=((long) long_quantum[5] << 16) | ((long) long_quantum[6] << 8) |
               (long) long_quantum[7];
            exponent=(long) ((((unsigned long) k & 127) << 4) |
              ((unsigned long) j >> 28));
            *(double *) long_quantum=exponential[exponent]*(16777216.0*
              (double) ((j & 0x0FFFFFFF) | 0x10000000)+(double) l);
            if ((exponent | j | l) == 0)
              *(double *) long_quantum=0.0;
            if ((k & 128) != 0)
              *(double *)long_quantum=(-(*(double *) long_quantum));
            pixel=(double) (*((double *) long_quantum));
          }
        scaled_pixel=scale*
          (pixel*fits_info.scale-fits_info.min_data-fits_info.zero);
        if (fits_info.bits_per_pixel == 16)
          if (scaled_pixel > 32767)
            scaled_pixel-=65536L;
        index=(IndexPacket) ((scaled_pixel < 0.0) ? 0 :
          ((unsigned long) scaled_pixel > ((1UL << image->depth)-1)) ?
          ((1UL << image->depth)-1) : (unsigned long) (scaled_pixel+0.5));
        index=ConstrainColormapIndex(image,index);
        indexes[x]=index;
        *q++=image->colormap[index];
      }
      if (SyncImagePixels(image) == MagickFalse)
        break;
      if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(y,image->rows) != MagickFalse))
        {
          status=image->progress_monitor(LoadImageTag,y,image->rows,
            image->client_data);
          if (status == MagickFalse)
            break;
        }
    }
    fits_pixels=(unsigned char *) RelinquishMagickMemory(fits_pixels);
    if (EOFBlob(image) != MagickFalse)
      {
        ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
          image->filename);
        break;
      }
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if (scene < (long) (fits_info.number_scenes-1))
      {
        /*
          Allocate next image structure.
        */
        AllocateNextImage(image_info,image);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            image=DestroyImageList(image);
            return((Image *) NULL);
          }
        image=SyncNextImageInList(image);
        if (image->progress_monitor != (MagickProgressMonitor) NULL)
          {
            status=image->progress_monitor(LoadImagesTag,TellBlob(image),
              GetBlobSize(image),image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
  }
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r F I T S I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterFITSImage() adds attributes for the FITS image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterFITSImage method is:
%
%      RegisterFITSImage(void)
%
*/
ModuleExport void RegisterFITSImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("FITS");
  entry->decoder=(DecoderHandler *) ReadFITSImage;
  entry->encoder=(EncoderHandler *) WriteFITSImage;
  entry->magick=(MagickHandler *) IsFITS;
  entry->adjoin=MagickFalse;
  entry->description=ConstantString("Flexible Image Transport System");
  entry->module=ConstantString("FITS");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r F I T S I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterFITSImage() removes format registrations made by the
%  FITS module from the list of supported formats.
%
%  The format of the UnregisterFITSImage method is:
%
%      UnregisterFITSImage(void)
%
*/
ModuleExport void UnregisterFITSImage(void)
{
  (void) UnregisterMagickInfo("FITS");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e F I T S I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteFITSImage() writes a Flexible Image Transport System image to a
%  file as gray scale intensities [0..255].
%
%  The format of the WriteFITSImage method is:
%
%      MagickBooleanType WriteFITSImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteFITSImage(const ImageInfo *image_info,
  Image *image)
{
  char
    buffer[MaxTextExtent],
    *fits_info;

  long
    y;

  MagickBooleanType
    status;

  MagickSizeType
    length;

  QuantumInfo
    quantum_info;

  register const PixelPacket
    *p;

  register long
    i;

  size_t
    packet_size;

  unsigned char
    *pixels;

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
    return(status);
  if (image_info->colorspace == UndefinedColorspace)
    (void) SetImageColorspace(image,RGBColorspace);
  /*
    Allocate image memory.
  */
  image->endian=MSBEndian;
  image->depth=GetImageQuantumDepth(image,MagickTrue);
  packet_size=(size_t) (image->depth > 8 ? 2 : 1);
  fits_info=(char *) AcquireMagickMemory(2880*sizeof(*fits_info));
  pixels=(unsigned char *) AcquireMagickMemory(packet_size*image->columns);
  if ((fits_info == (char *) NULL) || (pixels == (unsigned char *) NULL))
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  /*
    Initialize image header.
  */
  for (i=0; i < 2880; i++)
    fits_info[i]=' ';
  (void) CopyMagickString(buffer,"SIMPLE  =                    T",
    MaxTextExtent);
  (void) CopyMagickString(fits_info+0,buffer,2880);
  (void) FormatMagickString(buffer,MaxTextExtent,"BITPIX  =           %10lu",
    image->depth);
  (void) CopyMagickString(fits_info+80,buffer,2880);
  (void) CopyMagickString(buffer,"NAXIS   =                    2",
    MaxTextExtent);
  (void) CopyMagickString(fits_info+160,buffer,2880);
  (void) FormatMagickString(buffer,MaxTextExtent,"NAXIS1  =           %10lu",
    image->columns);
  (void) CopyMagickString(fits_info+240,buffer,2880);
  (void) FormatMagickString(buffer,MaxTextExtent,"NAXIS2  =           %10lu",
    image->rows);
  (void) CopyMagickString(fits_info+320,buffer,2880);
  (void) FormatMagickString(buffer,MaxTextExtent,"DATAMIN =           %10u",0);
  (void) CopyMagickString(fits_info+400,buffer,2880);
  (void) FormatMagickString(buffer,MaxTextExtent,"DATAMAX =           %10u",
    (1 << image->depth)-1);
  (void) CopyMagickString(fits_info+480,buffer,2880);
  (void) FormatMagickString(buffer,MaxTextExtent,"BZERO   =           %10u",
    image->depth > 8 ? 32768 : 0);
  (void) CopyMagickString(fits_info+560,buffer,2880);
  (void) CopyMagickString(buffer,"HISTORY Created by ImageMagick.",
    MaxTextExtent);
  (void) CopyMagickString(fits_info+640,buffer,2880);
  (void) CopyMagickString(buffer,"END",MaxTextExtent);
  (void) CopyMagickString(fits_info+720,buffer,2880);
  (void) WriteBlob(image,2880,(unsigned char *) fits_info);
  /*
    Convert image to fits scale PseudoColor class.
  */
  GetQuantumInfo(image_info,&quantum_info);
  for (y=(long) image->rows-1; y >= 0; y--)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    (void) ImportQuantumPixels(image,&quantum_info,GrayQuantum,pixels);
    (void) WriteBlob(image,packet_size*image->columns,pixels);
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(image->rows-y-1,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SaveImageTag,image->rows-y-1,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  (void) ResetMagickMemory(fits_info,0,2880*sizeof(*fits_info));
  length=packet_size*image->columns*image->rows;
  (void) WriteBlob(image,2880-((size_t) length % 2880),(unsigned char *)
    fits_info);
  fits_info=(char *) RelinquishMagickMemory(fits_info);
  pixels=(unsigned char *) RelinquishMagickMemory(pixels);
  CloseBlob(image);
  return(MagickTrue);
}
