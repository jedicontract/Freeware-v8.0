/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  SSSSS   TTTTT  RRRR   IIIII  N   N   GGGG                  %
%                  SS        T    R   R    I    NN  N  G                      %
%                   SSS      T    RRRR     I    N N N  G GGG                  %
%                     SS     T    R R      I    N  NN  G   G                  %
%                  SSSSS     T    R  R   IIIII  N   N   GGGG                  %
%                                                                             %
%                                                                             %
%                       ImageMagick String Methods                            %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                               August 2003                                   %
%                                                                             %
%                                                                             %
%  Copyright 1999-2005 ImageMagick Studio LLC, a non-profit organization      %
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
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/resource_.h"
#include "magick/semaphore.h"
#include "magick/signature.h"
#include "magick/string_.h"

/*
  Static declarations.
*/
#if !defined(HAVE_STRCASECMP) || !defined(HAVE_STRNCASECMP)
static const unsigned char
  AsciiMap[] =
  {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73,
    0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
    0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83,
    0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
    0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
    0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xe1, 0xe2, 0xe3, 0xe4, 0xc5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb,
    0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3,
    0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb,
    0xfc, 0xfd, 0xfe, 0xff,
  };
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e S t r i n g                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireString() allocates memory for a string and copies the source string
%  to that memory location (and returns it).
%
%  The format of the AcquireString method is:
%
%      char *AcquireString(const char *source)
%
%  A description of each parameter follows:
%
%    o source: A character string.
%
*/

static inline size_t CheckOverflowException(const size_t length,
  const size_t extend)
{
  if ((length+extend) < length)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowMagickFatalException(ResourceLimitFatalError,
        "MemoryAllocationFailed",message);
      message=(char *) RelinquishMagickMemory(message);
    }
  return(length+extend);
}

