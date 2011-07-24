#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <machine/limits.h>
#include <errno.h>
#include <unistd.h>
#include "darwinutils.h"
#include "tools.h"
#include <sys/param.h>

# define __canonicalize_file_name_darwin canonicalize_file_name_darwin
#  define __getcwd getcwd
# define __readlink readlink

char *strchrnul_darwin(const char *s, int c_in)
{
  char c = c_in;
  while(*s && (*s != c))
    s++;

  return (char *) s;
}

char *strndup_darwin(const char *str, size_t len)
{
  size_t t;
  char *dest;
  
  t = strlen(str);
  if(len < t)
  {
    t = len;
  }
  
  dest = (char *)calloc(t + 1, 1);
  
  if (NULL != dest)
  {
    memcpy(dest, str, t);
    dest[t] = '\0';
  }

  return dest;
}

ssize_t getdelim_darwin(char **lineptr, size_t *n, int delimiter, FILE *fp)
{
  ssize_t result = 0;
  size_t cur_len = 0;

  if(lineptr == NULL || n == NULL || fp == NULL)
  {
    errno = EINVAL;
    return -1;
  }

  flockfile (fp);

  if(*lineptr == NULL || *n == 0)
  {
    *n = 120;
    *lineptr = (char *) malloc (*n);
    if (*lineptr == NULL)
    {
      result = -1;
      goto unlock_return;
	}
  }

  for(;;)
  {
    int i;
    i = getc (fp);
    if (i == EOF)
    {
      result = -1;
      break;
    }

    if(cur_len + 1 >= *n)
	{
      size_t needed_max =
      SSIZE_MAX < SIZE_MAX ? (size_t) SSIZE_MAX + 1 : SIZE_MAX;
      size_t needed = 2 * *n + 1;
      char *new_lineptr;

      if(needed_max < needed)
        needed = needed_max;
      if(cur_len + 1 >= needed)
	  {
	    result = -1;
	    goto unlock_return;
	  }

	  new_lineptr = (char *)realloc(*lineptr, needed);
      if(new_lineptr == NULL)
      {
        result = -1;
        goto unlock_return;
      }

      *lineptr = new_lineptr;
      *n = needed;
    }

    (*lineptr)[cur_len] = i;
    cur_len++;

    if (i == delimiter)
      break;
  }
  (*lineptr)[cur_len] = '\0';
  result = cur_len ? cur_len : result;

  unlock_return:
  funlockfile (fp);
  return result;
}

ssize_t getline_darwin(char **lineptr, size_t *n, FILE *stream)
{
  return getdelim_darwin(lineptr, n, '\n', stream);
}

