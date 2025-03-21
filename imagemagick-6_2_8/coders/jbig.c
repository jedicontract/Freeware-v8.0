/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        JJJJJ  BBBB   IIIII   GGGG                           %
%                          J    B   B    I    G                               %
%                          J    BBBB     I    G  GG                           %
%                        J J    B   B    I    G   G                           %
%                        JJJ    BBBB   IIIII   GGG                            %
%                                                                             %
%                                                                             %
%                       Read/Write JBIG Image Format.                         %
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
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/nt-feature.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"
#if defined(HasJBIG)
#include "jbig.h"
#endif

/*
  Forward declarations.
*/
#if defined(HasJBIG)
static MagickBooleanType
  WriteJBIGImage(const ImageInfo *,Image *);
#endif

#if defined(HasJBIG)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d J B I G I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadJBIGImage() reads a JBIG image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadJBIGImage method is:
%
%      Image *ReadJBIGImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadJBIGImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
#define MaxBufferSize  8192

  Image
    *image;

  IndexPacket
    index;

  long
    length,
    y;

  MagickBooleanType
    status;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  register unsigned char
    *p;

  ssize_t
    count;

  struct jbg_dec_state
    jbig_info;

  unsigned char
    bit,
    *buffer,
    byte;

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
    Initialize JBIG toolkit.
  */
  jbg_dec_init(&jbig_info);
  jbg_dec_maxsize(&jbig_info,(unsigned long) image->columns,
    (unsigned long) image->rows);
  image->columns= jbg_dec_getwidth(&jbig_info);
  image->rows= jbg_dec_getheight(&jbig_info);
  image->depth=8;
  image->storage_class=PseudoClass;
  image->colors=2;
  /*
    Read JBIG file.
  */
  buffer=(unsigned char *) AcquireMagickMemory(MaxBufferSize);
  if (buffer == (unsigned char *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  status=JBG_EAGAIN;
  do
  {
    length=(long) ReadBlob(image,MaxBufferSize,buffer);
    if (length == 0)
      break;
    p=buffer;
    count=0;
    while ((length > 0) && ((status == JBG_EAGAIN) || (status == JBG_EOK)))
    {
      size_t
        count;

      status=jbg_dec_in(&jbig_info,p,length,&count);
      p+=count;
      length-=count;
    }
  } while ((status == JBG_EAGAIN) || (status == JBG_EOK));
  /*
    Create colormap.
  */
  image->columns=jbg_dec_getwidth(&jbig_info);
  image->rows=jbg_dec_getheight(&jbig_info);
  if (AllocateImageColormap(image,2) == MagickFalse)
    {
      buffer=(unsigned char *) RelinquishMagickMemory(buffer);
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    }
  image->colormap[0].red=0;
  image->colormap[0].green=0;
  image->colormap[0].blue=0;
  image->colormap[1].red=QuantumRange;
  image->colormap[1].green=QuantumRange;
  image->colormap[1].blue=QuantumRange;
  image->x_resolution=300;
  image->y_resolution=300;
  if (image_info->ping != MagickFalse)
    {
      CloseBlob(image);
      return(GetFirstImageInList(image));
    }
  /*
    Convert X bitmap image to pixel packets.
  */
  p=jbg_dec_getimage(&jbig_info,0);
  for (y=0; y < (long) image->rows; y++)
  {
    q=SetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    bit=0;
    byte=0;
    for (x=0; x < (long) image->columns; x++)
    {
      if (bit == 0)
        byte=(*p++);
      index=(byte & 0x80) ? 0 : 1;
      bit++;
      byte<<=1;
      if (bit == 8)
        bit=0;
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
  /*
    Free scale resource.
  */
  jbg_dec_free(&jbig_info);
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  CloseBlob(image);
  return(GetFirstImageInList(image));
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r J B I G I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterJBIGImage() adds attributes for the JBIG image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterJBIGImage method is:
%
%      RegisterJBIGImage(void)
%
*/
ModuleExport void RegisterJBIGImage(void)
{
#define JBIGDescription  "Joint Bi-level Image experts Group interchange format"

  char
    version[MaxTextExtent];

  MagickInfo
    *entry;

  *version='\0';
#if defined(JBG_VERSION)
  (void) CopyMagickString(version,JBG_VERSION,MaxTextExtent);
#endif
  entry=SetMagickInfo("BIE");
#if defined(HasJBIG)
  entry->decoder=(DecoderHandler *) ReadJBIGImage;
  entry->encoder=(EncoderHandler *) WriteJBIGImage;
#endif
  entry->adjoin=MagickFalse;
  entry->description=ConstantString(JBIGDescription);
  if (*version != '\0')
    entry->version=ConstantString(version);
  entry->module=ConstantString("JBIG");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("JBG");
#if defined(HasJBIG)
  entry->decoder=(DecoderHandler *) ReadJBIGImage;
  entry->encoder=(EncoderHandler *) WriteJBIGImage;
#endif
  entry->description=ConstantString(JBIGDescription);
  if (*version != '\0')
    entry->version=ConstantString(version);
  entry->module=ConstantString("JBIG");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("JBIG");
#if defined(HasJBIG)
  entry->decoder=(DecoderHandler *) ReadJBIGImage;
  entry->encoder=(EncoderHandler *) WriteJBIGImage;
#endif
  entry->description=ConstantString(JBIGDescription);
  if (*version != '\0')
    entry->version=ConstantString(version);
  entry->module=ConstantString("JBIG");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r J B I G I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterJBIGImage() removes format registrations made by the
%  JBIG module from the list of supported formats.
%
%  The format of the UnregisterJBIGImage method is:
%
%      UnregisterJBIGImage(void)
%
*/
ModuleExport void UnregisterJBIGImage(void)
{
  (void) UnregisterMagickInfo("BIE");
  (void) UnregisterMagickInfo("JBG");
  (void) UnregisterMagickInfo("JBIG");
}

#if defined(HasJBIG)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e J B I G I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteJBIGImage() writes an image in the JBIG encoded image format.
%
%  The format of the WriteJBIGImage method is:
%
%      MagickBooleanType WriteJBIGImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/

static void JBIGEncode(unsigned char *pixels,size_t length,void *data)
{
  Image
    *image;

  image=(Image *) data;
  (void) WriteBlob(image,length,pixels);
}

static MagickBooleanType WriteJBIGImage(const ImageInfo *image_info,
  Image *image)
{
  double
    version;

  long
    y;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    x;

  register unsigned char
    *q;

  struct jbg_enc_state
    jbig_info;

  unsigned char
    bit,
    byte,
    *pixels,
    polarity;

  unsigned long
    number_packets;

  /*
    Open image file.
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
  version=strtod(JBG_VERSION,(char **) NULL);
  scene=0;
  do
  {
    /*
      Allocate pixel data.
    */
    if (image_info->colorspace == UndefinedColorspace)
      (void) SetImageColorspace(image,RGBColorspace);
    number_packets=((image->columns+7) >> 3)*image->rows;
    pixels=(unsigned char *) AcquireMagickMemory(number_packets);
    if (pixels == (unsigned char *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    /*
      Convert pixels to a bitmap.
    */
    SetImageType(image,BilevelType);
    polarity=PixelIntensityToQuantum(&image->colormap[0]) < (QuantumRange/2);
    if (image->colors == 2)
      polarity=PixelIntensityToQuantum(&image->colormap[0]) >
        PixelIntensityToQuantum(&image->colormap[1]);
    q=pixels;
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      bit=0;
      byte=0;
      for (x=0; x < (long) image->columns; x++)
      {
        byte<<=1;
        if (indexes[x] == polarity)
          byte|=0x01;
        bit++;
        if (bit == 8)
          {
            *q++=byte;
            bit=0;
            byte=0;
          }
       }
      if (bit != 0)
        *q++=byte << (8-bit);
      if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(y,image->rows) != MagickFalse))
        {
          status=image->progress_monitor(SaveImageTag,y,image->rows,
            image->client_data);
          if (status == MagickFalse)
            break;
        }
    }
    /*
      Initialize JBIG info structure.
    */
    jbg_enc_init(&jbig_info,image->columns,image->rows,1,&pixels,
      (void (*)(unsigned char *,size_t,void *)) JBIGEncode,image);
    if (image_info->scene != 0)
      jbg_enc_layers(&jbig_info,(int) image_info->scene);
    else
      {
        long
          sans_offset;

        unsigned long
          x_resolution,
          y_resolution;

        x_resolution=640;
        y_resolution=480;
        sans_offset=0;
        if (image_info->density != (char *) NULL)
          {
            GeometryInfo
              geometry_info;

            MagickStatusType
              flags;

            flags=ParseGeometry(image_info->density,&geometry_info);
            x_resolution=geometry_info.rho;
            y_resolution=geometry_info.sigma;
            if ((flags & SigmaValue) == 0)
              y_resolution=x_resolution;
          }
        if (image->units == PixelsPerCentimeterResolution)
          {
            x_resolution*=2.54;
            y_resolution*=2.54;
          }
        (void) jbg_enc_lrlmax(&jbig_info,x_resolution,y_resolution);
      }
    (void) jbg_enc_lrange(&jbig_info,-1,-1);
    jbg_enc_options(&jbig_info,JBG_ILEAVE | JBG_SMID,JBG_TPDON | JBG_TPBON |
      JBG_DPON,version < 1.6 ? -1 : 0,-1,-1);
    /*
      Write JBIG image.
    */
    jbg_enc_out(&jbig_info);
    jbg_enc_free(&jbig_info);
    pixels=(unsigned char *) RelinquishMagickMemory(pixels);
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
  CloseBlob(image);
  return(MagickTrue);
}
#endif
