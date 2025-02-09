/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            EEEEE  PPPP   TTTTT                              %
%                            E      P   P    T                                %
%                            EEE    PPPP     T                                %
%                            E      P        T                                %
%                            EEEEE  P        T                                %
%                                                                             %
%                                                                             %
%           Read/Write Encapsulated Postscript Format (with preview).         %
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
#include "magick/color.h"
#include "magick/constitute.h"
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/delegate.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/quantize.h"
#include "magick/resource_.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/transform.h"
#include "magick/utility.h"

/*
  Typedef declarations.
*/
typedef struct _EPTInfo
{
  unsigned long
    magick;

  MagickOffsetType
    postscript_offset,
    tiff_offset;

  size_t
    postscript_length,
    tiff_length;

  unsigned char
    *postscript,
    *tiff;
} EPTInfo;

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteEPTImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s E P T                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsEPT() returns MagickTrue if the image format type, identified by the
%  magick string, is EPT.
%
%  The format of the IsEPT method is:
%
%      MagickBooleanType IsEPT(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsEPT(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(MagickFalse);
  if (memcmp(magick,"\305\320\323\306",4) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d E P T I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadEPTImage() reads a binary Postscript image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadEPTImage method is:
%
%      Image *ReadEPTImage(const ImageInfo *image_info,
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
static Image *ReadEPTImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  EPTInfo
    ept_info;

  Image
    *image;

  ImageInfo
    *read_info;

  ssize_t
    count;

  MagickBooleanType
    status;

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
  ept_info.magick=ReadBlobLSBLong(image);
  if (ept_info.magick != 0xc6d3d0c5ul)
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  ept_info.postscript_offset=(MagickOffsetType) ReadBlobLSBLong(image);
  ept_info.postscript_length=ReadBlobLSBLong(image);
  (void) ReadBlobLSBLong(image);
  (void) ReadBlobLSBLong(image);
  ept_info.tiff_offset=(MagickOffsetType) ReadBlobLSBLong(image);
  ept_info.tiff_length=ReadBlobLSBLong(image);
  (void) ReadBlobLSBShort(image);
  ept_info.postscript=(unsigned char *)
    AcquireMagickMemory(ept_info.postscript_length);
  ept_info.tiff=(unsigned char *) AcquireMagickMemory(ept_info.tiff_length);
  if ((ept_info.postscript == (unsigned char *) NULL) ||
      (ept_info.tiff == (unsigned char *) NULL))
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  (void) SeekBlob(image,ept_info.tiff_offset,SEEK_SET);
  count=ReadBlob(image,ept_info.tiff_length,ept_info.tiff);
  if (count != (ssize_t) (ept_info.tiff_length))
    ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
  (void) SeekBlob(image,ept_info.postscript_offset,SEEK_SET);
  count=ReadBlob(image,ept_info.postscript_length,ept_info.postscript);
  if (count != (ssize_t) (ept_info.postscript_length))
    ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
  CloseBlob(image);
  image=DestroyImage(image);
  read_info=CloneImageInfo(image_info);
  (void) CopyMagickString(read_info->magick,"EPS",MaxTextExtent);
  image=BlobToImage(read_info,ept_info.postscript,ept_info.postscript_length,
    exception);
  if (image == (Image *) NULL)
    {
      (void) CopyMagickString(read_info->magick,"TIFF",MaxTextExtent);
      image=BlobToImage(read_info,ept_info.tiff,ept_info.tiff_length,exception);
    }
  read_info=DestroyImageInfo(read_info);
  if (image != (Image *) NULL)
    (void) CopyMagickString(image->filename,image_info->filename,MaxTextExtent);
  ept_info.tiff=(unsigned char *) RelinquishMagickMemory(ept_info.tiff);
  ept_info.postscript=(unsigned char *)
    RelinquishMagickMemory(ept_info.postscript);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r E P T I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterEPTImage() adds attributes for the EPT image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterEPTImage method is:
%
%      RegisterEPTImage(void)
%
*/
ModuleExport void RegisterEPTImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("EPT");
  entry->decoder=(DecoderHandler *) ReadEPTImage;
  entry->encoder=(EncoderHandler *) WriteEPTImage;
  entry->magick=(MagickHandler *) IsEPT;
  entry->adjoin=MagickFalse;
  entry->blob_support=MagickFalse;
  entry->description=ConstantString(
    "Encapsulated PostScript with TIFF preview");
  entry->module=ConstantString("EPT");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("EPT2");
  entry->decoder=(DecoderHandler *) ReadEPTImage;
  entry->encoder=(EncoderHandler *) WriteEPTImage;
  entry->magick=(MagickHandler *) IsEPT;
  entry->adjoin=MagickFalse;
  entry->blob_support=MagickFalse;
  entry->description=ConstantString(
    "Encapsulated PostScript Level II with TIFF preview");
  entry->module=ConstantString("EPT");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("EPT3");
  entry->decoder=(DecoderHandler *) ReadEPTImage;
  entry->encoder=(EncoderHandler *) WriteEPTImage;
  entry->magick=(MagickHandler *) IsEPT;
  entry->blob_support=MagickFalse;
  entry->description=ConstantString(
    "Encapsulated PostScript Level III with TIFF preview");
  entry->module=ConstantString("EPT");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r E P T I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterEPTImage() removes format registrations made by the
%  EPT module from the list of supported formats.
%
%  The format of the UnregisterEPTImage method is:
%
%      UnregisterEPTImage(void)
%
*/
ModuleExport void UnregisterEPTImage(void)
{
  (void) UnregisterMagickInfo("EPT");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e E P T I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteEPTImage() writes an image in the Encapsulated Postscript format
%  with a TIFF preview.
%
%  The format of the WriteEPTImage method is:
%
%      MagickBooleanType WriteEPTImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteEPTImage(const ImageInfo *image_info,Image *image)
{
  EPTInfo
    ept_info;

  Image
    *write_image;

  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  /*
    Write EPT image.
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
  write_image=CloneImage(image,0,0,MagickTrue,&image->exception);
  if (write_image == (Image *) NULL)
    return(MagickFalse);
  DestroyBlob(write_image);
  write_image->blob=CloneBlobInfo((BlobInfo *) NULL);
  write_info=CloneImageInfo(image_info);
  (void) CopyMagickString(write_info->magick,"EPS",MaxTextExtent);
  if (LocaleCompare(image_info->magick,"EPT2") == 0)
    (void) CopyMagickString(write_info->magick,"EPS2",MaxTextExtent);
  if (LocaleCompare(image_info->magick,"EPT3") == 0)
    (void) CopyMagickString(write_info->magick,"EPS3",MaxTextExtent);
  (void) ResetMagickMemory(&ept_info,0,sizeof(ept_info));
  ept_info.magick=0xc6d3d0c5ul;
  ept_info.postscript=(unsigned char *) ImageToBlob(write_info,write_image,
    &ept_info.postscript_length,&image->exception);
  write_image=DestroyImage(write_image);
  write_info=DestroyImageInfo(write_info);
  if (ept_info.postscript == (void *) NULL)
    return(MagickFalse);
  write_image=CloneImage(image,0,0,MagickTrue,&image->exception);
  if (write_image == (Image *) NULL)
    return(MagickFalse);
  DestroyBlob(write_image);
  write_image->blob=CloneBlobInfo((BlobInfo *) NULL);
  write_info=CloneImageInfo(image_info);
  (void) CopyMagickString(write_info->magick,"TIFF",MaxTextExtent);
  FormatMagickString(write_info->filename,MaxTextExtent,"tiff:%.1024s",
    write_info->filename); 
  (void) TransformImage(&write_image,(char *) NULL,"512x512>");
  if ((write_image->storage_class == DirectClass) ||
      (write_image->colors > 256))
    {
      QuantizeInfo
        quantize_info;

      /*
        EPT preview requires that the image is colormapped.
      */
      GetQuantizeInfo(&quantize_info);
      quantize_info.dither=IsPaletteImage(write_image,&image->exception) ==
        MagickFalse ? MagickTrue : MagickFalse;
      (void) QuantizeImage(&quantize_info,write_image);
    }
  write_image->compression=NoCompression;
  ept_info.tiff=(unsigned char *) ImageToBlob(write_info,write_image,
    &ept_info.tiff_length,&image->exception);
  write_image=DestroyImage(write_image);
  write_info=DestroyImageInfo(write_info);
  if (ept_info.tiff == (void *) NULL)
    {
      ept_info.postscript=(unsigned char *)
        RelinquishMagickMemory(ept_info.postscript);
      return(MagickFalse);
    }
  /*
    Write EPT image.
  */
  (void) WriteBlobLSBLong(image,ept_info.magick);
  (void) WriteBlobLSBLong(image,30);
  (void) WriteBlobLSBLong(image,ept_info.postscript_length);
  (void) WriteBlobLSBLong(image,0);
  (void) WriteBlobLSBLong(image,0);
  (void) WriteBlobLSBLong(image,ept_info.postscript_length+30);
  (void) WriteBlobLSBLong(image,ept_info.tiff_length);
  (void) WriteBlobLSBShort(image,0xffff);
  (void) WriteBlob(image,ept_info.postscript_length,ept_info.postscript);
  (void) WriteBlob(image,ept_info.tiff_length,ept_info.tiff);
  /*
    Relinquish resources.
  */
  ept_info.postscript=(unsigned char *)
    RelinquishMagickMemory(ept_info.postscript);
  ept_info.tiff=(unsigned char *) RelinquishMagickMemory(ept_info.tiff);
  CloseBlob(image);
  return(MagickTrue);
}
