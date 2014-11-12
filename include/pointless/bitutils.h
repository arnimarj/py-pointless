#ifndef __BITUTILS__H__
#define __BITUTILS__H__

#ifndef __cplusplus
#include <limits.h>
#include <stdint.h>
#else
#include <climits>
#include <cstdint>
#endif

void bm_set_(void* bitmask, uint64_t bit_index);
void bm_reset_(void* bitmask, uint64_t bit_index);
unsigned char bm_is_set_(void* bitmask, uint64_t bit_index);

#endif
