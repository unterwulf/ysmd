#ifndef _COMPAT_H_
#define _COMPAT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include "lang.h"
#include "rijndael/rijndael-api-fst.h"

#define THREADS_COUNT    0x3

#include <pthread.h>
#include <semaphore.h>

/* used in *nix systems lacking posix data types */
#ifdef MISSING_SIGNED
typedef signed char    int8_t;
typedef signed short   int16_t;
typedef signed int     int32_t;
#endif

#ifdef MISSING_UNSIGNED
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#ifndef __RCSID
#define __RCSID(x)
#endif

#if !defined (TRUE)
#define FALSE 0
#define TRUE !FALSE
#endif

#define SOCK_READ(x,y,z) read(x,y,z)
#define SOCK_WRITE(x,y,z) write(x,y,z)

#define NUM_ELEM_ARR(arr)    ((ssize_t)(sizeof((arr))/sizeof((arr)[0])))

#endif
