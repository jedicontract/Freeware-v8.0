/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%           CCCC   OOO   M   M  PPPP   RRRR   EEEEE   SSSSS  SSSSS            %
%          C      O   O  MM MM  P   P  R   R  E       SS     SS               %
%          C      O   O  M M M  PPPP   RRRR   EEE      SSS    SSS             %
%          C      O   O  M   M  P      R R    E          SS     SS            %
%           CCCC   OOO   M   M  P      R  R   EEEEE   SSSSS  SSSSS            %
%                                                                             %
%                                                                             %
%                  Image Compression/Decompression Methods                    %
%                                                                             %
%                           Software Design                                   %
%                             John Cristy                                     %
%                              May  1993                                      %
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
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/color-private.h"
#include "magick/compress.h"
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/resource_.h"
#include "magick/string_.h"
#if defined(HasTIFF)
#if defined(HAVE_TIFFCONF_H)
#include "tiffconf.h"
#endif
#include "tiffio.h"
#define CCITTParam  "-1"
#else
#define CCITTParam  "0"
#endif
#if defined(HasZLIB)
#include "zlib.h"
#endif

/*
  Typedef declarations.
*/
typedef struct _Ascii85Info
{
  long
    offset,
    line_break;

  unsigned char
    buffer[10];
} Ascii85Info;

typedef struct HuffmanTable
{
  unsigned long
    id,
    code,
    length,
    count;
} HuffmanTable;

/*
  Huffman coding declarations.
*/
#define TWId  23
#define MWId  24
#define TBId  25
#define MBId  26
#define EXId  27

static const HuffmanTable
  MBTable[]=
  {
    { MBId, 0x0f, 10, 64 }, { MBId, 0xc8, 12, 128 },
    { MBId, 0xc9, 12, 192 }, { MBId, 0x5b, 12, 256 },
    { MBId, 0x33, 12, 320 }, { MBId, 0x34, 12, 384 },
    { MBId, 0x35, 12, 448 }, { MBId, 0x6c, 13, 512 },
    { MBId, 0x6d, 13, 576 }, { MBId, 0x4a, 13, 640 },
    { MBId, 0x4b, 13, 704 }, { MBId, 0x4c, 13, 768 },
    { MBId, 0x4d, 13, 832 }, { MBId, 0x72, 13, 896 },
    { MBId, 0x73, 13, 960 }, { MBId, 0x74, 13, 1024 },
    { MBId, 0x75, 13, 1088 }, { MBId, 0x76, 13, 1152 },
    { MBId, 0x77, 13, 1216 }, { MBId, 0x52, 13, 1280 },
    { MBId, 0x53, 13, 1344 }, { MBId, 0x54, 13, 1408 },
    { MBId, 0x55, 13, 1472 }, { MBId, 0x5a, 13, 1536 },
    { MBId, 0x5b, 13, 1600 }, { MBId, 0x64, 13, 1664 },
    { MBId, 0x65, 13, 1728 }, { MBId, 0x00, 0, 0 }
  };

static const HuffmanTable
  EXTable[]=
  {
    { EXId, 0x08, 11, 1792 }, { EXId, 0x0c, 11, 1856 },
    { EXId, 0x0d, 11, 1920 }, { EXId, 0x12, 12, 1984 },
    { EXId, 0x13, 12, 2048 }, { EXId, 0x14, 12, 2112 },
    { EXId, 0x15, 12, 2176 }, { EXId, 0x16, 12, 2240 },
    { EXId, 0x17, 12, 2304 }, { EXId, 0x1c, 12, 2368 },
    { EXId, 0x1d, 12, 2432 }, { EXId, 0x1e, 12, 2496 },
    { EXId, 0x1f, 12, 2560 }, { EXId, 0x00, 0, 0 }
  };

static const HuffmanTable
  MWTable[]=
  {
    { MWId, 0x1b, 5, 64 }, { MWId, 0x12, 5, 128 },
    { MWId, 0x17, 6, 192 }, { MWId, 0x37, 7, 256 },
    { MWId, 0x36, 8, 320 }, { MWId, 0x37, 8, 384 },
    { MWId, 0x64, 8, 448 }, { MWId, 0x65, 8, 512 },
    { MWId, 0x68, 8, 576 }, { MWId, 0x67, 8, 640 },
    { MWId, 0xcc, 9, 704 }, { MWId, 0xcd, 9, 768 },
    { MWId, 0xd2, 9, 832 }, { MWId, 0xd3, 9, 896 },
    { MWId, 0xd4, 9, 960 }, { MWId, 0xd5, 9, 1024 },
    { MWId, 0xd6, 9, 1088 }, { MWId, 0xd7, 9, 1152 },
    { MWId, 0xd8, 9, 1216 }, { MWId, 0xd9, 9, 1280 },
    { MWId, 0xda, 9, 1344 }, { MWId, 0xdb, 9, 1408 },
    { MWId, 0x98, 9, 1472 }, { MWId, 0x99, 9, 1536 },
    { MWId, 0x9a, 9, 1600 }, { MWId, 0x18, 6, 1664 },
    { MWId, 0x9b, 9, 1728 }, { MWId, 0x00, 0, 0 }
  };

