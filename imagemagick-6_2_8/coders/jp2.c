/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                              JJJ  PPPP    222                               %
%                               J   P   P  2   2                              %
%                               J   PPPP     22                               %
%                            J  J   P       2                                 %
%                             JJ    P      22222                              %
%                                                                             %
%                                                                             %
%                     Read/Write JPEG-2000 Image Format.                      %
%                                                                             %
%                                John Cristy                                  %
%                                Nathan Brown                                 %
%                                 June 2001                                   %
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
#include "magick/colorspace.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/option.h"
#include "magick/profile.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#if defined(HasJP2)
#if !defined(uchar)
#define uchar  unsigned char
#endif
#if !defined(ushort)
#define ushort  unsigned short
#endif
#if !defined(uint)
#define uint  unsigned int
#endif
#if !defined(longlong)
#define longlong  long long
#endif
#if !defined(ulonglong)
#define ulonglong  unsigned long long
#endif

#include "jasper/jasper.h"
#endif

/*
  Forward declarations.
*/
#if defined(HasJP2)
static MagickBooleanType
  WriteJP2Image(const ImageInfo *,Image *);
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s J P 2                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsJP2() returns MagickTrue if the image format type, identified by the
%  magick string, is JP2.
%
%  The format of the IsJP2 method is:
%
%      MagickBooleanType IsJP2(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
%
*/
static MagickBooleanType IsJP2(const unsigned char *magick,const size_t length)
{
  if (length < 9)
    return(MagickFalse);
  if (memcmp(magick+4,"\152\120\040\040\015",5) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s J P C                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsJPC()() returns MagickTrue if the image format type, identified by the
%  magick string, is JPC.
%
%  The format of the IsJPC method is:
%
%      MagickBooleanType IsJPC(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
%
*/
static MagickBooleanType IsJPC(const unsigned char *magick,const size_t length)
{
  if (length < 2)
    return(MagickFalse);
  if (memcmp(magick,"\377\117",2) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d J P 2 I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadJP2Image() reads a JPEG 2000 Image file (JP2) or JPEG 2000
%  codestream (JPC) image file and returns it.  It allocates the memory
%  necessary for the new Image structure and returns a pointer to the new
%  image or set of images.
%
%  JP2 support is originally written by Nathan Brown, nathanbrown@letu.edu.
%
%  The format of the ReadJP2Image method is:
%
%      Image *ReadJP2Image(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/
#if defined(HasJP2)

typedef struct _StreamManager
{
  jas_stream_t
    *stream;

  Image
    *image;
} StreamManager;

static int BlobRead(jas_stream_obj_t *object,char *buffer,const int length)
{
  ssize_t
    count;

  StreamManager
    *source;

  source=(StreamManager *) object;
  count=ReadBlob(source->image,(size_t) length,(void *) buffer);
  return((int) count);
}

static int BlobWrite(jas_stream_obj_t *object,char *buffer,const int length)
{
  size_t
    count;

  StreamManager
    *source;

  source=(StreamManager *) object;
  count=WriteBlob(source->image,(size_t) length,(void *) buffer);
  return((int) count);
}

static long BlobSeek(jas_stream_obj_t *object,long offset,int origin)
{
  StreamManager
    *source;

  source=(StreamManager *) object;
  return(SeekBlob(source->image,offset,origin));
}

static int BlobClose(jas_stream_obj_t *object)
{
  StreamManager
    *source;

  source=(StreamManager *) object;
  CloseBlob(source->image);
  return(0);
}

static jas_stream_ops_t
  StreamOperators =
  {
    BlobRead,
    BlobWrite,
    BlobSeek,
    BlobClose
  };

static jas_stream_t *JP2StreamManager(Image *image)
{
  jas_stream_t
    *stream;

  StreamManager
    *source;

  stream=(jas_stream_t *) malloc(sizeof(*stream));
  if (stream == (jas_stream_t *) NULL)
    return((jas_stream_t *) NULL);
  (void) ResetMagickMemory(stream,0,sizeof(*stream));
  stream->rwlimit_=(-1);
  stream->obj_=(jas_stream_obj_t *) malloc(sizeof(StreamManager));
  if (stream->obj_ == (jas_stream_obj_t *) NULL)
    return((jas_stream_t *) NULL);
  (void) ResetMagickMemory(stream->obj_,0,sizeof(StreamManager));
  stream->ops_=(&StreamOperators);
  stream->openmode_=JAS_STREAM_READ | JAS_STREAM_WRITE | JAS_STREAM_BINARY;
  stream->bufbase_=stream->tinybuf_;
  stream->bufsize_=1;
  stream->bufstart_=(&stream->bufbase_[JAS_STREAM_MAXPUTBACK]);
  stream->ptr_=stream->bufstart_;
  stream->bufmode_|=JAS_STREAM_UNBUF & JAS_STREAM_BUFMODEMASK;
  source=(StreamManager *) stream->obj_;
  source->image=image;
  return(stream);
}

static Image *ReadJP2Image(const ImageInfo *image_info,ExceptionInfo *exception)
{
  Image
    *image;

  long
    y;

  jas_cmprof_t
    *cm_profile;

  jas_iccprof_t
    *icc_profile;

  jas_image_t
    *jp2_image;

  jas_matrix_t
    *pixels[4];

  jas_stream_t
    *jp2_stream;

  MagickBooleanType
    status;

  register long
    i,
    x;

  register PixelPacket
    *q;

  int
    components[4];

  unsigned int
    maximum_component_depth,
    scale[4],
    x_step[4],
    y_step[4];

  unsigned long
    number_components;

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
    Initialize JPEG 2000 API.
  */
  jp2_stream=JP2StreamManager(image);
  if (jp2_stream == (jas_stream_t *) NULL)
    ThrowReaderException(DelegateError,"UnableToManageJP2Stream");
  jp2_image=jas_image_decode(jp2_stream,-1,0);
  if (jp2_image == (jas_image_t *) NULL)
    {
      (void) jas_stream_close(jp2_stream);
      ThrowReaderException(DelegateError,"UnableToDecodeImageFile");
    }
  switch (jas_clrspc_fam(jas_image_clrspc(jp2_image)))
  {
    case JAS_CLRSPC_FAM_RGB:
    {
      components[0]=jas_image_getcmptbytype(jp2_image,JAS_IMAGE_CT_RGB_R);
      components[1]=jas_image_getcmptbytype(jp2_image,JAS_IMAGE_CT_RGB_G);
      components[2]=jas_image_getcmptbytype(jp2_image,JAS_IMAGE_CT_RGB_B);
      if ((components[0] < 0) || (components[1] < 0) || (components[2] < 0))
        {
          (void) jas_stream_close(jp2_stream);
          jas_image_destroy(jp2_image);
          ThrowReaderException(CorruptImageError,"MissingImageChannel");
        }
      number_components=3;
      components[3]=jas_image_getcmptbytype(jp2_image,3);
      if (components[3] > 0)
        {
          image->matte=MagickTrue;
          number_components++;
        }
      break;
    }
    case JAS_CLRSPC_FAM_GRAY:
    {
      components[0]=jas_image_getcmptbytype(jp2_image,JAS_IMAGE_CT_GRAY_Y);
      if (components[0] < 0)
        {
          (void) jas_stream_close(jp2_stream);
          jas_image_destroy(jp2_image);
          ThrowReaderException(CorruptImageError,"MissingImageChannel");
        }
      number_components=1;
      break;
    }
    case JAS_CLRSPC_FAM_YCBCR:
    {
      components[0]=jas_image_getcmptbytype(jp2_image,JAS_IMAGE_CT_YCBCR_Y);
      components[1]=jas_image_getcmptbytype(jp2_image,JAS_IMAGE_CT_YCBCR_CB);
      components[2]=jas_image_getcmptbytype(jp2_image,JAS_IMAGE_CT_YCBCR_CR);
      if ((components[0] < 0) || (components[1] < 0) || (components[2] < 0))
        {
          (void) jas_stream_close(jp2_stream);
          jas_image_destroy(jp2_image);
          ThrowReaderException(CorruptImageError,"MissingImageChannel");
        }
      number_components=3;
      components[3]=jas_image_getcmptbytype(jp2_image,JAS_IMAGE_CT_UNKNOWN);
      if (components[3] > 0)
        {
          image->matte=MagickTrue;
          number_components++;
        }
      image->colorspace=YCbCrColorspace;
      break;
    }
    default:
    {
      (void) jas_stream_close(jp2_stream);
      jas_image_destroy(jp2_image);
      ThrowReaderException(CoderError,"ColorspaceModelIsNotSupported");
    }
  }
  image->columns=jas_image_width(jp2_image);
  image->rows=jas_image_height(jp2_image);
  for (i=0; i < (long) number_components; i++)
  {
    if ((((jas_image_cmptwidth(jp2_image,components[i])*
          jas_image_cmpthstep(jp2_image,components[i])) != image->columns)) ||
        (((jas_image_cmptheight(jp2_image,components[i])*
          jas_image_cmptvstep(jp2_image,components[i])) != image->rows)) ||
        (jas_image_cmpttlx(jp2_image,components[i]) != 0) ||
        (jas_image_cmpttly(jp2_image,components[i]) != 0) ||
        (jas_image_cmptsgnd(jp2_image,components[i]) != MagickFalse))
      {
        (void) jas_stream_close(jp2_stream);
        jas_image_destroy(jp2_image);
        ThrowReaderException(CoderError,"IrregularChannelGeometryNotSupported");
      }
    x_step[i]=jas_image_cmpthstep(jp2_image,components[i]);
    y_step[i]=jas_image_cmptvstep(jp2_image,components[i]);
  }
  /*
    Convert JPEG 2000 pixels.
  */
  if (image_info->ping != MagickFalse)
    {
      (void) jas_stream_close(jp2_stream);
      jas_image_destroy(jp2_image);
      return(GetFirstImageInList(image));
    }
  image->matte=number_components > 3;
  maximum_component_depth=0;
  for (i=0; i < (long) number_components; i++)
  {
    maximum_component_depth=Max(jas_image_cmptprec(jp2_image,components[i]),
      maximum_component_depth);
    pixels[i]=jas_matrix_create(1,((unsigned int) image->columns)/x_step[i]);
    if (pixels[i] == (jas_matrix_t *) NULL)
      {
        jas_image_destroy(jp2_image);
        ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
      }
  }
  if (maximum_component_depth <= 8)
    image->depth=Min(QuantumDepth,8);
  else
    image->depth=Min(QuantumDepth,16);
  for (i=0; i < (long) number_components; i++)
  {
    scale[i]=1;
    if (jas_image_cmptprec(jp2_image,components[i]) < 16)
      scale[i]=(1 << (16-jas_image_cmptprec(jp2_image,components[i])))+1;
  }
  if (number_components == 1)
    {
      image->storage_class=PseudoClass;
      image->colors=(image->depth == 8 ? 256 : MaxColormapSize);
      if (AllocateImageColormap(image,image->colors) == MagickFalse)
        ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    }
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (i=0; i < (long) number_components; i++)
      (void) jas_image_readcmpt(jp2_image,(short) components[i],0,
        ((unsigned int) y)/y_step[i],((unsigned int) image->columns)/x_step[i],
        1,pixels[i]);
    switch (number_components)
    {
      case 1:
      {
        register IndexPacket
          *indexes;

        IndexPacket
          index;

        /*
          Grayscale.
        */
        indexes=GetIndexes(image);
        if (image->depth == 8)
          for (x=0; x < (long) image->columns; x++)
          {
            index=ScaleQuantumToChar(ScaleShortToQuantum((unsigned short)
              (scale[0]*jas_matrix_getv(pixels[0],x))));
            index=ConstrainColormapIndex(image,index);
            *indexes++=index;
            *q++=image->colormap[index];
          }
        else
          for (x=0; x < (long) image->columns; x++)
          {
            index=(unsigned short) (scale[0]*
              jas_matrix_getv(pixels[0],x/x_step[0]));
            index=ConstrainColormapIndex(image,index);
            *indexes++=index;
            *q++=image->colormap[index];
          }
        break;
      }
      case 3:
      {
        /*
          RGB.
        */
        for (x=0; x < (long) image->columns; x++)
        {
          q->red=ScaleShortToQuantum((unsigned short) (scale[0]*
            jas_matrix_getv(pixels[0],x/x_step[0])));
          q->green=ScaleShortToQuantum((unsigned short) (scale[1]*
            jas_matrix_getv(pixels[1],x/x_step[1])));
          q->blue=ScaleShortToQuantum((unsigned short) (scale[2]*
            jas_matrix_getv(pixels[2],x/x_step[2])));
          q++;
        }
        break;
      }
      case 4:
      {
        /*
          RGBA.
        */
        for (x=0; x < (long) image->columns; x++)
        {
          q->red=ScaleShortToQuantum((unsigned short) (scale[0]*
            jas_matrix_getv(pixels[0],x/x_step[0])));
          q->green=ScaleShortToQuantum((unsigned short) (scale[1]*
            jas_matrix_getv(pixels[1],x/x_step[1])));
          q->blue=ScaleShortToQuantum((unsigned short) (scale[2]*
            jas_matrix_getv(pixels[2],x/x_step[2])));
          q->opacity=QuantumRange-ScaleShortToQuantum((unsigned short) (scale[3]*
            jas_matrix_getv(pixels[3],x/x_step[3])));
          q++;
        }
        break;
      }
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if (image->previous == (Image *) NULL)
     if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(y,image->rows) != MagickFalse))
        {
          status=image->progress_monitor(LoadImagesTag,y,image->rows,
            image->client_data);
          if (status == MagickFalse)
            break;
        }
  }
  cm_profile=jas_image_cmprof(jp2_image);
  icc_profile=(jas_iccprof_t *) NULL;
  if (cm_profile != (jas_cmprof_t *) NULL)
    icc_profile=jas_iccprof_createfromcmprof(cm_profile);
  if (icc_profile != (jas_iccprof_t *) NULL)
    {
      jas_stream_t
        *icc_stream;

      icc_stream=jas_stream_memopen(NULL,0);
      if ((icc_stream != (jas_stream_t *) NULL) &&
          (jas_iccprof_save(icc_profile,icc_stream) == 0) &&
          (jas_stream_flush(icc_stream) == 0))
        {
          StringInfo
            *icc_profile,
            *profile;

          jas_stream_memobj_t
            *blob;

          /*
            Extract the icc profile, handle errors without much noise.
          */
          blob=(jas_stream_memobj_t *) icc_stream->obj_;
          if (image->debug != MagickFalse)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "Profile: ICC, %lu bytes",(unsigned long) blob->len_);
          profile=AcquireStringInfo(blob->len_);
          SetStringInfoDatum(profile,blob->buf_);
          icc_profile=(StringInfo *) GetImageProfile(image,"icc");
          if (icc_profile == (StringInfo *) NULL)
            SetImageProfile(image,"icc",profile);
          else
            ConcatenateStringInfo(icc_profile,profile);
          profile=DestroyStringInfo(profile);
          (void) jas_stream_close(icc_stream);
        }
    }
  (void) jas_stream_close(jp2_stream);
  jas_image_destroy(jp2_image);
  for (i=0; i < (long) number_components; i++)
    jas_matrix_destroy(pixels[i]);
  return(GetFirstImageInList(image));
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r J P 2 I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterJP2Image() adds attributes for the JP2 image format to the list of
%  supported formats.  The attributes include the image format tag, a method
%  method to read and/or write the format, whether the format supports the
%  saving of more than one frame to the same file or blob, whether the format
%  supports native in-memory I/O, and a brief description of the format.
%
%  The format of the RegisterJP2Image method is:
%
%      RegisterJP2Image(void)
%
*/
ModuleExport void RegisterJP2Image(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("JP2");
  entry->description=ConstantString("JPEG-2000 File Format Syntax");
  entry->module=ConstantString("JP2");
  entry->magick=(MagickHandler *) IsJP2;
  entry->adjoin=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=MagickFalse;
#if defined(HasJP2)
  entry->decoder=(DecoderHandler *) ReadJP2Image;
  entry->encoder=(EncoderHandler *) WriteJP2Image;
#endif
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("JPC");
  entry->description=ConstantString("JPEG-2000 Code Stream Syntax");
  entry->module=ConstantString("JPC");
  entry->magick=(MagickHandler *) IsJPC;
  entry->adjoin=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=MagickFalse;
#if defined(HasJP2)
  entry->decoder=(DecoderHandler *) ReadJP2Image;
  entry->encoder=(EncoderHandler *) WriteJP2Image;
#endif
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("JPX");
  entry->description=ConstantString("JPEG-2000 File Format Syntax");
  entry->module=ConstantString("JPX");
  entry->magick=(MagickHandler *) IsJPC;
  entry->adjoin=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=MagickFalse;
#if defined(HasJP2)
  entry->decoder=(DecoderHandler *) ReadJP2Image;
  entry->encoder=(EncoderHandler *) WriteJP2Image;
#endif
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PGX");
  entry->description=ConstantString("JPEG-2000 VM Format");
  entry->module=ConstantString("PGX");
  entry->magick=(MagickHandler *) IsJPC;
  entry->adjoin=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->thread_support=MagickFalse;
#if defined(HasJP2)
  entry->decoder=(DecoderHandler *) ReadJP2Image;
#endif
  (void) RegisterMagickInfo(entry);
#if defined(HasJP2)
  jas_init();
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r J P 2 I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterJP2Image() removes format registrations made by the JP2 module
%  from the list of supported formats.
%
%  The format of the UnregisterJP2Image method is:
%
%      UnregisterJP2Image(void)
%
*/
ModuleExport void UnregisterJP2Image(void)
{
  (void) UnregisterMagickInfo("JP2");
  (void) UnregisterMagickInfo("JPC");
  (void) UnregisterMagickInfo("PGX");
#if defined(HasJP2)
  jas_cleanup();
#endif
}

#if defined(HasJP2)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e J P 2 I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteJP2Image() writes an image in the JPEG 2000 image format.
%
%  JP2 support originally written by Nathan Brown, nathanbrown@letu.edu
%
%  The format of the WriteJP2Image method is:
%
%      MagickBooleanType WriteJP2Image(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteJP2Image(const ImageInfo *image_info,Image *image)
{
  char
    *key,
    magick[MaxTextExtent],
    *options;

  const char
    *option;

  int
    format;

  long
    y;

  jas_image_cmptparm_t
    component_info[4];

  jas_image_t
    *jp2_image;

  jas_matrix_t
    *pixels[4];

  jas_stream_t
    *jp2_stream;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  unsigned long
    number_components;

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
  /*
    Intialize JPEG 2000 API.
  */
  if (image_info->colorspace == UndefinedColorspace)
    (void) SetImageColorspace(image,RGBColorspace);
  jp2_stream=JP2StreamManager(image);
  if (jp2_stream == (jas_stream_t *) NULL)
    ThrowWriterException(DelegateError,"UnableToManageJP2Stream");
  number_components=image->matte ? 4 : 3;
  if ((image_info->type != TrueColorType) &&
      IsGrayImage(image,&image->exception))
    number_components=1;
  for (i=0; i < (long) number_components; i++)
  {
    component_info[i].tlx=0;
    component_info[i].tly=0;
    component_info[i].hstep=1;
    component_info[i].vstep=1;
    component_info[i].width=(unsigned int) image->columns;
    component_info[i].height=(unsigned int) image->rows;
    component_info[i].prec=(unsigned int) image->depth <= 8 ? 8 : 16;
    component_info[i].sgnd=MagickFalse;
  }
  jp2_image=jas_image_create(number_components,component_info,
    JAS_CLRSPC_UNKNOWN);
  if (jp2_image == (jas_image_t *) NULL)
    ThrowWriterException(DelegateError,"UnableToCreateImage");
  if (number_components == 1)
    {
      /*
        sRGB Grayscale.
      */
      jas_image_setclrspc(jp2_image,JAS_CLRSPC_SGRAY);
      jas_image_setcmpttype(jp2_image,0,
        JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y));
    }
  else
    {
      /*
        sRGB.
      */
      if (number_components == 4 )
        jas_image_setcmpttype(jp2_image,3,JAS_IMAGE_CT_OPACITY);
      jas_image_setclrspc(jp2_image,JAS_CLRSPC_SRGB);
      jas_image_setcmpttype(jp2_image,0,
        JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_R));
      jas_image_setcmpttype(jp2_image,1,
        JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_G));
      jas_image_setcmpttype(jp2_image,2,
        JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_B));
    }
  /*
    Convert to JPEG 2000 pixels.
  */
  for (i=0; i < (long) number_components; i++)
  {
    pixels[i]=jas_matrix_create(1,(unsigned int) image->columns);
    if (pixels[i] == (jas_matrix_t *) NULL)
      {
        for (x=0; x < i; x++)
          jas_matrix_destroy(pixels[x]);
        jas_image_destroy(jp2_image);
        ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
      }
  }
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    if (image->depth <= 8)
      for (x=0; x < (long) image->columns; x++)
      {
        if (number_components == 1)
          jas_matrix_setv(pixels[0],x,
            ScaleQuantumToChar(PixelIntensityToQuantum(p)));
        else
          {
            jas_matrix_setv(pixels[0],x,ScaleQuantumToChar(p->red));
            jas_matrix_setv(pixels[1],x,ScaleQuantumToChar(p->green));
            jas_matrix_setv(pixels[2],x,ScaleQuantumToChar(p->blue));
            if (number_components > 3)
              jas_matrix_setv(pixels[3],x,
                ScaleQuantumToChar((Quantum) (QuantumRange-p->opacity)));
          }
        p++;
      }
    else
      for (x=0; x < (long) image->columns; x++)
      {
        if (number_components == 1)
          jas_matrix_setv(pixels[0],x,
            ScaleQuantumToShort(PixelIntensityToQuantum(p)));
        else
          {
            jas_matrix_setv(pixels[0],x,ScaleQuantumToShort(p->red));
            jas_matrix_setv(pixels[1],x,ScaleQuantumToShort(p->green));
            jas_matrix_setv(pixels[2],x,ScaleQuantumToShort(p->blue));
            if (number_components > 3)
              jas_matrix_setv(pixels[3],x,
                ScaleQuantumToShort((Quantum) (QuantumRange-p->opacity)));
          }
        p++;
      }
    for (i=0; i < (long) number_components; i++)
      (void) jas_image_writecmpt(jp2_image,(short) i,0,(unsigned int) y,
        (unsigned int) image->columns,1,pixels[i]);
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
  (void) CopyMagickString(magick,image_info->magick,MaxTextExtent);
  LocaleLower(magick);
  format=jas_image_strtofmt(magick);
  options=(char *) NULL;
  ResetImageOptionIterator(image_info);
  key=GetNextImageOption(image_info);
  while (key != (char *) NULL)
  {
    option=GetImageOption(image_info,key);
    if (option != (const char *) NULL)
      {
        if (LocaleNCompare(key,"jp2:",4) == 0)
          {
            ConcatenateString(&options,key+4);
            ConcatenateString(&options,"=");
            ConcatenateString(&options,option);
            ConcatenateString(&options," ");
          }
       }
    key=GetNextImageOption(image_info);
  }
  option=GetImageOption(image_info,"jp2:rate");
  if ((option == (const char *) NULL) &&
      (image->compression != LosslessJPEGCompression) &&
      (image->quality != UndefinedCompressionQuality) &&
      (image->quality < 99.5) && ((image->rows*image->columns) > 2500))
    {
      char
        option[MaxTextExtent];

      double
        alpha,
        header_size,
        number_pixels,
        rate,
        target_size;

      alpha=115-image->quality;
      rate=100.0/(alpha*alpha);
      header_size=550.0;
      header_size+=(number_components-1)*142;
      number_pixels=(double) image->rows*image->columns*number_components*
        (GetImageQuantumDepth(image,MagickTrue)/8);
      target_size=(number_pixels*rate)+header_size;
      rate=target_size/number_pixels;
      FormatMagickString(option,MaxTextExtent,"rate=%g",rate);
      ConcatenateString(&options,option);
    }
  status=jas_image_encode(jp2_image,jp2_stream,format,options);
  (void) jas_stream_close(jp2_stream);
  for (i=0; i < (long) number_components; i++)
    jas_matrix_destroy(pixels[i]);
  jas_image_destroy(jp2_image);
  if (status != MagickFalse)
    ThrowWriterException(DelegateError,"UnableToEncodeImageFile");
  return(MagickTrue);
}
#endif
e                                                                                                                                                                                                                                                    