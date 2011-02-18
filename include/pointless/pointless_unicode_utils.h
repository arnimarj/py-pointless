#ifndef __POINTLESS__UNICODE__UTILS__H__
#define __POINTLESS__UNICODE__UTILS__H__

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

// NB: caller must free() returned buffer
uint32_t* pointless_unicode_ascii_to_ucs4(char* ascii, const char** error);
uint32_t* pointless_unicode_ucs2_to_ucs4(uint16_t* ucs2);
uint16_t* pointless_unicode_ucs4_to_ucs2(uint32_t* ucs4, const char** error);

#endif