static const HuffmanTable
  TBTable[]=
  {
    { TBId, 0x37, 10, 0 }, { TBId, 0x02, 3, 1 }, { TBId, 0x03, 2, 2 },
    { TBId, 0x02, 2, 3 }, { TBId, 0x03, 3, 4 }, { TBId, 0x03, 4, 5 },
    { TBId, 0x02, 4, 6 }, { TBId, 0x03, 5, 7 }, { TBId, 0x05, 6, 8 },
    { TBId, 0x04, 6, 9 }, { TBId, 0x04, 7, 10 }, { TBId, 0x05, 7, 11 },
    { TBId, 0x07, 7, 12 }, { TBId, 0x04, 8, 13 }, { TBId, 0x07, 8, 14 },
    { TBId, 0x18, 9, 15 }, { TBId, 0x17, 10, 16 }, { TBId, 0x18, 10, 17 },
    { TBId, 0x08, 10, 18 }, { TBId, 0x67, 11, 19 }, { TBId, 0x68, 11, 20 },
    { TBId, 0x6c, 11, 21 }, { TBId, 0x37, 11, 22 }, { TBId, 0x28, 11, 23 },
    { TBId, 0x17, 11, 24 }, { TBId, 0x18, 11, 25 }, { TBId, 0xca, 12, 26 },
    { TBId, 0xcb, 12, 27 }, { TBId, 0xcc, 12, 28 }, { TBId, 0xcd, 12, 29 },
    { TBId, 0x68, 12, 30 }, { TBId, 0x69, 12, 31 }, { TBId, 0x6a, 12, 32 },
    { TBId, 0x6b, 12, 33 }, { TBId, 0xd2, 12, 34 }, { TBId, 0xd3, 12, 35 },
    { TBId, 0xd4, 12, 36 }, { TBId, 0xd5, 12, 37 }, { TBId, 0xd6, 12, 38 },
    { TBId, 0xd7, 12, 39 }, { TBId, 0x6c, 12, 40 }, { TBId, 0x6d, 12, 41 },
    { TBId, 0xda, 12, 42 }, { TBId, 0xdb, 12, 43 }, { TBId, 0x54, 12, 44 },
    { TBId, 0x55, 12, 45 }, { TBId, 0x56, 12, 46 }, { TBId, 0x57, 12, 47 },
    { TBId, 0x64, 12, 48 }, { TBId, 0x65, 12, 49 }, { TBId, 0x52, 12, 50 },
    { TBId, 0x53, 12, 51 }, { TBId, 0x24, 12, 52 }, { TBId, 0x37, 12, 53 },
    { TBId, 0x38, 12, 54 }, { TBId, 0x27, 12, 55 }, { TBId, 0x28, 12, 56 },
    { TBId, 0x58, 12, 57 }, { TBId, 0x59, 12, 58 }, { TBId, 0x2b, 12, 59 },
    { TBId, 0x2c, 12, 60 }, { TBId, 0x5a, 12, 61 }, { TBId, 0x66, 12, 62 },
    { TBId, 0x67, 12, 63 }, { TBId, 0x00, 0, 0 }
  };

static const HuffmanTable
  TWTable[]=
  {
    { TWId, 0x35, 8, 0 }, { TWId, 0x07, 6, 1 }, { TWId, 0x07, 4, 2 },
    { TWId, 0x08, 4, 3 }, { TWId, 0x0b, 4, 4 }, { TWId, 0x0c, 4, 5 },
    { TWId, 0x0e, 4, 6 }, { TWId, 0x0f, 4, 7 }, { TWId, 0x13, 5, 8 },
    { TWId, 0x14, 5, 9 }, { TWId, 0x07, 5, 10 }, { TWId, 0x08, 5, 11 },
    { TWId, 0x08, 6, 12 }, { TWId, 0x03, 6, 13 }, { TWId, 0x34, 6, 14 },
    { TWId, 0x35, 6, 15 }, { TWId, 0x2a, 6, 16 }, { TWId, 0x2b, 6, 17 },
    { TWId, 0x27, 7, 18 }, { TWId, 0x0c, 7, 19 }, { TWId, 0x08, 7, 20 },
    { TWId, 0x17, 7, 21 }, { TWId, 0x03, 7, 22 }, { TWId, 0x04, 7, 23 },
    { TWId, 0x28, 7, 24 }, { TWId, 0x2b, 7, 25 }, { TWId, 0x13, 7, 26 },
    { TWId, 0x24, 7, 27 }, { TWId, 0x18, 7, 28 }, { TWId, 0x02, 8, 29 },
    { TWId, 0x03, 8, 30 }, { TWId, 0x1a, 8, 31 }, { TWId, 0x1b, 8, 32 },
    { TWId, 0x12, 8, 33 }, { TWId, 0x13, 8, 34 }, { TWId, 0x14, 8, 35 },
    { TWId, 0x15, 8, 36 }, { TWId, 0x16, 8, 37 }, { TWId, 0x17, 8, 38 },
    { TWId, 0x28, 8, 39 }, { TWId, 0x29, 8, 40 }, { TWId, 0x2a, 8, 41 },
    { TWId, 0x2b, 8, 42 }, { TWId, 0x2c, 8, 43 }, { TWId, 0x2d, 8, 44 },
    { TWId, 0x04, 8, 45 }, { TWId, 0x05, 8, 46 }, { TWId, 0x0a, 8, 47 },
    { TWId, 0x0b, 8, 48 }, { TWId, 0x52, 8, 49 }, { TWId, 0x53, 8, 50 },
    { TWId, 0x54, 8, 51 }, { TWId, 0x55, 8, 52 }, { TWId, 0x24, 8, 53 },
    { TWId, 0x25, 8, 54 }, { TWId, 0x58, 8, 55 }, { TWId, 0x59, 8, 56 },
    { TWId, 0x5a, 8, 57 }, { TWId, 0x5b, 8, 58 }, { TWId, 0x4a, 8, 59 },
    { TWId, 0x4b, 8, 60 }, { TWId, 0x32, 8, 61 }, { TWId, 0x33, 8, 62 },
    { TWId, 0x34, 8, 63 }, { TWId, 0x00, 0, 0 }
  };

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A S C I I 8 5 E n c o d e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ASCII85Encode() encodes data in ASCII base-85 format.  ASCII base-85
%  encoding produces five ASCII printing characters from every four bytes of
%  binary data.
%
%  The format of the ASCII85Encode method is:
%
%      void Ascii85Encode(Image *image,const unsigned long code)
%
%  A description of each parameter follows:
%
%    o code: a binary unsigned char to encode to ASCII 85.
%
%    o file: write the encoded ASCII character to this file.
%
%
*/
#define MaxLineExtent  36

