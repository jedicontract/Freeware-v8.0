/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        JJJJJ  PPPP   EEEEE   GGGG                           %
%                          J    P   P  E      G                               %
%                          J    PPPP   EEE    G  GG                           %
%                        J J    P      E      G   G                           %
%                        JJJ    P      EEEEE   GGG                            %
%                                                                             %
%                                                                             %
%                       Read/Write JPEG Image Format.                         %
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
% This software is based in part on the work of the Independent JPEG Group.
% See ftp://ftp.uu.net/graphics/jpeg/jpegsrc.v6b.tar.gz for copyright and
% licensing restrictions.  Blob support contributed by Glenn Randers-Pehrson.
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
#include "magick/colorspace.h"
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/option.h"
#include "magick/profile.h"
#include "magick/resource_.h"
#include "magick/splay-tree.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/utility.h"
#include <setjmp.h>
#if defined(HasJPEG)
#define JPEG_INTERNAL_OPTIONS
#if defined(__MINGW32__)
# define XMD_H 1  /* Avoid conflicting typedef for INT32 */
#endif
#undef HAVE_STDLIB_H
#include "jpeglib.h"
#include "jerror.h"
#endif

/*
  Define declarations.
*/
#define ICC_MARKER  (JPEG_APP0+2)
#define ICC_PROFILE  "ICC_PROFILE"
#define IPTC_MARKER  (JPEG_APP0+13)
#define XML_MARKER  (JPEG_APP0+1)
#define MaxBufferExtent  8192

/*
  Typedef declarations.
*/
#if defined(HasJPEG)
typedef struct _DestinationManager
{
  struct jpeg_destination_mgr
    manager;

  Image
    *image;

  JOCTET
    *buffer;
} DestinationManager;

typedef struct _ErrorManager
{
  Image
    *image;

  jmp_buf
    error_recovery;
} ErrorManager;

typedef struct _SourceManager
{
  struct jpeg_source_mgr
    manager;

  Image
    *image;

  JOCTET
    *buffer;

  boolean
    start_of_blob;
} SourceManager;
#endif

