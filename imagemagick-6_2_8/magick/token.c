/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                    TTTTT   OOO   K   K  EEEEE  N   N                        %
%                      T    O   O  K  K   E      NN  N                        %
%                      T    O   O  KKK    EEE    N N N                        %
%                      T    O   O  K  K   E      N  NN                        %
%                      T     OOO   K   K  EEEEE  N   N                        %
%                                                                             %
%                                                                             %
%                        ImageMagick Token Methods                            %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                              January 1993                                   %
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
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/memory_.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k T o k e n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickToken() gets a token from the token stream.  A token is defined as a
%  sequence of characters delimited by whitespace (e.g. clip-path), a sequence
%  delimited with quotes (.e.g "Quote me"), or a sequence enclosed in
%  parenthesis (e.g. rgb(0,0,0)).
%
%  The format of the GetMagickToken method is:
%
%      void GetMagickToken(const char *start,char **end,char *token)
%
%  A description of each parameter follows:
%
%    o start: the start of the token sequence.
%
%    o end: point to the end of the token sequence.
%
%    o token: copy the token to this buffer.
%
%
*/
MagickExport void GetMagickToken(const char *start,char **end,char *token)
{
  register const char
    *p;

  register long
    i;

  i=0;
  for (p=start; *p != '\0'; )
  {
    while ((isspace((int) ((unsigned char) *p)) != 0) && (*p != '\0'))
      p++;
    if (*p == '\0')
      break;
    switch (*p)
    {
      case '"':
      case '\'':
      case '`':
      case '{':
      {
        register char
          escape;

        switch (*p)
        {
          case '"': escape='"'; break;
          case '\'': escape='\''; break;
          case '`': escape='\''; break;
          case '{': escape='}'; break;
          default: escape=(*p); break;
        }
        for (p++; *p != '\0'; p++)
        {
          if ((*p == '\\') && ((*(p+1) == escape) || (*(p+1) == '\\')))
            p++;
          else
            if (*p == escape)
              {
                p++;
                break;
              }
          token[i++]=(*p);
        }
        break;
      }
      case '/':
      {
        token[i++]=(*p++);
        if ((*p == '>') || (*p == '/'))
          token[i++]=(*p++);
        break;
      }
      default:
      {
        char
          *q;

        (void) strtod(p,&q);
        if (p != q)
          {
            for ( ; p < q; p++)
              token[i++]=(*p);
            if (*p == '%')
              token[i++]=(*p++);
            break;
          }
        if ((isalpha((int) ((unsigned char) *p)) == 0) &&
            (*p != *DirectorySeparator) && (*p != '#') && (*p != '<'))
          {
            token[i++]=(*p++);
            break;
          }
        for ( ; *p != '\0'; p++)
        {
          if (((isspace((int) ((unsigned char) *p)) != 0) || (*p == '=') ||
              (*p == ',') || (*p == ':')) && (*(p-1) != '\\'))
            break;
          if ((i > 0) && (*p == '<'))
            break;
          token[i++]=(*p);
          if (*p == '>')
            break;
          if (*p == '(')
            for (p++; *p != '\0'; p++)
            {
              token[i++]=(*p);
              if ((*p == ')') && (*(p-1) != '\\'))
                break;
            }
        }
        break;
      }
    }
    break;
  }
  token[i]='\0';
  if (LocaleNCompare(token,"url(#",5) == 0)
    {
      i=(long) strlen(token);
      (void) CopyMagickString(token,token+5,MaxTextExtent);
      token[i-6]='\0';
    }
  if (LocaleNCompare(token,"url(",4) == 0)
    {
      i=(long) strlen(token);
      (void) CopyMagickString(token,token+4,MaxTextExtent);
      token[i-5]='\0';
    }
  while (isspace((int) ((unsigned char) *p)) != 0)
    p++;
  if (end != (char **) NULL)
    *end=(char *) p;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G l o b E x p r e s s i o n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GlobExpression() returns MagickTrue if the expression matches the pattern.
%
%  The format of the GlobExpression function is:
%
%      MagickBooleanType GlobExpression(const char *expression,
%        const char *pattern)
%
%  A description of each parameter follows:
%
%    o expression: Specifies a pointer to a text string containing a file name.
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%
*/
MagickExport MagickBooleanType GlobExpression(const char *expression,
  const char *pattern)
{
  MagickBooleanType
    done;

  register const char
    *p;

  /*
    Return on empty pattern or '*'.
  */
  if (pattern == (char *) NULL)
    return(MagickTrue);
  if (*pattern == '\0')
    return(MagickTrue);
  if (LocaleCompare(pattern,"*") == 0)
    return(MagickTrue);
  p=pattern+strlen(pattern)-1;
  if ((*p == ']') && (strchr(pattern,'[') != (char *) NULL))
    {
      ExceptionInfo
        exception;

      ImageInfo
        *image_info;

      /*
        Determine if pattern is a scene, i.e. img0001.pcd[2].
      */
      image_info=CloneImageInfo((ImageInfo *) NULL);
      (void) CopyMagickString(image_info->filename,pattern,MaxTextExtent);
      GetExceptionInfo(&exception);
      (void) SetImageInfo(image_info,MagickTrue,&exception);
      (void) DestroyExceptionInfo(&exception);
      if (LocaleCompare(image_info->filename,pattern) != 0)
        {
          image_info=DestroyImageInfo(image_info);
          return(MagickFalse);
        }
      image_info=DestroyImageInfo(image_info);
    }
  /*
    Evaluate glob expression.
  */
  done=MagickFalse;
  while ((*pattern != '\0') && (done == MagickFalse))
  {
    if (*expression == '\0')
      if ((*pattern != '{') && (*pattern != '*'))
        break;
    switch (*pattern)
    {
      case '\\':
      {
        pattern++;
        if (*pattern != '\0')
          pattern++;
        break;
      }
      case '*':
      {
        MagickBooleanType
          status;

        pattern++;
        status=MagickFalse;
        while ((*expression != '\0') && (status == MagickFalse))
          status=GlobExpression(expression++,pattern);
        if (status != MagickFalse)
          {
            while (*expression != '\0')
              expression++;
            while (*pattern != '\0')
              pattern++;
          }
        break;
      }
      case '[':
      {
        char
          c;

        pattern++;
        for ( ; ; )
        {
          if ((*pattern == '\0') || (*pattern == ']'))
            {
              done=MagickTrue;
              break;
            }
          if (*pattern == '\\')
            {
              pattern++;
              if (*pattern == '\0')
                {
                  done=MagickTrue;
                  break;
                }
             }
          if (*(pattern+1) == '-')
            {
              c=(*pattern);
              pattern+=2;
              if (*pattern == ']')
                {
                  done=MagickTrue;
                  break;
                }
              if (*pattern == '\\')
                {
                  pattern++;
                  if (*pattern == '\0')
                    {
                      done=MagickTrue;
                      break;
                    }
                }
              if ((*expression < c) || (*expression > *pattern))
                {
                  pattern++;
                  continue;
                }
            }
          else
            if (*pattern != *expression)
              {
                pattern++;
                continue;
              }
          pattern++;
          while ((*pattern != ']') && (*pattern != '\0'))
          {
            if ((*pattern == '\\') && (*(pattern+1) != '\0'))
              pattern++;
            pattern++;
          }
          if (*pattern != '\0')
            {
              pattern++;
              expression++;
            }
          break;
        }
        break;
      }
      case '?':
      {
        pattern++;
        expression++;
        break;
      }
      case '{':
      {
        MagickBooleanType
          match;

        register const char
          *p;

        pattern++;
        while ((*pattern != '}') && (*pattern != '\0'))
        {
          p=expression;
          match=MagickTrue;
          while ((*p != '\0') && (*pattern != '\0') &&
                 (*pattern != ',') && (*pattern != '}') &&
                 (match != MagickFalse))
          {
            if (*pattern == '\\')
              pattern++;
            match=(*pattern == *p) ? MagickTrue : MagickFalse;
            p++;
            pattern++;
          }
          if (*pattern == '\0')
            {
              match=MagickFalse;
              done=MagickTrue;
              break;
            }
          else
            if (match != MagickFalse)
              {
                expression=p;
                while ((*pattern != '}') && (*pattern != '\0'))
                {
                  pattern++;
                  if (*pattern == '\\')
                    {
                      pattern++;
                      if (*pattern == '}')
                        pattern++;
                    }
                }
              }
            else
              {
                while ((*pattern != '}') && (*pattern != ',') &&
                       (*pattern != '\0'))
                {
                  pattern++;
                  if (*pattern == '\\')
                    {
                      pattern++;
                      if ((*pattern == '}') || (*pattern == ','))
                        pattern++;
                    }
                }
              }
            if (*pattern != '\0')
              pattern++;
          }
        break;
      }
      default:
      {
        if (*expression != *pattern)
          done=MagickTrue;
        else
          {
            expression++;
            pattern++;
          }
      }
    }
  }
  while (*pattern == '*')
    pattern++;
  return((*expression == '\0') && (*pattern == '\0') ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     I s G l o b                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsGlob() returns MagickTrue if the path specification contains a globbing
%  pattern.
%
%  The format of the IsGlob method is:
%
%      MagickBooleanType IsGlob(const char *geometry)
%
%  A description of each parameter follows:
%
%    o path: The path.
%
*/
MagickExport MagickBooleanType IsGlob(const char *path)
{
  MagickBooleanType
    status;

  status=(strchr(path,'*') != (char *) NULL) ||
    (strchr(path,'?') != (char *) NULL) ||
    (strchr(path,'{') != (char *) NULL) ||
    (strchr(path,'}') != (char *) NULL) ||
    (strchr(path,'[') != (char *) NULL) ||
    (strchr(path,']') != (char *) NULL) ? MagickTrue : MagickFalse;
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T o k e n i z e r                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Tokenizer() is a generalized, finite state token parser.  It extracts tokens
%  one at a time from a string of characters.  The characters used for white
%  space, for break characters, and for quotes can be specified.  Also,
%  characters in the string can be preceded by a specifiable escape character
%  which removes any special meaning the character may have.
%
%  Here is some terminology:
%
%    o token: A single unit of information in the form of a group of
%      characters.
%
%    o white space: Apace that gets ignored (except within quotes or when
%      escaped), like blanks and tabs. in addition, white space terminates a
%      non-quoted token.
%
%    o break set: One or more characters that separates non-quoted tokens.
%      Commas are a common break character. The usage of break characters to
%      signal the end of a token is the same as that of white space, except
%      multiple break characters with nothing or only white space between
%      generate a null token for each two break characters together.
%
%      For example, if blank is set to be the white space and comma is set to
%      be the break character, the line
%
%        A, B, C ,  , DEF
%
%        ... consists of 5 tokens:
%
%        1)  "A"
%        2)  "B"
%        3)  "C"
%        4)  "" (the null string)
%        5)  "DEF"
%
%    o Quote character: A character that, when surrounding a group of other
%      characters, causes the group of characters to be treated as a single
%      token, no matter how many white spaces or break characters exist in
%      the group. Also, a token always terminates after the closing quote.
%      For example, if ' is the quote character, blank is white space, and
%      comma is the break character, the following string
%
%        A, ' B, CD'EF GHI
%
%        ... consists of 4 tokens:
%
%        1)  "A"
%        2)  " B, CD" (note the blanks & comma)
%        3)  "EF"
%        4)  "GHI"
%
%      The quote characters themselves do not appear in the resultant
%      tokens.  The double quotes are delimiters i use here for
%      documentation purposes only.
%
%    o Escape character: A character which itself is ignored but which
%      causes the next character to be used as is.  ^ and \ are often used
%      as escape characters. An escape in the last position of the string
%      gets treated as a "normal" (i.e., non-quote, non-white, non-break,
%      and non-escape) character. For example, assume white space, break
%      character, and quote are the same as in the above examples, and
%      further, assume that ^ is the escape character. Then, in the string
%
%        ABC, ' DEF ^' GH' I ^ J K^ L ^
%
%        ... there are 7 tokens:
%
%        1)  "ABC"
%        2)  " DEF ' GH"
%        3)  "I"
%        4)  " "     (a lone blank)
%        5)  "J"
%        6)  "K L"
%        7)  "^"     (passed as is at end of line)
%
%  The format of the Tokenizer method is:
%
%      int Tokenizer(TokenInfo *token_info,const unsigned flag,char *token,
%        const size_t max_token_length,const char *line,const char *white,
%        const char *break_set,const char *quote,const char escape,
%        char *breaker,int *next,char *quoted)
%
%  A description of each parameter follows:
%
%    o flag: right now, only the low order 3 bits are used.
%
%        1 => convert non-quoted tokens to upper case
%        2 => convert non-quoted tokens to lower case
%        0 => do not convert non-quoted tokens
%
%    o token: a character string containing the returned next token
%
%    o max_token_length: the maximum size of "token".  Characters beyond
%      "max_token_length" are truncated.
%
%    o string: the string to be parsed.
%
%    o white: a string of the valid white spaces.  example:
%
%        char whitesp[]={" \t"};
%
%      blank and tab will be valid white space.
%
%    o break: a string of the valid break characters. example:
%
%        char breakch[]={";,"};
%
%      semicolon and comma will be valid break characters.
%
%    o quote: a string of the valid quote characters. An example would be
%
%        char whitesp[]={"'\"");
%
%      (this causes single and double quotes to be valid) Note that a
%      token starting with one of these characters needs the same quote
%      character to terminate it.
%
%      for example:
%
%        "ABC '
%
%      is unterminated, but
%
%        "DEF" and 'GHI'
%
%      are properly terminated.  Note that different quote characters
%      can appear on the same line; only for a given token do the quote
%      characters have to be the same.
%
%    o escape: the escape character (NOT a string ... only one
%      allowed). Use zero if none is desired.
%
%    o breaker: the break character used to terminate the current
%      token.  If the token was quoted, this will be the quote used.  If
%      the token is the last one on the line, this will be zero.
%
%    o next: this variable points to the first character of the
%      next token.  it gets reset by "tokenizer" as it steps through the
%      string.  Set it to 0 upon initialization, and leave it alone
%      after that.  You can change it if you want to jump around in the
%      string or re-parse from the beginning, but be careful.
%
%    o quoted: set to True if the token was quoted and MagickFalse
%      if not.  You may need this information (for example:  in C, a
%      string with quotes around it is a character string, while one
%      without is an identifier).
%
%    o result: 0 if we haven't reached EOS (end of string), and 1
%      if we have.
%
*/