static char *Ascii85Tuple(unsigned char *data)
{
  static char
    tuple[6];

  register long
    i,
    x;

  unsigned long
    code,
    quantum;

  code=((((unsigned long) data[0] << 8) | (unsigned long) data[1]) << 16) |
    ((unsigned long) data[2] << 8) | (unsigned long) data[3];
  if (code == 0L)
    {
      tuple[0]='z';
      tuple[1]='\0';
      return(tuple);
    }
  quantum=85UL*85UL*85UL*85UL;
  for (i=0; i < 4; i++)
  {
    x=(long) (code/quantum);
    code-=quantum*x;
    tuple[i]=(char) (x+(int) '!');
    quantum/=85L;
  }
  tuple[4]=(char) ((code % 85L)+(int) '!');
  tuple[5]='\0';
  return(tuple);
}

MagickExport void Ascii85Initialize(Image *image)
{
  /*
    Allocate image structure.
  */
  if (image->ascii85 == (Ascii85Info *) NULL)
    image->ascii85=(Ascii85Info *) AcquireMagickMemory(sizeof(*image->ascii85));
  if (image->ascii85 == (Ascii85Info *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      image->filename);
  (void) ResetMagickMemory(image->ascii85,0,sizeof(*image->ascii85));
  image->ascii85->line_break=MaxLineExtent << 1;
  image->ascii85->offset=0;
}

MagickExport void Ascii85Flush(Image *image)
{
  register char
    *tuple;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->ascii85 != (Ascii85Info *) NULL);
  if (image->ascii85->offset > 0)
    {
      image->ascii85->buffer[image->ascii85->offset]='\0';
      image->ascii85->buffer[image->ascii85->offset+1]='\0';
      image->ascii85->buffer[image->ascii85->offset+2]='\0';
      tuple=Ascii85Tuple(image->ascii85->buffer);
      (void) WriteBlob(image,(size_t) image->ascii85->offset+1,
        (const unsigned char *) (*tuple == 'z' ? "!!!!" : tuple));
    }
  (void) WriteBlobByte(image,'~');
  (void) WriteBlobByte(image,'>');
  (void) WriteBlobByte(image,'\n');
}

