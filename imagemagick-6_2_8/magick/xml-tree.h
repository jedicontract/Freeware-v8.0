/*
  Copyright 1999-2006 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.
  obtain a copy of the License at

    http://www.imagemagick.org/MagicksToolkit/script/license.php

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Magick's toolkit xml-tree methods.
*/
#ifndef _MAGICKCORE_XML_TREE_H
#define _MAGICKCORE_XML_TREE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <magick/exception.h>

typedef struct _XMLTreeInfo
  XMLTreeInfo;

extern MagickExport char
  *CanonicalXMLContent(const char *,const MagickBooleanType),
  *XMLTreeInfoToXML(XMLTreeInfo *);

extern MagickExport const char
  *GetXMLTreeAttribute(XMLTreeInfo *,const char *),
  *GetXMLTreeContent(XMLTreeInfo *),
  **GetXMLTreeProcessingInstructions(XMLTreeInfo *,const char *),
  *GetXMLTreeTag(XMLTreeInfo *);

extern MagickExport XMLTreeInfo
  *AddChildToXMLTree(XMLTreeInfo *,const char *,const size_t),
  *AddPathToXMLTree(XMLTreeInfo *,const char *,const size_t),
  *DestroyXMLTree(XMLTreeInfo *),
  *GetNextXMLTreeTag(XMLTreeInfo *),
  *GetXMLTreeChild(XMLTreeInfo *,const char *),
  *GetXMLTreeOrdered(XMLTreeInfo *),
  *GetXMLTreePath(XMLTreeInfo *,const char *),
  *GetXMLTreeSibling(XMLTreeInfo *),
  *NewXMLTree(const char *,ExceptionInfo *),
  *NewXMLTreeTag(const char *),
  *SetXMLTreeContent(XMLTreeInfo *,const char *);

extern MagickExport void
  RemoveTagFromXMLTree(XMLTreeInfo *),
  SetXMLTreeAttribute(XMLTreeInfo *,const char *,const char *);

#ifdef __cplusplus
}
#endif

#endif
