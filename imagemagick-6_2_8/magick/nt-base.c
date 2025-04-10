/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                                 N   N  TTTTT                                %
%                                 NN  N    T                                  %
%                                 N N N    T                                  %
%                                 N  NN    T                                  %
%                                 N   N    T                                  %
%                                                                             %
%                                                                             %
%                  Windows NT Utility Methods for ImageMagick                 %
%                                                                             %
%                               Software Design                               %
%                                 John Cristy                                 %
%                                December 1996                                %
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
#if defined(__WINDOWS__)
#include "magick/client.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/resource_.h"
#include "magick/timer.h"
#include "magick/string_.h"
#include "magick/utility.h"
#include "magick/version.h"
#if defined(HasLTDL)
#  include "ltdl.h"
#endif /* defined(HasLTDL) */
#include "magick/nt-base.h"

/*
  Define declarations.
*/
#if !defined(MAP_FAILED)
#define MAP_FAILED      ((void *) -1)
#endif

/*
  Static declarations.
*/
#if !defined(HasLTDL)
static char
  *lt_slsearchpath = (char *) NULL;
#endif

static GhostscriptVectors
  gs_vectors;

static void
  *gs_dll_handle = (void *) NULL;

/*
  External declarations.
*/
#if !defined(__WINDOWS__)
extern "C" BOOL WINAPI
  DllMain(HINSTANCE handle,DWORD reason,LPVOID lpvReserved);
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D l l M a i n                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DllMain() is an entry point to the DLL which is called when processes and
%  threads are initialized and terminated, or upon calls to the Windows
%  LoadLibrary and FreeLibrary functions.
%
%  The function returns TRUE of it succeeds, or FALSE if initialization fails.
%
%  The format of the DllMain method is:
%
%    BOOL WINAPI DllMain(HINSTANCE handle,DWORD reason,LPVOID lpvReserved)
%
%  A description of each parameter follows:
%
%    o handle: handle to the DLL module
%
%    o reason: reason for calling function:
%
%      DLL_PROCESS_ATTACH - DLL is being loaded into virtual address
%                           space of current process.
%      DLL_THREAD_ATTACH - Indicates that the current process is
%                          creating a new thread.  Called under the
%                          context of the new thread.
%      DLL_THREAD_DETACH - Indicates that the thread is exiting.
%                          Called under the context of the exiting
%                          thread.
%      DLL_PROCESS_DETACH - Indicates that the DLL is being unloaded
%                           from the virtual address space of the
%                           current process.
%
%    o lpvReserved: Used for passing additional info during DLL_PROCESS_ATTACH
%                   and DLL_PROCESS_DETACH.
%
*/
#if defined(_DLL) && defined( ProvideDllMain )
BOOL WINAPI DllMain(HINSTANCE handle,DWORD reason,LPVOID lpvReserved)
{
  switch (reason)
  {
    case DLL_PROCESS_ATTACH:
    {
      char
        *module_path,
        *new_path,
        *path;

      ssize_t
        count;

      module_path=(char *) AcquireMagickMemory(MaxTextExtent*
        sizeof(*module_path));
      new_path=(char *) AcquireMagickMemory(16*MaxTextExtent*sizeof(*new_path));
      path=(char *) AcquireMagickMemory(16*MaxTextExtent*sizeof(*path));
      if ((module_path == (char *) NULL) || (new_path == (char *) NULL) ||
         (path == (char *) NULL))
        return(FALSE);
      count=(ssize_t) GetModuleFileName(handle,module_path,MaxTextExtent);
      if (count != 0)
        {
          for ( ; count > 0; count--)
            if (module_path[count] == '\\')
              {
                module_path[count+1]='\0';
                break;
              }
          InitializeMagick(module_path);
          count=GetEnvironmentVariable("PATH",path,16*MaxTextExtent);
          if ((count != 0) && (strstr(path,module_path) == (char *) NULL))
            {
              if ((strlen(module_path)+count+1) < (16*MaxTextExtent-1))
                {
                  (void) FormatMagickString(new_path,16*MaxTextExtent,
                    "%s;%s",module_path,path);
                  SetEnvironmentVariable("PATH",new_path);
                }
              path=(char *) RelinquishMagickMemory(path);
            }
        }
      path=(char *) RelinquishMagickMemory(path);
      new_path=(char *) RelinquishMagickMemory(new_path);
      module_path=(char *) RelinquishMagickMemory(module_path);
      break;
    }
    case DLL_PROCESS_DETACH:
    {
      DestroyMagick();
      break;
    }
    default:
      break;
  }
  return(TRUE);
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x i t                                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Exit() calls TerminateProcess for Win95.
%
%  The format of the exit method is:
%
%      int Exit(int status)
%
%  A description of each parameter follows:
%
%    o status: an integer value representing the status of the terminating
%      process.
%
%
*/
MagickExport int Exit(int status)
{
  if (IsWindows95())
    TerminateProcess(GetCurrentProcess(),(unsigned int) status);
  exit(status);
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s W i n d o w s 9 5                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsWindows95() returns true if the system is Windows 95.
%
%  The format of the IsWindows95 method is:
%
%      int IsWindows95()
%
%
*/
MagickExport int IsWindows95()
{
  OSVERSIONINFO
    version_info;

  version_info.dwOSVersionInfoSize=sizeof(version_info);
  if (GetVersionEx(&version_info) &&
      (version_info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS))
    return(1);
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T C l o s e D i r e c t o r y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTCloseDirectory() closes the named directory stream and frees the DIR
%  structure.
%
%  The format of the NTCloseDirectory method is:
%
%      int NTCloseDirectory(DIR *entry)
%
%  A description of each parameter follows:
%
%    o entry: Specifies a pointer to a DIR structure.
%
*/
MagickExport int NTCloseDirectory(DIR *entry)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(entry != (DIR *) NULL);
  FindClose(entry->hSearch);
  entry=(DIR *) RelinquishMagickMemory(entry);
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T C l o s e L i b r a r y                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTCloseLibrary() unloads the module associated with the passed handle.
%
%  The format of the NTCloseLibrary method is:
%
%      void NTCloseLibrary(void *handle)
%
%  A description of each parameter follows:
%
%    o handle: Specifies a handle to a previously loaded dynamic module.
%
*/
MagickExport int NTCloseLibrary(void *handle)
{
  if (IsWindows95())
    return(FreeLibrary(handle));
  return(!(FreeLibrary(handle)));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T C o n t r o l H a n d l e r                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTControlHandler() registers a control handler that is activated when, for
%  example, a ctrl-c is received.
%
%  The format of the NTControlHandler method is:
%
%      int NTControlHandler(void)
%
*/

static BOOL ControlHandler(DWORD type)
{
  AsynchronousDestroyMagickResources();
  return(FALSE);
}

MagickExport int NTControlHandler(void)
{
  return(SetConsoleCtrlHandler((PHANDLER_ROUTINE) ControlHandler,TRUE));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T E l a p s e d T i m e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTElapsedTime() returns the elapsed time (in seconds) since the last call to
%  StartTimer().
%
%  The format of the ElapsedTime method is:
%
%      double NTElapsedTime(void)
%
*/
MagickExport double NTElapsedTime(void)
{
  union
  {
    FILETIME
      filetime;

    __int64
      filetime64;
  } elapsed_time;

  SYSTEMTIME
    system_time;

  GetSystemTime(&system_time);
  SystemTimeToFileTime(&system_time,&elapsed_time.filetime);
  return((double) 1.0e-7*elapsed_time.filetime64);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   N T E r r o r H a n d l e r                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTErrorHandler() displays an error reason and then terminates the program.
%
%  The format of the NTErrorHandler method is:
%
%      void NTErrorHandler(const ExceptionType error,const char *reason,
%        const char *description)
%
%  A description of each parameter follows:
%
%    o error: Specifies the numeric error category.
%
%    o reason: Specifies the reason to display before terminating the
%      program.
%
%    o description: Specifies any description to the reason.
%
%
*/
MagickExport void NTErrorHandler(const ExceptionType error,const char *reason,
  const char *description)
{
  char
    buffer[3*MaxTextExtent],
    *message;

  if (reason == (char *) NULL)
    {
      DestroyMagick();
      exit(0);
    }
  message=GetExceptionMessage(errno);
  if ((description != (char *) NULL) && errno)
    (void) FormatMagickString(buffer,MaxTextExtent,"%s: %s (%s) [%s].\n",
      GetClientName(),reason,description,message);
  else
    if (description != (char *) NULL)
      (void) FormatMagickString(buffer,MaxTextExtent,"%s: %s (%s).\n",
        GetClientName(),reason,description);
    else
      if (errno != 0)
        (void) FormatMagickString(buffer,MaxTextExtent,"%s: %s [%s].\n",
          GetClientName(),reason,message);
      else
        (void) FormatMagickString(buffer,MaxTextExtent,"%s: %s.\n",
          GetClientName(),reason);
  message=(char *) RelinquishMagickMemory(message);
  (void) MessageBox(NULL,buffer,"ImageMagick Exception",MB_OK | MB_TASKMODAL |
    MB_SETFOREGROUND | MB_ICONEXCLAMATION);
  DestroyMagick();
  exit(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T E x i t L i b r a r y                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTExitLibrary() exits the dynamic module loading subsystem.
%
%  The format of the NTExitLibrary method is:
%
%      int NTExitLibrary(void)
%
*/
MagickExport int NTExitLibrary(void)
{
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T F i l e T r u n c a t e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTFileTruncate() truncates a file to a specified length.
%
%  The format of the NTFileTruncate method is:
%
%      int NTFileTruncate(int file,const MagickSizeType length)
%
%  A description of each parameter follows:
%
%    o file: The file.
%
%    o length: The file length.
%
*/
MagickExport int NTFileTruncate(int file,const MagickSizeType length)
{
  DWORD
    file_pointer;

  long
    file_handle,
    high,
    low;

  file_handle=_get_osfhandle(file);
  if (file_handle == -1L)
    return(-1);
  low=(long) (length & 0xffffffffUL);
  high=(long) ((((MagickOffsetType) length) >> 32) & 0xffffffffUL);
  file_pointer=SetFilePointer((HANDLE) file_handle,low,&high,FILE_BEGIN);
  if ((file_pointer == 0xFFFFFFFF) && (GetLastError() != NO_ERROR))
    return(-1);
  if (SetEndOfFile((HANDLE) file_handle) == 0)
    return(-1);
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T G e t E x e c u t i o n P a t h                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTGetExecutionPath() returns the execution path of a program.
%
%  The format of the GetExecutionPath method is:
%
%      MagickBooleanType NTGetExecutionPath(char *path)
%
%
*/
MagickExport MagickBooleanType NTGetExecutionPath(char *path)
{
  GetModuleFileName(0,path,MaxTextExtent);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T G e t L a s t E r r o r                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTGetLastError() returns the last error that occurred.
%
%  The format of the NTGetLastError method is:
%
%      char *NTGetLastError(void)
%
*/
char *NTGetLastError(void)
{
  char
    *reason;

  int
    status;

  LPVOID
    buffer;

  status=FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_FROM_SYSTEM,NULL,GetLastError(),
    MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),(LPTSTR) &buffer,0,NULL);
  if (!status)
    reason=AcquireString("An unknown error occurred");
  else
    {
      reason=AcquireString((const char *) buffer);
      LocalFree(buffer);
    }
  return(reason);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T G e t L i b r a r y E r r o r                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Lt_dlerror() returns a pointer to a string describing the last error
%  associated with a lt_dl method.  Note that this function is not thread
%  safe so it should only be used under the protection of a lock.
%
%  The format of the NTGetLibraryError method is:
%
%      const char *NTGetLibraryError(void)
%
*/
MagickExport const char *NTGetLibraryError(void)
{
  static char
    last_error[MaxTextExtent];

  char
    *error;

  *last_error='\0';
  error=NTGetLastError();
  if (error)
    (void) CopyMagickString(last_error,error,MaxTextExtent);
  error=(char *) RelinquishMagickMemory(error);
  return(last_error);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T G e t L i b r a r y S y m b o l                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTGetLibrarySymbol() retrieve the procedure address of the method
%  specified by the passed character string.
%
%  The format of the NTGetLibrarySymbol method is:
%
%      void *NTGetLibrarySymbol(void *handle,const char *name)
%
%  A description of each parameter follows:
%
%    o handle: Specifies a handle to the previously loaded dynamic module.
%
%    o name: Specifies the procedure entry point to be returned.
%
*/
void *NTGetLibrarySymbol(void *handle,const char *name)
{
  LPFNDLLFUNC1
    lpfnDllFunc1;

  lpfnDllFunc1=(LPFNDLLFUNC1) GetProcAddress(handle,name);
  if (!lpfnDllFunc1)
    return((void *) NULL);
  return((void *) lpfnDllFunc1);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T G e t M o d u l e P a t h                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTGetModulePath() returns the path of the specified module.
%
%  The format of the GetModulePath method is:
%
%      MagickBooleanType NTGetModulePath(const char *module,char *path)
%
%  A description of each parameter follows:
%
%    modith: The module name.
%
%    path: The module path is returned here.
%
*/
MagickExport MagickBooleanType NTGetModulePath(const char *module,char *path)
{
  char
    module_path[MaxTextExtent];

  HMODULE
    handle;

  long
    length;

  *path='\0';
  handle=GetModuleHandle(module);
  if (handle == (HMODULE) NULL)
    return(MagickFalse);
  length=GetModuleFileName(handle,module_path,MaxTextExtent);
  if (length != 0)
    GetPathComponent(module_path,HeadPath,path);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T G h o s t s c r i p t D L L                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTGhostscriptDLL() obtains the path to the latest Ghostscript DLL.  The
%  method returns MagickFalse if a value is not obtained.
%
%  The format of the NTGhostscriptDLL method is:
%
%      int NTGhostscriptDLL(char *path, int path_length)
%
%  A description of each parameter follows:
%
%    o path: Pointer to buffer in which to return result.
%
%    o path_length: Length of buffer
%
*/

#define GS_PRODUCT_AFPL "AFPL Ghostscript"
#define GS_PRODUCT_ALADDIN "Aladdin Ghostscript"
#define GS_PRODUCT_GNU "GNU Ghostscript"
#define GS_PRODUCT_GPL "GPL Ghostscript"
#define GS_MINIMUM_VERSION 550

/*
  Get Ghostscript versions for given product.
  Store results starting at pver + 1 + offset.
  Returns total number of versions in pver.
*/
static int NTGhostscriptProductVersions(int *pver,int offset,
  const char *gs_productfamily)
{
  HKEY
    hkey,
    hkeyroot;

  DWORD
    cbData;

  char
    key[MaxTextExtent],
    *p;

  int
    n = 0,
    ver;

  (void) FormatMagickString(key,MaxTextExtent,"Software\\%s",gs_productfamily);
  hkeyroot = HKEY_LOCAL_MACHINE;
  if (RegOpenKeyExA(hkeyroot,key,0,KEY_READ,&hkey) == ERROR_SUCCESS)
    {
      /*
        Now enumerate the keys.
      */
      cbData = sizeof(key) / sizeof(char);
      while (RegEnumKeyA(hkey,n,key,cbData) == ERROR_SUCCESS)
      {
        n++;
        ver = 0;
        p = key;
        while (*p && (*p!='.')) {
          ver = (ver * 10) + (*p - '0')*100;
          p++;
        }
        if (*p == '.')
          p++;
        if (*p)
          {
            ver+=10*(*p-'0');
            p++;
          }
        if (*p)
          ver+=(*p - '0');
        if ((n+offset) < pver[0])
          pver[n+offset]=ver;
      }
      RegCloseKey(hkey);
    }
  return(n+offset);
}

/* Query registry to find which versions of Ghostscript are installed.
 * Return version numbers in an integer array.
 * On entry, the first element in the array must be the array size
 * in elements.
 * If all is well, TRUE is returned.
 * On exit, the first element is set to the number of Ghostscript
 * versions installed, and subsequent elements to the version
 * numbers of Ghostscript.
 * e.g. on entry {5, 0, 0, 0, 0}, on exit {3, 550, 600, 596, 0}
 * Returned version numbers may not be sorted.
 *
 * If Ghostscript is not installed at all, return FALSE
 * and set pver[0] to 0.
 * If the array is not large enough, return FALSE
 * and set pver[0] to the number of Ghostscript versions installed.
 */

static int NTGhostscriptEnumerateVersions(int *pver)
{
  int
    n;

  assert(pver != (int *) NULL);
  n=NTGhostscriptProductVersions(pver,0,GS_PRODUCT_AFPL);
  n=NTGhostscriptProductVersions(pver,n,GS_PRODUCT_ALADDIN);
  n=NTGhostscriptProductVersions(pver,n,GS_PRODUCT_GNU);
  n=NTGhostscriptProductVersions(pver,n,GS_PRODUCT_GPL);
  if (n >= pver[0])
    {
      pver[0]=n;
      return(FALSE);  /* too small */
    }
  if (n == 0)
    {
      pver[0] = 0;
      return(FALSE);  /* not installed */
    }
  pver[0]=n;
  return(TRUE);
}

/*
 Get a named registry value.
 Key = hkeyroot\\key, named value = name.
 name, ptr, plen and return values are the same as in gp_getenv();
*/
static int NTGetRegistryValue(HKEY hkeyroot,const char *key,const char *name,
  char *ptr,int *plen)
{
  HKEY
    hkey;

  DWORD
    cbData,
    keytype;

  BYTE
    b,
    *bptr = (BYTE *)ptr;

  LONG
    rc;

  if (RegOpenKeyExA(hkeyroot,key,0,KEY_READ,&hkey) == ERROR_SUCCESS)
    {
      keytype = REG_SZ;
      cbData = *plen;
      if (bptr == (BYTE *)NULL)
        bptr=(&b);  /* Registry API won't return ERROR_MORE_DATA */
      /* if ptr is NULL */
      rc=RegQueryValueExA(hkey,(char *) name,0,&keytype,bptr,&cbData);
      RegCloseKey(hkey);
      if (rc == ERROR_SUCCESS)
        {
          *plen = cbData;
          return 0;  /* found environment variable and copied it */
        } else if (rc == ERROR_MORE_DATA) {
        /* buffer wasn't large enough */
          *plen = cbData;
          return -1;
        }
    }
  return(1);  /* not found */
}

static int NTGhostscriptGetProductString(int gs_revision,const char *name,
  char *ptr,int len,const char *gs_productfamily)
{
  /* If using Win32, look in the registry for a value with
   * the given name.  The registry value will be under the key
   * HKEY_CURRENT_USER\Software\AFPL Ghostscript\N.NN
   * or if that fails under the key
   * HKEY_LOCAL_MACHINE\Software\AFPL Ghostscript\N.NN
   * where "AFPL Ghostscript" is actually gs_productfamily
   * and N.NN is obtained from gs_revision.
   */

  char
    dotversion[MaxTextExtent],
    key[MaxTextExtent];

  int
    code,
    length;

  DWORD version = GetVersion();

  if (((HIWORD(version) & 0x8000) != 0) && ((HIWORD(version) & 0x4000) == 0))
    {
      /* Win32s */
      return FALSE;
    }
  (void) FormatMagickString(dotversion,MaxTextExtent,"%d.%02d",(int)
    (gs_revision/100),(int) (gs_revision % 100));
  (void) FormatMagickString(key,MaxTextExtent,"Software\\%s\\%s",
    gs_productfamily,dotversion);
  length = len;
  code = NTGetRegistryValue(HKEY_CURRENT_USER, key, name, ptr, &length);
  if ( code == 0 )
    return TRUE;  /* found it */
  length = len;
  code = NTGetRegistryValue(HKEY_LOCAL_MACHINE, key, name, ptr, &length);
  if ( code == 0 )
    return TRUE;  /* found it */
  return FALSE;
}

static int NTGhostscriptGetString(int gs_revision,const char *name,char *ptr,
  int len)
{
  if (NTGhostscriptGetProductString(gs_revision,name,ptr,len,GS_PRODUCT_AFPL))
    return(TRUE);
  if (NTGhostscriptGetProductString(gs_revision,name,ptr,len,GS_PRODUCT_ALADDIN))
    return(TRUE);
  if (NTGhostscriptGetProductString(gs_revision,name,ptr,len,GS_PRODUCT_GNU))
    return(TRUE);
  if (NTGhostscriptGetProductString(gs_revision,name,ptr,len,GS_PRODUCT_GPL))
    return(TRUE);
  return(FALSE);
}

static int NTGetLatestGhostscript( void )
{
  int
    count,
    i,
    gsver,
    *ver;

  DWORD version = GetVersion();
  if ( ((HIWORD(version) & 0x8000)!=0) && ((HIWORD(version) & 0x4000)==0) )
    return FALSE;  /* win32s */

  count = 1;
  NTGhostscriptEnumerateVersions(&count);
  if (count < 1)
    return FALSE;
  ver = (int *) AcquireMagickMemory((count+1)*sizeof(*ver));
  if (ver == (int *)NULL)
    return FALSE;
  ver[0]=count+1;
  if (!NTGhostscriptEnumerateVersions(ver))
    {
      ver=(int *) RelinquishMagickMemory(ver);
      return FALSE;
    }
  gsver = 0;
  for (i=1; i<=ver[0]; i++) {
    if (ver[i] > gsver)
      gsver = ver[i];
  }
  ver=(int *) RelinquishMagickMemory(ver);
  return(gsver);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T G h o s t s c r i p t D L L                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTGhostscriptDLL() obtains the path to the latest Ghostscript DLL.  The
%  method returns MagickFalse if a value is not obtained.
%
%  The format of the NTGhostscriptDLL method is:
%
%      int NTGhostscriptDLL( char *path, int path_length)
%
%  A description of each parameter follows:
%
%    o path: Pointer to path buffer to update
%
%    o path_length: Allocation size of path buffer.
%
*/
MagickExport int NTGhostscriptDLL(char *path, int path_length)
{
  int
    gsver;

  char
    buf[256];

  *path='\0';
  gsver = NTGetLatestGhostscript();
  if ((gsver == FALSE) || (gsver < GS_MINIMUM_VERSION))
    return FALSE;

  if (!NTGhostscriptGetString(gsver, "GS_DLL", buf, sizeof(buf)))
    return FALSE;

  (void) CopyMagickString(path,buf,path_length+1);
  return(TRUE);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T G h o s t s c r i p t D L L V e c t o r s                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTGhostscriptDLLVectors() returns a GhostscriptVectors structure containing
%  function vectors to invoke Ghostscript DLL functions. A null pointer is
%  returned if there is an error with loading the DLL or retrieving the
%  function vectors.
%
%  The format of the NTGhostscriptDLLVectors method is:
%
%      const GhostscriptVectors *NTGhostscriptDLLVectors(void)
%
*/
MagickExport const GhostscriptVectors *NTGhostscriptDLLVectors( void )
{
  if (NTGhostscriptLoadDLL())
    return(&gs_vectors);
  return((GhostscriptVectors*) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T G h o s t s c r i p t E X E                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTGhostscriptEXE() obtains the path to the latest Ghostscript executable.
%  The method returns MagickFalse if a value is not obtained.
%
%  The format of the NTGhostscriptEXE method is:
%
%      int NTGhostscriptEXE(char *path, int path_length)
%
%  A description of each parameter follows:
%
%    o path: Pointer to buffer in which to return result.
%
%    o path_length: Length of buffer
%
*/
MagickExport int NTGhostscriptEXE(char *path, int path_length)
{
  int
    gsver;

  char
    buf[256],
    *p;

  (void) CopyMagickString(path,"gswin32c.exe",path_length);
  gsver = NTGetLatestGhostscript();
  if ((gsver == FALSE) || (gsver < GS_MINIMUM_VERSION))
    return FALSE;

  if (!NTGhostscriptGetString(gsver, "GS_DLL", buf, sizeof(buf)))
    return FALSE;

  p = strrchr(buf, '\\');
  if (p) {
    p++;
    *p = 0;
    (void) CopyMagickString(p,"gswin32c.exe",sizeof(buf));
    (void) CopyMagickString(path,buf,path_length+1);
    return TRUE;
  }

  return FALSE;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T G h o s t s c r i p t F o n t s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTGhostscriptFonts() gets the path to the Ghostscript fonts.  The method
%  returns MagickFalse if an error occurs otherwise MagickTrue.
%
%  The format of the NTGhostscriptFonts method is:
%
%      int NTGhostscriptFonts(char *path, int path_length)
%
%  A description of each parameter follows:
%
%    o path: Pointer to buffer in which to return result.
%
%    o path_length: Length of buffer.
%
*/
MagickExport int NTGhostscriptFonts(char *path,int path_length)
{
  char
    buffer[MaxTextExtent],
    filename[MaxTextExtent];

  int
    version;

  register char
    *p,
    *q;

  *path='\0';
  version=NTGetLatestGhostscript();
  if ((version == FALSE) || (version < GS_MINIMUM_VERSION))
    return(FALSE);
  if (!NTGhostscriptGetString(version,"GS_LIB",buffer,MaxTextExtent))
    return(FALSE);
  for (p=buffer-1; p != (char *) NULL; p=strchr(p+1,DirectoryListSeparator))
  {
    (void) CopyMagickString(path,p+1,path_length+1);
    q=strchr(path,DirectoryListSeparator);
    if (q != (char *) NULL)
      *q='\0';
    FormatMagickString(filename,MaxTextExtent,"%s%sfonts.dir",path,
      DirectorySeparator);
    if (IsAccessible(filename) != MagickFalse)
      return(TRUE);
  }
  return(FALSE);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T G h o s t s c r i p t L o a d D L L                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTGhostscriptLoadDLL() attempts to load the Ghostscript DLL and returns
%  MagickTrue if it succeeds.
%
%  The format of the NTGhostscriptLoadDLL method is:
%
%      int NTGhostscriptLoadDLL(void)
%
*/
MagickExport int NTGhostscriptLoadDLL(void)
{
  char
    module_path[MaxTextExtent];

  if (gs_dll_handle != (void *) NULL)
    return(MagickTrue);
  if (NTGhostscriptDLL(module_path,sizeof(module_path)) == MagickFalse)
    return(MagickFalse);
  gs_dll_handle=NTOpenLibrary(module_path);
  if (gs_dll_handle == (void *) NULL)
    return(MagickFalse);
  (void) ResetMagickMemory((void *) &gs_vectors,0,sizeof(GhostscriptVectors));
  gs_vectors.exit=(int (MagickDLLCall *)
    (gs_main_instance *)) NTGetLibrarySymbol(gs_dll_handle,"gsapi_exit");
  gs_vectors.init_with_args=(int (MagickDLLCall *)
    (gs_main_instance *,int,char **))
    (NTGetLibrarySymbol(gs_dll_handle,"gsapi_init_with_args"));
  gs_vectors.new_instance=(int (MagickDLLCall *) (gs_main_instance **,void *))
    (NTGetLibrarySymbol(gs_dll_handle,"gsapi_new_instance"));
  gs_vectors.run_string=(int (MagickDLLCall *)
    (gs_main_instance *,const char *,int,int *))
    (NTGetLibrarySymbol(gs_dll_handle,"gsapi_run_string"));
  gs_vectors.delete_instance=(void (MagickDLLCall *)(gs_main_instance *))
    (NTGetLibrarySymbol(gs_dll_handle,"gsapi_delete_instance"));
  if ((gs_vectors.exit == NULL) ||
      (gs_vectors.init_with_args == NULL) ||
      (gs_vectors.new_instance == NULL) ||
      (gs_vectors.run_string == NULL) ||
      (gs_vectors.delete_instance == NULL))
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T G h o s t s c r i p t U n L o a d D L L                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTGhostscriptUnLoadDLL() unloads the Ghostscript DLL if it is loaded.
%
%  The format of the NTGhostscriptUnLoadDLL method is:
%
%      int NTGhostscriptUnLoadDLL(void)
%
*/
MagickExport int NTGhostscriptUnLoadDLL(void)
{
  if (gs_dll_handle == (void *) NULL)
    return(MagickFalse);
  NTCloseLibrary(gs_dll_handle);
  gs_dll_handle=(void *) NULL;
  (void) ResetMagickMemory((void *) &gs_vectors,0,sizeof(GhostscriptVectors));
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T I n i t i a l i z e L i b r a r y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTInitializeLibrary() initializes the dynamic module loading subsystem.
%
%  The format of the NTInitializeLibrary method is:
%
%      int NTInitializeLibrary(void)
%
*/
MagickExport int NTInitializeLibrary(void)
{
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  N T M a p M e m o r y                                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Mmap() emulates the Unix method of the same name.
%
%  The format of the NTMapMemory method is:
%
%    MagickExport void *NTMapMemory(char *address,size_t length,int protection,
%      int access,int file,MagickOffsetType offset)
%
*/
MagickExport void *NTMapMemory(char *address,size_t length,int protection,
  int flags,int file,MagickOffsetType offset)
{
  DWORD
    high,
    low,
    mode;

  HANDLE
    file_handle,
    file_map;

  void
    *map;

  mode=0;
  if ((protection & PROT_WRITE) != 0)
    {
      mode=PAGE_READWRITE;
      if ((flags & MAP_PRIVATE) != 0)
        mode=PAGE_WRITECOPY;
    }
  else
    if ((protection & PROT_READ) != 0)
      mode=PAGE_READONLY;
  file_handle=INVALID_HANDLE_VALUE;
  if ((file != -1) || ((flags & MAP_ANONYMOUS) == 0))
    file_handle=(HANDLE) _get_osfhandle(file);
  low=(DWORD) (length & 0xffffffffUL);
  high=(DWORD) ((((MagickOffsetType) length) >> 32) & 0xffffffffUL);
  file_map=CreateFileMapping(file_handle,0,mode,high,low,0);
  map=(void *) NULL;
  if (file_map != (HANDLE) NULL)
    {
      mode=0;
      if ((protection & PROT_WRITE) != 0)
        {
          mode=FILE_MAP_WRITE;
          if ((flags & MAP_PRIVATE) != 0)
            mode=FILE_MAP_COPY;
        }
      else
        if ((protection & PROT_READ) != 0)
          mode=FILE_MAP_READ;
      low=(DWORD) (offset & 0xffffffffUL);
      high=(DWORD) ((offset >> 32) & 0xffffffffUL);
      map=(void *) MapViewOfFile(file_map,mode,high,low,length);
      CloseHandle(file_map);
    }
  if (map == (void *) NULL)
    return((void *) MAP_FAILED);
  return((void *) ((char *) map));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T O p e n D i r e c t o r y                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTOpenDirectory() opens the directory named by filename and associates a
%  directory stream with it.
%
%  The format of the NTOpenDirectory method is:
%
%      DIR *NTOpenDirectory(const char *path)
%
%  A description of each parameter follows:
%
%    o entry: Specifies a pointer to a DIR structure.
%
*/
MagickExport DIR *NTOpenDirectory(const char *path)
{
  char
    file_specification[MaxTextExtent];

  DIR
    *entry;

  size_t
    length;

  assert(path != (const char *) NULL);
  length=CopyMagickString(file_specification,path,MaxTextExtent);
  if (length >= MaxTextExtent)
    return((DIR *) NULL);
  length=ConcatenateMagickString(file_specification,DirectorySeparator,
    MaxTextExtent);
  if (length >= MaxTextExtent)
    return((DIR *) NULL);
  entry=(DIR *) AcquireMagickMemory(sizeof(DIR));
  if (entry != (DIR *) NULL)
    {
      entry->firsttime=TRUE;
      entry->hSearch=FindFirstFile(file_specification,&entry->Win32FindData);
    }
  if (entry->hSearch == INVALID_HANDLE_VALUE)
    {
      length=ConcatenateMagickString(file_specification,"\\*.*",MaxTextExtent);
      if (length >= MaxTextExtent)
        {
          entry=(DIR *) RelinquishMagickMemory(entry);
          return((DIR *) NULL);
        }
      entry->hSearch=FindFirstFile(file_specification,&entry->Win32FindData);
      if (entry->hSearch == INVALID_HANDLE_VALUE)
        {
          entry=(DIR *) RelinquishMagickMemory(entry);
          return((DIR *) NULL);
        }
    }
  return(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T O p e n L i b r a r y                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTOpenLibrary() loads a dynamic module into memory and returns a handle that
%  can be used to access the various procedures in the module.
%
%  The format of the NTOpenLibrary method is:
%
%      void *NTOpenLibrary(const char *filename)
%
%  A description of each parameter follows:
%
%    o path: Specifies a pointer to string representing dynamic module that
%      is to be loaded.
%
*/

static const char *GetSearchPath( void )
{
#ifdef HasLTDL
  return(lt_dlgetsearchpath());
#else
  return(lt_slsearchpath);
#endif
}

MagickExport void *NTOpenLibrary(const char *filename)
{
#define MaxPathElements  31

  char
    buffer[MaxTextExtent];

  int
    index;

  register const char
    *p,
    *q;

  register int
    i;

  UINT
    mode;

  void
    *handle;

  mode=SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
  handle=(void *) LoadLibraryEx(filename,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
  if ((handle != (void *) NULL) || (GetSearchPath() == (char *) NULL))
    {
      SetErrorMode(mode);
      return(handle);
    }
  p=(char *) GetSearchPath();
  index=0;
  while (index < MaxPathElements)
  {
    q=strchr(p,DirectoryListSeparator);
    if (q == (char *) NULL)
      {
        (void) CopyMagickString(buffer,p,MaxTextExtent);
        (void) ConcatenateMagickString(buffer,"\\",MaxTextExtent);
        (void) ConcatenateMagickString(buffer,filename,MaxTextExtent);
        handle=(void *) LoadLibraryEx(buffer,NULL,
          LOAD_WITH_ALTERED_SEARCH_PATH);
        break;
      }
    i=q-p;
    (void) CopyMagickString(buffer,p,i+1);
    (void) ConcatenateMagickString(buffer,"\\",MaxTextExtent);
    (void) ConcatenateMagickString(buffer,filename,MaxTextExtent);
    handle=(void *) LoadLibraryEx(buffer,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
    if (handle != (void *) NULL)
      break;
    p=q+1;
  }
  SetErrorMode(mode);
  return(handle);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    N T R e a d D i r e c t o r y                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTReadDirectory() returns a pointer to a structure representing the
%  directory entry at the current position in the directory stream to which
%  entry refers.
%
%  The format of the NTReadDirectory
%
%      NTReadDirectory(entry)
%
%  A description of each parameter follows:
%
%    o entry: Specifies a pointer to a DIR structure.
%
*/
MagickExport struct dirent *NTReadDirectory(DIR *entry)
{
  int
    status;

  size_t
    length;

  if (entry == (DIR *) NULL)
    return((struct dirent *) NULL);
  if (!entry->firsttime)
    {
      status=FindNextFile(entry->hSearch,&entry->Win32FindData);
      if (status == 0)
        return((struct dirent *) NULL);
    }
  length=CopyMagickString(entry->file_info.d_name,
    entry->Win32FindData.cFileName,sizeof(entry->file_info.d_name));
  if (length >= sizeof(entry->file_info.d_name))
    return((struct dirent *) NULL);
  entry->firsttime=FALSE;
  entry->file_info.d_namlen=(int) strlen(entry->file_info.d_name);
  return(&entry->file_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T R e g i s t r y K e y L o o k u p                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTRegistryKeyLookup() returns ImageMagick installation path settings
%  stored in the Windows Registry.  Path settings are specific to the
%  installed ImageMagick version so that multiple Image Magick installations
%  may coexist.
%
%  Values are stored in the registry under a base path path similar to
%  "HKEY_LOCAL_MACHINE/SOFTWARE\ImageMagick\5.5.7\Q:16". The provided subkey
%  is appended to this base path to form the full key.
%
%  The format of the NTRegistryKeyLookup method is:
%
%      char *NTRegistryKeyLookup(const char *subkey)
%
%  A description of each parameter follows:
%
%    o subkey: Specifies a string that identifies the registry object.
%      Currently supported sub-keys include: "BinPath", "ConfigurePath",
%      "LibPath", "CoderModulesPath", "FilterModulesPath", "SharePath".
%
*/
MagickExport char *NTRegistryKeyLookup(const char *subkey)
{
  char
    package_key[MaxTextExtent],
    *value;

  DWORD
    size,
    type;

  HKEY
    registry_key;

  LONG
    status;

  /*
    Look-up base key.
  */
  (void) FormatMagickString(package_key,MaxTextExtent,"SOFTWARE\\%s\\%s\\Q:%d",
    MagickPackageName,MagickLibVersionText,QuantumDepth);
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),"%s",package_key);
  registry_key=(HKEY) INVALID_HANDLE_VALUE;
  status=RegOpenKeyExA(HKEY_LOCAL_MACHINE,package_key,0,KEY_READ,&registry_key);
  if (status != ERROR_SUCCESS)
    {
      registry_key=(HKEY) INVALID_HANDLE_VALUE;
      return((char *) NULL);
    }
  /*
    Look-up sub key.
  */
  size=32;
  value=(char *) AcquireMagickMemory(size);
  if (value == (char *) NULL)
    {
      RegCloseKey(registry_key);
      return((char *) NULL);
    }
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),"%s",subkey);
  status=RegQueryValueExA(registry_key,subkey,0,&type,value,&size);
  if ((status == ERROR_MORE_DATA) && (type == REG_SZ))
    {
      value=(char *) ResizeMagickMemory(value,size);
      if (value == (char *) NULL)
        {
          RegCloseKey(registry_key);
          return((char *) NULL);
        }
      status=RegQueryValueExA(registry_key,subkey,0,&type,value,&size);
    }
  RegCloseKey(registry_key);
  if ((type != REG_SZ) || (status != ERROR_SUCCESS))
    value=(char *) RelinquishMagickMemory(value);
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T R e p o r t E v e n t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTReportEvent() reports an event.
%
%  The format of the NTReportEvent method is:
%
%      MagickBooleanType NTReportEvent(const char *event,
%        const MagickBooleanType error)
%
%  A description of each parameter follows:
%
%    o event: The event.
%
%    o error: MagickTrue the event is an error.
%
*/
MagickExport MagickBooleanType NTReportEvent(const char *event,
  const MagickBooleanType error)
{
  const char
    *events[1];

  HANDLE
    handle;

  WORD
    type;

  handle=RegisterEventSource(NULL,"ImageMagick");
  if (handle == NULL)
    return(MagickFalse);
  events[0]=event;
  type=error ? EVENTLOG_ERROR_TYPE : EVENTLOG_WARNING_TYPE;
  ReportEvent(handle,type,0,0,NULL,1,0,events,NULL);
  DeregisterEventSource(handle);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T R e s o u r c e T o B l o b                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTResourceToBlob() returns a blob containing the contents of the resource
%  in the current executable specified by the id parameter. This is currently
%  used to retrieve MGK files tha have been embedded into the various command
%  line utilities.
%
%  The format of the NTResourceToBlob method is:
%
%      unsigned char *NTResourceToBlob(const char *id)
%
%  A description of each parameter follows:
%
%    o id: Specifies a string that identifies the resource.
%
*/
MagickExport unsigned char *NTResourceToBlob(const char *id)
{
  char
    path[MaxTextExtent];

  DWORD
    length;

  HGLOBAL
    global;

  HMODULE
    handle;

  HRSRC
    resource;

  unsigned char
    *blob,
    *value;

  assert(id != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",id);
  (void) FormatMagickString(path,MaxTextExtent,"%s%s%s",GetClientPath(),
    DirectorySeparator,GetClientName());
  if (IsAccessible(path) != MagickFalse)
    handle=GetModuleHandle(path);
  else
    handle=GetModuleHandle(0);
  if (!handle)
    return((char *) NULL);
  resource=FindResource(handle,id,"IMAGEMAGICK");
  if (!resource)
    return((char *) NULL);
  global=LoadResource(handle,resource);
  if (!global)
    return((char *) NULL);
  length=SizeofResource(handle,resource);
  value=(unsigned char *) LockResource(global);
  if (!value)
    {
      FreeResource(global);
      return((char *) NULL);
    }
  blob=(unsigned char *) AcquireMagickMemory(length+MaxTextExtent);
  if (blob != (unsigned char *) NULL)
    {
      (void) CopyMagickMemory(blob,value,length);
      blob[length]='\0';
    }
  UnlockResource(global);
  FreeResource(global);
  return(blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T S e e k D i r e c t o r y                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTSeekDirectory() sets the position of the next NTReadDirectory() operation
%  on the directory stream.
%
%  The format of the NTSeekDirectory method is:
%
%      void NTSeekDirectory(DIR *entry,long position)
%
%  A description of each parameter follows:
%
%    o entry: Specifies a pointer to a DIR structure.
%
%    o position: specifies the position associated with the directory
%      stream.
%
*/
MagickExport void NTSeekDirectory(DIR *entry,long position)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(entry != (DIR *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T S e t S e a r c h P a t h                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTSetSearchPath() sets the current locations that the subsystem should
%  look at to find dynamically loadable modules.
%
%  The format of the NTSetSearchPath method is:
%
%      int NTSetSearchPath(const char *path)
%
%  A description of each parameter follows:
%
%    o path: Specifies a pointer to string representing the search path
%      for DLL's that can be dynamically loaded.
%
*/
MagickExport int NTSetSearchPath(const char *path)
{
#ifdef HasLTDL
  lt_dlsetsearchpath(path);
#else /*!HasLTDL*/
  if (lt_slsearchpath != (char *) NULL)
    lt_slsearchpath=(char *) RelinquishMagickMemory(lt_slsearchpath);
  if (path != (char *) NULL)
    lt_slsearchpath=AcquireString(path);
#endif /*HasLTDL*/
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  N T S y n c M e m o r y                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTSyncMemory() emulates the Unix method of the same name.
%
%  The format of the NTSyncMemory method is:
%
%      int NTSyncMemory(void *address,size_t length,int flags)
%
%  A description of each parameter follows:
%
%    o address: The address of the binary large object.
%
%    o length: The length of the binary large object.
%
%    o flags: Option flags (ignored for Windows).
%
*/
MagickExport int NTSyncMemory(void *address,size_t length,int flags)
{
  if (FlushViewOfFile(address,length) == MagickFalse)
    return(-1);
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T S y s t e m C o m m a n d                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTSystemCommand() executes the specified command and waits until it
%  terminates.  The returned value is the exit status of the command.
%
%  The format of the NTSystemComman method is:
%
%      int NTSystemComman(const char *command)
%
%  A description of each parameter follows:
%
%    o command: This string is the command to execute.
%
*/
MagickExport int NTSystemCommand(const char *command)
{
  char
    local_command[MaxTextExtent];

  DWORD
    child_status;

  int
    status;

  MagickBooleanType
    background_process;

  PROCESS_INFORMATION
    process_info;

  STARTUPINFO
    startup_info;

  if (command == (char *) NULL)
    return(-1);
  GetStartupInfo(&startup_info);
  startup_info.dwFlags=STARTF_USESHOWWINDOW;
  startup_info.wShowWindow=SW_SHOWMINNOACTIVE;
  (void) CopyMagickString(local_command,command,MaxTextExtent);
  background_process=command[strlen(command)-1] == '&';
  if (background_process)
    local_command[strlen(command)-1]='\0';
  if (command[strlen(command)-1] == '|')
     local_command[strlen(command)-1]='\0';
   else
     startup_info.wShowWindow=SW_SHOWDEFAULT;
  status=CreateProcess((LPCTSTR) NULL,local_command,
    (LPSECURITY_ATTRIBUTES) NULL,(LPSECURITY_ATTRIBUTES) NULL,(BOOL) FALSE,
    (DWORD) NORMAL_PRIORITY_CLASS,(LPVOID) NULL,(LPCSTR) NULL,&startup_info,
    &process_info);
  if (status == 0)
    return(-1);
  if (background_process)
    return(status == 0);
  status=WaitForSingleObject(process_info.hProcess,INFINITE);
  if (status != WAIT_OBJECT_0)
    return (status);
  status=GetExitCodeProcess(process_info.hProcess,&child_status);
  if (status == 0)
    return(-1);
  CloseHandle(process_info.hProcess);
  CloseHandle(process_info.hThread);
  return((int) child_status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T S y s t e m C o n i f i g u r a t i o n                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTSystemConfiguration() provides a way for the application to determine
%  values for system limits or options at runtime.
%
%  The format of the exit method is:
%
%      long NTSystemConfiguration(int name)
%
%  A description of each parameter follows:
%
%    o name: _SC_PAGE_SIZE or _SC_PHYS_PAGES.
%
*/
MagickExport long NTSystemConfiguration(int name)
{
  switch (name)
  {
    case _SC_PAGESIZE:
    {
      SYSTEM_INFO
        system_info;

      GetSystemInfo(&system_info);
      return(system_info.dwPageSize);
    }
    case _SC_PHYS_PAGES:
    {
      HMODULE
        handle;

      LPFNDLLFUNC2
        module;

      NTMEMORYSTATUSEX
        status;

      SYSTEM_INFO
        system_info;

      handle=GetModuleHandle("kernel32.dll");
      if (handle == (HMODULE) NULL)
        return(0L);
      GetSystemInfo(&system_info);
      module=(LPFNDLLFUNC2) NTGetLibrarySymbol(handle,"GlobalMemoryStatusEx");
      if (module == (LPFNDLLFUNC2) NULL)
        {
          MEMORYSTATUS
            status;

          GlobalMemoryStatus(&status);
          return((long) status.dwTotalPhys/system_info.dwPageSize);
        }
      status.dwLength=sizeof(status);
      if (module(&status) == 0)
        return(0L);
      return((long) status.ullTotalPhys/system_info.dwPageSize);
    }
    case _SC_OPEN_MAX:
      return(2048);
    default:
      break;
  }
  return(-1);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T T e l l D i r e c t o r y                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTTellDirectory() returns the current location associated with the named
%  directory stream.
%
%  The format of the NTTellDirectory method is:
%
%      long NTTellDirectory(DIR *entry)
%
%  A description of each parameter follows:
%
%    o entry: Specifies a pointer to a DIR structure.
%
*/
MagickExport long NTTellDirectory(DIR *entry)
{
  assert(entry != (DIR *) NULL);
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  N T U n m a p M e m o r y                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTUnmapMemory() emulates the Unix munmap method.
%
%  The format of the NTUnmapMemory method is:
%
%      int NTUnmapMemory(void *map,size_t length)
%
%  A description of each parameter follows:
%
%    o map: The address of the binary large object.
%
%    o length: The length of the binary large object.
%
*/
MagickExport int NTUnmapMemory(void *map,size_t length)
{
  if (UnmapViewOfFile(map) == 0)
    return(-1);
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T U s e r T i m e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTUserTime() returns the total time the process has been scheduled (e.g.
%  seconds) since the last call to StartTimer().
%
%  The format of the UserTime method is:
%
%      double NTUserTime(void)
%
*/
MagickExport double NTUserTime(void)
{
  DWORD
    status;

  FILETIME
    create_time,
    exit_time;

  OSVERSIONINFO
    OsVersionInfo;

  union
  {
    FILETIME
      filetime;

    __int64
      filetime64;
  } kernel_time;

  union
  {
    FILETIME
      filetime;

    __int64
      filetime64;
  } user_time;

  OsVersionInfo.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
  GetVersionEx(&OsVersionInfo);
  if (OsVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT)
    return(NTElapsedTime());
  status=GetProcessTimes(GetCurrentProcess(),&create_time,&exit_time,
    &kernel_time.filetime,&user_time.filetime);
  if (status != TRUE)
    return(0.0);
  return((double) 1.0e-7*(kernel_time.filetime64+user_time.filetime64));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   N T W a r n i n g H a n d l e r                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NTWarningHandler() displays a warning reason.
%
%  The format of the NTWarningHandler method is:
%
%      void NTWarningHandler(const ExceptionType warning,const char *reason,
%        const char *description)
%
%  A description of each parameter follows:
%
%    o warning: Specifies the numeric warning category.
%
%    o reason: Specifies the reason to display before terminating the
%      program.
%
%    o description: Specifies any description to the reason.
%
*/
MagickExport void NTWarningHandler(const ExceptionType warning,
  const char *reason,const char *description)
{
  char
    buffer[2*MaxTextExtent];

  if (reason == (char *) NULL)
    return;
  if (description == (char *) NULL)
    (void) FormatMagickString(buffer,MaxTextExtent,"%s: %s.\n",GetClientName(),
      reason);
  else
    (void) FormatMagickString(buffer,MaxTextExtent,"%s: %s (%s).\n",
      GetClientName(),reason,description);
  (void) MessageBox(NULL,buffer,"ImageMagick Warning",MB_OK | MB_TASKMODAL |
    MB_SETFOREGROUND | MB_ICONINFORMATION);
}
#endif
