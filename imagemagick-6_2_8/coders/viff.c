/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        V   V  IIIII  FFFFF  FFFFF                           %
%                        V   V    I    F      F                               %
%                        V   V    I    FFF    FFF                             %
%                         V V     I    F      F                               %
%                          V    IIIII  F      F                               %
%                                                                             %
%                                                                             %
%                Read/Write Khoros Visualization Image Format.                %
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
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/quantum.h"
#include "magick/static.h"
#include "magick/string_.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteVIFFImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s V I F F                                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsVIFF() returns MagickTrue if the image format type, identified by the
%  magick string, is VIFF.
%
%  The format of the IsVIFF method is:
%
%      MagickBooleanType IsVIFF(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsVIFF(const unsigned char *magick,const size_t length)
{
  if (length < 2)
    return(MagickFalse);
  if (memcmp(magick,"\253\001",2) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d V I F F I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadVIFFImage() reads a Khoros Visualization image file and returns
%  it.  It allocates the memory necessary for the new Image structure and
%  returns a pointer to the new image.
%
%  The format of the ReadVIFFImage method is:
%
%      Image *ReadVIFFImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: Method ReadVIFFImage returns a pointer to the image after
%      reading.  A null image is returned if there is a memory shortage or if
%      the image cannot be read.
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadVIFFImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
#define VFF_CM_genericRGB  15
#define VFF_CM_ntscRGB  1
#define VFF_CM_NONE  0
#define VFF_DEP_DECORDER  0x4
#define VFF_DEP_NSORDER  0x8
#define VFF_DES_RAW  0
#define VFF_LOC_IMPLICIT  1
#define VFF_MAPTYP_NONE  0
#define VFF_MAPTYP_1_BYTE  1
#define VFF_MAPTYP_2_BYTE  2
#define VFF_MAPTYP_4_BYTE  4
#define VFF_MAPTYP_FLOAT  5
#define VFF_MAPTYP_DOUBLE  7
#define VFF_MS_NONE  0
#define VFF_MS_ONEPERBAND  1
#define VFF_MS_SHARED  3
#define VFF_TYP_BIT  0
#define VFF_TYP_1_BYTE  1
#define VFF_TYP_2_BYTE  2
#define VFF_TYP_4_BYTE  4
#define VFF_TYP_FLOAT  5
#define VFF_TYP_DOUBLE  9

  typedef struct _ViffInfo
  {
    unsigned char
      identifier,
      file_type,
      release,
      version,
      machine_dependency,
      reserve[3];

    char
      comment[512];

    unsigned long
      rows,
      columns,
      subrows;

    long
      x_offset,
      y_offset;

    float
      x_bits_per_pixel,
      y_bits_per_pixel;

    unsigned long
      location_type,
      location_dimension,
      number_of_images,
      number_data_bands,
      data_storage_type,
      data_encode_scheme,
      map_scheme,
      map_storage_type,
      map_rows,
      map_columns,
      map_subrows,
      map_enable,
      maps_per_cycle,
      color_space_model;
  } ViffInfo;

  double
    min_value,
    scale_factor,
    value;

  Image
    *image;

  int
    bit;

  long
    y;

  MagickBooleanType
    status;

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

  unsigned char
    buffer[7],
    *viff_pixels;

  unsigned long
    bytes_per_pixel,
    lsb_first,
    max_packets;

  ViffInfo
    viff_info;

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
    Read VIFF header (1024 bytes).
  */
  count=ReadBlob(image,1,&viff_info.identifier);
  do
  {
    /*
      Verify VIFF identifier.
    */
    if ((count == 0) || ((unsigned char) viff_info.identifier != 0xab))
      ThrowReaderException(CorruptImageError,"NotAVIFFImage");
    /*
      Initialize VIFF image.
    */
    count=ReadBlob(image,7,buffer);
    viff_info.file_type=buffer[0];
    viff_info.release=buffer[1];
    viff_info.version=buffer[2];
    viff_info.machine_dependency=buffer[3];
    count=ReadBlob(image,512,(unsigned char *) viff_info.comment);
    viff_info.comment[511]='\0';
    if (strlen(viff_info.comment) > 4)
      (void) SetImageAttribute(image,"Comment",viff_info.comment);
    if ((viff_info.machine_dependency == VFF_DEP_DECORDER) ||
        (viff_info.machine_dependency == VFF_DEP_NSORDER))
      {
        viff_info.rows=ReadBlobLSBLong(image);
        viff_info.columns=ReadBlobLSBLong(image);
        viff_info.subrows=ReadBlobLSBLong(image);
        viff_info.x_offset=(long) ReadBlobLSBLong(image);
        viff_info.y_offset=(long) ReadBlobLSBLong(image);
        viff_info.x_bits_per_pixel=(float) ReadBlobLSBLong(image);
        viff_info.y_bits_per_pixel=(float) ReadBlobLSBLong(image);
        viff_info.location_type=ReadBlobLSBLong(image);
        viff_info.location_dimension=ReadBlobLSBLong(image);
        viff_info.number_of_images=ReadBlobLSBLong(image);
        viff_info.number_data_bands=ReadBlobLSBLong(image);
        viff_info.data_storage_type=ReadBlobLSBLong(image);
        viff_info.data_encode_scheme=ReadBlobLSBLong(image);
        viff_info.map_scheme=ReadBlobLSBLong(image);
        viff_info.map_storage_type=ReadBlobLSBLong(image);
        viff_info.map_rows=ReadBlobLSBLong(image);
        viff_info.map_columns=ReadBlobLSBLong(image);
        viff_info.map_subrows=ReadBlobLSBLong(image);
        viff_info.map_enable=ReadBlobLSBLong(image);
        viff_info.maps_per_cycle=ReadBlobLSBLong(image);
        viff_info.color_space_model=ReadBlobLSBLong(image);
      }
    else
      {
        viff_info.rows=ReadBlobMSBLong(image);
        viff_info.columns=ReadBlobMSBLong(image);
        viff_info.subrows=ReadBlobMSBLong(image);
        viff_info.x_offset=(long) ReadBlobMSBLong(image);
        viff_info.y_offset=(long) ReadBlobMSBLong(image);
        viff_info.x_bits_per_pixel=(float) ReadBlobMSBLong(image);
        viff_info.y_bits_per_pixel=(float) ReadBlobMSBLong(image);
        viff_info.location_type=ReadBlobMSBLong(image);
        viff_info.location_dimension=ReadBlobMSBLong(image);
        viff_info.number_of_images=ReadBlobMSBLong(image);
        viff_info.number_data_bands=ReadBlobMSBLong(image);
        viff_info.data_storage_type=ReadBlobMSBLong(image);
        viff_info.data_encode_scheme=ReadBlobMSBLong(image);
        viff_info.map_scheme=ReadBlobMSBLong(image);
        viff_info.map_storage_type=ReadBlobMSBLong(image);
        viff_info.map_rows=ReadBlobMSBLong(image);
        viff_info.map_columns=ReadBlobMSBLong(image);
        viff_info.map_subrows=ReadBlobMSBLong(image);
        viff_info.map_enable=ReadBlobMSBLong(image);
        viff_info.maps_per_cycle=ReadBlobMSBLong(image);
        viff_info.color_space_model=ReadBlobMSBLong(image);
      }
    for (i=0; i < 420; i++)
      (void) ReadBlobByte(image);
    image->columns=viff_info.rows;
    image->rows=viff_info.columns;
    image->depth=viff_info.x_bits_per_pixel <= 8 ? 8 : QuantumDepth;
    /*
      Verify that we can read this VIFF image.
    */
    number_pixels=(MagickSizeType) viff_info.columns*viff_info.rows;
    if (number_pixels != (size_t) number_pixels)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    if (number_pixels == 0)
      ThrowReaderException(CoderError,"ImageColumnOrRowSizeIsNotSupported");
    if ((viff_info.data_storage_type != VFF_TYP_BIT) &&
        (viff_info.data_storage_type != VFF_TYP_1_BYTE) &&
        (viff_info.data_storage_type != VFF_TYP_2_BYTE) &&
        (viff_info.data_storage_type != VFF_TYP_4_BYTE) &&
        (viff_info.data_storage_type != VFF_TYP_FLOAT) &&
        (viff_info.data_storage_type != VFF_TYP_DOUBLE))
      ThrowReaderException(CoderError,"DataStorageTypeIsNotSupported");
    if (viff_info.data_encode_scheme != VFF_DES_RAW)
      ThrowReaderException(CoderError,"DataEncodingSchemeIsNotSupported");
    if ((viff_info.map_storage_type != VFF_MAPTYP_NONE) &&
        (viff_info.map_storage_type != VFF_MAPTYP_1_BYTE) &&
        (viff_info.map_storage_type != VFF_MAPTYP_2_BYTE) &&
        (viff_info.map_storage_type != VFF_MAPTYP_4_BYTE) &&
        (viff_info.map_storage_type != VFF_MAPTYP_FLOAT) &&
        (viff_info.map_storage_type != VFF_MAPTYP_DOUBLE))
      ThrowReaderException(CoderError,"MapStorageTypeIsNotSupported");
    if ((viff_info.color_space_model != VFF_CM_NONE) &&
        (viff_info.color_space_model != VFF_CM_ntscRGB) &&
        (viff_info.color_space_model != VFF_CM_genericRGB))
      ThrowReaderException(CoderError,"ColorspaceModelIsNotSupported");
    if (viff_info.location_type != VFF_LOC_IMPLICIT)
      ThrowReaderException(CoderError,"LocationTypeIsNotSupported");
    if (viff_info.number_of_images != 1)
      ThrowReaderException(CoderError,"NumberOfImagesIsNotSupported");
    if (viff_info.map_rows == 0)
      viff_info.map_scheme=VFF_MS_NONE;
    switch ((int) viff_info.map_scheme)
    {
      case VFF_MS_NONE:
      {
        if (viff_info.number_data_bands < 3)
          {
            /*
              Create linear color ramp.
            */
            image->colors=image->depth <= 8 ? 256 : 65536L;
            if (viff_info.data_storage_type == VFF_TYP_BIT)
              image->colors=2;
            if (AllocateImageColormap(image,image->colors) == MagickFalse)
              ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        break;
      }
      case VFF_MS_ONEPERBAND:
      case VFF_MS_SHARED:
      {
        unsigned char
          *viff_colormap;

        /*
          Allocate VIFF colormap.
        */
        switch ((int) viff_info.map_storage_type)
        {
          case VFF_MAPTYP_1_BYTE: bytes_per_pixel=1; break;
          case VFF_MAPTYP_2_BYTE: bytes_per_pixel=2; break;
          case VFF_MAPTYP_4_BYTE: bytes_per_pixel=4; break;
          case VFF_MAPTYP_FLOAT: bytes_per_pixel=4; break;
          case VFF_MAPTYP_DOUBLE: bytes_per_pixel=8; break;
          default: bytes_per_pixel=1; break;
        }
        image->colors=viff_info.map_columns;
        if (AllocateImageColormap(image,image->colors) == MagickFalse)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        viff_colormap=(unsigned char *)
          AcquireMagickMemory(bytes_per_pixel*image->colors*viff_info.map_rows);
        if (viff_colormap == (unsigned char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        /*
          Read VIFF raster colormap.
        */
        count=ReadBlob(image,bytes_per_pixel*image->colors*viff_info.map_rows,
          viff_colormap);
        lsb_first=1;
        if (*(char *) &lsb_first &&
            ((viff_info.machine_dependency != VFF_DEP_DECORDER) &&
             (viff_info.machine_dependency != VFF_DEP_NSORDER)))
          switch ((int) viff_info.map_storage_type)
          {
            case VFF_MAPTYP_2_BYTE:
            {
              MSBOrderShort(viff_colormap,(bytes_per_pixel*image->colors*
                viff_info.map_rows));
              break;
            }
            case VFF_MAPTYP_4_BYTE:
            case VFF_MAPTYP_FLOAT:
            {
              MSBOrderLong(viff_colormap,(bytes_per_pixel*image->colors*
                viff_info.map_rows));
              break;
            }
            default: break;
          }
        for (i=0; i < (long) (viff_info.map_rows*image->colors); i++)
        {
          switch ((int) viff_info.map_storage_type)
          {
            case VFF_MAPTYP_2_BYTE: value=((short *) viff_colormap)[i]; break;
            case VFF_MAPTYP_4_BYTE: value=((int *) viff_colormap)[i]; break;
            case VFF_MAPTYP_FLOAT: value=((float *) viff_colormap)[i]; break;
            case VFF_MAPTYP_DOUBLE: value=((double *) viff_colormap)[i]; break;
            default: value=viff_colormap[i]; break;
          }
          if (i < (long) image->colors)
            {
              image->colormap[i].red=ScaleCharToQuantum((unsigned char) value);
              image->colormap[i].green=
                ScaleCharToQuantum((unsigned char) value);
              image->colormap[i].blue=ScaleCharToQuantum((unsigned char) value);
            }
          else
            if (i < (long) (2*image->colors))
              image->colormap[i % image->colors].green=
                ScaleCharToQuantum((unsigned char) value);
            else
              if (i < (long) (3*image->colors))
                image->colormap[i % image->colors].blue=
                  ScaleCharToQuantum((unsigned char) value);
        }
        viff_colormap=(unsigned char *) RelinquishMagickMemory(viff_colormap);
        break;
      }
      default:
        ThrowReaderException(CoderError,"ColormapTypeNotSupported");
    }
    /*
      Initialize image structure.
    */
    image->matte=viff_info.number_data_bands == 4 ? MagickTrue : MagickFalse;
    image->storage_class=
      (viff_info.number_data_bands < 3 ? PseudoClass : DirectClass);
    image->columns=viff_info.rows;
    image->rows=viff_info.columns;
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    /*
      Allocate VIFF pixels.
    */
    switch ((int) viff_info.data_storage_type)
    {
      case VFF_TYP_2_BYTE: bytes_per_pixel=2; break;
      case VFF_TYP_4_BYTE: bytes_per_pixel=4; break;
      case VFF_TYP_FLOAT: bytes_per_pixel=4; break;
      case VFF_TYP_DOUBLE: bytes_per_pixel=8; break;
      default: bytes_per_pixel=1; break;
    }
    if (viff_info.data_storage_type == VFF_TYP_BIT)
      max_packets=((image->columns+7) >> 3)*image->rows;
    else
      max_packets=number_pixels*viff_info.number_data_bands;
    viff_pixels=(unsigned char *)
      AcquireMagickMemory(bytes_per_pixel*max_packets*sizeof(Quantum));
    if (viff_pixels == (unsigned char *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    count=ReadBlob(image,bytes_per_pixel*max_packets,viff_pixels);
    lsb_first=1;
    if (*(char *) &lsb_first &&
        ((viff_info.machine_dependency != VFF_DEP_DECORDER) &&
         (viff_info.machine_dependency != VFF_DEP_NSORDER)))
      switch ((int) viff_info.data_storage_type)
      {
        case VFF_TYP_2_BYTE:
        {
          MSBOrderShort(viff_pixels,bytes_per_pixel*max_packets);
          break;
        }
        case VFF_TYP_4_BYTE:
        case VFF_TYP_FLOAT:
        {
          MSBOrderLong(viff_pixels,bytes_per_pixel*max_packets);
          break;
        }
        default: break;
      }
    min_value=0.0;
    scale_factor=1.0;
    if ((viff_info.data_storage_type != VFF_TYP_1_BYTE) &&
        (viff_info.map_scheme == VFF_MS_NONE))
      {
        double
          max_value;

        /*
          Determine scale factor.
        */
        switch ((int) viff_info.data_storage_type)
        {
          case VFF_TYP_2_BYTE: value=((short *) viff_pixels)[0]; break;
          case VFF_TYP_4_BYTE: value=((int *) viff_pixels)[0]; break;
          case VFF_TYP_FLOAT: value=((float *) viff_pixels)[0]; break;
          case VFF_TYP_DOUBLE: value=((double *) viff_pixels)[0]; break;
          default: value=viff_pixels[0]; break;
        }
        max_value=value;
        min_value=value;
        for (i=0; i < (long) max_packets; i++)
        {
          switch ((int) viff_info.data_storage_type)
          {
            case VFF_TYP_2_BYTE: value=((short *) viff_pixels)[i]; break;
            case VFF_TYP_4_BYTE: value=((int *) viff_pixels)[i]; break;
            case VFF_TYP_FLOAT: value=((float *) viff_pixels)[i]; break;
            case VFF_TYP_DOUBLE: value=((double *) viff_pixels)[i]; break;
            default: value=viff_pixels[i]; break;
          }
          if (value > max_value)
            max_value=value;
          else
            if (value < min_value)
              min_value=value;
        }
        if ((min_value == 0) && (max_value == 0))
          scale_factor=0;
        else
          if (min_value == max_value)
            {
              scale_factor=QuantumRange/min_value;
              min_value=0;
            }
          else
            scale_factor=QuantumRange/(max_value-min_value);
      }
    /*
      Convert pixels to Quantum size.
    */
    p=(unsigned char *) viff_pixels;
    for (i=0; i < (long) max_packets; i++)
    {
      switch ((int) viff_info.data_storage_type)
      {
        case VFF_TYP_2_BYTE: value=((short *) viff_pixels)[i]; break;
        case VFF_TYP_4_BYTE: value=((int *) viff_pixels)[i]; break;
        case VFF_TYP_FLOAT: value=((float *) viff_pixels)[i]; break;
        case VFF_TYP_DOUBLE: value=((double *) viff_pixels)[i]; break;
        default: value=viff_pixels[i]; break;
      }
      if (viff_info.map_scheme == VFF_MS_NONE)
        {
          value=(value-min_value)*scale_factor;
          if (value > QuantumRange)
            value=QuantumRange;
          else
            if (value < 0)
              value=0;
        }
      *p=(Quantum) value;
      p++;
    }
    /*
      Convert VIFF raster image to pixel packets.
    */
    p=(unsigned char *) viff_pixels;
    if (viff_info.data_storage_type == VFF_TYP_BIT)
      {
        unsigned int
          polarity;

        /*
          Convert bitmap scanline.
        */
        SetImageType(image,BilevelType);
        polarity=PixelIntensityToQuantum(&image->colormap[0]) < (QuantumRange/2);
        if (image->colors >= 2)
          polarity=PixelIntensityToQuantum(&image->colormap[0]) >
            PixelIntensityToQuantum(&image->colormap[1]);
        for (y=0; y < (long) image->rows; y++)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          for (x=0; x < (long) (image->columns-7); x+=8)
          {
            for (bit=0; bit < 8; bit++)
              indexes[x+bit]=(IndexPacket)
                ((*p) & (0x01 << bit) ? !polarity : polarity);
            p++;
          }
          if ((image->columns % 8) != 0)
            {
              for (bit=0; bit < (long) (image->columns % 8); bit++)
                indexes[x+bit]=(IndexPacket)
                  ((*p) & (0x01 << bit) ? !polarity : polarity);
              p++;
            }
          if (SyncImagePixels(image) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(y,image->rows) != MagickFalse))
              {
                status=image->progress_monitor(LoadImageTag,y,image->rows,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
        }
      }
    else
      if (image->storage_class == PseudoClass)
        for (y=0; y < (long) image->rows; y++)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          for (x=0; x < (long) image->columns; x++)
            indexes[x]=(*p++);
          if (SyncImagePixels(image) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                (QuantumTick(y,image->rows) != MagickFalse))
              {
                status=image->progress_monitor(LoadImageTag,y,image->rows,
                  image->client_data);
                if (status == MagickFalse)
                  break;
              }
        }
      else
        {
          /*
            Convert DirectColor scanline.
          */
          number_pixels=(MagickSizeType) image->columns*image->rows;
          for (y=0; y < (long) image->rows; y++)
          {
            q=SetImagePixels(image,0,y,image->columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) image->columns; x++)
            {
              q->red=ScaleCharToQuantum(*p);
              q->green=ScaleCharToQuantum(*(p+number_pixels));
              q->blue=ScaleCharToQuantum(*(p+2*number_pixels));
              if (image->colors != 0)
                {
                  q->red=image->colormap[q->red].red;
                  q->green=image->colormap[q->green].green;
                  q->blue=image->colormap[q->blue].blue;
                }
              q->opacity=(Quantum) (image->matte ? QuantumRange-
                ScaleCharToQuantum(*(p+number_pixels*3)) : OpaqueOpacity);
              p++;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
            if (image->previous == (Image *) NULL)
              if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
                  (QuantumTick(y,image->rows) != MagickFalse))
                {
                  status=image->progress_monitor(LoadImageTag,y,image->rows,
                    image->client_data);
                  if (status == MagickFalse)
                    break;
                }
          }
        }
    viff_pixels=(unsigned char *) RelinquishMagickMemory(viff_pixels);
    if (image->storage_class == PseudoClass)
      (void) SyncImage(image);
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
    count=ReadBlob(image,1,&viff_info.identifier);
    if ((count != 0) && (viff_info.identifier == 0xab))
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
  } while ((count != 0) && (viff_info.identifier == 0xab));
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r V I F F I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterVIFFImage() adds attributes for the VIFF image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterVIFFImage method is:
%
%      RegisterVIFFImage(void)
%
*/
ModuleExport void RegisterVIFFImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("VIFF");
  entry->decoder=(DecoderHandler *) ReadVIFFImage;
  entry->encoder=(EncoderHandler *) WriteVIFFImage;
  entry->magick=(MagickHandler *) IsVIFF;
  entry->description=ConstantString("Khoros Visualization image");
  entry->module=ConstantString("VIFF");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("XV");
  entry->decoder=(DecoderHandler *) ReadVIFFImage;
  entry->encoder=(EncoderHandler *) WriteVIFFImage;
  entry->description=ConstantString("Khoros Visualization image");
  entry->module=ConstantString("VIFF");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r V I F F I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterVIFFImage() removes format registrations made by the
%  VIFF module from the list of supported formats.
%
%  The format of the UnregisterVIFFImage method is:
%
%      UnregisterVIFFImage(void)
%
*/
ModuleExport void UnregisterVIFFImage(void)
{
  (void) UnregisterMagickInfo("VIFF");
  (void) UnregisterMagickInfo("XV");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e V I F F I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteVIFFImage() writes an image to a file in the VIFF image format.
%
%  The format of the WriteVIFFImage method is:
%
%      MagickBooleanType WriteVIFFImage(const ImageInfo *image_info,
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
static MagickBooleanType WriteVIFFImage(const ImageInfo *image_info,
  Image *image)
{
#define VFF_CM_genericRGB  15
#define VFF_CM_NONE  0
#define VFF_DEP_IEEEORDER  0x2
#define VFF_DES_RAW  0
#define VFF_LOC_IMPLICIT  1
#define VFF_MAPTYP_NONE  0
#define VFF_MAPTYP_1_BYTE  1
#define VFF_MS_NONE  0
#define VFF_MS_ONEPERBAND  1
#define VFF_TYP_BIT  0
#define VFF_TYP_1_BYTE  1

  typedef struct _ViffInfo
  {
    char
      identifier,
      file_type,
      release,
      version,
      machine_dependency,
      reserve[3],
      comment[512];

    unsigned long
      rows,
      columns,
      subrows;

    long
      x_offset,
      y_offset;

    unsigned int
      x_bits_per_pixel,
      y_bits_per_pixel;

    unsigned long
      location_type,
      location_dimension,
      number_of_images,
      number_data_bands,
      data_storage_type,
      data_encode_scheme,
      map_scheme,
      map_storage_type,
      map_rows,
      map_columns,
      map_subrows,
      map_enable,
      maps_per_cycle,
      color_space_model;
  } ViffInfo;

  const ImageAttribute
    *attribute;

  long
    y;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  MagickSizeType
    number_pixels,
    packets;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    x;

  register long
    i;

  register unsigned char
    *q;

  unsigned char
    buffer[8],
    *viff_pixels;

  ViffInfo
    viff_info;

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
  (void) ResetMagickMemory(&viff_info,0,sizeof(ViffInfo));
  scene=0;
  do
  {
    /*
      Initialize VIFF image structure.
    */
    if (image_info->colorspace == UndefinedColorspace)
      (void) SetImageColorspace(image,RGBColorspace);
    if (IsGrayImage(image,&image->exception) != MagickFalse)
      (void) SetImageStorageClass(image,DirectClass);
    viff_info.identifier=(char) 0xab;
    viff_info.file_type=1;
    viff_info.release=1;
    viff_info.version=3;
    viff_info.machine_dependency=VFF_DEP_IEEEORDER;  /* IEEE byte ordering */
    *viff_info.comment='\0';
    attribute=GetImageAttribute(image,"Comment");
    if (attribute != (const ImageAttribute *) NULL)
      (void) CopyMagickString(viff_info.comment,attribute->value,
        Min(strlen(attribute->value),511)+1);
    viff_info.rows=image->columns;
    viff_info.columns=image->rows;
    viff_info.subrows=0;
    viff_info.x_offset=(~0);
    viff_info.y_offset=(~0);
    viff_info.x_bits_per_pixel=0;
    viff_info.y_bits_per_pixel=0;
    viff_info.location_type=VFF_LOC_IMPLICIT;
    viff_info.location_dimension=0;
    viff_info.number_of_images=1;
    viff_info.data_encode_scheme=VFF_DES_RAW;
    viff_info.map_scheme=VFF_MS_NONE;
    viff_info.map_storage_type=VFF_MAPTYP_NONE;
    viff_info.map_rows=0;
    viff_info.map_columns=0;
    viff_info.map_subrows=0;
    viff_info.map_enable=1;  /* no colormap */
    viff_info.maps_per_cycle=0;
    number_pixels=(MagickSizeType) image->columns*image->rows;
    if (image->storage_class == DirectClass)
      {
        /*
          Full color VIFF raster.
        */
        viff_info.number_data_bands=image->matte ? 4 : 3;
        viff_info.color_space_model=VFF_CM_genericRGB;
        viff_info.data_storage_type=VFF_TYP_1_BYTE;
        packets=viff_info.number_data_bands*number_pixels;
      }
    else
      {
        viff_info.number_data_bands=1;
        viff_info.color_space_model=VFF_CM_NONE;
        viff_info.data_storage_type=VFF_TYP_1_BYTE;
        packets=number_pixels;
        if (IsGrayImage(image,&image->exception) == MagickFalse)
          {
            /*
              Colormapped VIFF raster.
            */
            viff_info.map_scheme=VFF_MS_ONEPERBAND;
            viff_info.map_storage_type=VFF_MAPTYP_1_BYTE;
            viff_info.map_rows=3;
            viff_info.map_columns=image->colors;
          }
        else
          if (image->colors <= 2)
            {
              /*
                Monochrome VIFF raster.
              */
              viff_info.data_storage_type=VFF_TYP_BIT;
              packets=((image->columns+7) >> 3)*image->rows;
            }
      }
    /*
      Write VIFF image header (pad to 1024 bytes).
    */
    buffer[0]=viff_info.identifier;
    buffer[1]=viff_info.file_type;
    buffer[2]=viff_info.release;
    buffer[3]=viff_info.version;
    buffer[4]=viff_info.machine_dependency;
    buffer[5]=viff_info.reserve[0];
    buffer[6]=viff_info.reserve[1];
    buffer[7]=viff_info.reserve[2];
    (void) WriteBlob(image,8,buffer);
    (void) WriteBlob(image,512,(unsigned char *) viff_info.comment);
    (void) WriteBlobMSBLong(image,viff_info.rows);
    (void) WriteBlobMSBLong(image,viff_info.columns);
    (void) WriteBlobMSBLong(image,viff_info.subrows);
    (void) WriteBlobMSBLong(image,(unsigned long) viff_info.x_offset);
    (void) WriteBlobMSBLong(image,(unsigned long) viff_info.y_offset);
    viff_info.x_bits_per_pixel=(63 << 24) | (128 << 16);
    (void) WriteBlobMSBLong(image,(unsigned long) viff_info.x_bits_per_pixel);
    viff_info.y_bits_per_pixel=(63 << 24) | (128 << 16);
    (void) WriteBlobMSBLong(image,(unsigned long) viff_info.y_bits_per_pixel);
    (void) WriteBlobMSBLong(image,viff_info.location_type);
    (void) WriteBlobMSBLong(image,viff_info.location_dimension);
    (void) WriteBlobMSBLong(image,viff_info.number_of_images);
    (void) WriteBlobMSBLong(image,viff_info.number_data_bands);
    (void) WriteBlobMSBLong(image,viff_info.data_storage_type);
    (void) WriteBlobMSBLong(image,viff_info.data_encode_scheme);
    (void) WriteBlobMSBLong(image,viff_info.map_scheme);
    (void) WriteBlobMSBLong(image,viff_info.map_storage_type);
    (void) WriteBlobMSBLong(image,viff_info.map_rows);
    (void) WriteBlobMSBLong(image,viff_info.map_columns);
    (void) WriteBlobMSBLong(image,viff_info.map_subrows);
    (void) WriteBlobMSBLong(image,viff_info.map_enable);
    (void) WriteBlobMSBLong(image,viff_info.maps_per_cycle);
    (void) WriteBlobMSBLong(image,viff_info.color_space_model);
    for (i=0; i < 420; i++)
      (void) WriteBlobByte(image,'\0');
    /*
      Convert MIFF to VIFF raster pixels.
    */
    viff_pixels=(unsigned char *)
      AcquireMagickMemory((size_t) packets*sizeof(*viff_pixels));
    if (viff_pixels == (unsigned char *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    q=viff_pixels;
    if (image->storage_class == DirectClass)
      {
        /*
          Convert DirectClass packet to VIFF RGB pixel.
        */
        number_pixels=(MagickSizeType) image->columns*image->rows;
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            *q=ScaleQuantumToChar(p->red);
            *(q+number_pixels)=ScaleQuantumToChar(p->green);
            *(q+number_pixels*2)=ScaleQuantumToChar(p->blue);
            if (image->matte != MagickFalse)
              *(q+number_pixels*3)=ScaleQuantumToChar((Quantum)
                (QuantumRange-p->opacity));
            p++;
            q++;
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
      }
    else
      if (IsGrayImage(image,&image->exception) == MagickFalse)
        {
          unsigned char
            *viff_colormap;

          /*
            Dump colormap to file.
          */
          viff_colormap=(unsigned char *) AcquireMagickMemory(3*image->colors);
          if (viff_colormap == (unsigned char *) NULL)
            ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
          q=viff_colormap;
          for (i=0; i < (long) image->colors; i++)
            *q++=ScaleQuantumToChar(image->colormap[i].red);
          for (i=0; i < (long) image->colors; i++)
            *q++=ScaleQuantumToChar(image->colormap[i].green);
          for (i=0; i < (long) image->colors; i++)
            *q++=ScaleQuantumToChar(image->colormap[i].blue);
          (void) WriteBlob(image,3*image->colors,viff_colormap);
          viff_colormap=(unsigned char *) RelinquishMagickMemory(viff_colormap);
          /*
            Convert PseudoClass packet to VIFF colormapped pixels.
          */
          q=viff_pixels;
          for (y=0; y < (long) image->rows; y++)
          {
            p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
            if (p == (const PixelPacket *) NULL)
              break;
            indexes=GetIndexes(image);
            for (x=0; x < (long) image->columns; x++)
              *q++=indexes[x];
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
        }
      else
        if (image->colors <= 2)
          {
            long
              x,
              y;

            register unsigned char
              bit,
              byte,
              polarity;

            /*
              Convert PseudoClass image to a VIFF monochrome image.
            */
            SetImageType(image,BilevelType);
            polarity=PixelIntensityToQuantum(&image->colormap[0]) < (QuantumRange/2);
            if (image->colors == 2)
              polarity=PixelIntensityToQuantum(&image->colormap[0]) <
                PixelIntensityToQuantum(&image->colormap[1]);
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              indexes=GetIndexes(image);
              bit=0;
              byte=0;
              for (x=0; x < (long) image->columns; x++)
              {
                byte>>=1;
                if (indexes[x] == polarity)
                  byte|=0x80;
                bit++;
                if (bit == 8)
                  {
                    *q++=byte;
                    bit=0;
                    byte=0;
                  }
              }
              if (bit != 0)
                *q++=byte >> (8-bit);
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
          }
        else
          {
            /*
              Convert PseudoClass packet to VIFF grayscale pixel.
            */
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              for (x=0; x < (long) image->columns; x++)
              {
                *q++=(unsigned char) PixelIntensityToQuantum(p);
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
          }
    (void) WriteBlob(image,(size_t) packets,viff_pixels);
    viff_pixels=(unsigned char *) RelinquishMagickMemory(viff_pixels);
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