MagickExport void Ascii85Encode(Image *image,const unsigned char code)
{
  long
    n;

  register char
    *q;

  register unsigned char
    *p;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(image->ascii85 != (Ascii85Info *) NULL);
  image->ascii85->buffer[image->ascii85->offset]=code;
  image->ascii85->offset++;
  if (image->ascii85->offset < 4)
    return;
  p=image->ascii85->buffer;
  for (n=image->ascii85->offset; n >= 4; n-=4)
  {
    for (q=Ascii85Tuple(p); *q != '\0'; q++)
    {
      image->ascii85->line_break--;
      if ((image->ascii85->line_break < 0) && (*q != '%'))
        {
          (void) WriteBlobByte(image,'\n');
          image->ascii85->line_break=2*MaxLineExtent;
        }
      (void) WriteBlobByte(image,(unsigned char) *q);
    }
    p+=8;
  }
  image->ascii85->offset=n;
  p-=4;
  for (n=0; n < 4; n++)
    image->ascii85->buffer[n]=(*p++);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   H u f f m a n D e c o d e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  HuffmanDecodeImage() uncompresses an image via Huffman-coding.
%
%  The format of the HuffmanDecodeImage method is:
%
%      MagickBooleanType HuffmanDecodeImage(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%
*/
MagickExport MagickBooleanType HuffmanDecodeImage(Image *image)
{
#define HashSize  1021
#define MBHashA  293
#define MBHashB  2695
#define MWHashA  3510
#define MWHashB  1178

#define InitializeHashTable(hash,table,a,b) \
{ \
  entry=table; \
  while (entry->code != 0) \
  {  \
    hash[((entry->length+a)*(entry->code+b)) % HashSize]=(HuffmanTable *) entry; \
    entry++; \
  } \
}

#define InputBit(bit)  \
{  \
  if ((mask & 0xff) == 0)  \
    {  \
      byte=ReadBlobByte(image);  \
      if (byte == EOF)  \
        break;  \
      mask=0x80;  \
    }  \
  runlength++;  \
  bit=(unsigned long) ((byte & mask) != 0 ? 0x01 : 0x00); \
  mask>>=1;  \
  if (bit != 0)  \
    runlength=0;  \
}

  const HuffmanTable
    *entry;

  HuffmanTable
    **mb_hash,
    **mw_hash;

  IndexPacket
    index;

  int
    byte;

  long
    count,
    y;

  MagickBooleanType
    status;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  register unsigned char
    *p;

  unsigned char
    *scanline;

  unsigned int
    bail,
    color;

  unsigned long
    bit,
    code,
    mask,
    length,
    null_lines,
    runlength;

  /*
    Allocate buffers.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  mb_hash=(HuffmanTable **) AcquireMagickMemory(HashSize*sizeof(*mb_hash));
  mw_hash=(HuffmanTable **) AcquireMagickMemory(HashSize*sizeof(*mw_hash));
  scanline=(unsigned char *) AcquireMagickMemory((size_t) image->columns);
  if ((mb_hash == (HuffmanTable **) NULL) ||
      (mw_hash == (HuffmanTable **) NULL) ||
      (scanline == (unsigned char *) NULL))
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  /*
    Initialize Huffman tables.
  */
  for (i=0; i < HashSize; i++)
  {
    mb_hash[i]=(HuffmanTable *) NULL;
    mw_hash[i]=(HuffmanTable *) NULL;
  }
  InitializeHashTable(mw_hash,TWTable,MWHashA,MWHashB);
  InitializeHashTable(mw_hash,MWTable,MWHashA,MWHashB);
  InitializeHashTable(mw_hash,EXTable,MWHashA,MWHashB);
  InitializeHashTable(mb_hash,TBTable,MBHashA,MBHashB);
  InitializeHashTable(mb_hash,MBTable,MBHashA,MBHashB);
  InitializeHashTable(mb_hash,EXTable,MBHashA,MBHashB);
  /*
    Uncompress 1D Huffman to runlength encoded pixels.
  */
  byte=0;
  mask=0;
  null_lines=0;
  runlength=0;
  while (runlength < 11)
   InputBit(bit);
  do { InputBit(bit); } while ((int) bit == 0);
  image->x_resolution=204.0;
  image->y_resolution=196.0;
  image->units=PixelsPerInchResolution;
  for (y=0; ((y < (long) image->rows) && (null_lines < 3)); )
  {
    /*
      Initialize scanline to white.
    */
    p=scanline;
    for (x=0; x < (long) image->columns; x++)
      *p++=(unsigned char) 0;
    /*
      Decode Huffman encoded scanline.
    */
    color=MagickTrue;
    code=0;
    count=0;
    length=0;
    runlength=0;
    x=0;
    for ( ; ; )
    {
      if (byte == EOF)
        break;
      if (x >= (long) image->columns)
        {
          while (runlength < 11)
            InputBit(bit);
          do { InputBit(bit); } while ((int) bit == 0);
          break;
        }
      bail=MagickFalse;
      do
      {
        if (runlength < 11)
          InputBit(bit)
        else
          {
            InputBit(bit);
            if ((int) bit != 0)
              {
                null_lines++;
                if (x != 0)
                  null_lines=0;
                bail=MagickTrue;
                break;
              }
          }
        code=(code << 1)+(unsigned long) bit;
        length++;
      } while (code == 0);
      if (bail != MagickFalse)
        break;
      if (length > 13)
        {
          while (runlength < 11)
           InputBit(bit);
          do { InputBit(bit); } while ((int) bit == 0);
          break;
        }
      if (color != MagickFalse)
        {
          if (length < 4)
            continue;
          entry=mw_hash[((length+MWHashA)*(code+MWHashB)) % HashSize];
        }
      else
        {
          if (length < 2)
            continue;
          entry=mb_hash[((length+MBHashA)*(code+MBHashB)) % HashSize];
        }
      if (entry == (const HuffmanTable *) NULL)
        continue;
      if ((entry->length != length) || (entry->code != code))
        continue;
      switch (entry->id)
      {
        case TWId:
        case TBId:
        {
          count+=entry->count;
          if ((x+count) > (long) image->columns)
            count=(long) image->columns-x;
          if (count > 0)
            {
              if (color != MagickFalse)
                {
                  x+=count;
                  count=0;
                }
              else
                for ( ; count > 0; count--)
                  scanline[x++]=(unsigned char) 1;
            }
          color=(unsigned int)
            ((color == MagickFalse) ? MagickTrue : MagickFalse);
          break;
        }
        case MWId:
        case MBId:
        case EXId:
        {
          count+=entry->count;
          break;
        }
        default:
          break;
      }
      code=0;
      length=0;
    }
    /*
      Transfer scanline to image pixels.
    */
    p=scanline;
    q=SetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      index=(IndexPacket) (*p++);
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
    y++;
  }
  image->rows=(unsigned long) Max(y-3,1);
  image->compression=FaxCompression;
  /*
    Free decoder memory.
  */
  mw_hash=(HuffmanTable **) RelinquishMagickMemory(mw_hash);
  mb_hash=(HuffmanTable **) RelinquishMagickMemory(mb_hash);
  scanline=(unsigned char *) RelinquishMagickMemory(scanline);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   H u f f m a n E n c o d e I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  HuffmanEncodeImage() compresses an image via Huffman-coding.
%
%  The format of the HuffmanEncodeImage method is:
%
%      MagickBooleanType HuffmanEncodeImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: The image info..
%
%    o image: The image.
%
*/
MagickExport MagickBooleanType HuffmanEncodeImage(const ImageInfo *image_info,
  Image *image)
{
#define HuffmanOutputCode(entry)  \
{  \
  mask=1 << (entry->length-1);  \
  while (mask != 0)  \
  {  \
    OutputBit(((entry->code & mask) != 0 ? 1 : 0));  \
    mask>>=1;  \
  }  \
}

#define OutputBit(count)  \
{  \
  if (count > 0)  \
    byte=byte | bit;  \
  bit>>=1;  \
  if ((int) (bit & 0xff) == 0)   \
    {  \
      if (LocaleCompare(image_info->magick,"FAX") == 0) \
        (void) WriteBlobByte(image,(unsigned char) byte);  \
      else \
        Ascii85Encode(image,byte); \
      byte='\0';  \
      bit=(unsigned char) 0x80;  \
    }  \
}

  const HuffmanTable
    *entry;

  int
    k,
    runlength;

  long
    n,
    y;

  Image
    *huffman_image;

  IndexPacket
    polarity;

  MagickBooleanType
    status;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register const PixelPacket
    *p;

  register unsigned char
    *q;

  unsigned char
    byte,
    bit,
    *scanline;

  unsigned long
    mask,
    width;

  /*
    Allocate scanline buffer.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  width=image->columns;
  if (LocaleCompare(image_info->magick,"FAX") == 0)
    width=Max(image->columns,1728);
  scanline=(unsigned char *) AcquireMagickMemory((size_t) width+1);
  if (scanline == (unsigned char *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  huffman_image=CloneImage(image,0,0,MagickTrue,&image->exception);
  if (huffman_image == (Image *) NULL)
    return(MagickFalse);
  (void) SetImageType(huffman_image,BilevelType);
  byte='\0';
  bit=(unsigned char) 0x80;
  if (LocaleCompare(image_info->magick,"FAX") != 0)
    Ascii85Initialize(image);
  else
    {
      /*
        End of line.
      */
      for (k=0; k < 11; k++)
        OutputBit(0);
      OutputBit(1);
    }
  /*
    Compress runlength encoded to 1D Huffman pixels.
  */
  polarity=(IndexPacket) (PixelIntensityToQuantum(
    &huffman_image->colormap[0]) >= (QuantumRange/2) ? 0x00 : 0x01);
  if (huffman_image->colors == 2)
    polarity=(IndexPacket)
      (PixelIntensityToQuantum(&huffman_image->colormap[0]) <
       PixelIntensityToQuantum(&huffman_image->colormap[1]) ? 0x00 : 0x01);
  q=scanline;
  for (i=0; i < (long) width; i++)
    *q++=(unsigned char) polarity;
  q=scanline;
  for (y=0; y < (long) huffman_image->rows; y++)
  {
    p=AcquireImagePixels(huffman_image,0,y,huffman_image->columns,1,
      &huffman_image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    indexes=GetIndexes(huffman_image);
    for (x=0; x < (long) huffman_image->columns; x++)
    {
      *q=(unsigned char)
        (indexes[x] == polarity ? polarity == 0 : polarity != 0);
      q++;
    }
    /*
      Huffman encode scanline.
    */
    q=scanline;
    for (n=(long) width; n > 0; )
    {
      /*
        Output white run.
      */
      for (runlength=0; ((n > 0) && ((Quantum) *q == polarity)); n--)
      {
        q++;
        runlength++;
      }
      if (runlength >= 64)
        {
          if (runlength < 1792)
            entry=MWTable+((runlength/64)-1);
          else
            entry=EXTable+(Min(runlength,2560)-1792)/64;
          runlength-=entry->count;
          HuffmanOutputCode(entry);
        }
      entry=TWTable+Min(runlength,63);
      HuffmanOutputCode(entry);
      if (n != 0)
        {
          /*
            Output black run.
          */
          for (runlength=0; (((Quantum) *q != polarity) && (n > 0)); n--)
          {
            q++;
            runlength++;
          }
          if (runlength >= 64)
            {
              entry=MBTable+((runlength/64)-1);
              if (runlength >= 1792)
                entry=EXTable+(Min(runlength,2560)-1792)/64;
              runlength-=entry->count;
              HuffmanOutputCode(entry);
            }
          entry=TBTable+Min(runlength,63);
          HuffmanOutputCode(entry);
        }
    }
    /*
      End of line.
    */
    for (k=0; k < 11; k++)
      OutputBit(0);
    OutputBit(1);
    q=scanline;
    if (GetPreviousImageInList(huffman_image) == (Image *) NULL)
      if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(y,huffman_image->rows) != MagickFalse))
        {
          status=image->progress_monitor(LoadImageTag,y,huffman_image->rows,
            image->client_data);
          if (status == MagickFalse)
            break;
        }
  }
  /*
    End of page.
  */
  for (i=0; i < 6; i++)
  {
    for (k=0; k < 11; k++)
      OutputBit(0);
    OutputBit(1);
  }
  /*
    Flush bits.
  */
  if (((int) bit != 0x80) != 0)
    {
      if (LocaleCompare(image_info->magick,"FAX") == 0)
        (void) WriteBlobByte(image,byte);
      else
        Ascii85Encode(image, byte);
    }
  if (LocaleCompare(image_info->magick,"FAX") != 0)
    Ascii85Flush(image);
  huffman_image=DestroyImage(huffman_image);
  scanline=(unsigned char *) RelinquishMagickMemory(scanline);
  return(MagickTrue);
}

#if defined(HasTIFF)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   H u f f m a n 2 D E n c o d e I m a g e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Huffman2DEncodeImage() compresses an image via two-dimensional
%  Huffman-coding.
%
%  The format of the Huffman2DEncodeImage method is:
%
%      MagickBooleanType Huffman2DEncodeImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: The image info..
%
%    o image: The image.
%
*/
MagickExport MagickBooleanType Huffman2DEncodeImage(const ImageInfo *image_info,
  Image *image)
{
  char
    filename[MaxTextExtent];

  Image
    *huffman_image;

  ImageInfo
    *write_info;

  int
    unique_file;

  long
    j;

  MagickBooleanType
    status;

  register long
    i;

  ssize_t
    count;

  TIFF
    *tiff;

  uint16
    fillorder;

  unsigned char
    *buffer;

  unsigned long
    *byte_count,
    strip_size;

  /*
    Write image as CCITTFax4 TIFF image to a temporary file.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  huffman_image=CloneImage(image,0,0,MagickTrue,&image->exception);
  if (huffman_image == (Image *) NULL)
    return(MagickFalse);
  DestroyBlob(huffman_image);
  huffman_image->blob=CloneBlobInfo((BlobInfo *) NULL);
  (void) SetImageType(huffman_image,BilevelType);
  unique_file=AcquireUniqueFileResource(filename);
  (void) FormatMagickString(huffman_image->filename,MaxTextExtent,"tiff:%s",
    filename);
  write_info=CloneImageInfo(image_info);
  SetImageInfoFile(write_info,fdopen(unique_file,"wb"));
  write_info->compression=Group4Compression;
  status=WriteImage(write_info,huffman_image);
  write_info=DestroyImageInfo(write_info);
  if (status == MagickFalse)
    return(MagickFalse);
  tiff=TIFFOpen(filename,"rb");
  if (tiff == (TIFF *) NULL)
    {
      (void) RelinquishUniqueFileResource(filename);
      ThrowFileException(&image->exception,FileOpenError,"UnableToOpenFile",
        image_info->filename);
      return(MagickFalse);
    }
  /*
    Allocate raw strip buffer.
  */
  byte_count=0;
  (void) TIFFGetField(tiff,TIFFTAG_STRIPBYTECOUNTS,&byte_count);
  strip_size=byte_count[0];
  for (i=1; i < (long) TIFFNumberOfStrips(tiff); i++)
    if (byte_count[i] > strip_size)
      strip_size=byte_count[i];
  buffer=(unsigned char *) AcquireMagickMemory((size_t) strip_size);
  if (buffer == (unsigned char *) NULL)
    {
      TIFFClose(tiff);
      (void) RelinquishUniqueFileResource(filename);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image_info->filename);
    }
  /*
    Compress runlength encoded to 2D Huffman pixels.
  */
  fillorder=FILLORDER_LSB2MSB;
  (void) TIFFGetFieldDefaulted(tiff,TIFFTAG_FILLORDER,&fillorder);
  for (i=0; i < (long) TIFFNumberOfStrips(tiff); i++)
  {
    count=(ssize_t) TIFFReadRawStrip(tiff,(uint32) i,buffer,(long)
      byte_count[i]);
    if (fillorder == FILLORDER_LSB2MSB)
      TIFFReverseBits(buffer,(unsigned long) count);
    for (j=0; j < (long) count; j++)
      (void) WriteBlobByte(image,(unsigned char) buffer[j]);
  }
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  TIFFClose(tiff);
  huffman_image=DestroyImage(huffman_image);
  (void) RelinquishUniqueFileResource(filename);
  return(MagickTrue);
}
#else
MagickExport MagickBooleanType Huffman2DEncodeImage(const ImageInfo *image_info,
  Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  ThrowBinaryException(MissingDelegateError,"TIFFLibraryIsNotAvailable",
    image->filename);
}
#endif

