#ifndef __POINTLESS__UNICODE__UTILS__H__
#define __POINTLESS__UNICODE__UTILS__H__

#ifndef __cplusplus
#include <limits.h>
#include <stdint.h>
#else
#include <climits>
#include <cstdint>
#endif

#include <assert.h>

#include <pointless/pointless_defs.h>
#include <pointless/pointless_int_ops.h>

// range check
int pointless_is_ucs4_ascii(uint32_t* s);
int pointless_is_ucs4_ucs2(uint32_t* s);
int pointless_is_ucs2_ascii(uint16_t* s);

// length check
size_t pointless_ucs4_len(uint32_t* s);
size_t pointless_ucs2_len(uint16_t* s);
size_t pointless_ascii_len(uint8_t* s);
size_t pointless_ucs1_len(uint8_t* s);
size_t pointless_wchar_len(wchar_t* s);

// strcpy()
void pointless_ucs4_cpy(uint32_t* dst, const uint32_t* src);
void pointless_ucs2_cpy(uint16_t* dst, const uint16_t* src);
void pointless_ascii_cpy(uint8_t* dst, const uint8_t* src);

// converters, caller must pointless_free() buffers

// ucs-4
uint16_t* pointless_ucs4_to_ucs2(uint32_t* ucs4);
uint8_t* pointless_ucs4_to_ascii(uint32_t* ucs4);

// ucs-2
uint32_t* pointless_ucs2_to_ucs4(uint16_t* ucs2);
uint8_t* pointless_ucs2_to_ascii(uint16_t* ucs2);

// ascii
uint32_t* pointless_ascii_to_ucs4(uint8_t* ascii);

#endif
