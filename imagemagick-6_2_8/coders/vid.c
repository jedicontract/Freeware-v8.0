/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            V   V  IIIII  DDDD                               %
%                            V   V    I    D   D                              %
%                            V   V    I    D   D                              %
%                             V V     I    D   D                              %
%                              V    IIIII  DDDD                               %
%                                                                             %
%                                                                             %
%             Return a Visual Image Directory for matching images.            %
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
#include "magick/montage.h"
#include "magick/resize.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/utility.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteVIDImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d V I D I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadVIDImage reads one of more images and creates a Visual Image
%  Directory file.  It allocates the memory necessary for the new Image
%  structure and returns a pointer to the new image.
%
%  The format of the ReadVIDImage method is:
%
%      Image *ReadVIDImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadVIDImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
#define ClientName  "montage"

  char
    **filelist;

  Image
    *image,
    *images,
    *montage_image,
    *next_image,
    *thumbnail_image;

  ImageInfo
    *read_info;

  int
    number_files;

  MagickBooleanType
    status;

  MontageInfo
    *montage_info;

  RectangleInfo
    geometry;

  register long
    i;

  /*
    Expand the filename.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage(image_info);
  filelist=(char **) AcquireMagickMemory(sizeof(filelist));
  if (filelist == (char **) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  filelist[0]=(char *) image_info->filename;
  number_files=1;
  status=ExpandFilenames(&number_files,&filelist);
  if ((status == MagickFalse) || (number_files == 0))
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  image=DestroyImage(image);
  /*
    Read each image and convert them to a tile.
  */
  images=NewImageList();
  read_info=CloneImageInfo(image_info);
  SetImageInfoBlob(read_info,(void *) NULL,0);
  (void) SetImageInfoProgressMonitor(read_info,(MagickProgressMonitor) NULL,
    (void *) NULL);
  if (read_info->size == (char *) NULL)
    (void) CloneString(&read_info->size,DefaultTileGeometry);
  for (i=0; i < (long) number_files; i++)
  {
    if (image_info->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"name: %s",
        filelist[i]);
    (void) CopyMagickString(read_info->filename,filelist[i],MaxTextExtent);
    filelist[i]=(char *) RelinquishMagickMemory(filelist[i]);
    *read_info->magick='\0';
    next_image=ReadImage(read_info,exception);
    CatchException(exception);
    if (next_image == (Image *) NULL)
      break;
    (void) SetImageAttribute(next_image,"Label",DefaultTileLabel);
    if (image_info->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"geometry: %ldx%ld",
        next_image->columns,next_image->rows);
    SetGeometry(next_image,&geometry);
    (void) ParseMetaGeometry(read_info->size,&geometry.x,&geometry.y,
      &geometry.width,&geometry.height);
    thumbnail_image=ThumbnailImage(next_image,geometry.width,geometry.height,
      exception);
    if (thumbnail_image != (Image *) NULL)
      {
        next_image=DestroyImage(next_image);
        next_image=thumbnail_image;
      }
    if (image_info->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "thumbnail geometry: %ldx%ld",next_image->columns,next_image->rows);
    AppendImageToList(&images,next_image);
    if (images->progress_monitor != (MagickProgressMonitor) NULL)
      {
        status=images->progress_monitor(LoadImagesTag,i,(MagickSizeType)
          number_files,images->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  read_info=DestroyImageInfo(read_info);
  filelist=(char **) RelinquishMagickMemory(filelist);
  if (images == (Image *) NULL)
    ThrowReaderException(CorruptImageError,
      "ImageFileDoesNotContainAnyImageData");
  /*
    Create the visual image directory.
  */
  montage_info=CloneMontageInfo(image_info,(MontageInfo *) NULL);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"creating montage");
  montage_image=MontageImageList(image_info,montage_info,
    GetFirstImageInList(images),exception);
  montage_info=DestroyMontageInfo(montage_info);
  images=DestroyImageList(images);
  return(montage_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r V I D I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterVIDImage() adds attributes for the VID image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterVIDImage method is:
%
%      RegisterVIDImage(void)
%
*/
ModuleExport void RegisterVIDImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("VID");
  entry->decoder=(DecoderHandler *) ReadVIDImage;
  entry->encoder=(EncoderHandler *) WriteVIDImage;
  entry->description=ConstantString("Visual Image Directory");
  entry->module=ConstantString("VID");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r V I D I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterVIDImage() removes format registrations made by the
%  VID module from the list of supported formats.
%
%  The format of the UnregisterVIDImage method is:
%
%      UnregisterVIDImage(void)
%
*/
ModuleExport void UnregisterVIDImage(void)
{
  (void) UnregisterMagickInfo("VID");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e V I D I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteVIDImage() writes an image to a file in VID X image format.
%
%  The format of the WriteVIDImage method is:
%
%      MagickBooleanType WriteVIDImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteVIDImage(const ImageInfo *image_info,Image *image)
{
  Image
    *montage_image;

  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  MontageInfo
    *montage_info;

  register Image
    *p;

  /*
    Create the visual image directory.
  */
  for (p=image; p != (Image *) NULL; p=GetNextImageInList(p))
    (void) SetImageAttribute(p,"Label",DefaultTileLabel);
  montage_info=CloneMontageInfo(image_info,(MontageInfo *) NULL);
  montage_image=MontageImageList(image_info,montage_info,image,
    &image->exception);
  if (montage_image == (Image *) NULL)
    ThrowWriterException(CorruptImageError,image->exception.reason);
  (void) CopyMagickString(montage_image->filename,image_info->filename,
    MaxTextExtent);
  write_info=CloneImageInfo(image_info);
  (void) SetImageInfo(write_info,MagickTrue,&image->exception);
  if (LocaleCompare(write_info->magick,"VID") == 0)
    (void) FormatMagickString(montage_image->filename,MaxTextExtent,
      "miff:%s",image_info->filename);
  status=WriteImage(write_info,montage_image);
  montage_image=DestroyImage(montage_image);
  write_info=DestroyImageInfo(write_info);
  return(status);
}
