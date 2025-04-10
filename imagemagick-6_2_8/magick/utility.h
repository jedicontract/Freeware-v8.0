/*
  Copyright 1999-2006 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.
  
  You may not use this file except in compliance with the License.
  obtain a copy of the License at
  
    http://www.imagemagick.org/script/license.php
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore utility methods.
*/
#ifndef _MAGICKCORE_UTILITY_H
#define _MAGICKCORE_UTILITY_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum
{
  UndefinedPath,
  MagickPath,
  RootPath,
  HeadPath,
  TailPath,
  BasePath,
  ExtensionPath,
  SubimagePath,
  CanonicalPath
} PathType;

extern MagickExport char
  *Base64Encode(const unsigned char *,const size_t,size_t *),
  **GetPathComponents(const char *,unsigned long *),
  **ListFiles(const char *,const char *,unsigned long *);

extern MagickExport int
  SystemCommand(const MagickBooleanType,const char *);

extern MagickExport MagickBooleanType
  AcquireUniqueFilename(char *),
  AcquireUniqueSymbolicLink(const char *,char *),
  ExpandFilenames(int *,char ***),
  GetExecutionPath(char *),
  IsAccessible(const char *),
  IsMagickTrue(const char *);

extern MagickExport unsigned char
  *Base64Decode(const char *, size_t *);

extern MagickExport unsigned long
  MultilineCensus(const char *);

extern MagickExport void
  AppendImageFormat(const char *,char *),
  ChopPathComponents(char *,const unsigned long),
  ExpandFilename(char *),
  GetPathComponent(const char *,PathType,char *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