char * __realpath_darwin(const char *name, char *resolved)
{
  char *rpath, *dest, *extra_buf = NULL;
  const char *start, *end, *rpath_limit;
  long int path_max;
  int num_links = 0;

  if(name == NULL)
  {
    __set_errno (EINVAL);
    return NULL;
  }

  if(name[0] == '\0')
  {
    __set_errno (ENOENT);
    return NULL;
  }

  path_max = PATH_MAX;

  if(resolved == NULL)
  {
    rpath = (char*)malloc (path_max);
    if (rpath == NULL)
      return NULL;
  }
  else
    rpath = resolved;
  rpath_limit = rpath + path_max;

  if(name[0] != '/')
  {
    if(!__getcwd (rpath, path_max))
    {
      rpath[0] = '\0';
      goto error;
    }
    dest = strchr (rpath, '\0');
  }
  else
  {
    rpath[0] = '/';
    dest = rpath + 1;
  }

  for (start = end = name; *start; start = end)
  {
    struct stat st;
    int n;

    while(*start == '/')
      ++start;

    for(end = start; *end && *end != '/'; ++end)
      ;

    if(end - start == 0)
      break;
    else if(end - start == 1 && start[0] == '.')
      ;
    else if(end - start == 2 && start[0] == '.' && start[1] == '.')
    {
      if (dest > rpath + 1)
        while ((--dest)[-1] != '/')
          ;
    }
    else
    {
      size_t new_size;

      if(dest[-1] != '/')
        *dest++ = '/';

      if(dest + (end - start) >= rpath_limit)
      {
        ptrdiff_t dest_offset = dest - rpath;
        char *new_rpath;

        if(resolved)
        {
          __set_errno(ENAMETOOLONG);
          if(dest > rpath + 1)
            dest--;
          *dest = '\0';
          goto error;
        }
        new_size = rpath_limit - rpath;
        if(end - start + 1 > path_max)
          new_size += end - start + 1;
        else
          new_size += path_max;
        new_rpath = (char *) realloc (rpath, new_size);
        if(new_rpath == NULL)
          goto error;
        rpath = new_rpath;
        rpath_limit = rpath + new_size;

        dest = rpath + dest_offset;
      }

      memcpy (dest, start, end - start);
      dest += end - start;
      *dest = '\0';

      if(lstat (rpath, &st) < 0)
        goto error;

      if(S_ISLNK(st.st_mode))
      {
        char *buf = (char*)__alloca (path_max);
        size_t len;
        
        if(++num_links > MAXSYMLINKS)
        {
          __set_errno(ELOOP);
          goto error;
        }

        n = __readlink(rpath, buf, path_max);
        if (n < 0)
          goto error;
        buf[n] = '\0';

        if(!extra_buf)
          extra_buf = (char*)__alloca(path_max);

        len = strlen (end);
        if((long int) (n + len) >= path_max)
        {
          __set_errno (ENAMETOOLONG);
          goto error;
        }

        memmove(&extra_buf[n], end, len + 1);
        name = end = (char*)memcpy(extra_buf, buf, n);

        if(buf[0] == '/')
          dest = rpath + 1;
        else if (dest > rpath + 1)
          while ((--dest)[-1] != '/');
      }
    }
  }
  if(dest > rpath + 1 && dest[-1] == '/')
    --dest;
  *dest = '\0';

  return resolved ? (char*)memcpy (resolved, rpath, dest - rpath + 1) : rpath;

  error:
  if(resolved)
    strcpy (resolved, rpath);
  else
    free (rpath);
  return NULL;
}


char *__canonicalize_file_name_darwin(const char *name)
{
  return __realpath_darwin(name, NULL);
}

int sscanf_darwin(const char *str, const char *format, ...)
{
  va_list list;
  const char *p;
  char *s, *c;
  char **a;
  int **d;
  unsigned int **x, **u;
  int pos = 0, width = 0, count = 0;
  char temp[1];
  
  va_start( list, format );
  
  for( p = format ; *p ; ++p )
  {
    width=0;
    if( str[pos]==' ')
    {
      pos++;
    }

    if( *p != '%' )
    {
    }
    else
    {
      switch( *++p )
      {

        case '0':
          if(str[pos]=='0')
            pos++;
            p++;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
          temp[0]=*p;
          width = atoi(temp);
          switch( *++p )
          {
            case 'X':
            {
              x = va_arg(list, unsigned int**);
              read_x_w(str, x, width, &pos);
              count++;
              p++;
              break;
            }

            case 's':
            {
              //string
              s = va_arg(list, char *);
              read_s_w(str, s, width, &pos);
              count++;
              pos++;
              p++;
              break;
            }
            default:
            ;
          }
          break;
          // length
        }
        case 's':
        {
          //string
          p++;
          break;
        }

        case 'c':
        {
          //char
          c = va_arg(list, char*);
          read_c(str, c, *++p, &pos);
          pos++;
          count++;
          break;
        }


        case 'a':
        {
          //string with alloc
          if(*(p+=2)=='^')
          {
            if(*(p+1)!='\n')
            {
              a = va_arg(list, char**);
              read_a(str, a, *++p, &pos);
              p++;
              count++;
              pos++;
            }
            else
            {
              a = va_arg(list, char**);
              read_a(str, a, *++p, &pos);
              p++;
              count++;
              pos++;
            }
          }
          break;
        }

        case 'd':
        {
          //int
          d = va_arg(list, int**);
          if(*(p+1)==' ')
            read_d(str, d, *(p+=2), &pos);
          else
            read_d(str, d, *++p, &pos);
          pos++;
          count++;
          p--;
          break;
        }

        case 'u':
        {
          //int
          u = va_arg(list, unsigned int**);
          if(*(p+1)==' ')
            read_u(str, u, *(p+=2), &pos);
          else
            read_u(str, u, *++p, &pos);
          count++;
          pos++;
          p--;
          break;
        }

        case 'X':
        {
          //hex
          x = va_arg(list, unsigned int**);
          read_x(str, x, *++p, &pos);
          count++;
          pos++;
          break;
        }
        
        default:
          break;
      }
    }
  }
  va_end( list );
  return count;
  
}

