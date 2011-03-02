#ifndef __POINTLESS_MALLOC__
#define __POINTLESS_MALLOC__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void* pointless_calloc(size_t nmemb, size_t size);
void* pointless_malloc(size_t size);
void pointless_free(void* ptr);
void* pointless_realloc(void* ptr, size_t size);
char* pointless_strdup(const char* s);
void pointless_malloc_stats();
size_t pointless_malloc_sizeof(void* ptr);

#endif
