/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  M   M   AAA    GGGG  IIIII   CCCC  K   K                   %
%                  MM MM  A   A  G        I    C      K  K                    %
%                  M M M  AAAAA  G GGG    I    C      KKK                     %
%                  M   M  A   A  G   G    I    C      K  K                    %
%                  M   M  A   A   GGGG  IIIII   CCCC  K   K                   %
%                                                                             %
%                                                                             %
%               Methods to Read or List ImageMagick Image formats             %
%                                                                             %
%                            Software Design                                  %
%                            Bob Friesenhahn                                  %
%                              John Cristy                                    %
%                             November 1998                                   %
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
#include "magick/cache.h"
#include "magick/coder.h"
#include "magick/client.h"
#include "magick/coder.h"
#include "magick/configure.h"
#include "magick/constitute.h"
#include "magick/delegate.h"
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/locale_.h"
#include "magick/log.h"
#include "magick/magic.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/module.h"
#if defined(__WINDOWS__)
# include "magick/nt-feature.h"
#endif
#include "magick/random_.h"
#include "magick/registry.h"
#include "magick/resource_.h"
#include "magick/semaphore.h"
#include "magick/signature.h"
#include "magick/splay-tree.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/utility.h"
#include "magick/xwindow-private.h"

/*
  Define declarations.
*/
#if !defined(RETSIGTYPE)
# define RETSIGTYPE  void
#endif
#if !defined(SIG_DFL)
# define SIG_DFL  ((SignalHandler *) 0)
#endif
#if !defined(SIG_ERR)
# define SIG_ERR  ((SignalHandler *) -1)
#endif
#if !defined(SIGMAX)
#define SIGMAX  64
#endif

/*
  Typedef declarations.
*/
typedef RETSIGTYPE
  SignalHandler(int);

/*
  Global declarations.
*/
static SemaphoreInfo
  *magick_semaphore = (SemaphoreInfo *) NULL;

static SignalHandler
  *signal_handlers[SIGMAX] = { (SignalHandler *) NULL };

static SplayTreeInfo
  *magick_list = (SplayTreeInfo *) NULL;

static volatile MagickBooleanType
  instantiate_magick = MagickFalse;  /* double-checked locking pattern */

