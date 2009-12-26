#ifndef __ENUM__UTILS__H__
#define __ENUM__UTILS__H__

//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <unistd.h>

// simple macros
#define ICEIL(a, div) (((a) % (div)) ? (((a) / (div)) + 1) : ((a) / (div)))
#define SIMPLE_CMP(a, b) (((a) == (b)) ? 0 : ((a) < (b) ? -1 : +1))
#define SIMPLE_MIN(a, b) ((a) < (b) ? (a) : (b))
#define SIMPLE_MAX(a, b) ((a) > (b) ? (a) : (b))

//FILE* pointless_fopen(const char* dir, const char* fname, const char* flags, const char** error);
//int pointless_fd_size(int fd, size_t* s);

#endif