/*
  Forward declarations.
*/
#if defined(HasJPEG)
static MagickBooleanType
  WriteJPEGImage(const ImageInfo *,Image *);
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s J P E G                                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsJPEG() returns MagickTrue if the image format type, identified by the
%  magick string, is JPEG.
%
%  The format of the IsJPEG  method is:
%
%      MagickBooleanType IsJPEG(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsJPEG(const unsigned char *magick,const size_t length)
{
  if (length < 3)
    return(MagickFalse);
  if (memcmp(magick,"\377\330\377",3) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

#if defined(HasJPEG)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d J P E G I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadJPEGImage() reads a JPEG image file and returns it.  It allocates
%  the memory necessary for the new Image structure and returns a pointer to
%  the new image.
%
%  The format of the ReadJPEGImage method is:
%
%      Image *ReadJPEGImage(const ImageInfo *image_info,
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

static MagickBooleanType EmitMessage(j_common_ptr jpeg_info,int level)
{
  char
    message[JMSG_LENGTH_MAX];

  ErrorManager
    *error_manager;

  Image
    *image;

  (jpeg_info->err->format_message)(jpeg_info,message);
  error_manager=(ErrorManager *) jpeg_info->client_data;
  image=error_manager->image;
  if (level < 0)
    {
      if ((jpeg_info->err->num_warnings == 0) ||
          (jpeg_info->err->trace_level >= 3))
        ThrowBinaryException(CorruptImageWarning,(char *) message,
          image->filename);
      jpeg_info->err->num_warnings++;
    }
  else
    if (jpeg_info->err->trace_level >= level)
      ThrowBinaryException(CoderError,(char *) message,image->filename);
  return(MagickTrue);
}

static boolean FillInputBuffer(j_decompress_ptr cinfo)
{
  SourceManager
    *source;

  source=(SourceManager *) cinfo->src;
  source->manager.bytes_in_buffer=(size_t)
    ReadBlob(source->image,MaxBufferExtent,source->buffer);
  if (source->manager.bytes_in_buffer == 0)
    {
      if (source->start_of_blob != 0)
        ERREXIT(cinfo,JERR_INPUT_EMPTY);
      WARNMS(cinfo,JWRN_JPEG_EOF);
      source->buffer[0]=(JOCTET) 0xff;
      source->buffer[1]=(JOCTET) JPEG_EOI;
      source->manager.bytes_in_buffer=2;
    }
  source->manager.next_input_byte=source->buffer;
  source->start_of_blob=FALSE;
  return(TRUE);
}

static int GetCharacter(j_decompress_ptr jpeg_info)
{
  if (jpeg_info->src->bytes_in_buffer == 0)
    (void) (*jpeg_info->src->fill_input_buffer)(jpeg_info);
  jpeg_info->src->bytes_in_buffer--;
  return((int) GETJOCTET(*jpeg_info->src->next_input_byte++));
}

static void InitializeSource(j_decompress_ptr cinfo)
{
  SourceManager
    *source;

  source=(SourceManager *) cinfo->src;
  source->start_of_blob=TRUE;
}

static void JPEGErrorHandler(j_common_ptr jpeg_info)
{
  ErrorManager
    *error_manager;

  (void) EmitMessage(jpeg_info,0);
  error_manager=(ErrorManager *) jpeg_info->client_data;
  longjmp(error_manager->error_recovery,1);
}

static boolean ReadComment(j_decompress_ptr jpeg_info)
{
  char
    *comment;

  ErrorManager
    *error_manager;

  Image
    *image;

  register char
    *p;

  register long
    i;

  size_t
    length;

  /*
    Determine length of comment.
  */
  error_manager=(ErrorManager *) jpeg_info->client_data;
  image=error_manager->image;
  length=(size_t) ((unsigned long) GetCharacter(jpeg_info) << 8);
  length+=GetCharacter(jpeg_info);
  length-=2;
  if (length <= 0)
    return(MagickTrue);
  comment=(char *) AcquireMagickMemory(length+MaxTextExtent);
  if (comment == (char *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  /*
    Read comment.
  */
  i=(long) length-1;
  for (p=comment; i-- >= 0; p++)
    *p=(char) GetCharacter(jpeg_info);
  *p='\0';
  (void) SetImageAttribute(image,"Comment",comment);
  comment=(char *) RelinquishMagickMemory(comment);
  return(MagickTrue);
}

static boolean ReadICCProfile(j_decompress_ptr jpeg_info)
{
  char
    magick[12];

  ErrorManager
    *error_manager;

  Image
    *image;

  MagickBooleanType
    status;

  register long
    i;

  register unsigned char
    *p;

  size_t
    length;

  StringInfo
    *icc_profile,
    *profile;

  /*
    Read color profile.
  */
  length=(size_t) ((unsigned long) GetCharacter(jpeg_info) << 8);
  length+=(size_t) GetCharacter(jpeg_info);
  length-=2;
  if (length <= 14)
    {
      while (length-- > 0)
        (void) GetCharacter(jpeg_info);
      return(MagickTrue);
    }
  for (i=0; i < 12; i++)
    magick[i]=(char) GetCharacter(jpeg_info);
  if (LocaleCompare(magick,ICC_PROFILE) != 0)
    {
      /*
        Not a ICC profile, return.
      */
      for (i=0; i < (long) (length-12); i++)
        (void) GetCharacter(jpeg_info);
      return(MagickTrue);
    }
  (void) GetCharacter(jpeg_info);  /* id */
  (void) GetCharacter(jpeg_info);  /* markers */
  length-=14;
  error_manager=(ErrorManager *) jpeg_info->client_data;
  image=error_manager->image;
  profile=AcquireStringInfo(length);
  if (profile == (StringInfo *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  p=profile->datum;
  for (i=(long) profile->length-1; i >= 0; i--)
    *p++=(unsigned char) GetCharacter(jpeg_info);
  icc_profile=(StringInfo *) GetImageProfile(image,"icc");
  if (icc_profile != (StringInfo *) NULL)
    {
      ConcatenateStringInfo(icc_profile,profile);
      profile=DestroyStringInfo(profile);
    }
  else
    {
      status=SetImageProfile(image,"icc",profile);
      profile=DestroyStringInfo(profile);
      if (status == MagickFalse)
        ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
          image->filename);
    }
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "Profile: ICC, %lu bytes",(unsigned long) length);
  return(MagickTrue);
}

static boolean ReadIPTCProfile(j_decompress_ptr jpeg_info)
{
  char
    magick[MaxTextExtent];

  ErrorManager
    *error_manager;

  Image
    *image;

  MagickBooleanType
    status;

  register long
    i;

  register unsigned char
    *p;

  size_t
    length,
    tag_length;

  StringInfo
    *iptc_profile,
    *profile;

  /*
    Determine length of binary data stored here.
  */
  length=(size_t) ((unsigned long) GetCharacter(jpeg_info) << 8);
  length+=(size_t) GetCharacter(jpeg_info);
  if (length <= 2)
    return(MagickTrue);
  length-=2;
  tag_length=0;
  error_manager=(ErrorManager *) jpeg_info->client_data;
  image=error_manager->image;
  iptc_profile=(StringInfo *) GetImageProfile(image,"iptc");
  if (iptc_profile == (StringInfo *) NULL)
    {
      /*
        Validate that this was written as a Photoshop resource format slug.
      */
      for (i=0; i < 10; i++)
        magick[i]=(char) GetCharacter(jpeg_info);
      magick[10]='\0';
      if (length <= 10)
        return(MagickTrue);
      length-=10;
      if (LocaleCompare(magick,"Photoshop ") != 0)
        {
          /*
            Not a IPTC profile, return.
          */
          for (i=0; i < (long) length; i++)
            (void) GetCharacter(jpeg_info);
          return(MagickTrue);
        }
      /*
        Remove the version number.
      */
      for (i=0; i < 4; i++)
        (void) GetCharacter(jpeg_info);
      if (length <= 4)
        return(MagickTrue);
      length-=4;
      tag_length=0;
    }
  if (length == 0)
    return(MagickTrue);
  profile=AcquireStringInfo(length);
  if (profile == (StringInfo *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  p=profile->datum;
  for (i=(long) profile->length-1; i >= 0; i--)
    *p++=(unsigned char) GetCharacter(jpeg_info);
  iptc_profile=(StringInfo *) GetImageProfile(image,"iptc");
  if (iptc_profile != (StringInfo *) NULL)
    {
      ConcatenateStringInfo(iptc_profile,profile);
      profile=DestroyStringInfo(profile);
    }
  else
    {
      status=SetImageProfile(image,"iptc",profile);
      profile=DestroyStringInfo(profile);
      if (status == MagickFalse)
        ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
          image->filename);
    }
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "Profile: iptc, %lu bytes",(unsigned long) length);
  return(MagickTrue);
}

static boolean ReadProfile(j_decompress_ptr jpeg_info)
{
  char
    name[MaxTextExtent];

  ErrorManager
    *error_manager;

  Image
    *image;

  int
    marker;

  MagickBooleanType
    status;

  register long
    i;

  register unsigned char
    *p;

  size_t
    length;

  StringInfo
    *profile;

  /*
    Read generic profile.
  */
  length=(size_t) ((unsigned long) GetCharacter(jpeg_info) << 8);
  length+=(size_t) GetCharacter(jpeg_info);
  if (length <= 2)
    return(MagickTrue);
  length-=2;
  marker=jpeg_info->unread_marker-JPEG_APP0;
  error_manager=(ErrorManager *) jpeg_info->client_data;
  image=error_manager->image;
  (void) FormatMagickString(name,MaxTextExtent,"APP%d",marker);
  profile=AcquireStringInfo(length);
  if (profile == (StringInfo *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  p=profile->datum;
  for (i=(long) profile->length-1; i >= 0; i--)
    *p++=(unsigned char) GetCharacter(jpeg_info);
  if (marker == 1)
    {
      p=profile->datum;
      if ((length > 4) && (LocaleNCompare((char *) p,"exif",4) == 0))
        (void) CopyMagickString(name,"exif",MaxTextExtent);
      if ((length > 5) && (LocaleNCompare((char *) p,"http:",5) == 0))
        (void) CopyMagickString(name,"xmp",MaxTextExtent);
    }
  status=SetImageProfile(image,name,profile);
  profile=DestroyStringInfo(profile);
  if (status == MagickFalse)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "Profile: %s, %lu bytes",name,(unsigned long) length);
  return(MagickTrue);
}

static void SkipInputData(j_decompress_ptr cinfo,long number_bytes)
{
  SourceManager
    *source;

  if (number_bytes <= 0)
    return;
  source=(SourceManager *) cinfo->src;
  while (number_bytes > (long) source->manager.bytes_in_buffer)
  {
    number_bytes-=(long) source->manager.bytes_in_buffer;
    (void) FillInputBuffer(cinfo);
  }
  source->manager.next_input_byte+=(size_t) number_bytes;
  source->manager.bytes_in_buffer-=(size_t) number_bytes;
}

static void TerminateSource(j_decompress_ptr cinfo)
{
  cinfo=cinfo;
}

static void JPEGSourceManager(j_decompress_ptr cinfo,Image *image)
{
  SourceManager
    *source;

  cinfo->src=(struct jpeg_source_mgr *) (*cinfo->mem->alloc_small)
    ((j_common_ptr) cinfo,JPOOL_IMAGE,sizeof(SourceManager));
  source=(SourceManager *) cinfo->src;
  source->buffer=(JOCTET *) (*cinfo->mem->alloc_small)
    ((j_common_ptr) cinfo,JPOOL_IMAGE,MaxBufferExtent*sizeof(JOCTET));
  source=(SourceManager *) cinfo->src;
  source->manager.init_source=InitializeSource;
  source->manager.fill_input_buffer=FillInputBuffer;
  source->manager.skip_input_data=SkipInputData;
  source->manager.resync_to_restart=jpeg_resync_to_restart;
  source->manager.term_source=TerminateSource;
  source->manager.bytes_in_buffer=0;
  source->manager.next_input_byte=NULL;
  source->image=image;
}

static Image *ReadJPEGImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  char
    sampling_factors[MaxTextExtent],
    value[MaxTextExtent];

  ErrorManager
    error_manager;

  Image
    *image;

  long
    x,
    y;

  JSAMPLE
    *jpeg_pixels;

  JSAMPROW
    scanline[1];

  MagickBooleanType
    debug,
    status;

  MagickSizeType
    number_pixels;

  register IndexPacket
    *indexes;

  register long
    i;

  struct jpeg_decompress_struct
    jpeg_info;

  struct jpeg_error_mgr
    jpeg_error;

  register JSAMPLE
    *p;

  register PixelPacket
    *q;

  unsigned long
    index,
    units;

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
  debug=IsEventLogging();
  image=AllocateImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Initialize image structure.
  */
  jpeg_info.err=jpeg_std_error(&jpeg_error);
  jpeg_info.err->emit_message=(void (*)(j_common_ptr,int)) EmitMessage;
  jpeg_info.err->error_exit=(void (*)(j_common_ptr)) JPEGErrorHandler;
  jpeg_pixels=(JSAMPLE *) NULL;
  error_manager.image=image;
  if (setjmp(error_manager.error_recovery) != 0)
    {
      jpeg_destroy_decompress(&jpeg_info);
      InheritException(exception,&image->exception);
      CloseBlob(image);
      number_pixels=(MagickSizeType) image->columns*image->rows;
      if (number_pixels != 0)
        return(GetFirstImageInList(image));
      image=DestroyImage(image);
      return(image);
    }
  jpeg_info.client_data=(void *) &error_manager;
  jpeg_create_decompress(&jpeg_info);
  JPEGSourceManager(&jpeg_info,image);
  jpeg_set_marker_processor(&jpeg_info,JPEG_COM,ReadComment);
  jpeg_set_marker_processor(&jpeg_info,ICC_MARKER,ReadICCProfile);
  jpeg_set_marker_processor(&jpeg_info,IPTC_MARKER,ReadIPTCProfile);
  for (i=1; i < 16; i++)
    if ((i != 2) && (i != 13) && (i != 14))
      jpeg_set_marker_processor(&jpeg_info,(int) (JPEG_APP0+i),ReadProfile);
  i=jpeg_read_header(&jpeg_info,MagickTrue);
  if ((image_info->colorspace == YCbCrColorspace) ||
      (image_info->colorspace == Rec601YCbCrColorspace) ||
      (image_info->colorspace == Rec709YCbCrColorspace))
    {
      jpeg_info.out_color_space=JCS_YCbCr;
      image->colorspace=YCbCrColorspace;
    }
  if (jpeg_info.out_color_space == JCS_CMYK)
    image->colorspace=CMYKColorspace;
  /*
    Set image resolution.
  */
  units=0;
  if ((jpeg_info.saw_JFIF_marker != 0) && (jpeg_info.X_density != 1) &&
      (jpeg_info.Y_density != 1))
    {
      image->x_resolution=(double) jpeg_info.X_density;
      image->y_resolution=(double) jpeg_info.Y_density;
      units=(unsigned long) jpeg_info.density_unit;
    }
  if (units == 1)
    image->units=PixelsPerInchResolution;
  if (units == 2)
    image->units=PixelsPerCentimeterResolution;
  number_pixels=(MagickSizeType) image->columns*image->rows;
  if (image_info->size != (char *) NULL)
    {
      double
        scale_factor;

      /*
        Let the JPEG library subsample for us.
      */
      jpeg_calc_output_dimensions(&jpeg_info);
      image->magick_columns=jpeg_info.output_width;
      image->magick_rows=jpeg_info.output_height;
      scale_factor=(double) jpeg_info.output_width/image->columns;
      if (scale_factor > ((double) jpeg_info.output_height/image->rows))
        scale_factor=(double) jpeg_info.output_height/image->rows;
      jpeg_info.scale_denom=(unsigned int) scale_factor;
      jpeg_calc_output_dimensions(&jpeg_info);
      if (image->debug != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Scale factor: %ld",
          (long) scale_factor);
    }
  if (image_info->subrange != 0)
    {
      jpeg_info.scale_denom=(unsigned int) image_info->subrange;
      jpeg_calc_output_dimensions(&jpeg_info);
    }
#if (JPEG_LIB_VERSION >= 61) && defined(D_PROGRESSIVE_SUPPORTED)
#ifdef D_LOSSLESS_SUPPORTED
  image->interlace=jpeg_info.process == JPROC_PROGRESSIVE ?
    PlaneInterlace : NoInterlace;
  image->compression=jpeg_info.process == JPROC_LOSSLESS ?
    LosslessJPEGCompression : JPEGCompression;
  if (jpeg_info.data_precision > 8)
    (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
      "12-bit JPEG not supported. Reducing pixel data to 8 bits","`%s'",
      image->filename);
#else
  image->interlace=jpeg_info.progressive_mode != 0 ?
    PlaneInterlace : NoInterlace;
  image->compression=JPEGCompression;
#endif
#else
  image->compression=JPEGCompression;
  image->interlace=PlaneInterlace;
#endif
  if ((image_info->colors != 0) && (image_info->colors <= 256))
    {
      /*
        Let the JPEG library quantize for us.
      */
      jpeg_info.quantize_colors=MagickTrue;
      jpeg_info.desired_number_of_colors=(int) image_info->colors;
      if (AllocateImageColormap(image,image_info->colors) == MagickFalse)
        ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    }
  (void) jpeg_start_decompress(&jpeg_info);
  image->columns=jpeg_info.output_width;
  image->rows=jpeg_info.output_height;
  image->depth=(unsigned long) jpeg_info.data_precision;
  if ((jpeg_info.output_components == 1) &&
      (jpeg_info.quantize_colors == MagickFalse))
    {
      if (AllocateImageColormap(image,1UL << image->depth) == MagickFalse)
        ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    }
  if (image->debug != MagickFalse)
    {
      if (image->interlace == PlaneInterlace)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Interlace: progressive");
      else
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Interlace: nonprogressive");
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Data precision: %d",
        (int) jpeg_info.data_precision);
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Geometry: %dx%d",
        (int) jpeg_info.output_width,(int) jpeg_info.output_height);
    }
  image->quality=UndefinedCompressionQuality;
#ifdef D_LOSSLESS_SUPPORTED
  if (image->compression == LosslessJPEGCompression)
    {
      image->quality=100;
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "Quality: 100 (lossless)");
    }
  else
#endif
  {
    long
      value,
      sum;

    register long
      j;

    /*
      Determine the JPEG compression quality from the quantization tables.
    */
    sum=0;
    for (i=0; i < NUM_QUANT_TBLS; i++)
    {
      if (jpeg_info.quant_tbl_ptrs[i] != NULL)
        for (j=0; j < DCTSIZE2; j++)
          sum+=jpeg_info.quant_tbl_ptrs[i]->quantval[j];
     }
     if ((jpeg_info.quant_tbl_ptrs[0] != NULL) &&
         (jpeg_info.quant_tbl_ptrs[1] != NULL))
       {
         long
           hash[101] =
           {
             1020, 1015,  932,  848,  780,  735,  702,  679,  660,  645,
              632,  623,  613,  607,  600,  594,  589,  585,  581,  571,
              555,  542,  529,  514,  494,  474,  457,  439,  424,  410,
              397,  386,  373,  364,  351,  341,  334,  324,  317,  309,
              299,  294,  287,  279,  274,  267,  262,  257,  251,  247,
              243,  237,  232,  227,  222,  217,  213,  207,  202,  198,
              192,  188,  183,  177,  173,  168,  163,  157,  153,  148,
              143,  139,  132,  128,  125,  119,  115,  108,  104,   99,
               94,   90,   84,   79,   74,   70,   64,   59,   55,   49,
               45,   40,   34,   30,   25,   20,   15,   11,    6,    4,
                0
           },
           sums[101] =
           {
             32640, 32635, 32266, 31495, 30665, 29804, 29146, 28599, 28104,
             27670, 27225, 26725, 26210, 25716, 25240, 24789, 24373, 23946,
             23572, 22846, 21801, 20842, 19949, 19121, 18386, 17651, 16998,
             16349, 15800, 15247, 14783, 14321, 13859, 13535, 13081, 12702,
             12423, 12056, 11779, 11513, 11135, 10955, 10676, 10392, 10208,
              9928,  9747,  9564,  9369,  9193,  9017,  8822,  8639,  8458,
              8270,  8084,  7896,  7710,  7527,  7347,  7156,  6977,  6788,
              6607,  6422,  6236,  6054,  5867,  5684,  5495,  5305,  5128,
              4945,  4751,  4638,  4442,  4248,  4065,  3888,  3698,  3509,
              3326,  3139,  2957,  2775,  2586,  2405,  2216,  2037,  1846,
              1666,  1483,  1297,  1109,   927,   735,   554,   375,   201,
               128,     0
           };

         value=(long) (jpeg_info.quant_tbl_ptrs[0]->quantval[2]+
           jpeg_info.quant_tbl_ptrs[0]->quantval[53]+
           jpeg_info.quant_tbl_ptrs[1]->quantval[0]+
           jpeg_info.quant_tbl_ptrs[1]->quantval[DCTSIZE2-1]);
         for (i=0; i < 100; i++)
         {
           if ((value < hash[i]) && (sum < sums[i]))
             continue;
           image->quality=(unsigned long) i+1;
           if (image->debug != MagickFalse)
             {
               if ((value > hash[i]) || (sum > sums[i]))
                 (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "Quality: %ld (approximate)",image->quality);
               else
                 (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                   "Quality: %ld",image->quality);
             }
           break;
         }
       }
     else
       if (jpeg_info.quant_tbl_ptrs[0] != NULL)
         {
           long
             hash[101] =
             {
               510,  505,  422,  380,  355,  338,  326,  318,  311,  305,
               300,  297,  293,  291,  288,  286,  284,  283,  281,  280,
               279,  278,  277,  273,  262,  251,  243,  233,  225,  218,
               211,  205,  198,  193,  186,  181,  177,  172,  168,  164,
               158,  156,  152,  148,  145,  142,  139,  136,  133,  131,
               129,  126,  123,  120,  118,  115,  113,  110,  107,  105,
               102,  100,   97,   94,   92,   89,   87,   83,   81,   79,
                76,   74,   70,   68,   66,   63,   61,   57,   55,   52,
                50,   48,   44,   42,   39,   37,   34,   31,   29,   26,
                24,   21,   18,   16,   13,   11,    8,    6,    3,    2,
                 0
             },
             sums[101] =
             {
               16320, 16315, 15946, 15277, 14655, 14073, 13623, 13230, 12859,
               12560, 12240, 11861, 11456, 11081, 10714, 10360, 10027,  9679,
                9368,  9056,  8680,  8331,  7995,  7668,  7376,  7084,  6823,
                6562,  6345,  6125,  5939,  5756,  5571,  5421,  5240,  5086,
                4976,  4829,  4719,  4616,  4463,  4393,  4280,  4166,  4092,
                3980,  3909,  3835,  3755,  3688,  3621,  3541,  3467,  3396,
                3323,  3247,  3170,  3096,  3021,  2952,  2874,  2804,  2727,
                2657,  2583,  2509,  2437,  2362,  2290,  2211,  2136,  2068,
                1996,  1915,  1858,  1773,  1692,  1620,  1552,  1477,  1398,
                1326,  1251,  1179,  1109,  1031,   961,   884,   814,   736,
                 667,   592,   518,   441,   369,   292,   221,   151,    86,
                  64,     0
             };

           value=(long) (jpeg_info.quant_tbl_ptrs[0]->quantval[2]+
             jpeg_info.quant_tbl_ptrs[0]->quantval[53]);
           for (i=0; i < 100; i++)
           {
             if ((value < hash[i]) && (sum < sums[i]))
               continue;
             image->quality=(unsigned long) i+1;
             if (image->debug != MagickFalse)
               {
                 if ((value > hash[i]) || (sum > sums[i]))
                   (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                     "Quality: %ld (approximate)",i+1);
                 else
                   (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                     "Quality: %ld",i+1);
               }
             break;
           }
         }
  }
  (void) FormatMagickString(value,MaxTextExtent,"%ld",(long)
    jpeg_info.out_color_space);
  (void) SetImageAttribute(image,"JPEG-Colorspace",value);
  /*
    Set image sampling factors.
  */
  switch (jpeg_info.out_color_space)
  {
    case JCS_CMYK:
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Colorspace: CMYK");
      (void) FormatMagickString(sampling_factors,MaxTextExtent,
        "%dx%d,%dx%d,%dx%d,%dx%d",jpeg_info.comp_info[0].h_samp_factor,
        jpeg_info.comp_info[0].v_samp_factor,
        jpeg_info.comp_info[1].h_samp_factor,
        jpeg_info.comp_info[1].v_samp_factor,
        jpeg_info.comp_info[2].h_samp_factor,
        jpeg_info.comp_info[2].v_samp_factor,
        jpeg_info.comp_info[3].h_samp_factor,
        jpeg_info.comp_info[3].v_samp_factor);
        break;
    }
    case JCS_GRAYSCALE:
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "Colorspace: GRAYSCALE");
      (void) FormatMagickString(sampling_factors,MaxTextExtent,"%dx%d",
        jpeg_info.comp_info[0].h_samp_factor,
        jpeg_info.comp_info[0].v_samp_factor);
      break;
    }
    case JCS_RGB:
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Colorspace: RGB");
      (void) FormatMagickString(sampling_factors,MaxTextExtent,
        "%dx%d,%dx%d,%dx%d",jpeg_info.comp_info[0].h_samp_factor,
        jpeg_info.comp_info[0].v_samp_factor,
        jpeg_info.comp_info[1].h_samp_factor,
        jpeg_info.comp_info[1].v_samp_factor,
        jpeg_info.comp_info[2].h_samp_factor,
        jpeg_info.comp_info[2].v_samp_factor);
      break;
    }
    default:
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Colorspace: %d",
        jpeg_info.out_color_space);
      (void) FormatMagickString(sampling_factors,MaxTextExtent,
        "%dx%d,%dx%d,%dx%d,%dx%d",jpeg_info.comp_info[0].h_samp_factor,
        jpeg_info.comp_info[0].v_samp_factor,
        jpeg_info.comp_info[1].h_samp_factor,
        jpeg_info.comp_info[1].v_samp_factor,
        jpeg_info.comp_info[2].h_samp_factor,
        jpeg_info.comp_info[2].v_samp_factor,
        jpeg_info.comp_info[3].h_samp_factor,
        jpeg_info.comp_info[3].v_samp_factor);
      break;
    }
  }
  (void) SetImageAttribute(image,"JPEG-Sampling-factors",sampling_factors);
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "Sampling Factors: %s",sampling_factors);
  if (image_info->ping != MagickFalse)
    {
      jpeg_destroy_decompress(&jpeg_info);
      CloseBlob(image);
      return(GetFirstImageInList(image));
    }
  jpeg_pixels=(JSAMPLE *) AcquireMagickMemory((size_t)
    jpeg_info.output_components*image->columns*sizeof(JSAMPLE));
  if (jpeg_pixels == (JSAMPLE *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  /*
    Convert JPEG pixels to pixel packets.
  */
  if (setjmp(error_manager.error_recovery) != 0)
    {
      jpeg_pixels=(unsigned char *) RelinquishMagickMemory(jpeg_pixels);
      jpeg_destroy_decompress(&jpeg_info);
      InheritException(exception,&image->exception);
      CloseBlob(image);
      number_pixels=(MagickSizeType) image->columns*image->rows;
      if (number_pixels != 0)
        return(GetFirstImageInList(image));
      image=DestroyImage(image);
      return(image);
    }
  if (jpeg_info.quantize_colors != MagickFalse)
    {
      image->colors=(unsigned long) jpeg_info.actual_number_of_colors;
      if (jpeg_info.out_color_space == JCS_GRAYSCALE)
        for (i=0; i < (long) image->colors; i++)
        {
          image->colormap[i].red=ScaleCharToQuantum(jpeg_info.colormap[0][i]);
          image->colormap[i].green=image->colormap[i].red;
          image->colormap[i].blue=image->colormap[i].red;
        }
      else
        for (i=0; i < (long) image->colors; i++)
        {
          image->colormap[i].red=ScaleCharToQuantum(jpeg_info.colormap[0][i]);
          image->colormap[i].green=ScaleCharToQuantum(jpeg_info.colormap[1][i]);
          image->colormap[i].blue=ScaleCharToQuantum(jpeg_info.colormap[2][i]);
        }
    }
  scanline[0]=(JSAMPROW) jpeg_pixels;
  for (y=0; y < (long) image->rows; y++)
  {
    if (jpeg_read_scanlines(&jpeg_info,scanline,1) != 1)
      {
        (void) ThrowMagickException(&image->exception,GetMagickModule(),
          CorruptImageWarning,"SkipToSyncByte","`%s'",image->filename);
        continue;
      }
    p=jpeg_pixels;
    q=SetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    if (jpeg_info.data_precision > 8)
      {
        if (jpeg_info.output_components == 1)
          for (x=0; x < (long) image->columns; x++)
          {
            index=ConstrainColormapIndex(image,(unsigned long)
              (GETJSAMPLE(*p) << 4));
            indexes[x]=(IndexPacket) index;
            *q++=image->colormap[index];
            p++;
          }
        else
          for (x=0; x < (long) image->columns; x++)
          {
            q->red=ScaleShortToQuantum((unsigned char) (GETJSAMPLE(*p++) << 4));
            q->green=ScaleShortToQuantum((unsigned char)
              (GETJSAMPLE(*p++) << 4));
            q->blue=ScaleShortToQuantum((unsigned char)
              (GETJSAMPLE(*p++) << 4));
            if (image->colorspace == CMYKColorspace)
              indexes[x]=ScaleShortToQuantum((unsigned char)
                (GETJSAMPLE(*p++) << 4));
            q++;
          }
      }
    else
      if (jpeg_info.output_components == 1)
        for (x=0; x < (long) image->columns; x++)
        {
          index=ConstrainColormapIndex(image,(unsigned long) GETJSAMPLE(*p));
          indexes[x]=(IndexPacket) index;
          *q++=image->colormap[index];
          p++;
        }
      else
        for (x=0; x < (long) image->columns; x++)
        {
          q->red=ScaleCharToQuantum((unsigned char) GETJSAMPLE(*p++));
          q->green=ScaleCharToQuantum((unsigned char) GETJSAMPLE(*p++));
          q->blue=ScaleCharToQuantum((unsigned char) GETJSAMPLE(*p++));
          if (image->colorspace == CMYKColorspace)
            indexes[x]=ScaleCharToQuantum((unsigned char) GETJSAMPLE(*p++));
          q++;
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
  if (image->colorspace == CMYKColorspace)
    {
      /*
        Correct CMYK levels.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) image->columns; x++)
        {
          q->red=(Quantum) (QuantumRange-q->red);
          q->green=(Quantum) (QuantumRange-q->green);
          q->blue=(Quantum) (QuantumRange-q->blue);
          indexes[x]=(IndexPacket) (QuantumRange-indexes[x]);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
    }
  /*
    Free jpeg resources.
  */
  (void) jpeg_finish_decompress(&jpeg_info);
  jpeg_destroy_decompress(&jpeg_info);
  jpeg_pixels=(unsigned char *) RelinquishMagickMemory(jpeg_pixels);
  CloseBlob(image);
  InheritException(exception,&image->exception);
  return(GetFirstImageInList(image));
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r J P E G I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterJPEGImage() adds attributes for the JPEG image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterJPEGImage method is:
%
%      RegisterJPEGImage(void)
%
*/
ModuleExport void RegisterJPEGImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("JPEG");
  entry->thread_support=MagickFalse;
#if defined(HasJPEG)
  entry->decoder=(DecoderHandler *) ReadJPEGImage;
  entry->encoder=(EncoderHandler *) WriteJPEGImage;
#endif
  entry->magick=(MagickHandler *) IsJPEG;
  entry->adjoin=MagickFalse;
  entry->description=ConstantString(
    "Joint Photographic Experts Group JFIF format");
#if defined(JPEG_LIB_VERSION)
  {
    char
      version[MaxTextExtent];

    (void) FormatMagickString(version,MaxTextExtent,"%d",JPEG_LIB_VERSION);
    entry->version=ConstantString(version);
  }
#endif
  entry->module=ConstantString("JPEG");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("JPG");
  entry->thread_support=MagickFalse;
#if defined(HasJPEG)
  entry->decoder=(DecoderHandler *) ReadJPEGImage;
  entry->encoder=(EncoderHandler *) WriteJPEGImage;
#endif
  entry->adjoin=MagickFalse;
  entry->description=ConstantString(
    "Joint Photographic Experts Group JFIF format");
  entry->module=ConstantString("JPEG");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PJPEG");
  entry->thread_support=MagickFalse;
#if defined(HasJPEG)
  entry->decoder=(DecoderHandler *) ReadJPEGImage;
  entry->encoder=(EncoderHandler *) WriteJPEGImage;
#endif
  entry->adjoin=MagickFalse;
  entry->description=ConstantString(
    "Progessive Joint Photographic Experts Group JFIF");
  entry->module=ConstantString("JPEG");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r J P E G I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterJPEGImage() removes format registrations made by the
%  JPEG module from the list of supported formats.
%
%  The format of the UnregisterJPEGImage method is:
%
%      UnregisterJPEGImage(void)
%
*/
ModuleExport void UnregisterJPEGImage(void)
{
  (void) UnregisterMagickInfo("JPEG");
  (void) UnregisterMagickInfo("JPG");
}

#if defined(HasJPEG)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  W r i t e J P E G I m a g e                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteJPEGImage() writes a JPEG image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the WriteJPEGImage method is:
%
%      MagickBooleanType WriteJPEGImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o jpeg_image:  The image.
%
%
*/

static boolean EmptyOutputBuffer(j_compress_ptr cinfo)
{
  DestinationManager
    *destination;

  destination=(DestinationManager *) cinfo->dest;
  destination->manager.free_in_buffer=(size_t) WriteBlob(destination->image,
    MaxBufferExtent,destination->buffer);
  if (destination->manager.free_in_buffer != MaxBufferExtent)
    ERREXIT(cinfo,JERR_FILE_WRITE);
  destination->manager.next_output_byte=destination->buffer;
  return(TRUE);
}

static void InitializeDestination(j_compress_ptr cinfo)
{
  DestinationManager
    *destination;

  destination=(DestinationManager *) cinfo->dest;
  destination->buffer=(JOCTET *) (*cinfo->mem->alloc_small)
    ((j_common_ptr) cinfo,JPOOL_IMAGE,MaxBufferExtent*sizeof(JOCTET));
  destination->manager.next_output_byte=destination->buffer;
  destination->manager.free_in_buffer=MaxBufferExtent;
}

static void TerminateDestination(j_compress_ptr cinfo)
{
  DestinationManager
    *destination;

  destination=(DestinationManager *) cinfo->dest;
  if ((MaxBufferExtent-(int) destination->manager.free_in_buffer) > 0)
    {
      ssize_t
        count;

      count=WriteBlob(destination->image,MaxBufferExtent-
        destination->manager.free_in_buffer,destination->buffer);
      if (count != (ssize_t)
          (MaxBufferExtent-destination->manager.free_in_buffer))
        ERREXIT(cinfo,JERR_FILE_WRITE);
    }
  if (SyncBlob(destination->image) != MagickFalse)
    ERREXIT(cinfo,JERR_FILE_WRITE);
}

static void WriteProfile(j_compress_ptr jpeg_info,Image *image)
{
  const char
    *name;

  const StringInfo
    *profile;

  register long
    i;

  size_t
    length;

  StringInfo
    *custom_profile;

  unsigned long
    tag_length;

  /*
    Save image profile as a APP marker.
  */
  custom_profile=AcquireStringInfo(65535L);
  ResetImageProfileIterator(image);
  for (name=GetNextImageProfile(image); name != (const char *) NULL; )
  {
    profile=GetImageProfile(image,name);
    if ((LocaleCompare(name,"EXIF") == 0) || (LocaleCompare(name,"XMP") == 0))
      for (i=0; i < (long) profile->length; i+=65533L)
      {
        length=Min(profile->length-i,65533L);
        jpeg_write_marker(jpeg_info,XML_MARKER,profile->datum+i,
          (unsigned int) length);
      }
    if (LocaleCompare(name,"ICC") == 0)
      {
        tag_length=14;
        (void) CopyMagickMemory(custom_profile->datum,ICC_PROFILE,tag_length);
        for (i=0; i < (long) profile->length; i+=65519L)
        {
          length=Min(profile->length-i,65519L);
          custom_profile->datum[12]=(unsigned char) ((i/65519L)+1);
          custom_profile->datum[13]=(unsigned char) (profile->length/65519L+1);
          (void) CopyMagickMemory(custom_profile->datum+tag_length,
            profile->datum+i,length);
          jpeg_write_marker(jpeg_info,ICC_MARKER,custom_profile->datum,
            (unsigned int) (length+tag_length));
        }
      }
    if (LocaleCompare(name,"IPTC") == 0)
      {
        unsigned long
          roundup;

        if (LocaleNCompare((char *) profile->datum,"8BIM",4) == 0)
          {
            (void) CopyMagickMemory(custom_profile->datum,"Photoshop 3.0\0",14);
            tag_length=14;
          }
        else
          {
            (void) CopyMagickMemory(custom_profile->datum,
              "Photoshop 3.0\08BIM\04\04\0\0\0\0",24);
            custom_profile->datum[13]=0x00;
            custom_profile->datum[24]=profile->length >> 8;
            custom_profile->datum[25]=profile->length & 0xff;
            tag_length=26;
          }
        for (i=0; i < (long) profile->length; i+=65500L)
        {
          length=Min(profile->length-i,65500L);
          roundup=(unsigned long) (length & 0x01);
          (void) CopyMagickMemory(custom_profile->datum+tag_length,
            profile->datum+i,length);
          if (roundup != 0)
            custom_profile->datum[length+tag_length]='\0';
          jpeg_write_marker(jpeg_info,IPTC_MARKER,custom_profile->datum,
            (unsigned int) (length+tag_length+roundup));
        }
      }
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "%s profile: %lu bytes",name,(unsigned long) profile->length);
    name=GetNextImageProfile(image);
  }
  custom_profile=DestroyStringInfo(custom_profile);
}

static void JPEGDestinationManager(j_compress_ptr cinfo,Image * image)
{
  DestinationManager
    *destination;

  cinfo->dest=(struct jpeg_destination_mgr *) (*cinfo->mem->alloc_small)
    ((j_common_ptr) cinfo,JPOOL_IMAGE,sizeof(DestinationManager));
  destination=(DestinationManager *) cinfo->dest;
  destination->manager.init_destination=InitializeDestination;
  destination->manager.empty_output_buffer=EmptyOutputBuffer;
  destination->manager.term_destination=TerminateDestination;
  destination->image=image;
}

static char **SamplingFactorToList(const char *text)
{
  char
    **textlist;

  register char
    *q;

  register const char
    *p;

  register long
    i;

  unsigned long
    lines;

  if (text == (char *) NULL)
    return((char **) NULL);
  /*
    Convert string to an ASCII list.
  */
  lines=1;
  for (p=text; *p != '\0'; p++)
    if (*p == ',')
      lines++;
  textlist=(char **)
    AcquireMagickMemory(((size_t) lines+MaxTextExtent)*sizeof(*textlist));
  if (textlist == (char **) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"UnableToConvertText",
      text);
  p=text;
  for (i=0; i < (long) lines; i++)
  {
    for (q=(char *) p; *q != '\0'; q++)
      if (*q == ',')
        break;
    textlist[i]=(char *) AcquireMagickMemory((size_t) (q-p)+MaxTextExtent);
    if (textlist[i] == (char *) NULL)
      ThrowMagickFatalException(ResourceLimitFatalError,"UnableToConvertText",
        text);
    (void) CopyMagickString(textlist[i],p,(size_t) (q-p+1));
    if (*q == '\r')
      q++;
    p=q+1;
  }
  textlist[i]=(char *) NULL;
  return(textlist);
}

static MagickBooleanType WriteJPEGImage(const ImageInfo *image_info,
  Image *image)
{
  char
    *sampling_factors;

  const ImageAttribute
    *attribute;

  ErrorManager
    error_manager;

  JSAMPLE
    *jpeg_pixels;

  JSAMPROW
    scanline[1];

  long
    y;

  MagickBooleanType
    status;

  MagickSizeType
    number_pixels;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register JSAMPLE
    *q;

  register long
    i,
    x;

  struct jpeg_compress_struct
    jpeg_info;

  struct jpeg_error_mgr
    jpeg_error;

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
    Initialize JPEG parameters.
  */
  jpeg_info.client_data=(void *) image;
  jpeg_info.err=jpeg_std_error(&jpeg_error);
  jpeg_info.err->emit_message=(void (*)(j_common_ptr,int)) EmitMessage;
  jpeg_info.err->error_exit=(void (*)(j_common_ptr)) JPEGErrorHandler;
  error_manager.image=image;
  jpeg_pixels=(JSAMPLE *) NULL;
  if (setjmp(error_manager.error_recovery) != 0)
    {
      jpeg_destroy_compress(&jpeg_info);
      if (jpeg_pixels != (unsigned char *) NULL)
        jpeg_pixels=(unsigned char *) RelinquishMagickMemory(jpeg_pixels);
      CloseBlob(image);
      return(MagickFalse);
    }
  jpeg_info.client_data=(void *) &error_manager;
  jpeg_create_compress(&jpeg_info);
  JPEGDestinationManager(&jpeg_info,image);
  jpeg_info.image_width=(unsigned int) image->columns;
  jpeg_info.image_height=(unsigned int) image->rows;
  jpeg_info.input_components=3;
  jpeg_info.data_precision=8;
  jpeg_info.in_color_space=JCS_RGB;
  switch (image_info->colorspace)
  {
    case CMYKColorspace:
    {
      jpeg_info.input_components=4;
      jpeg_info.in_color_space=JCS_CMYK;
      (void) SetImageColorspace(image,CMYKColorspace);
      break;
    }
    case YCbCrColorspace:
    case Rec601YCbCrColorspace:
    case Rec709YCbCrColorspace:
    {
      jpeg_info.in_color_space=JCS_YCbCr;
      (void) SetImageColorspace(image,YCbCrColorspace);
      break;
    }
    case GRAYColorspace:
    case Rec601LumaColorspace:
    case Rec709LumaColorspace:
    {
      jpeg_info.input_components=1;
      jpeg_info.in_color_space=JCS_GRAYSCALE;
      (void) SetImageColorspace(image,GRAYColorspace);
      break;
    }
    default:
    {
      if (image->colorspace == CMYKColorspace)
        {
          jpeg_info.input_components=4;
          jpeg_info.in_color_space=JCS_CMYK;
          break;
        }
      if ((image->colorspace == YCbCrColorspace) ||
          (image->colorspace == Rec601YCbCrColorspace) ||
          (image->colorspace == Rec709YCbCrColorspace))
        {
          jpeg_info.in_color_space=JCS_YCbCr;
          break;
        }
      if (image_info->colorspace == UndefinedColorspace)
        (void) SetImageColorspace(image,RGBColorspace);
      break;
    }
  }
  if ((image_info->type != TrueColorType) &&
      (IsGrayImage(image,&image->exception) != MagickFalse))
    {
      jpeg_info.input_components=1;
      jpeg_info.in_color_space=JCS_GRAYSCALE;
    }
  jpeg_set_defaults(&jpeg_info);
  if ((jpeg_info.data_precision != 12) && (image->depth <= 8))
    jpeg_info.data_precision=8;
  else
    if (sizeof(JSAMPLE) > 1)
      jpeg_info.data_precision=12;
  jpeg_info.density_unit=(UINT8) 1;
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "Image resolution: %ld,%ld",(long) (image->x_resolution+0.5),
      (long) (image->y_resolution+0.5));
  if ((image->x_resolution != 0.0) && (image->y_resolution != 0.0))
    {
      /*
        Set image resolution.
      */
      jpeg_info.write_JFIF_header=MagickTrue;
      jpeg_info.X_density=(UINT16) image->x_resolution;
      jpeg_info.Y_density=(UINT16) image->y_resolution;
      if (image->units == PixelsPerInchResolution)
        jpeg_info.density_unit=(UINT8) 1;
      if (image->units == PixelsPerCentimeterResolution)
        jpeg_info.density_unit=(UINT8) 2;
    }
  number_pixels=(MagickSizeType) image->columns*image->rows;
  if (number_pixels == (MagickSizeType) ((size_t) number_pixels))
    {
      /*
        Perform optimization only if available memory resources permit it.
      */
      status=AcquireMagickResource(AreaResource,number_pixels);
      if (status != MagickFalse)
        {
          const char
            *option;

          option=GetImageOption(image_info,"jpeg:optimize-coding");
          if ((option == (const char *) NULL) ||
              (LocaleCompare(option,"true") == 0))
            jpeg_info.optimize_coding=MagickTrue;
        }
      RelinquishMagickResource(AreaResource,number_pixels);
    }
#if (JPEG_LIB_VERSION >= 61) && defined(C_PROGRESSIVE_SUPPORTED)
  if ((LocaleCompare(image_info->magick,"PJPEG") == 0) ||
      (image_info->interlace != NoInterlace))
    jpeg_simple_progression(&jpeg_info);
  if (image->debug != MagickFalse)
    {
      if (image_info->interlace != NoInterlace)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Interlace: progressive");
      else
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Interlace: non-progressive");
    }
#else
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "Interlace: nonprogressive");
#endif
  if ((image->compression != LosslessJPEGCompression) &&
      (image->quality != UndefinedCompressionQuality) &&
      (image->quality <= 100))
    {
      jpeg_set_quality(&jpeg_info,(int) image->quality,MagickTrue);
      if (image->debug != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Quality: %lu",
          image->quality);
    }
  else
    {
#if !defined(C_LOSSLESS_SUPPORTED)
      jpeg_set_quality(&jpeg_info,100,MagickTrue);
      if (image->debug != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Quality: 100");
#else
      if (image->quality < 100)
        (void) ThrowMagickException(&image->exception,GetMagickModule(),
          CoderWarning,"LosslessToLossyJPEGConversion",image->filename);
      else
        {
          int
            point_transform,
            predictor;

          predictor=image->quality/100;  /* range 1-7 */
          point_transform=image->quality % 20;  /* range 0-15 */
          jpeg_simple_lossless(&jpeg_info,predictor,point_transform);
          if (image->debug != MagickFalse)
            {
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "Compression: lossless");
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "Predictor: %d",predictor);
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "Point Transform: %d",point_transform);
            }
        }
#endif
    }
  sampling_factors=(char *) NULL;
  attribute=GetImageAttribute(image,"JPEG-Sampling-factors");
  if ((attribute != (const ImageAttribute *) NULL) &&
      (attribute->value != (char *) NULL))
    {
      sampling_factors=attribute->value;
      if (image->debug != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "  Input sampling-factors=%s",sampling_factors);
    }
  if (image_info->sampling_factor != (char *) NULL)
    sampling_factors=image_info->sampling_factor;
  if (sampling_factors == (char *) NULL)
    {
      if (image->quality >= 90)
        for (i=0; i < MAX_COMPONENTS; i++)
        {
          jpeg_info.comp_info[i].h_samp_factor=1;
          jpeg_info.comp_info[i].v_samp_factor=1;
        }
    }
  else
    {
      char
        **sampling_factor;

      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      /*
        Set sampling factor.
      */
      i=0;
      sampling_factor=SamplingFactorToList(sampling_factors);
      if (sampling_factor != (char **) NULL)
        {
          for (i=0; i < MAX_COMPONENTS; i++)
          {
            if (sampling_factor[i] == (char *) NULL)
              break;
            flags=ParseGeometry(sampling_factor[i],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=geometry_info.rho;
            jpeg_info.comp_info[i].h_samp_factor=(int) geometry_info.rho;
            jpeg_info.comp_info[i].v_samp_factor=(int) geometry_info.sigma;
            sampling_factor[i]=(char *)
              RelinquishMagickMemory(sampling_factor[i]);
          }
          sampling_factor=(char **) RelinquishMagickMemory(sampling_factor);
        }
      for ( ; i < MAX_COMPONENTS; i++)
      {
        jpeg_info.comp_info[i].h_samp_factor=1;
        jpeg_info.comp_info[i].v_samp_factor=1;
      }
    }
  jpeg_start_compress(&jpeg_info,MagickTrue);
  if (image->debug != MagickFalse)
    {
      if (image->storage_class == PseudoClass)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Storage class: PseudoClass");
      else
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Storage class: DirectClass");
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Depth: %lu",
        image->depth);
      if (image->colors != 0)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Number of colors: %lu",image->colors);
      else
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Number of colors: unspecified");
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "JPEG data precision: %d",(int) jpeg_info.data_precision);
      switch (image_info->colorspace)
      {
        case CMYKColorspace:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Storage class: DirectClass");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: CMYK");
          break;
        }
        case YCbCrColorspace:
        case Rec601YCbCrColorspace:
        case Rec709YCbCrColorspace:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: YCbCr");
          break;
        }
        default:
          break;
      }
      switch (image->colorspace)
      {
        case CMYKColorspace:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: CMYK");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d,%dx%d,%dx%d,%dx%d",
            jpeg_info.comp_info[0].h_samp_factor,
            jpeg_info.comp_info[0].v_samp_factor,
            jpeg_info.comp_info[1].h_samp_factor,
            jpeg_info.comp_info[1].v_samp_factor,
            jpeg_info.comp_info[2].h_samp_factor,
            jpeg_info.comp_info[2].v_samp_factor,
            jpeg_info.comp_info[3].h_samp_factor,
            jpeg_info.comp_info[3].v_samp_factor);
          break;
        }
        case GRAYColorspace:
        case Rec601LumaColorspace:
        case Rec709LumaColorspace:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: GRAY");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d",jpeg_info.comp_info[0].h_samp_factor,
            jpeg_info.comp_info[0].v_samp_factor);
          break;
        }
        case RGBColorspace:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            " Image colorspace is RGB");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d,%dx%d,%dx%d",
            jpeg_info.comp_info[0].h_samp_factor,
            jpeg_info.comp_info[0].v_samp_factor,
            jpeg_info.comp_info[1].h_samp_factor,
            jpeg_info.comp_info[1].v_samp_factor,
            jpeg_info.comp_info[2].h_samp_factor,
            jpeg_info.comp_info[2].v_samp_factor);
          break;
        }
        case YCbCrColorspace:
        case Rec601YCbCrColorspace:
        case Rec709YCbCrColorspace:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: YCbCr");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d,%dx%d,%dx%d",
            jpeg_info.comp_info[0].h_samp_factor,
            jpeg_info.comp_info[0].v_samp_factor,
            jpeg_info.comp_info[1].h_samp_factor,
            jpeg_info.comp_info[1].v_samp_factor,
            jpeg_info.comp_info[2].h_samp_factor,
            jpeg_info.comp_info[2].v_samp_factor);
          break;
        }
        default:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Colorspace: %d",
            image->colorspace);
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d,%dx%d,%dx%d,%dx%d",
            jpeg_info.comp_info[0].h_samp_factor,
            jpeg_info.comp_info[0].v_samp_factor,
            jpeg_info.comp_info[1].h_samp_factor,
            jpeg_info.comp_info[1].v_samp_factor,
            jpeg_info.comp_info[2].h_samp_factor,
            jpeg_info.comp_info[2].v_samp_factor,
            jpeg_info.comp_info[3].h_samp_factor,
            jpeg_info.comp_info[3].v_samp_factor);
          break;
        }
      }
    }
  /*
    Write JPEG profiles.
  */
  attribute=GetImageAttribute(image,"Comment");
  if ((attribute != (const ImageAttribute *) NULL) &&
      (attribute->value != (char *) NULL))
    for (i=0; i < (long) strlen(attribute->value); i+=65533L)
      jpeg_write_marker(&jpeg_info,JPEG_COM,(unsigned char *) attribute->value+
        i,(unsigned int) Min((long) strlen(attribute->value+i),65533L));
  if (image->profiles != (void *) NULL)
    WriteProfile(&jpeg_info,image);
  /*
    Convert MIFF to JPEG raster pixels.
  */
  jpeg_pixels=(JSAMPLE *) AcquireMagickMemory((size_t)
    jpeg_info.input_components*image->columns*sizeof(*jpeg_pixels));
  if (jpeg_pixels == (JSAMPLE *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  scanline[0]=(JSAMPROW) jpeg_pixels;
  if (jpeg_info.data_precision > 8)
    {
      if (jpeg_info.in_color_space == JCS_GRAYSCALE)
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          q=jpeg_pixels;
          for (x=0; x < (long) image->columns; x++)
          {
            *q++=(JSAMPLE)
              (ScaleQuantumToShort(PixelIntensityToQuantum(p)) >> 4);
            p++;
          }
          (void) jpeg_write_scanlines(&jpeg_info,scanline,1);
          if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
              (QuantumTick(y,image->rows) != MagickFalse))
            {
              status=image->progress_monitor(SaveImageTag,y,image->rows,
                image->client_data);
              if (status == MagickFalse)
                break;
            }
        }
      else
        if ((jpeg_info.in_color_space == JCS_RGB) ||
            (jpeg_info.in_color_space == JCS_YCbCr))
          for (y=0; y < (long) image->rows; y++)
          {
            p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
            if (p == (const PixelPacket *) NULL)
              break;
            q=jpeg_pixels;
            for (x=0; x < (long) image->columns; x++)
            {
              *q++=(JSAMPLE) (ScaleQuantumToShort(p->red) >> 4);
              *q++=(JSAMPLE) (ScaleQuantumToShort(p->green) >> 4);
              *q++=(JSAMPLE) (ScaleQuantumToShort(p->blue) >> 4);
              p++;
            }
            (void) jpeg_write_scanlines(&jpeg_info,scanline,1);
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(y,image->rows) != MagickFalse))
              {
                status=image->progress_monitor(SaveImageTag,y,image->rows,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
          }
        else
          for (y=0; y < (long) image->rows; y++)
          {
            p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
            if (p == (const PixelPacket *) NULL)
              break;
            q=jpeg_pixels;
            indexes=GetIndexes(image);
            for (x=0; x < (long) image->columns; x++)
            {
              /*
                Convert DirectClass packets to contiguous CMYK scanlines.
              */
              *q++=(JSAMPLE) (4095-(ScaleQuantumToShort(p->red) >> 4));
              *q++=(JSAMPLE) (4095-(ScaleQuantumToShort(p->green) >> 4));
              *q++=(JSAMPLE) (4095-(ScaleQuantumToShort(p->blue) >> 4));
              *q++=(JSAMPLE) (4095-(ScaleQuantumToShort(indexes[x]) >> 4));
              p++;
            }
            (void) jpeg_write_scanlines(&jpeg_info,scanline,1);
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(y,image->rows) != MagickFalse))
              {
                status=image->progress_monitor(SaveImageTag,y,image->rows,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
          }
    }
  else
    if (jpeg_info.in_color_space == JCS_GRAYSCALE)
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
        if (p == (const PixelPacket *) NULL)
          break;
        q=jpeg_pixels;
        for (x=0; x < (long) image->columns; x++)
        {
          *q++=(JSAMPLE) ScaleQuantumToChar(PixelIntensityToQuantum(p));
          p++;
        }
        (void) jpeg_write_scanlines(&jpeg_info,scanline,1);
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(SaveImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
    else
      if ((jpeg_info.in_color_space == JCS_RGB) ||
          (jpeg_info.in_color_space == JCS_YCbCr))
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          q=jpeg_pixels;
          for (x=0; x < (long) image->columns; x++)
          {
            *q++=(JSAMPLE) ScaleQuantumToChar(p->red);
            *q++=(JSAMPLE) ScaleQuantumToChar(p->green);
            *q++=(JSAMPLE) ScaleQuantumToChar(p->blue);
            p++;
          }
          (void) jpeg_write_scanlines(&jpeg_info,scanline,1);
          if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
              (QuantumTick(y,image->rows) != MagickFalse))
            {
              status=image->progress_monitor(SaveImageTag,y,image->rows,
                image->client_data);
              if (status == MagickFalse)
                break;
            }
        }
      else
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          q=jpeg_pixels;
          indexes=GetIndexes(image);
          for (x=0; x < (long) image->columns; x++)
          {
            /*
              Convert DirectClass packets to contiguous CMYK scanlines.
            */
            *q++=(JSAMPLE) (ScaleQuantumToChar((Quantum)
              (QuantumRange-p->red)));
            *q++=(JSAMPLE) (ScaleQuantumToChar((Quantum)
              (QuantumRange-p->green)));
            *q++=(JSAMPLE) (ScaleQuantumToChar((Quantum)
              (QuantumRange-p->blue)));
            *q++=(JSAMPLE) (ScaleQuantumToChar((Quantum)
              (QuantumRange-indexes[x])));
            p++;
          }
          (void) jpeg_write_scanlines(&jpeg_info,scanline,1);
          if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
              (QuantumTick(y,image->rows) != MagickFalse))
            {
              status=image->progress_monitor(SaveImageTag,y,image->rows,
                image->client_data);
              if (status == MagickFalse)
                break;
            }
        }
  if (y == (long) image->rows)
    jpeg_finish_compress(&jpeg_info);
  /*
    Free resources.
  */
  jpeg_destroy_compress(&jpeg_info);
  jpeg_pixels=(unsigned char *) RelinquishMagickMemory(jpeg_pixels);
  CloseBlob(image);
  return(MagickTrue);
}
#endif