#define IN_WHITE 0
#define IN_TOKEN 1
#define IN_QUOTE 2
#define IN_OZONE 3

static long sindex(int c,const char *string)
{
  register const char
    *p;

  for (p=string; *p != '\0'; p++)
    if (c == (int) (*p))
      return(p-string);
  return(-1);
}

static void StoreToken(TokenInfo *token_info,char *string,
  size_t max_token_length,int c)
{
  register long
    i;

  if ((token_info->offset < 0) ||
      ((size_t) token_info->offset >= (max_token_length-1)))
    return;
  i=token_info->offset++;
  string[i]=(char) c;
  if (token_info->state == IN_QUOTE)
    return;
  switch (token_info->flag & 0x03)
  {
    case 1:
    {
      string[i]=(char) toupper(c);
      break;
    }
    case 2:
    {
      string[i]=(char) tolower(c);
      break;
    }
    default:
      break;
  }
}

MagickExport int Tokenizer(TokenInfo *token_info,const unsigned flag,
  char *token,const size_t max_token_length,const char *line,const char *white,
	const char *break_set,const char *quote,const char escape,char *breaker,
  int *next,char *quoted)
{
  int
    c;

  register long
    i;

  *breaker='\0';
  *quoted='\0';
  if (line[*next] == '\0')
    return(1);
  token_info->state=IN_WHITE;
  token_info->quote=(char) MagickFalse;
  token_info->flag=flag;
  for (token_info->offset=0; (int) line[*next] != 0; (*next)++)
  {
    c=(int) line[*next];
    i=sindex(c,break_set);
    if (i >= 0)
      {
        switch (token_info->state)
        {
          case IN_WHITE:
          case IN_TOKEN:
          case IN_OZONE:
          {
            (*next)++;
            *breaker=break_set[i];
            token[token_info->offset]='\0';
            return(0);
          }
          case IN_QUOTE:
          {
            StoreToken(token_info,token,max_token_length,c);
            break;
          }
        }
        continue;
      }
    i=sindex(c,quote);
    if (i >= 0)
      {
        switch (token_info->state)
        {
          case IN_WHITE:
          {
            token_info->state=IN_QUOTE;
            token_info->quote=quote[i];
            *quoted=(char) MagickTrue;
            break;
          }
          case IN_QUOTE:
          {
            if (quote[i] != token_info->quote)
              StoreToken(token_info,token,max_token_length,c);
            else
              {
                token_info->state=IN_OZONE;
                token_info->quote='\0';
              }
            break;
          }
          case IN_TOKEN:
          case IN_OZONE:
          {
            *breaker=(char) c;
            token[token_info->offset]='\0';
            return(0);
          }
        }
        continue;
      }
    i=sindex(c,white);
    if (i >= 0)
      {
        switch (token_info->state)
        {
          case IN_WHITE:
          case IN_OZONE:
            break;
          case IN_TOKEN:
          {
            token_info->state=IN_OZONE;
            break;
          }
          case IN_QUOTE:
          {
            StoreToken(token_info,token,max_token_length,c);
            break;
          }
        }
        continue;
      }
    if (c == (int) escape)
      {
        if (line[(*next)+1] == '\0')
          {
            *breaker='\0';
            StoreToken(token_info,token,max_token_length,c);
            (*next)++;
            token[token_info->offset]='\0';
            return(0);
          }
        switch (token_info->state)
        {
          case IN_WHITE:
          {
            (*next)--;
            token_info->state=IN_TOKEN;
            break;
          }
          case IN_TOKEN:
          case IN_QUOTE:
          {
            (*next)++;
            c=(int) line[*next];
            StoreToken(token_info,token,max_token_length,c);
            break;
          }
          case IN_OZONE:
          {
            token[token_info->offset]='\0';
            return(0);
          }
        }
        continue;
      }
    switch (token_info->state)
    {
      case IN_WHITE:
        token_info->state=IN_TOKEN;
      case IN_TOKEN:
      case IN_QUOTE:
      {
        StoreToken(token_info,token,max_token_length,c);
        break;
      }
      case IN_OZONE:
      {
        token[token_info->offset]='\0';
        return(0);
      }
    }
  }
  token[token_info->offset]='\0';
  return(0);
}