MagickExport char *AcquireString(const char *source)
{
  char
    *destination;

  size_t
    length;

  length=0;
  if (source != (char *) NULL)
    length+=strlen(source);
  length=CheckOverflowException(length,MaxTextExtent);
  destination=(char *) AcquireMagickMemory(length*sizeof(*destination));
  if (destination == (char *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"UnableToAcquireString",
      source);
  *destination='\0';
  if (source != (char *) NULL)
    (void) CopyMagickString(destination,source,length*sizeof(*destination));
  return(destination);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e S t r i n g I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireStringInfo() allocates the StringInfo structure.
%
%  The format of the AcquireStringInfo method is:
%
%      StringInfo *AcquireStringInfo(const size_t length)
%
%  A description of each parameter follows:
%
%    o length: The string length.
%
*/
MagickExport StringInfo *AcquireStringInfo(const size_t length)
{
  StringInfo
    *string_info;

  string_info=(StringInfo *) AcquireMagickMemory(sizeof(*string_info));
  if (string_info == (StringInfo *) NULL)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowMagickFatalException(ResourceLimitFatalError,
        "MemoryAllocationFailed",message);
      message=(char *) RelinquishMagickMemory(message);
    }
  (void) ResetMagickMemory(string_info,0,sizeof(*string_info));
  string_info->signature=MagickSignature;
  string_info->length=length;
  if (string_info->length != 0)
    {
      size_t
        length;

      length=CheckOverflowException(string_info->length,MaxTextExtent);
      string_info->datum=(unsigned char *)
        AcquireMagickMemory(length*sizeof(*string_info->datum));
      if (string_info->datum == (unsigned char *) NULL)
        {
          char
            *message;

          message=GetExceptionMessage(errno);
          ThrowMagickFatalException(ResourceLimitFatalError,
            "MemoryAllocationFailed",message);
          message=(char *) RelinquishMagickMemory(message);
        }
    }
  return(string_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e S t r i n g                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneString() allocates memory for the destination string and copies
%  the source string to that memory location.
%
%  The format of the CloneString method is:
%
%      char *CloneString(char **destination,const char *source)
%
%  A description of each parameter follows:
%
%    o destination:  A pointer to a character string.
%
%    o source: A character string.
%
%
*/
MagickExport char *CloneString(char **destination,const char *source)
{
  size_t
    length;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(destination != (char **) NULL);
  if (source == (const char *) NULL)
    {
      if (*destination != (char *) NULL)
        *destination=(char *) RelinquishMagickMemory(*destination);
      return(*destination);
    }
  if (*destination == (char *) NULL)
    {
      *destination=AcquireString(source);
      return(*destination);
    }
  length=CheckOverflowException(strlen(source),MaxTextExtent);
  *destination=(char *) ResizeMagickMemory(*destination,length*
    sizeof(*destination));
  if (*destination == (char *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"UnableToAcquireString",
      source);
  (void) CopyMagickString(*destination,source,length*sizeof(*destination));
  return(*destination);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e S t r i n g I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneStringInfo() clones a copy of the StringInfo structure.
%
%  The format of the CloneStringInfo method is:
%
%      StringInfo *CloneStringInfo(const StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: The string info.
%
*/
MagickExport StringInfo *CloneStringInfo(const StringInfo *string_info)
{
  StringInfo
    *clone_info;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickSignature);
  clone_info=AcquireStringInfo(string_info->length);
  if (string_info->length != 0)
    (void) CopyMagickMemory(clone_info->datum,string_info->datum,
      string_info->length+MaxTextExtent);
  return(clone_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p a r e S t r i n g I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompareStringInfo() compares the two datums target and source.  It returns
%  an integer less than, equal to, or greater than zero if target is found,
%  respectively, to be less than, to match, or be greater than source.
%
%  The format of the CompareStringInfo method is:
%
%      int CompareStringInfo(const StringInfo *target,const StringInfo *source)
%
%  A description of each parameter follows:
%
%    o target: The target string.
%
%    o source: The source string.
%
*/
MagickExport int CompareStringInfo(const StringInfo *target,
  const StringInfo *source)
{
  register unsigned char
    *p,
    *q;

  register size_t
    i;

  assert(target != (StringInfo *) NULL);
  assert(target->signature == MagickSignature);
  assert(source != (StringInfo *) NULL);
  assert(source->signature == MagickSignature);
  p=target->datum;
  q=source->datum;
  for (i=0; i < Min(source->length,target->length); i++)
  {
    if (*p != *q)
      break;
    p++;
    q++;
  }
  if ((i == source->length) && (source->length == target->length))
    return(0);
  return(*p-(int) *q);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n c a t e n a t e M a g i c k S t r i n g                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConcatenateMagickString() concatenates the source string to the destination
%  string.  The destination buffer is always null-terminated even if the
%  string must be truncated.
%
%  The format of the ConcatenateMagickString method is:
%
%      size_t ConcatenateMagickString(char *destination,const char *source,
%        const size_t length)
%
%  A description of each parameter follows:
%
%    o destination: The destination string.
%
%    o source: The source string.
%
%    o length: The length of the destination string.
%
*/
MagickExport size_t ConcatenateMagickString(char *destination,
  const char *source,const size_t length)
{
#if !defined(HAVE_STRLCAT)
  register char
    *q;

  register const char
    *p;

  register size_t
    i;

  size_t
    count;

  assert(destination != (char *) NULL);
  assert(source != (const char *) NULL);
  assert(length >= 1);
  p=source;
  q=destination;
  i=length;
  while ((i-- != 0) && (*q != '\0'))
    q++;
  count=(size_t) (q-destination);
  i=length-count;
  if (i == 0)
    return(count+strlen(p));
  while (*p != '\0')
  {
    if (i != 1)
      {
        *q++=(*p);
        i--;
      }
    p++;
  }
  *q='\0';
  return(count+(p-source));
#else
  return(strlcat(destination,source,length));
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n c a t e n a t e S t r i n g                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConcatenateString() appends a copy of string source, including the
%  terminating null character, to the end of string destination.
%
%  The format of the ConcatenateString method is:
%
%      MagickBooleanType ConcatenateString(char **destination,
%        const char *source)
%
%  A description of each parameter follows:
%
%    o destination:  A pointer to a character string.
%
%    o source: A character string.
%
%
*/
MagickExport MagickBooleanType ConcatenateString(char **destination,
  const char *source)
{
  size_t
    length;

  assert(destination != (char **) NULL);
  if (source == (const char *) NULL)
    return(MagickTrue);
  if (*destination == (char *) NULL)
    {
      *destination=AcquireString(source);
      return(MagickTrue);
    }
  length=CheckOverflowException(strlen(*destination)+strlen(source),
    MaxTextExtent);
  *destination=(char *) ResizeMagickMemory(*destination,
    length*sizeof(*destination));
  if (*destination == (char *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,
      "UnableToConcatenateString",source);
  (void) ConcatenateMagickString(*destination,source,length*
    sizeof(*destination));
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n c a t e n a t e S t r i n g I n f o                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConcatenateStringInfo() concatenates the source string to the destination
%  string.
%
%  The format of the ConcatenateStringInfo method is:
%
%      void ConcatenateStringInfo(StringInfo *string_info,
%        const StringInfo *source)
%
%  A description of each parameter follows:
%
%    o string_info: The string info.
%
%    o source: The source string.
%
*/
MagickExport void ConcatenateStringInfo(StringInfo *string_info,
  const StringInfo *source)
{
  size_t
    length;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickSignature);
  assert(source != (const StringInfo *) NULL);
  length=string_info->length;
  (void) CheckOverflowException(string_info->length,source->length);
  SetStringInfoLength(string_info,string_info->length+source->length);
  (void) CopyMagickMemory(string_info->datum+length,source->datum,
    source->length);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n f i g u r e F i l e T o S t r i n g I n f o                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConfigureFileToStringInfo() returns the contents of a configure file as a
%  string.
%
%  The format of the ConfigureFileToStringInfo method is:
%
%      StringInfo *ConfigureFileToStringInfo(const char *filename)
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: The filename.
%
*/
MagickExport StringInfo *ConfigureFileToStringInfo(const char *filename)
{
  char
    *string;

  int
    file;

  MagickOffsetType
    offset;

  size_t
    length;

  StringInfo
    *string_info;

  void
    *map;

  assert(filename != (const char *) NULL);
  file=open(filename,O_RDONLY | O_BINARY);
  if (file == -1)
    return((StringInfo *) NULL);
  offset=(MagickOffsetType) MagickSeek(file,0,SEEK_END);
  if ((offset < 0) || (offset != (MagickOffsetType) ((ssize_t) offset)))
    {
      file=close(file)-1;
      return((StringInfo *) NULL);
    }
  length=(size_t) offset;
  string=(char *) AcquireMagickMemory((length+1)*sizeof(*string));
  if (string == (char *) NULL)
    {
      file=close(file)-1;
      return((StringInfo *) NULL);
    }
  map=MapBlob(file,ReadMode,0,length);
  if (map != (void *) NULL)
    {
      (void) CopyMagickMemory(string,map,length);
      (void) UnmapBlob(map,length);
    }
  else
    {
      register size_t
        i;

      ssize_t
        count;

      (void) MagickSeek(file,0,SEEK_SET);
      for (i=0; i < length; i+=count)
      {
        count=read(file,string+i,(size_t) Min(length-i,(size_t)
          MagickMaxBufferSize));
        if (count <= 0)
          {
            count=0;
            if (errno != EINTR)
              break;
          }
      }
      if (i < length)
        {
          file=close(file)-1;
          string=(char *) RelinquishMagickMemory(string);
          return((StringInfo *) NULL);
        }
    }
  string[length]='\0';
  file=close(file)-1;
  string_info=AcquireStringInfo(0);
  (void) CopyMagickString(string_info->path,filename,MaxTextExtent);
  string_info->length=strlen(string)+1;
  string_info->datum=(unsigned char *) string;
  return(string_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n s t a n t S t r i n g                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConstantString() allocates memory for a string and copies the source string
%  to that memory location (and returns it).  Use it for strings that you do
%  do not expect to change over its lifetime.
%
%  The format of the ConstantString method is:
%
%      char *ConstantString(const char *source)
%
%  A description of each parameter follows:
%
%    o source: A character string.
%
*/
MagickExport char *ConstantString(const char *source)
{
  char
    *destination;

  size_t
    length;

  length=0;
  if (source != (char *) NULL)
    length+=strlen(source);
  length++;
  destination=(char *) AcquireMagickMemory(length*sizeof(*destination));
  if (destination == (char *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"UnableToAcquireString",
      source);
  *destination='\0';
  if (source != (char *) NULL)
    (void) CopyMagickString(destination,source,length*sizeof(*destination));
  return(destination);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o p y M a g i c k S t r i n g                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CopyMagickString() copies the source string to the destination string.  The
%  destination buffer is always null-terminated even if the string must be
%  truncated.
%
%  The format of the CopyMagickString method is:
%
%      size_t CopyMagickString(const char *destination,char *source,
%        const size_t length)
%
%  A description of each parameter follows:
%
%    o destination: The destination string.
%
%    o source: The source string.
%
%    o length: The length of the destination string.
%
*/
MagickExport size_t CopyMagickString(char *destination,const char *source,
  const size_t length)
{
#if !defined(HAVE_STRLCPY)
  register char
    *q;

  register const char
    *p;

  register size_t
    i;

  assert(destination != (char *) NULL);
  assert(source != (const char *) NULL);
  assert(length >= 1);
  p=source;
  q=destination;
  i=length;
  if ((i != 0) && (--i != 0))
    do
    {
      if ((*q++=(*p++)) == '\0')
        break;
    } while (--i != 0);
  if (i == 0)
    {
      if (length != 0)
        *q='\0';
      while (*p++ != '\0');
    }
  return((size_t) (p-source-1));
#else
  return(strlcpy(destination,source,length));
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y S t r i n g                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyString() destroys memory associated with a string.
%
%  The format of the DestroyString method is:
%
%      char *DestroyString(char *string)
%
%  A description of each parameter follows:
%
%    o string: The string.
%
*/
MagickExport char *DestroyString(char *string)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(string != (char *) NULL);
  string=(char *) RelinquishMagickMemory(string);
  return(string);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y S t r i n g I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyStringInfo() destroys memory associated with the StringInfo structure.
%
%  The format of the DestroyStringInfo method is:
%
%      StringInfo *DestroyStringInfo(StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: The string info.
%
*/
MagickExport StringInfo *DestroyStringInfo(StringInfo *string_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickSignature);
  if (string_info->datum != (unsigned char *) NULL)
    string_info->datum=(unsigned char *)
      RelinquishMagickMemory(string_info->datum);
  string_info->signature=(~MagickSignature);
  string_info=(StringInfo *) RelinquishMagickMemory(string_info);
  return(string_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y S t r i n g L i s t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyStringList() zeros memory associated with a string list.
%
%  The format of the DestroyStringList method is:
%
%      char **DestroyStringList(char **list)
%
%  A description of each parameter follows:
%
%    o list: The string list.
%
*/
MagickExport char **DestroyStringList(char **list)
{
  register long
    i;

  assert(list != (char **) NULL);
  for (i=0; list[i] != (char *) NULL; i++)
    list[i]=DestroyString(list[i]);
  list=(char **) RelinquishMagickMemory(list);
  return(list);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E s c a p e S t r i n g                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EscapeString() allocates memory for a backslash-escaped version of a
%  source text string, copies the escaped version of the text to that
%  memory location while adding backslash characters, and returns the
%  escaped string.
%
%  The format of the EscapeString method is:
%
%      char *EscapeString(const char *source,const char escape)
%
%  A description of each parameter follows:
%
%    o allocate_string:  Method EscapeString returns the escaped string.
%
%    o source: A character string.
%
%    o escape: The quoted string termination character to escape (e.g. '"').
%
*/
MagickExport char *EscapeString(const char *source,const char escape)
{
  char
    *destination;

  register char
    *q;

  register const char
    *p;

  size_t
    length;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(source != (const char *) NULL);
  length=CheckOverflowException(strlen(source),1);
  for (p=source; *p != '\0'; p++)
    if ((*p == '\\') || (*p == escape))
      length++;
  destination=(char *) AcquireMagickMemory(length*sizeof(*destination));
  if (destination == (char *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"UnableToEscapeString",
      source);
  *destination='\0';
  if (source != (char *) NULL)
    {
      q=destination;
      for (p=source; *p != '\0'; p++)
      {
        if ((*p == '\\') || (*p == escape))
          *q++= '\\';
        *q++=(*p);
      }
      *q='\0';
    }
  return(destination);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F i l e T o S t r i n g                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FileToString() returns the contents of a file as a string.
%
%  The format of the FileToString method is:
%
%      char *FileToString(const char *filename,const size_t extent,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: The filename.
%
%    o extent: Maximum length of the string.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport char *FileToString(const char *filename,const size_t extent,
  ExceptionInfo *exception)
{
  size_t
    length;

  assert(filename != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  assert(exception != (ExceptionInfo *) NULL);
  return((char *) FileToBlob(filename,extent,&length,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F i l e T o S t r i n g I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FileToStringInfo() returns the contents of a file as a string.
%
%  The format of the FileToStringInfo method is:
%
%      StringInfo *FileToStringInfo(const char *filename,const size_t extent,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: The filename.
%
%    o extent: Maximum length of the string.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport StringInfo *FileToStringInfo(const char *filename,
  const size_t extent,ExceptionInfo *exception)
{
  StringInfo
    *string_info;

  assert(filename != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  assert(exception != (ExceptionInfo *) NULL);
  string_info=AcquireStringInfo(0);
  (void) CopyMagickString(string_info->path,filename,MaxTextExtent);
  string_info->datum=FileToBlob(filename,extent,&string_info->length,exception);
  if (string_info->datum == (unsigned char *) NULL)
    {
      string_info=DestroyStringInfo(string_info);
      return((StringInfo *) NULL);
    }
  return(string_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  F o r m a t M a g i c k S i z e                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FormatMagickSize() converts a size to a human readable format, for example,
%  14kb, 234mb, 2.7gb, or 3.0tb.  Scaling is done by repetitively dividing by
%  1024.
%
%  The format of the FormatMagickSize method is:
%
%      long FormatMagickSize(const MagickSizeType size,char *format)
%
%  A description of each parameter follows:
%
%    o size:  convert this size to a human readable format.
%
%    o format:  human readable format.
%
*/
MagickExport long FormatMagickSize(const MagickSizeType size,char *format)
{
  double
    length;

  register long
    i;

  static char
    *units[7] = { "", "kb", "mb", "gb", "tb", "pb", "eb" };

  length=(double) ((MagickOffsetType) (size/2))*2.0;
  for (i=0; (length > 1024.0) && (i < 6); i++)
    length/=1024.0;
  return(FormatMagickString(format,MaxTextExtent,"%.2g%s",length,units[i]));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  F o r m a t M a g i c k S t r i n g                                        %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FormatMagickString() prints formatted output of a variable argument list.
%
%  The format of the FormatMagickString method is:
%
%      long FormatMagickString(char *string,const size_t length,
%        const char *format,...)
%
%  A description of each parameter follows.
%
%   o string:  FormatMagickString() returns the formatted string in this
%     character buffer.
%
%   o length: The maximum length of the string.
%
%   o format:  A string describing the format to use to write the remaining
%     arguments.
%
*/

MagickExport long FormatMagickStringList(char *string,const size_t length,
  const char *format,va_list operands)
{
  int
    n;

#if defined(HAVE_VSNPRINTF)
  n=vsnprintf(string,length,format,operands);
#else
  n=vsprintf(string,format,operands);
#endif
  if (n < 0)
    string[length-1]='\0';
  return((long) n);
}

MagickExport long FormatMagickString(char *string,const size_t length,
  const char *format,...)
{
  long
    n;

  va_list
    operands;

  va_start(operands,format);
  n=(long) FormatMagickStringList(string,length,format,operands);
  va_end(operands);
  return(n);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  F o r m a t M a g i c k T i m e                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FormatMagickTime() returns the specified time in the Internet date/time
%  format and the length of the timestamp.
%
%  The format of the FormatMagickTime method is:
%
%      size_t FormatMagickTime(const time_t time,const size_t length,
%        char *timestamp)
%
%  A description of each parameter follows.
%
%   o time:  the time since the Epoch (00:00:00 UTC, January 1, 1970),
%     measured in seconds.
%
%   o length: The maximum length of the string.
%
%   o timestamp:  Return the Internet date/time here.
%
*/
MagickExport long FormatMagickTime(const time_t time,const size_t length,
  char *timestamp)
{
  long
    count;

  struct tm
    local_meridian,
    utc_meridian;

  time_t
    offset;

  assert(timestamp != (char *) NULL);
  local_meridian=(*localtime(&time));
  utc_meridian=(*gmtime(&time));
  offset=(time_t) ((local_meridian.tm_min-utc_meridian.tm_min)/60+
    local_meridian.tm_hour-utc_meridian.tm_hour+24*((local_meridian.tm_year-
    utc_meridian.tm_year) != 0 ? (local_meridian.tm_year-utc_meridian.tm_year) :
    (local_meridian.tm_yday-utc_meridian.tm_yday)));
  count=FormatMagickString(timestamp,length,
    "%04d-%02d-%02dT%02d:%02d:%02d%+03ld:00",local_meridian.tm_year+1900,
    local_meridian.tm_mon+1,local_meridian.tm_mday,local_meridian.tm_hour,
    local_meridian.tm_min,local_meridian.tm_sec,offset);
  return(count);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t E n v i r o n m e n t V a l u e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetEnvironmentValue() returns the environment string that matches the
%  specified name.
%
%  The format of the GetEnvironmentValue method is:
%
%      char *GetEnvironmentValue(const char *name)
%
%  A description of each parameter follows:
%
%    o name: the environment name.
%
*/
MagickExport char *GetEnvironmentValue(const char *name)
{
  const char
    *environment;

  environment=getenv(name);
  if (environment == (const char *) NULL)
    return((char *) NULL);
  return(ConstantString(environment));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L o c a l e C o m p a r e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LocaleCompare() performs a case-insensitive comparison of two strings
%  byte-by-byte, according to the ordering of the current locale encoding.
%  LocaleCompare returns an integer greater than, equal to, or less than 0,
%  if the string pointed to by p is greater than, equal to, or less than the
%  string pointed to by q respectively.  The sign of a non-zero return value
%  is determined by the sign of the difference between the values of the first< %  pair of bytes that differ in the strings being compared.
%
%  The format of the LocaleCompare method is:
%
%      long LocaleCompare(const char *p,const char *q)
%
%  A description of each parameter follows:
%
%    o p: A pointer to a character string.
%
%    o q: A pointer to a character string to compare to p.
%
%
*/
MagickExport long LocaleCompare(const char *p,const char *q)
{
  if ((p == (char *) NULL) && (q == (char *) NULL))
    return(0);
  if (p == (char *) NULL)
    return(-1);
  if (q == (char *) NULL)
    return(1);
#if defined(HAVE_STRCASECMP)
  return((long) strcasecmp(p,q));
#else
  {
    register unsigned char
      c,
      d;

    for ( ; ; )
    {
      c=(unsigned char) *p;
      d=(unsigned char) *q;
      if ((c == '\0') || (AsciiMap[c] != AsciiMap[d]))
        break;
      p++;
      q++;
    }
    return((long) AsciiMap[c]-AsciiMap[d]);
  }
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L o c a l e L o w e r                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LocaleLower() transforms all of the characters in the supplied
%  null-terminated string, changing all uppercase letters to lowercase.
%
%  The format of the LocaleLower method is:
%
%      void LocaleLower(char *string)
%
%  A description of each parameter follows:
%
%    o string: A pointer to the string to convert to lower-case Locale.
%
%
*/
MagickExport void LocaleLower(char *string)
{
  register char
    *q;

  assert(string != (char *) NULL);
  for (q=string; *q != '\0'; q++)
    *q=(char) tolower((int) *q);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L o c a l e N C o m p a r e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LocaleNCompare() performs a case-insensitive comparison of two
%  strings byte-by-byte, according to the ordering of the current locale
%  encoding. LocaleNCompare returns an integer greater than, equal to, or
%  less than 0, if the string pointed to by p is greater than, equal to, or
%  less than the string pointed to by q respectively.  The sign of a non-zero
%  return value is determined by the sign of the difference between the
%  values of the first pair of bytes that differ in the strings being
%  compared.  The LocaleNCompare method makes the same comparison as
%  LocaleCompare but looks at a maximum of n bytes.  Bytes following a
%  null byte are not compared.
%
%  The format of the LocaleNCompare method is:
%
%      long LocaleNCompare(const char *p,const char *q,const size_t n)
%
%  A description of each parameter follows:
%
%    o p: A pointer to a character string.
%
%    o q: A pointer to a character string to compare to p.
%
%    o length: The number of characters to compare in strings p & q.
%
%
*/
MagickExport long LocaleNCompare(const char *p,const char *q,
  const size_t length)
{
  if (p == (char *) NULL)
    return(-1);
  if (q == (char *) NULL)
    return(1);
#if defined(HAVE_STRNCASECMP)
  return((long) strncasecmp(p,q,length));
#else
  {
    register size_t
      n;

    register unsigned char
      c,
      d;

    for (n=length; n != 0; n--)
    {
      c=(unsigned char) *p;
      d=(unsigned char) *q;
      if (AsciiMap[c] != AsciiMap[d])
        return(AsciiMap[c]-AsciiMap[d]);
      if (c == '\0')
        return(0L);
      p++;
      q++;
    }
    return(0L);
  }
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L o c a l e U p p e r                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LocaleUpper() transforms all of the characters in the supplied
%  null-terminated string, changing all lowercase letters to uppercase.
%
%  The format of the LocaleUpper method is:
%
%      void LocaleUpper(char *string)
%
%  A description of each parameter follows:
%
%    o string: A pointer to the string to convert to upper-case Locale.
%
%
*/
MagickExport void LocaleUpper(char *string)
{
  register char
    *q;

  assert(string != (char *) NULL);
  for (q=string; *q != '\0'; q++)
    *q=(char) toupper((int) *q);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P r i n t S t r i n g I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PrintStringInfo() prints the string.
%
%  The format of the PrintStringInfo method is:
%
%      void PrintStringInfo(FILE *file,const char *id,
%        const StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o file: The file, typically stdout.
%
%    o id: The string id.
%
%    o string_info: The string info.
%
*/
MagickExport void PrintStringInfo(FILE *file,const char *id,
  const StringInfo *string_info)
{
  register const char
    *p;

  register size_t
    i,
    j;

  assert(id != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",id);
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickSignature);
  p=(char *) string_info->datum;
  for (i=0; i < string_info->length; i++)
  {
    if (((int) ((unsigned char) *p) < 32) &&
        (isspace((int) ((unsigned char) *p)) == 0))
      break;
    p++;
  }
  if (i == string_info->length)
    {
      (void) fputs((char *) string_info->datum,file);
      (void) fputc('\n',file);
      return;
    }
  /*
    Convert string to a HEX list.
  */
  p=(char *) string_info->datum;
  for (i=0; i < string_info->length; i+=0x14)
  {
    (void) fprintf(file,"0x%08lx: ",(unsigned long) (0x14*i));
    for (j=1; j <= (size_t) Min(string_info->length-i,0x14); j++)
    {
      (void) fprintf(file,"%02lx",(unsigned long) (*(p+j)) & 0xff);
      if ((j % 0x04) == 0)
        (void) fputc(' ',file);
    }
    for ( ; j <= 0x14; j++)
    {
      (void) fputc(' ',file);
      (void) fputc(' ',file);
      if ((j % 0x04) == 0)
        (void) fputc(' ',file);
    }
    (void) fputc(' ',file);
    for (j=1; j <= (size_t) Min(string_info->length-i,0x14); j++)
    {
      if (isprint((int) ((unsigned char) *p)) != 0)
        (void) fputc(*p,file);
      else
        (void) fputc('-',file);
      p++;
    }
    (void) fputc('\n',file);
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t S t r i n g I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetStringInfo() reset the string to all null bytes.
%
%  The format of the ResetStringInfo method is:
%
%      void ResetStringInfo(StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: The string info.
%
*/
MagickExport void ResetStringInfo(StringInfo *string_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickSignature);
  (void) ResetMagickMemory(string_info->datum,0,string_info->length);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t S t r i n g I n f o                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStringInfo() copies the source string to the destination string.
%
%  The format of the SetStringInfo method is:
%
%      void SetStringInfo(StringInfo *string_info,const StringInfo *source)
%
%  A description of each parameter follows:
%
%    o string_info: The string info.
%
%    o source: The source string.
%
*/
MagickExport void SetStringInfo(StringInfo *string_info,
  const StringInfo *source)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickSignature);
  assert(source != (StringInfo *) NULL);
  assert(source->signature == MagickSignature);
  (void) ResetMagickMemory(string_info->datum,0,string_info->length);
  (void) CopyMagickMemory(string_info->datum,source->datum,
    Min(string_info->length,source->length));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t S t r i n g I n f o D a t u m                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStringInfoDatum() copies bytes from the source string for the length of
%  the destination string.
%
%  The format of the SetStringInfoDatum method is:
%
%      void SetStringInfoDatum(StringInfo *string_info,
%        const unsigned char *source)
%
%  A description of each parameter follows:
%
%    o string_info: The string info.
%
%    o source: The source string.
%
*/
MagickExport void SetStringInfoDatum(StringInfo *string_info,
  const unsigned char *source)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickSignature);
  assert(source != (const unsigned char *) NULL);
  (void) CopyMagickMemory(string_info->datum,source,string_info->length);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t S t r i n g I n f o L e n g t h                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetStringInfoLength() set the string length to the specified value.
%
%  The format of the SetStringInfoLength method is:
%
%      void SetStringInfoLength(StringInfo *string_info,const size_t length)
%
%  A description of each parameter follows:
%
%    o string_info: The string info.
%
%    o length: The string length.
%
*/
MagickExport void SetStringInfoLength(StringInfo *string_info,
  const size_t length)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickSignature);
  string_info->length=length;
  (void) CheckOverflowException(length,MaxTextExtent);
  if (string_info->datum == (unsigned char *) NULL)
    string_info->datum=(unsigned char *)
      AcquireMagickMemory((length+MaxTextExtent)*sizeof(*string_info->datum));
  else
    string_info->datum=(unsigned char *) ResizeMagickMemory(string_info->datum,
      (length+MaxTextExtent)*sizeof(*string_info->datum));
  if (string_info->datum == (unsigned char *) NULL)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowMagickFatalException(ResourceLimitFatalError,
        "MemoryAllocationFailed",message);
      message=(char *) RelinquishMagickMemory(message);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S p l i t S t r i n g I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SplitStringInfo() splits a string into two and returns it.
%
%  The format of the SplitStringInfo method is:
%
%      StringInfo *SplitStringInfo(StringInfo *string_info,const size_t offset)
%
%  A description of each parameter follows:
%
%    o string_info: The string info.
%
*/
MagickExport StringInfo *SplitStringInfo(StringInfo *string_info,
  const size_t offset)
{
  StringInfo
    *split_info;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(string_info != (StringInfo *) NULL);
  assert(string_info->signature == MagickSignature);
  if (offset > string_info->length)
    return((StringInfo *) NULL);
  split_info=AcquireStringInfo(offset);
  SetStringInfo(split_info,string_info);
  (void) memmove(string_info->datum,string_info->datum+offset,
    string_info->length-offset+MaxTextExtent);
  SetStringInfoLength(string_info,string_info->length-offset);
  return(split_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t r i n g T o S t r i n g I n f o                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringToStringInfo() returns the contents of a file as a string.
%
%  The format of the StringToStringInfo method is:
%
%      StringInfo *StringToStringInfo(const char *string)
%
%  A description of each parameter follows:
%
%    o string:  The string.
%
*/
MagickExport StringInfo *StringToStringInfo(const char *string)
{
  StringInfo
    *string_info;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(string != (const char *) NULL);
  string_info=AcquireStringInfo(strlen(string)+1);
  SetStringInfoDatum(string_info,(const unsigned char *) string);
  return(string_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t r i n g I n f o T o S t r i n g                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringInfoToString() converts a string info string to a C string.
%
%  The format of the StringInfoToString method is:
%
%      char *StringInfoToString(const StringInfo *string_info)
%
%  A description of each parameter follows:
%
%    o string_info: The string.
%
*/
MagickExport char *StringInfoToString(const StringInfo *string_info)
{
  char
    *string;

  size_t
    length;

  length=CheckOverflowException(string_info->length,MaxTextExtent);
  string=(char *) AcquireMagickMemory(length*sizeof(*string));
  if (string == (char *) NULL)
    {
      char
        *message;

      message=GetExceptionMessage(errno);
      ThrowMagickFatalException(ResourceLimitFatalError,
        "MemoryAllocationFailed",message);
      message=(char *) RelinquishMagickMemory(message);
    }
  (void) CopyMagickString(string,(char *) string_info->datum,
    length*sizeof(*string));
  return(string);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  S t r i n g T o A r g v                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringToArgv() converts a text string into command line arguments.
%
%  The format of the StringToArgv method is:
%
%      char **StringToArgv(const char *text,int *argc)
%
%  A description of each parameter follows:
%
%    o argv:  Method StringToArgv returns the string list unless an error
%      occurs, otherwise NULL.
%
%    o text:  Specifies the string to segment into a list.
%
%    o argc:  This integer pointer returns the number of arguments in the
%      list.
%
%
*/
MagickExport char **StringToArgv(const char *text,int *argc)
{
  char
    **argv;

  register const char
    *p,
    *q;

  register long
    i;

  *argc=0;
  if (text == (char *) NULL)
    return((char **) NULL);
  /*
    Determine the number of arguments.
  */
  for (p=text; *p != '\0'; )
  {
    while (isspace((int) ((unsigned char) *p)) != 0)
      p++;
    (*argc)++;
    if (*p == '"')
      for (p++; (*p != '"') && (*p != '\0'); p++);
    if (*p == '\'')
      for (p++; (*p != '\'') && (*p != '\0'); p++);
    while ((isspace((int) ((unsigned char) *p)) == 0) && (*p != '\0'))
      p++;
  }
  (*argc)++;
  argv=(char **) AcquireMagickMemory((size_t) (*argc+1)*sizeof(*argv));
  if (argv == (char **) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,
      "UnableToConvertStringToARGV",text);
  /*
    Convert string to an ASCII list.
  */
  argv[0]=AcquireString("magick");
  p=text;
  for (i=1; i < (long) *argc; i++)
  {
    while (isspace((int) ((unsigned char) *p)) != 0)
      p++;
    q=p;
    if (*q == '"')
      {
        p++;
        for (q++; (*q != '"') && (*q != '\0'); q++);
      }
    else
      if (*q == '\'')
        {
          for (q++; (*q != '\'') && (*q != '\0'); q++);
          q++;
        }
      else
        while ((isspace((int) ((unsigned char) *q)) == 0) && (*q != '\0'))
          q++;
    argv[i]=(char *) AcquireMagickMemory(((q-p)+MaxTextExtent)*sizeof(**argv));
    if (argv[i] == (char *) NULL)
      {
        for (i--; i >= 0; i--)
          argv[i]=(char *) RelinquishMagickMemory(argv[i]);
        argv=(char **) RelinquishMagickMemory(argv);
        ThrowMagickFatalException(ResourceLimitFatalError,
          "UnableToConvertStringToARGV",text);
      }
    (void) CopyMagickString(argv[i],p,(size_t) (q-p+1));
    p=q;
    while ((isspace((int) ((unsigned char) *p)) == 0) && (*p != '\0'))
      p++;
  }
  argv[i]=(char *) NULL;
  return(argv);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  S t r i n g T o D o u b l e                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringToDouble() converts a text string to a double.  If the string contains
%  a percent sign (e.g. 50%) that percentage of the interval is returned.
%
%  The format of the StringToDouble method is:
%
%      double StringToDouble(const char *string,const double interval)
%
%  A description of each parameter follows:
%
%    o string:  Specifies the string representing a value.
%
%    o interval:  Specifies the interval; used for obtaining a percentage.
%
%
*/
MagickExport double StringToDouble(const char *string,const double interval)
{
  char
    *q;

  double
    value;

  assert(string != (char *) NULL);
  value=strtod(string,&q);
  if (strchr(q,'%') != (char *) NULL)
    value*=interval/100.0;
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  S t r i n g T o L i s t                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StringToList() converts a text string into a list by segmenting the text
%  string at each carriage return discovered.  The list is converted to HEX
%  characters if any control characters are discovered within the text string.
%
%  The format of the StringToList method is:
%
%      char **StringToList(const char *text)
%
%  A description of each parameter follows:
%
%    o list:  Method StringToList returns the string list unless an error
%      occurs, otherwise NULL.
%
%    o text:  Specifies the string to segment into a list.
%
%
*/
MagickExport char **StringToList(const char *text)
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
  for (p=text; *p != '\0'; p++)
    if (((int) ((unsigned char) *p) < 32) &&
        (isspace((int) ((unsigned char) *p)) == 0))
      break;
  if (*p == '\0')
    {
      /*
        Convert string to an ASCII list.
      */
      lines=1;
      for (p=text; *p != '\0'; p++)
        if (*p == '\n')
          lines++;
      textlist=(char **)
        AcquireMagickMemory((size_t) (lines+1)*sizeof(*textlist));
      if (textlist == (char **) NULL)
        ThrowMagickFatalException(ResourceLimitFatalError,"UnableToConvertText",
          text);
      p=text;
      for (i=0; i < (long) lines; i++)
      {
        for (q=(char *) p; *q != '\0'; q++)
          if ((*q == '\r') || (*q == '\n'))
            break;
        textlist[i]=(char *)
          AcquireMagickMemory(((q-p)+MaxTextExtent)*sizeof(*textlist));
        if (textlist[i] == (char *) NULL)
          ThrowMagickFatalException(ResourceLimitFatalError,
            "UnableToConvertText",text);
        (void) CopyMagickString(textlist[i],p,(size_t) (q-p+1));
        if (*q == '\r')
          q++;
        p=q+1;
      }
    }
  else
    {
      char
        hex_string[MaxTextExtent];

      register long
        j;

      /*
        Convert string to a HEX list.
      */
      lines=(unsigned long) (strlen(text)/0x14)+1;
      textlist=(char **)
        AcquireMagickMemory((size_t) (lines+1)*sizeof(*textlist));
      if (textlist == (char **) NULL)
        ThrowMagickFatalException(ResourceLimitFatalError,"UnableToConvertText",
          text);
      p=text;
      for (i=0; i < (long) lines; i++)
      {
        textlist[i]=(char *)
          AcquireMagickMemory(2*MaxTextExtent*sizeof(*textlist));
        if (textlist[i] == (char *) NULL)
          ThrowMagickFatalException(ResourceLimitFatalError,
            "UnableToConvertText",text);
        (void) FormatMagickString(textlist[i],MaxTextExtent,"0x%08lx: ",0x14*i);
        q=textlist[i]+strlen(textlist[i]);
        for (j=1; j <= (long) Min(strlen(p),0x14); j++)
        {
          (void) FormatMagickString(hex_string,MaxTextExtent,"%02x",*(p+j));
          (void) CopyMagickString(q,hex_string,MaxTextExtent);
          q+=2;
          if ((j % 0x04) == 0)
            *q++=' ';
        }
        for ( ; j <= 0x14; j++)
        {
          *q++=' ';
          *q++=' ';
          if ((j % 0x04) == 0)
            *q++=' ';
        }
        *q++=' ';
        for (j=1; j <= (long) Min(strlen(p),0x14); j++)
        {
          if (isprint((int) ((unsigned char) *p)) != 0)
            *q++=(*p);
          else
            *q++='-';
          p++;
        }
        *q='\0';
      }
    }
  textlist[i]=(char *) NULL;
  return(textlist);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t r i p S t r i n g                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StripString() strips any whitespace or quotes from the beginning and end of
%  a string of characters.
%
%  The format of the StripString method is:
%
%      void StripString(char *message)
%
%  A description of each parameter follows:
%
%    o message: Specifies an array of characters.
%
%
*/
MagickExport void StripString(char *message)
{
  register char
    *p,
    *q;

  size_t
    length;

  assert(message != (char *) NULL);
  if (*message == '\0')
    return;
  length=strlen(message);
  p=message;
  while (isspace((int) ((unsigned char) *p)) != 0)
    p++;
  if ((*p == '\'') || (*p == '"'))
    p++;
  q=message+length-1;
  while ((isspace((int) ((unsigned char) *q)) != 0) && (q > p))
    q--;
  if (q > p)
    if ((*q == '\'') || (*q == '"'))
      q--;
  (void) CopyMagickMemory(message,p,(size_t) (q-p+1));
  message[q-p+1]='\0';
  for (p=message; *p != '\0'; p++)
    if (*p == '\n')
      *p=' ';
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S u b s t i t u t e S t r i n g                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SubstituteString() performs string substitution on a buffer, replacing the
%  buffer with the substituted version. Buffer must be allocate from the heap.
%
%  The format of the SubstituteString method is:
%
%      MagickBooleanType SubstituteString(char **buffer,const char *search,
%        const char *replace)
%
%  A description of each parameter follows:
%
%    o buffer: The buffer to perform replacements on. Replaced with new
%      allocation if a replacement is made.
%
%    o search: String to search for.
%
%    o replace: Replacement string.
%
*/
MagickExport MagickBooleanType SubstituteString(char **buffer,
  const char *search,const char *replace)
{
  char
    *result;

  const char
    *match,
    *source;

  MagickOffsetType
    destination_offset;

  size_t
    allocate_length,
    copy_length,
    replace_length,
    result_length,
    search_length;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(buffer != (char **) NULL);
  assert(*buffer != (char *) NULL);
  assert(search != (const char*) NULL);
  assert(replace != (const char*) NULL);
  if (strcmp(search,replace) == 0)
    return(MagickTrue);
  source=(*buffer);
  match=strstr(source,search);
  if (match == (char *) NULL)
    return(MagickFalse);
  allocate_length=CheckOverflowException(strlen(source),MaxTextExtent);
  result=(char *) AcquireMagickMemory(allocate_length*sizeof(*result));
  if (result == (char *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"UnableToAcquireString",
      search);
  *result='\0';
  result_length=0;
  destination_offset=0;
  replace_length=strlen(replace);
  for (search_length=strlen(search); match != (char *) NULL; )
  {
    /*
      Copy portion before match.
    */
    copy_length=(size_t) (match-source);
    if (copy_length != 0)
      {
        result_length+=copy_length;
        if ((result_length+MaxTextExtent) >= allocate_length)
          {
            allocate_length+=copy_length;
            result=(char *) ResizeMagickMemory(result,
              (allocate_length+MaxTextExtent)*sizeof(*result));
            if (result == (char *) NULL)
              ThrowMagickFatalException(ResourceLimitFatalError,
                "UnableToAcquireString",search);
          }
        (void) CopyMagickString(result+destination_offset,source,copy_length+1);
        destination_offset+=copy_length;
      }
    /*
      Copy replacement.
    */
    result_length+=replace_length;
    if ((result_length+MaxTextExtent) >= allocate_length)
      {
        allocate_length+=replace_length;
        result=(char *) ResizeMagickMemory(result,(allocate_length+
          MaxTextExtent)*sizeof(*result));
        if (result == (char *) NULL)
          ThrowMagickFatalException(ResourceLimitFatalError,
            "UnableToAcquireString",search);
      }
    (void) ConcatenateMagickString(result+destination_offset,replace,
      (allocate_length+MaxTextExtent)*sizeof(*result));
    destination_offset+=replace_length;
    /*
      Find next match.
    */
    source=match;
    source+=search_length;
    match=strstr(source,search);
  }
  /*
    Copy remaining string.
  */
  copy_length=strlen(source);
  result_length+=copy_length;
  if ((result_length+MaxTextExtent) >= allocate_length)
    {
      allocate_length+=copy_length;
      result=(char *) ResizeMagickMemory(result,
        (allocate_length+MaxTextExtent)*sizeof(*result));
      if (result == (char *) NULL)
        ThrowMagickFatalException(ResourceLimitFatalError,
          "UnableToAcquireString",search);
    }
  (void) ConcatenateMagickString(result+destination_offset,source,
    (allocate_length+MaxTextExtent-destination_offset)*sizeof(*result));
  (void) RelinquishMagickMemory(*buffer);
  *buffer=result;
  return(MagickTrue);
}
