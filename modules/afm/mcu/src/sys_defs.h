/*
 * sys_defs.h
 *
 *  Created on: Jan 25, 2026
 *      Author: akiel
 */

#ifndef INCLUDE_SYS_DEFS_H_
#define INCLUDE_SYS_DEFS_H_

#include <errno.h>
#include <sys/stat.h>
#include <stddef.h> // For size_t


#undef errno
extern int errno;
extern int  _end;

__attribute__((weak)) int _close(int file)
{
  return -1;
}

__attribute__((weak)) int _lseek(int file, int ptr, int dir)
{
  return 0;
}

__attribute__((weak)) int _read(int file, char *ptr, int len)
{
  return 0;
}

__attribute__((weak)) int _write(int file, char *ptr, int len)
{
  return len;
}

__attribute__((weak)) void _exit(int status)
{
  __asm("BKPT #0");
  while(1);
}

__attribute__((weak)) caddr_t _sbrk(int incr)
{
  static char *heap_end;
  char *prev_heap_end;

  if (heap_end == 0)
  {
    heap_end = (caddr_t)&_end;
  }

  prev_heap_end = heap_end;
  if (heap_end + incr > (char *)__get_MSP())
  {
    errno = ENOMEM;
    return (caddr_t)-1;
  }

  heap_end += incr;
  return (caddr_t)prev_heap_end;
}

__attribute__((weak)) int _kill(int pid, int sig)
{
  errno = EINVAL;
  return -1;
}

__attribute__((weak)) int _getpid(void)
{
  return 1;
}

__attribute__((weak)) int _fstat(int file, struct stat *st)
{
  st->st_mode = S_IFCHR;
  return 0;
}

__attribute__((weak)) int _isatty(int file)
{
  return 1;
}


#endif /* INCLUDE_SYS_DEFS_H_ */
