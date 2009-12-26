#ifndef __BITUTILS__H__
#define __BITUTILS__H__

#include <stdint.h>

void bm_set(void* bitmask, uint64_t bit_index);
void bm_reset(void* bitmask, uint64_t bit_index);
unsigned char bm_is_set(void* bitmask, uint64_t bit_index);

#endif