#if defined(HasJPEG)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   J P E G E n c o d e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  JPEGEncodeImage() compresses an image via JPEG compression.
%
%  The format of the JPEGEncodeImage method is:
%
%      MagickBooleanType JPEGEncodeImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: The image info..
%
%    o image: The image.
%
*/
MagickExport MagickBooleanType JPEGEncodeImage(const ImageInfo *image_info,
  Image *image)
{
  Image
    *jpeg_image;

  ImageInfo
    *jpeg_info;

  size_t
    length;

  unsigned char
    *blob;

  /*
    Write image as JPEG image to a temporary file.
  */
  jpeg_image=CloneImage(image,0,0,MagickTrue,&image->exception);
  if (jpeg_image == (Image *) NULL)
    ThrowWriterException(CoderError,image->exception.reason);
  DestroyBlob(jpeg_image);
  jpeg_image->blob=CloneBlobInfo((BlobInfo *) NULL);
  jpeg_info=CloneImageInfo(image_info);
  *jpeg_info->filename='\0';
  (void) CopyMagickString(jpeg_info->magick,"JPEG",MaxTextExtent);
  length=0;
  blob=ImageToBlob(jpeg_info,jpeg_image,&length,&image->exception);
  jpeg_info=DestroyImageInfo(jpeg_info);
  if (blob == (void *) NULL)
    ThrowWriterException(CoderError,image->exception.reason);
  (void) WriteBlob(image,length,blob);
  jpeg_image=DestroyImage(jpeg_image);
  blob=(unsigned char *) RelinquishMagickMemory(blob);
  return(MagickTrue);
}
#else
MagickExport MagickBooleanType JPEGEncodeImage(const ImageInfo *image_info,
  Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  ThrowBinaryException(MissingDelegateError,"JPEGLibraryIsNotAvailable",
    image->filename);
}
#endif

