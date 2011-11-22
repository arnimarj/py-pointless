#ifndef __POINTLESS_MALLOC__H__
#define __POINTLESS_MALLOC__H__

#include <stdlib.h>
#include <string.h>

void* pointless_calloc(size_t nmemb, size_t size);
void* pointless_malloc(size_t size);
void pointless_free(void* ptr);
void* pointless_realloc(void* ptr, size_t size);
char* pointless_strdup(const char* s);

#endif
