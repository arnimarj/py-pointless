#include <pointless/bitutils.h>

void bm_set_(void* bitmask, uint64_t bit_index)
{
	unsigned char* bit = (unsigned char*)bitmask + (bit_index / 8);
	*bit |= 1 << (bit_index % 8);
}

void bm_reset_(void* bitmask, uint64_t bit_index)
{
	unsigned char* bit = (unsigned char*)bitmask + (bit_index / 8);
	*bit &= ~(1 << (bit_index % 8));
}

unsigned char bm_is_set_(void* bitmask, uint64_t bit_index)
{
	unsigned char* bit = (unsigned char*)bitmask + (bit_index / 8);
	return (*bit & (1 << (bit_index % 8)));
}