#if defined(HasJP2)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   J P 2 E n c o d e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  JP2EncodeImage() compresses an image via JPEG-2000 compression.
%
%  The format of the JP2EncodeImage method is:
%
%      MagickBooleanType JP2EncodeImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: The image info..
%
%    o image: The image.
%
*/
MagickExport MagickBooleanType JP2EncodeImage(const ImageInfo *image_info,
  Image *image)
{
  Image
    *jp2_image;

  ImageInfo
    *jp2_info;

  size_t
    length;

  unsigned char
    *blob;

  /*
    Write image as JP2 image to a temporary file.
  */
  jp2_image=CloneImage(image,0,0,MagickTrue,&image->exception);
  if (jp2_image == (Image *) NULL)
    ThrowWriterException(CoderError,image->exception.reason);
  DestroyBlob(jp2_image);
  jp2_image->blob=CloneBlobInfo((BlobInfo *) NULL);
  jp2_info=CloneImageInfo(image_info);
  *jp2_info->filename='\0';
  (void) CopyMagickString(jp2_info->magick,"JP2",MaxTextExtent);
  length=0;
  blob=ImageToBlob(jp2_info,jp2_image,&length,&image->exception);
  jp2_info=DestroyImageInfo(jp2_info);
  if (blob == (void *) NULL)
    ThrowWriterException(CoderError,image->exception.reason);
  (void) WriteBlob(image,length,blob);
  jp2_image=DestroyImage(jp2_image);
  blob=(unsigned char *) RelinquishMagickMemory(blob);
  return(MagickTrue);
}
#else
MagickExport MagickBooleanType JP2EncodeImage(
  const ImageInfo *magick_unused(image_info),Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  ThrowBinaryException(MissingDelegateError,"JP2LibraryIsNotAvailable",
    image->filename);
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L Z W E n c o d e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LZWEncodeImage() compresses an image via LZW-coding specific to Postscript
%  Level II or Portable Document Format.
%
%  The format of the LZWEncodeImage method is:
%
%      MagickBooleanType LZWEncodeImage(Image *image,const size_t length,
%        unsigned char *pixels)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o length:  A value that specifies the number of pixels to compress.
%
%    o pixels: The address of an unsigned array of characters containing the
%      pixels to compress.
%
%
*/
MagickExport MagickBooleanType LZWEncodeImage(Image *image,const size_t length,
  unsigned char *pixels)
{
#define LZWClr  256UL  /* Clear Table Marker */
#define LZWEod  257UL  /* End of Data marker */
#define OutputCode(code) \
{ \
    accumulator+=code << (32-code_width-number_bits); \
    number_bits+=code_width; \
    while (number_bits >= 8) \
    { \
        (void) WriteBlobByte(image,(unsigned char) (accumulator >> 24)); \
        accumulator=accumulator << 8; \
        number_bits-=8; \
    } \
}

  typedef struct _TableType
  {
    long
      prefix,
      suffix,
      next;
  } TableType;

  long
    index;

  register long
    i;

  TableType
    *table;

  unsigned long
    accumulator,
    number_bits,
    code_width,
    last_code,
    next_index;

  /*
    Allocate string table.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(pixels != (unsigned char *) NULL);
  table=(TableType *) AcquireMagickMemory((1 << 12)*sizeof(*table));
  if (table == (TableType *) NULL)
    return(MagickFalse);
  /*
    Initialize variables.
  */
  accumulator=0;
  code_width=9;
  number_bits=0;
  last_code=0;
  OutputCode(LZWClr);
  for (index=0; index < 256; index++)
  {
    table[index].prefix=(-1);
    table[index].suffix=(short) index;
    table[index].next=(-1);
  }
  next_index=LZWEod+1;
  code_width=9;
  last_code=(unsigned long) pixels[0];
  for (i=1; i < (long) length; i++)
  {
    /*
      Find string.
    */
    index=(long) last_code;
    while (index != -1)
      if ((table[index].prefix != (long) last_code) ||
          (table[index].suffix != (long) pixels[i]))
        index=table[index].next;
      else
        {
          last_code=(unsigned long) index;
          break;
        }
    if (last_code != (unsigned long) index)
      {
        /*
          Add string.
        */
        OutputCode(last_code);
        table[next_index].prefix=(long) last_code;
        table[next_index].suffix=(short) pixels[i];
        table[next_index].next=table[last_code].next;
        table[last_code].next=(long) next_index;
        next_index++;
        /*
          Did we just move up to next bit width?
        */
        if ((next_index >> code_width) != 0)
          {
            code_width++;
            if (code_width > 12)
              {
                /*
                  Did we overflow the max bit width?
                */
                code_width--;
                OutputCode(LZWClr);
                for (index=0; index < 256; index++)
                {
                  table[index].prefix=(-1);
                  table[index].suffix=index;
                  table[index].next=(-1);
                }
                next_index=LZWEod+1;
                code_width=9;
              }
            }
          last_code=(unsigned long) pixels[i];
      }
  }
  /*
    Flush tables.
  */
  OutputCode(last_code);
  OutputCode(LZWEod);
  if (number_bits != 0)
    (void) WriteBlobByte(image,(unsigned char) (accumulator >> 24));
  table=(TableType *) RelinquishMagickMemory(table);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a c k b i t s E n c o d e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PackbitsEncodeImage() compresses an image via Macintosh Packbits encoding
%  specific to Postscript Level II or Portable Document Format.  To ensure
%  portability, the binary Packbits bytes are encoded as ASCII Base-85.
%
%  The format of the PackbitsEncodeImage method is:
%
%      MagickBooleanType PackbitsEncodeImage(Image *image,const size_t length,
%        unsigned char *pixels)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o length:  A value that specifies the number of pixels to compress.
%
%    o pixels: The address of an unsigned array of characters containing the
%      pixels to compress.
%
%
*/
MagickExport MagickBooleanType PackbitsEncodeImage(Image *image,
  const size_t length,unsigned char *pixels)
{
  int
    count;

  register long
    i,
    j;

  unsigned char
    *packbits;

  /*
    Compress pixels with Packbits encoding.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(pixels != (unsigned char *) NULL);
  packbits=(unsigned char *) AcquireMagickMemory(128);
  if (packbits == (unsigned char *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  i=(long) length;
  while (i != 0)
  {
    switch (i)
    {
      case 1:
      {
        i--;
        (void) WriteBlobByte(image,(unsigned char) 0);
        (void) WriteBlobByte(image,*pixels);
        break;
      }
      case 2:
      {
        i-=2;
        (void) WriteBlobByte(image,(unsigned char) 1);
        (void) WriteBlobByte(image,*pixels);
        (void) WriteBlobByte(image,pixels[1]);
        break;
      }
      case 3:
      {
        i-=3;
        if ((*pixels == *(pixels+1)) && (*(pixels+1) == *(pixels+2)))
          {
            (void) WriteBlobByte(image,(unsigned char) ((256-3)+1));
            (void) WriteBlobByte(image,*pixels);
            break;
          }
        (void) WriteBlobByte(image,(unsigned char) 2);
        (void) WriteBlobByte(image,*pixels);
        (void) WriteBlobByte(image,pixels[1]);
        (void) WriteBlobByte(image,pixels[2]);
        break;
      }
      default:
      {
        if ((*pixels == *(pixels+1)) && (*(pixels+1) == *(pixels+2)))
          {
            /*
              Packed run.
            */
            count=3;
            while (((long) count < i) && (*pixels == *(pixels+count)))
            {
              count++;
              if (count >= 127)
                break;
            }
            i-=count;
            (void) WriteBlobByte(image,(unsigned char) ((256-count)+1));
            (void) WriteBlobByte(image,*pixels);
            pixels+=count;
            break;
          }
        /*
          Literal run.
        */
        count=0;
        while ((*(pixels+count) != *(pixels+count+1)) ||
               (*(pixels+count+1) != *(pixels+count+2)))
        {
          packbits[count+1]=pixels[count];
          count++;
          if (((long) count >= (i-3)) || (count >= 127))
            break;
        }
        i-=count;
        *packbits=(unsigned char) (count-1);
        for (j=0; j <= (long) count; j++)
          (void) WriteBlobByte(image,packbits[j]);
        pixels+=count;
        break;
      }
    }
  }
  (void) WriteBlobByte(image,(unsigned char) 128);  /* EOD marker */
  packbits=(unsigned char *) RelinquishMagickMemory(packbits);
  return(MagickTrue);
}