/*
  Forward declarations.
*/
static MagickBooleanType
  InitializeMagickList(ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y M a g i c k                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyMagick() destroys the ImageMagick environment.
%
%  The format of the DestroyMagick function is:
%
%      DestroyMagick(void)
%
%
*/
MagickExport void DestroyMagick(void)
{
#if defined(HasX11)
  DestroyXResources();
#endif
  DestroyConstitute();
  DestroyConfigureList();
  DestroyTypeList();
  DestroyColorList();
#if defined(__WINDOWS__)
  NTGhostscriptUnLoadDLL();
#endif
  DestroyMagicList();
  DestroyDelegateList();
  DestroyMagickList();
  DestroyCoderList();
  DestroyMagickResources();
  DestroyMagickRegistry();
  DestroyCacheResources();
  DestroyRandomReservoir();
  DestroyLocaleList();
  DestroyLogList();
  instantiate_magick=MagickFalse;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y M a g i c k L i s t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyMagickList() deallocates memory associated with the MagickInfo list.
%
%  The format of the DestroyMagickList method is:
%
%      void DestroyMagickList(void)
%
*/
MagickExport void DestroyMagickList(void)
{
#if defined(SupportMagickModules)
  DestroyModuleList();
#endif
#if !defined(BuildMagickModules)
  UnregisterStaticModules();
#endif
  AcquireSemaphoreInfo(&magick_semaphore);
  if (magick_list != (SplayTreeInfo *) NULL)
    magick_list=DestroySplayTree(magick_list);
  RelinquishSemaphoreInfo(magick_semaphore);
  magick_semaphore=DestroySemaphoreInfo(magick_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t I m a g e M a g i c k                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageMagick() searches for an image format that matches the specified
%  magick string.  If one is found the name is returned otherwise NULL.
%
%  The format of the GetImageMagick method is:
%
%      const char *GetImageMagick(const unsigned char *magick,
%        const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: The image format we are searching for.
%
%    o length: The length of the binary string.
%
%
*/
MagickExport const char *GetImageMagick(const unsigned char *magick,
  const size_t length)
{
  ExceptionInfo
    exception;

  register const MagickInfo
    *p;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(magick != (const unsigned char *) NULL);
  GetExceptionInfo(&exception);
  p=GetMagickInfo("*",&exception);
  (void) DestroyExceptionInfo(&exception);
  if (p == (const MagickInfo *) NULL)
    return((const char *) NULL);
  AcquireSemaphoreInfo(&magick_semaphore);
  ResetSplayTreeIterator(magick_list);
  p=(const MagickInfo *) GetNextValueInSplayTree(magick_list);
  while (p != (const MagickInfo *) NULL)
  {
    if ((p->magick != (MagickHandler *) NULL) &&
        (p->magick(magick,length) != 0))
      break;
    p=(const MagickInfo *) GetNextValueInSplayTree(magick_list);
  }
  RelinquishSemaphoreInfo(magick_semaphore);
  if (p != (MagickInfo *) NULL)
    return(p->name);
  return((const char *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k A d j o i n                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickAdjoin() returns MagickTrue if the magick adjoin is MagickTrue.
%
%  The format of the GetMagickAdjoin method is:
%
%      MagickBooleanType GetMagickAdjoin(const MagickInfo *magick_info)
%
%  A description of each parameter follows:
%
%    o magick_info:  The magick info.
%
*/
MagickExport MagickBooleanType GetMagickAdjoin(const MagickInfo *magick_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(magick_info != (MagickInfo *) NULL);
  assert(magick_info->signature == MagickSignature);
  return(magick_info->adjoin);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k B l o b S u p p o r t                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickBlobSupport() returns MagickTrue if the magick supports blobs.
%
%  The format of the GetMagickBlobSupport method is:
%
%      MagickBooleanType GetMagickBlobSupport(const MagickInfo *magick_info)
%
%  A description of each parameter follows:
%
%    o magick_info:  The magick info.
%
*/
MagickExport MagickBooleanType GetMagickBlobSupport(
  const MagickInfo *magick_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(magick_info != (MagickInfo *) NULL);
  assert(magick_info->signature == MagickSignature);
  return(magick_info->blob_support);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k D e c o d e r                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickDecoder() returns the magick decoder.
%
%  The format of the GetMagickDecoder method is:
%
%      DecoderHandler *GetMagickDecoder(const MagickInfo *magick_info)
%
%  A description of each parameter follows:
%
%    o magick_info:  The magick info.
%
*/
MagickExport DecoderHandler *GetMagickDecoder(const MagickInfo *magick_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(magick_info != (MagickInfo *) NULL);
  assert(magick_info->signature == MagickSignature);
  return((DecoderHandler *) magick_info->decoder);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k D e s c r i p t i o n                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickDescription() returns the magick description.
%
%  The format of the GetMagickDescription method is:
%
%      const char *GetMagickDescription(const MagickInfo *magick_info)
%
%  A description of each parameter follows:
%
%    o magick_info:  The magick info.
%
*/
MagickExport const char *GetMagickDescription(const MagickInfo *magick_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(magick_info != (MagickInfo *) NULL);
  assert(magick_info->signature == MagickSignature);
  return(magick_info->description);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k E n c o d e r                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickEncoder() returns the magick encoder.
%
%  The format of the GetMagickEncoder method is:
%
%      EncoderHandler *GetMagickEncoder(const MagickInfo *magick_info)
%
%  A description of each parameter follows:
%
%    o magick_info:  The magick info.
%
*/
MagickExport EncoderHandler *GetMagickEncoder(const MagickInfo *magick_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(magick_info != (MagickInfo *) NULL);
  assert(magick_info->signature == MagickSignature);
  return((EncoderHandler *) magick_info->encoder);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k E n d i a n S u p p o r t                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickEndianSupport() returns the MagickTrue if the coder respects
%  endianess other than MSBEndian.
%
%  The format of the GetMagickEndianSupport method is:
%
%      EndianType GetMagickEndianSupport(const MagickInfo *magick_info)
%
%  A description of each parameter follows:
%
%    o magick_info:  The magick info.
%
*/
MagickExport MagickBooleanType GetMagickEndianSupport(
  const MagickInfo *magick_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(magick_info != (MagickInfo *) NULL);
  assert(magick_info->signature == MagickSignature);
  return(magick_info->endian_support);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k I n f o                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickInfo() returns a pointer MagickInfo structure that matches
%  the specified name.  If name is NULL, the head of the image format list
%  is returned.
%
%  The format of the GetMagickInfo method is:
%
%      const MagickInfo *GetMagickInfo(const char *name,Exception *exception)
%
%  A description of each parameter follows:
%
%    o name: The image format we are looking for.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport const MagickInfo *GetMagickInfo(const char *name,
  ExceptionInfo *exception)
{
  register const MagickInfo
    *p;

  assert(exception != (ExceptionInfo *) NULL);
  if ((magick_list == (SplayTreeInfo *) NULL) ||
      (instantiate_magick == MagickFalse))
    if (InitializeMagickList(exception) == MagickFalse)
      return((const MagickInfo *) NULL);
  if ((name == (const char *) NULL) || (LocaleCompare(name,"*") == 0))
    {
#if defined(SupportMagickModules)
      if (LocaleCompare(name,"*") == 0)
        (void) OpenModules(exception);
#endif
      ResetSplayTreeIterator(magick_list);
      return((const MagickInfo *) GetNextValueInSplayTree(magick_list));
    }
  /*
    Find name in list.
  */
  AcquireSemaphoreInfo(&magick_semaphore);
  ResetSplayTreeIterator(magick_list);
  p=(const MagickInfo *) GetNextValueInSplayTree(magick_list);
  while (p != (const MagickInfo *) NULL)
  {
    if (LocaleCompare(p->name,name) == 0)
      break;
    p=(const MagickInfo *) GetNextValueInSplayTree(magick_list);
  }
#if defined(SupportMagickModules)
  if (p == (const MagickInfo *) NULL)
    {
      if (*name != '\0')
        (void) OpenModule(name,exception);
      ResetSplayTreeIterator(magick_list);
      p=(const MagickInfo *) GetNextValueInSplayTree(magick_list);
      while (p != (const MagickInfo *) NULL)
      {
        if (LocaleCompare(p->name,name) == 0)
          break;
        p=(const MagickInfo *) GetNextValueInSplayTree(magick_list);
      }
    }
#endif
  RelinquishSemaphoreInfo(magick_semaphore);
  return(p);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k I n f o L i s t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickInfoList() returns any image formats that match the specified
%  pattern.
%
%  The format of the GetMagickInfoList function is:
%
%      const MagickInfo **GetMagickInfoList(const char *pattern,
%        unsigned long *number_formats,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_formats:  This integer returns the number of formats in the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int MagickInfoCompare(const void *x,const void *y)
{
  const MagickInfo
    **p,
    **q;

  p=(const MagickInfo **) x,
  q=(const MagickInfo **) y;
  return(LocaleCompare((*p)->name,(*q)->name));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport const MagickInfo **GetMagickInfoList(const char *pattern,
  unsigned long *number_formats,ExceptionInfo *exception)
{
  const MagickInfo
    **formats;

  register const MagickInfo
    *p;

  register long
    i;

  /*
    Allocate magick list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_formats != (unsigned long *) NULL);
  *number_formats=0;
  p=GetMagickInfo("*",exception);
  if (p == (const MagickInfo *) NULL)
    return((const MagickInfo **) NULL);
  formats=(const MagickInfo **) AcquireMagickMemory((size_t)
    (GetNumberOfNodesInSplayTree(magick_list)+1)*sizeof(*formats));
  if (formats == (const MagickInfo **) NULL)
    return((const MagickInfo **) NULL);
  /*
    Generate magick list.
  */
  AcquireSemaphoreInfo(&magick_semaphore);
  ResetSplayTreeIterator(magick_list);
  p=(const MagickInfo *) GetNextValueInSplayTree(magick_list);
  for (i=0; p != (const MagickInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->name,pattern) != MagickFalse))
      formats[i++]=p;
    p=(const MagickInfo *) GetNextValueInSplayTree(magick_list);
  }
  RelinquishSemaphoreInfo(magick_semaphore);
  qsort((void *) formats,(size_t) i,sizeof(*formats),MagickInfoCompare);
  formats[i]=(MagickInfo *) NULL;
  *number_formats=(unsigned long) i;
  return(formats);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k L i s t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickList() returns any image formats that match the specified pattern.
%
%  The format of the GetMagickList function is:
%
%      char **GetMagickList(const char *pattern,unsigned long *number_formats,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_formats:  This integer returns the number of formats in the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int MagickCompare(const void *x,const void *y)
{
  register const char
    **p,
    **q;

  p=(const char **) x;
  q=(const char **) y;
  return(LocaleCompare(*p,*q));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport char **GetMagickList(const char *pattern,
  unsigned long *number_formats,ExceptionInfo *exception)
{
  char
    **formats;

  register const MagickInfo
    *p;

  register long
    i;

  /*
    Allocate magick list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_formats != (unsigned long *) NULL);
  *number_formats=0;
  p=GetMagickInfo("*",exception);
  if (p == (const MagickInfo *) NULL)
    return((char **) NULL);
  formats=(char **) AcquireMagickMemory((size_t)
    (GetNumberOfNodesInSplayTree(magick_list)+1)*sizeof(*formats));
  if (formats == (char **) NULL)
    return((char **) NULL);
  AcquireSemaphoreInfo(&magick_semaphore);
  ResetSplayTreeIterator(magick_list);
  p=(const MagickInfo *) GetNextValueInSplayTree(magick_list);
  for (i=0; p != (const MagickInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->name,pattern) != MagickFalse))
      formats[i++]=ConstantString(p->name);
    p=(const MagickInfo *) GetNextValueInSplayTree(magick_list);
  }
  RelinquishSemaphoreInfo(magick_semaphore);
  qsort((void *) formats,(size_t) i,sizeof(*formats),MagickCompare);
  formats[i]=(char *) NULL;
  *number_formats=(unsigned long) i;
  return(formats);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k S e e k a b l e S t r e a m                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickSeekableStream() returns MagickTrue if the magick supports a
%  seekable stream.
%
%  The format of the GetMagickSeekableStream method is:
%
%      MagickBooleanType GetMagickSeekableStream(const MagickInfo *magick_info)
%
%  A description of each parameter follows:
%
%    o magick_info:  The magick info.
%
*/
MagickExport MagickBooleanType GetMagickSeekableStream(
  const MagickInfo *magick_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(magick_info != (MagickInfo *) NULL);
  assert(magick_info->signature == MagickSignature);
  return(magick_info->seekable_stream);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k T h r e a d S u p p o r t                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickThreadSupport() returns MagickTrue if the magick supports threads.
%
%  The format of the GetMagickThreadSupport method is:
%
%      MagickBooleanType GetMagickThreadSupport(const MagickInfo *magick_info)
%
%  A description of each parameter follows:
%
%    o magick_info:  The magick info.
%
*/
MagickExport MagickBooleanType GetMagickThreadSupport(
  const MagickInfo *magick_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(magick_info != (MagickInfo *) NULL);
  assert(magick_info->signature == MagickSignature);
  return(magick_info->thread_support);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n i t i a l i z e M a g i c k                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeMagick() initializes the ImageMagick environment.
%
%  The format of the InitializeMagick function is:
%
%      InitializeMagick(const char *path)
%
%  A description of each parameter follows:
%
%    o path: The execution path of the current ImageMagick client.
%
%
*/

static SignalHandler *SetMagickSignalHandler(int signal_number,
  SignalHandler *handler)
{
#if defined(HAVE_SIGACTION) && defined(HAVE_SIGEMPTYSET)
  int
    status;

  sigset_t
    mask;

  struct sigaction
    action,
    previous_action;

  sigemptyset(&mask);
  sigaddset(&mask,signal_number);
  sigprocmask(SIG_BLOCK,&mask,NULL);
  action.sa_mask=mask;
  action.sa_handler=handler;
  action.sa_flags=0;
#if defined(SA_INTERRUPT)
  action.sa_flags|=SA_INTERRUPT;
#endif
  status=sigaction(signal_number,&action,&previous_action);
  if (status < 0)
    return(SIG_ERR);
  sigprocmask(SIG_UNBLOCK,&mask,NULL);
  return(previous_action.sa_handler);
#else
  return(signal(signal_number,handler));
#endif
}

static void MagickSignalHandler(int signal_number)
{
#if !defined(HAVE_SIGACTION)
  (void) signal(signal_number,SIG_IGN);
#endif
  AsynchronousDestroyMagickResources();
  instantiate_magick=MagickFalse;
  (void) SetMagickSignalHandler(signal_number,signal_handlers[signal_number]);
#if defined(HAVE_RAISE)
  if (signal_handlers[signal_number] != MagickSignalHandler)
    raise(signal_number);
#endif
#if !defined(HAVE__EXIT)
  exit(signal_number);
#else
#if defined(SIGHUP)
  if (signal_number == SIGHUP)
    exit(signal_number);
#endif
#if defined(SIGBREAK)
  if (signal_number == SIGBREAK)
    exit(signal_number);
#endif
#if defined(SIGINT) && !defined(__WINDOWS__)
  if (signal_number == SIGINT)
    exit(signal_number);
#endif
#if defined(SIGTERM)
  if (signal_number == SIGTERM)
    exit(signal_number);
#endif
  _exit(signal_number);
#endif
}

static SignalHandler *RegisterMagickSignalHandler(int signal_number)
{
  SignalHandler
    *handler;

  handler=SetMagickSignalHandler(signal_number,MagickSignalHandler);
  if (handler == SIG_ERR)
    return(handler);
  if (handler != SIG_DFL)
    handler=SetMagickSignalHandler(signal_number,handler);
  else
    (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
      "Register handler for signal: %d",signal_number);
  return(handler);
}

MagickExport void InitializeMagick(const char *path)
{
  char
    *events,
    execution_path[MaxTextExtent],
    filename[MaxTextExtent];

  ExceptionInfo
    exception;

  time_t
    seconds;

  /*
    Initialize the Magick environment.
  */
  (void) setlocale(LC_ALL,"");
  (void) setlocale(LC_NUMERIC,"C");
  seconds=time((time_t *) NULL);
  srand((unsigned int) (seconds+getpid()));
  InitializeSemaphore();
  events=GetEnvironmentValue("MAGICK_DEBUG");
  if (events != (char *) NULL)
    {
      (void) SetLogEventMask(events);
      events=(char *) RelinquishMagickMemory(events);
    }
#if defined(__WINDOWS__)
  (void) NTControlHandler();
#if defined(_DEBUG) && !defined(__BORLANDC__)
  if (IsEventLogging() != MagickFalse)
    {
      int
        debug;

      debug=_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
      debug|=_CRTDBG_CHECK_ALWAYS_DF |_CRTDBG_DELAY_FREE_MEM_DF |
        _CRTDBG_LEAK_CHECK_DF;
      if (0)
        {
          debug=_CrtSetDbgFlag(debug);
          _ASSERTE(_CrtCheckMemory());
        }
    }
#endif
#endif
  /*
    Set client name and execution path.
  */
  (void) GetExecutionPath(execution_path);
  if ((path != (const char *) NULL) && (*path != '\0'))
    (void) CopyMagickString(execution_path,path,MaxTextExtent);
  GetPathComponent(execution_path,TailPath,filename);
  (void) SetClientName(filename);
  GetPathComponent(execution_path,HeadPath,execution_path);
  (void) SetClientPath(execution_path);
  /*
    Set signal handlers.
  */
#if defined(SIGABRT)
  if (signal_handlers[SIGABRT] == (SignalHandler *) NULL)
    signal_handlers[SIGABRT]=RegisterMagickSignalHandler(SIGABRT);
#endif
#if defined(SIGBREAK)
  if (signal_handlers[SIGBREAK] == (SignalHandler *) NULL)
    signal_handlers[SIGBREAK]=RegisterMagickSignalHandler(SIGBREAK);
#endif
#if defined(SIGFPE)
  if (signal_handlers[SIGFPE] == (SignalHandler *) NULL)
    signal_handlers[SIGFPE]=RegisterMagickSignalHandler(SIGFPE);
#endif
#if defined(SIGHUP)
  if (signal_handlers[SIGHUP] == (SignalHandler *) NULL)
    signal_handlers[SIGHUP]=RegisterMagickSignalHandler(SIGHUP);
#endif
#if defined(SIGINT) && !defined(__WINDOWS__)
  if (signal_handlers[SIGINT] == (SignalHandler *) NULL)
    signal_handlers[SIGINT]=RegisterMagickSignalHandler(SIGINT);
#endif
#if defined(SIGQUIT)
  if (signal_handlers[SIGQUIT] == (SignalHandler *) NULL)
    signal_handlers[SIGQUIT]=RegisterMagickSignalHandler(SIGQUIT);
#endif
#if defined(SIGTERM)
  if (signal_handlers[SIGTERM] == (SignalHandler *) NULL)
    signal_handlers[SIGTERM]=RegisterMagickSignalHandler(SIGTERM);
#endif
#if defined(SIGXCPU)
  if (signal_handlers[SIGXCPU] == (SignalHandler *) NULL)
    signal_handlers[SIGXCPU]=RegisterMagickSignalHandler(SIGXCPU);
#endif
#if defined(SIGXFSZ)
  if (signal_handlers[SIGXFSZ] == (SignalHandler *) NULL)
    signal_handlers[SIGXFSZ]=RegisterMagickSignalHandler(SIGXFSZ);
#endif
  /*
    Initialize magick resources.
  */
  InitializeMagickResources();
  GetExceptionInfo(&exception);
  (void) GetMagickInfo((char *) NULL,&exception);
  (void) DestroyExceptionInfo(&exception);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e M a g i c k L i s t                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeMagickList() initializes the magick list.
%
%  The format of the InitializeMagickList() method is:
%
%      InitializeMagickList(Exceptioninfo *exception)
%
%  A description of each parameter follows.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static void *DestroyMagickNode(void *magick_info)
{
  register MagickInfo
    *p;

  p=(MagickInfo *) magick_info;
  if (p->name != (char *) NULL)
    p->name=(char *) RelinquishMagickMemory(p->name);
  if (p->description != (char *) NULL)
    p->description=(char *) RelinquishMagickMemory(p->description);
  if (p->version != (char *) NULL)
    p->version=(char *) RelinquishMagickMemory(p->version);
  if (p->note != (char *) NULL)
    p->note=(char *) RelinquishMagickMemory(p->note);
  if (p->module != (char *) NULL)
    p->module=(char *) RelinquishMagickMemory(p->module);
  return(RelinquishMagickMemory(p));
}

static MagickBooleanType InitializeMagickList(ExceptionInfo *exception)
{
  if ((magick_list == (SplayTreeInfo *) NULL) &&
      (instantiate_magick == MagickFalse))
    {
      AcquireSemaphoreInfo(&magick_semaphore);
      if ((magick_list == (SplayTreeInfo *) NULL) &&
          (instantiate_magick == MagickFalse))
        {
          MagickBooleanType
            status;

          MagickInfo
            *magick_info;

          magick_list=NewSplayTree(CompareSplayTreeString,
            RelinquishMagickMemory,DestroyMagickNode);
          if (magick_list == (SplayTreeInfo *) NULL)
            {
              char
                *message;

              message=GetExceptionMessage(errno);
              ThrowMagickFatalException(ResourceLimitFatalError,
                "MemoryAllocationFailed",message);
              message=(char *) RelinquishMagickMemory(message);
            }
          magick_info=SetMagickInfo("tmp");
          magick_info->stealth=MagickTrue;
          status=AddValueToSplayTree(magick_list,ConstantString(
            magick_info->name),magick_info);
          if (status == MagickFalse)
            {
              char
                *message;

              message=GetExceptionMessage(errno);
              ThrowMagickFatalException(ResourceLimitFatalError,
                "MemoryAllocationFailed",message);
              message=(char *) RelinquishMagickMemory(message);
            }
          magick_info=SetMagickInfo("clipmask");
          magick_info->stealth=MagickTrue;
          status=AddValueToSplayTree(magick_list,ConstantString(
            magick_info->name),magick_info);
          if (status == MagickFalse)
            {
              char
                *message;

              message=GetExceptionMessage(errno);
              ThrowMagickFatalException(ResourceLimitFatalError,
                "MemoryAllocationFailed",message);
              message=(char *) RelinquishMagickMemory(message);
            }
#if defined(SupportMagickModules)
          (void) GetModuleInfo((char *) NULL,exception);
#endif
#if !defined(BuildMagickModules)
          RegisterStaticModules();
#endif
          instantiate_magick=MagickTrue;
        }
      RelinquishSemaphoreInfo(magick_semaphore);
    }
  return(magick_list != (SplayTreeInfo *) NULL ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I s M a g i c k C o n f l i c t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsMagickConflict() returns true if the image format is not a valid image
%  format or conflicts with a logical drive (.e.g. X:).
%
%  The format of the IsMagickConflict method is:
%
%      MagickBooleanType IsMagickConflict(const char *magick)
%
%  A description of each parameter follows:
%
%    o magick: Specifies the image format.
%
*/
MagickExport MagickBooleanType IsMagickConflict(const char *magick)
{
  const DelegateInfo
    *delegate_info;

  const MagickInfo
    *magick_info;

  ExceptionInfo
    exception;

  assert(magick != (char *) NULL);
  GetExceptionInfo(&exception);
  magick_info=GetMagickInfo(magick,&exception);
  delegate_info=GetDelegateInfo(magick,(char *) NULL,&exception);
  if (delegate_info == (const DelegateInfo *) NULL)
    delegate_info=GetDelegateInfo((char *) NULL,magick,&exception);
  (void) DestroyExceptionInfo(&exception);
  if ((magick_info == (const MagickInfo *) NULL) &&
      (delegate_info == (const DelegateInfo *) NULL))
    return(MagickTrue);
#if defined(macintosh)
  return(MACIsMagickConflict(magick));
#endif
#if defined(vms)
  return(VMSIsMagickConflict(magick));
#endif
#if defined(__WINDOWS__)
  return(NTIsMagickConflict(magick));
#endif
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  L i s t M a g i c k I n f o                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListMagickInfo() lists the image formats to a file.
%
%  The format of the ListMagickInfo method is:
%
%      MagickBooleanType ListMagickInfo(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file: A file handle.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType ListMagickInfo(FILE *file,
  ExceptionInfo *exception)
{
  const MagickInfo
    **magick_info;

  long
    j;

  register long
    i;

  unsigned long
    number_formats;

  if (file == (FILE *) NULL)
    file=stdout;
  magick_info=GetMagickInfoList("*",&number_formats,exception);
  if (magick_info == (const MagickInfo **) NULL)
    return(MagickFalse);
  ClearMagickException(exception);
#if !defined(SupportMagickModules)
  (void) fprintf(file,"   Format  Mode  Description\n");
#else
  (void) fprintf(file,"   Format  Module    Mode  Description\n");
#endif
  (void) fprintf(file,"--------------------------------------------------------"
    "-----------------------\n");
  for (i=0; i < (long) number_formats; i++)
  {
    if (magick_info[i]->stealth != MagickFalse)
      continue;
    (void) fprintf(file,"%9s%c ",magick_info[i]->name != (char *) NULL ?
      magick_info[i]->name : "",
      magick_info[i]->blob_support != MagickFalse ? '*' : ' ');
#if defined(SupportMagickModules)
    {
      char
        module[MaxTextExtent];

      *module='\0';
      if (magick_info[i]->module != (char *) NULL)
        (void) CopyMagickString(module,magick_info[i]->module,MaxTextExtent);
      (void) ConcatenateMagickString(module,"          ",MaxTextExtent);
      module[9]='\0';
      (void) fprintf(file,"%9s ",module);
    }
#endif
    (void) fprintf(file,"%c%c%c ",magick_info[i]->decoder ? 'r' : '-',
      magick_info[i]->encoder ? 'w' : '-',magick_info[i]->encoder != NULL &&
      magick_info[i]->adjoin != MagickFalse ? '+' : '-');
    if (magick_info[i]->description != (char *) NULL)
      (void) fprintf(file,"  %s",magick_info[i]->description);
    if (magick_info[i]->version != (char *) NULL)
      (void) fprintf(file," (%s)",magick_info[i]->version);
    (void) fprintf(file,"\n");
    if (magick_info[i]->note != (char *) NULL)
      {
        char
          **text;

        text=StringToList(magick_info[i]->note);
        if (text != (char **) NULL)
          {
            for (j=0; text[j] != (char *) NULL; j++)
            {
              (void) fprintf(file,"           %s\n",text[j]);
              text[j]=(char *) RelinquishMagickMemory(text[j]);
            }
            text=(char **) RelinquishMagickMemory(text);
          }
      }
  }
  (void) fprintf(file,"\n* native blob support\n\n");
  (void) fflush(file);
  magick_info=(const MagickInfo **)
    RelinquishMagickMemory((void *) magick_info);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  I s M a g i c k I n s t a n t i a t e d                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsMagickInstantiated() returns MagickTrue if the ImageMagick environment
%  is currently instantiated:  InitializeMagick() has been called but
%  MagickDestroy() has not.
%
%  The format of the IsMagickInstantiated method is:
%
%      MagickBooleanType IsMagickInstantiated(void)
%
*/
MagickExport MagickBooleanType IsMagickInstantiated(void)
{
  return(instantiate_magick);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  M a g i c k T o M i m e                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickToMime() returns the officially registered (or de facto) MIME
%  media-type corresponding to a magick string.  If there is no registered
%  media-type, then the string "image/x-magick" (all lower case) is returned.
%  The returned string must be deallocated by the user.
%
%  The format of the MagickToMime method is:
%
%      char *MagickToMime(const char *magick)
%
%  A description of each parameter follows.
%
%   o  magick:  ImageMagick format specification "magick" tag.
%
%
*/
MagickExport char *MagickToMime(const char *magick)
{
  typedef struct _MediaType
  {
    const char
      *magick,
      *media;
  } MediaType;

  char
    media[MaxTextExtent];

  MediaType
    *entry;

  static MediaType
    MediaTypes[] =
    {
      { "avi",   "video/avi" },
      { "cgm",   "image/cgm;Version=4;ProfileId=WebCGM" }, /* W3 WebCGM */
      { "dcm",   "application/dicom" }, /* Incomplete.  See RFC 3240 */
      { "epdf",  "application/pdf" },
      { "epi",   "application/postscript" },
      { "eps",   "application/postscript" },
      { "eps2",  "application/postscript" },
      { "eps3",  "application/postscript" },
      { "epsf",  "application/postscript" },
      { "ept",   "application/postscript" },
      { "fax",   "image/g3fax" },
      { "fpx",   "image/vnd.fpx" },
      { "g3",    "image/g3fax" },
      { "gif",   "image/gif" },
      { "gif87", "image/gif" },
      { "jpg",   "image/jpeg" },
      { "jpeg",  "image/jpeg" },
      { "mng",   "video/x-mng" },
      { "mpeg",  "video/mpeg" },
      { "png",   "image/png" },
      { "pdf",   "application/pdf" },
      { "ps",    "application/postscript" },
      { "ps2",   "application/postscript" },
      { "ps3",   "application/postscript" },
      { "svg",   "image/svg+xml" },
      { "tif",   "image/tiff" },
      { "tiff",  "image/tiff" },
      { "wbmp",  "image/vnd.wap.wbmp" },
      { (char *) NULL, (char *) NULL }
    };

  for (entry=MediaTypes; entry->magick != (char *) NULL; entry++)
    if (LocaleCompare(entry->magick,magick) == 0)
      return(AcquireString(entry->media));
  (void) FormatMagickString(media,MaxTextExtent,"image/x-%s",magick);
  LocaleLower(media+8);
  return(AcquireString(media));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e g i s t e r M a g i c k I n f o                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterMagickInfo() adds attributes for a particular image format to the
%  list of supported formats.  The attributes include the image format name,
%  a method to read and/or write the format, whether the format supports the
%  saving of more than one frame to the same file or blob, whether the format
%  supports native in-memory I/O, and a brief description of the format.
%
%  The format of the RegisterMagickInfo method is:
%
%      MagickInfo *RegisterMagickInfo(MagickInfo *magick_info)
%
%  A description of each parameter follows:
%
%    o magick_info: The magick info.
%
*/
MagickExport MagickInfo *RegisterMagickInfo(MagickInfo *magick_info)
{
  MagickBooleanType
    status;

  /*
    Delete any existing name.
  */
  assert(magick_info != (MagickInfo *) NULL);
  assert(magick_info->signature == MagickSignature);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",magick_info->name);
  if (magick_list == (SplayTreeInfo *) NULL)
    return((MagickInfo *) NULL);
  status=AddValueToSplayTree(magick_list,ConstantString(magick_info->name),
    magick_info);
  if (status == MagickFalse)
    ThrowMagickFatalException(ResourceLimitFatalError,
      "UnableToAllocateMagickInfo",magick_info->name);
  return(magick_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t M a g i c k I n f o                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetMagickInfo() allocates a MagickInfo structure and initializes the members
%  to default values.
%
%  The format of the SetMagickInfo method is:
%
%      MagickInfo *SetMagickInfo(const char *name)
%
%  A description of each parameter follows:
%
%    o magick_info: Method SetMagickInfo returns the allocated and initialized
%      MagickInfo structure.
%
%    o name: a character string that represents the image format associated
%      with the MagickInfo structure.
%
%
*/
MagickExport MagickInfo *SetMagickInfo(const char *name)
{
  MagickInfo
    *magick_info;

  assert(name != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",name);
  magick_info=(MagickInfo *) AcquireMagickMemory(sizeof(*magick_info));
  if (magick_info == (MagickInfo *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,
      "UnableToAllocateMagickInfo",name);
  (void) ResetMagickMemory(magick_info,0,sizeof(*magick_info));
  magick_info->name=ConstantString(name);
  magick_info->adjoin=MagickTrue;
  magick_info->blob_support=MagickTrue;
  magick_info->thread_support=MagickTrue;
  magick_info->signature=MagickSignature;
  return(magick_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   U n r e g i s t e r M a g i c k I n f o                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterMagickInfo() removes a name from the magick info list.  It returns
%  MagickFalse if the name does not exist in the list otherwise MagickTrue.
%
%  The format of the UnregisterMagickInfo method is:
%
%      MagickBooleanType UnregisterMagickInfo(const char *name)
%
%  A description of each parameter follows:
%
%    o name: a character string that represents the image format we are
%      looking for.
%
*/
MagickExport MagickBooleanType UnregisterMagickInfo(const char *name)
{
  register const MagickInfo
    *p;

  MagickBooleanType
    status;

  assert(name != (const char *) NULL);
  if (magick_list == (SplayTreeInfo *) NULL)
    return(MagickFalse);
  if (GetNumberOfNodesInSplayTree(magick_list) == 0)
    return(MagickFalse);
  status=MagickFalse;
  AcquireSemaphoreInfo(&magick_semaphore);
  ResetSplayTreeIterator(magick_list);
  p=(const MagickInfo *) GetNextValueInSplayTree(magick_list);
  while (p != (const MagickInfo *) NULL)
  {
    if (LocaleCompare(p->name,name) == 0)
      break;
    p=(const MagickInfo *) GetNextValueInSplayTree(magick_list);
  }
  RelinquishSemaphoreInfo(magick_semaphore);
  return(RemoveNodeByValueFromSplayTree(magick_list,p));
}