void read_a(const char *str, char **ptr, const char delimiter, int *position)
{
  int i=0, j=0;
  int start = *position;
  char *temp;
  
  while(str[*position]!=delimiter&&str[*position]!='\0')
  {
    (*position)++;
    i++;
  }
  temp = (char*)calloc(i+1, sizeof(char) );
  for( j=0;j<i;j++)
  {
    temp[j]=str[start];
    start++;
  }
  temp[j]='\0';
  *ptr = temp;
}


void read_d(const char *str, int **ptr, const char delimiter, int *position)
{

  int i=0, j=0;
  int start = *position;
  char *temp;

  while(str[*position]!=delimiter&&str[*position]!='\0'&&str[*position]!=' ')
  {
    (*position)++;
    i++;
  }
  temp = (char*)calloc(i+1, sizeof(char) );
  for( j=0;j<i;j++)
  {
    temp[j]=str[start];
    start++;
  }
  temp[j]='\0';
  *ptr = (int*)atoi(temp);
}


void read_x(const char *str, unsigned int **ptr, const char delimiter, int *position)
{

  int i=0, j=0;
  int start = *position;
  char *temp;
#if defined (__i386__)
  unsigned int x;
  x = (unsigned int)ptr;
#else // 64 Bit
	unsigned long x;
	x = (unsigned long)ptr;
#endif
  while(str[*position]!=delimiter&&str[*position]!='\0'&&str[*position]!=' ')
  {
    (*position)++;
    i++;
  }
  temp = (char*)calloc(i+1, sizeof(char) );
  for( j=0;j<i;j++)
  {
    temp[j]=str[start];
    start++;
  }
  temp[j]='\0';
#if defined (__i386__)
	sscanf(temp, "%X", &x);
#else // 64 Bit
	sscanf(temp, "%lX", &x);
#endif
}

void read_x_w(const char *str, unsigned int **ptr, int width, int *position)
{

  int i=0, j=0;
  int start = *position;
  char *temp;
#if defined (__i386__)
	unsigned int x;
	x = (unsigned int)ptr;
#else // 64 Bit
	unsigned long x;
	x = (unsigned long)ptr;
#endif	
  temp = (char*)calloc(i+1, sizeof(char) );
  for( j=0;j<width;j++)
  {
    temp[j]=str[start];
    start++;
    (*position)++;
  }
  temp[j]='\0';
#if defined (__i386__)
	sscanf(temp, "%X", &x);
#else // 64 Bit
	sscanf(temp, "%lX", &x);
#endif
}

void read_s_w(const char *str, char *ptr, int width, int *position)
{

  int j=0;
  int start = *position;

  while(str[*position]!='\0'&&j<width&&str[*position]!=' ')
  {
    ptr[j]=str[start];
    start++;
    (*position)++;
    j++;
  }
  ptr[j]='\0';
}

void read_c(const char *str, char *ptr, const char delimiter, int *position)
{
  int i=0, j=0;
  int start = *position;
  char *temp;
  
  while(str[*position]!=delimiter&&str[*position]!='\0')
  {
    (*position)++;
    i++;
  }
  temp = (char*)calloc(i+1, sizeof(char) );
  for( j=0;j<i;j++)
  {
    temp[j]=str[start];
    start++;
  }
  *ptr = temp[0];
}

void read_u(const char *str, unsigned int **ptr, const char delimiter, int *position)
{

  int i=0, j=0;
  int start = *position;
  char *temp;

  while(str[*position]!=delimiter&&str[*position]!='\0'&&str[*position]!=' ')
  {
    (*position)++;
    i++;
  }
  temp = (char*)calloc(i+1, sizeof(char) );
  for( j=0;j<i;j++)
  {
    temp[j]=str[start];
    start++;
  }
  temp[j]='\0';
  *ptr = (unsigned int*)atoi(temp);
}