#if defined(HasZLIB)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   Z L I B E n c o d e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ZLIBEncodeImage compresses an image via ZLIB-coding specific to
%  Postscript Level II or Portable Document Format.
%
%  The format of the ZLIBEncodeImage method is:
%
%      MagickBooleanType ZLIBEncodeImage(Image *image,const size_t length,
%        unsigned char *pixels)
%
%  A description of each parameter follows:
%
%    o file: The address of a structure of type FILE.  ZLIB encoded pixels
%      are written to this file.
%
%    o length:  A value that specifies the number of pixels to compress.
%
%    o pixels: The address of an unsigned array of characters containing the
%      pixels to compress.
%
*/
MagickExport MagickBooleanType ZLIBEncodeImage(Image *image,const size_t length,
  unsigned char *pixels)
{
  int
    status;

  register long
    i;

  size_t
    compressed_packets;

  unsigned char
    *compressed_pixels;

  z_stream
    stream;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  compressed_packets=(size_t) (1.001*length+12);
  compressed_pixels=(unsigned char *) AcquireMagickMemory(compressed_packets);
  if (compressed_pixels == (unsigned char *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  stream.next_in=pixels;
  stream.avail_in=(unsigned int) length;
  stream.next_out=compressed_pixels;
  stream.avail_out=(unsigned int) compressed_packets;
  stream.zalloc=(alloc_func) NULL;
  stream.zfree=(free_func) NULL;
  stream.opaque=(voidpf) NULL;
  status=deflateInit(&stream,(int) (image->quality ==
    UndefinedCompressionQuality ? 7 : Min(image->quality/10,9)));
  if (status == Z_OK)
    {
      status=deflate(&stream,Z_FINISH);
      if (status == Z_STREAM_END)
        status=deflateEnd(&stream);
      else
        (void) deflateEnd(&stream);
      compressed_packets=(size_t) stream.total_out;
    }
  if (status != Z_OK)
    ThrowBinaryException(CoderError,"UnableToZipCompressImage",image->filename)
  else
    for (i=0; i < (long) compressed_packets; i++)
      (void) WriteBlobByte(image,compressed_pixels[i]);
  compressed_pixels=(unsigned char *) RelinquishMagickMemory(compressed_pixels);
  return(status == Z_OK ? MagickTrue : MagickFalse);
}
#else
MagickExport MagickBooleanType ZLIBEncodeImage(Image *image,
  const size_t magick_unused(length),unsigned char *magick_unused(pixels))
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  ThrowBinaryException(MissingDelegateError,"ZLIBLibraryIsNotAvailable",
    image->filename);
}
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        