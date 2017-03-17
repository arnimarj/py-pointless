#include <pointless/pointless_unicode_utils.h>

#define POINTLESS_BELOW_RANGE(s, i_max) do {while (*(s) && *(s) <= (i_max)) (s)++; return (*(s) == 0);} while (0);
#define POINTLESS_STRING_LEN(s) do {size_t i = 0; while (*(s)) {(s)++; i++;} return i;} while (0);
#define POINTLESS_STRING_CPY(dst, src) do {while (*(src)) {*(dst++) = *(src++);} *(dst) = 0;} while (0);

int pointless_is_ucs4_ascii(uint32_t* s)
	{ POINTLESS_BELOW_RANGE(s, UINT8_MAX); }
int pointless_is_ucs4_ucs2(uint32_t* s)
	{ POINTLESS_BELOW_RANGE(s, UINT16_MAX); }
int pointless_is_ucs2_ascii(uint16_t* s)
	{ POINTLESS_BELOW_RANGE(s, UINT8_MAX); }

size_t pointless_ucs4_len(uint32_t* s)
	{ POINTLESS_STRING_LEN(s); }
size_t pointless_ucs2_len(uint16_t* s)
	{ POINTLESS_STRING_LEN(s); }
size_t pointless_ascii_len(uint8_t* s)
	{ POINTLESS_STRING_LEN(s); }
size_t pointless_ucs1_len(uint8_t* s)
	{ POINTLESS_STRING_LEN(s); }
size_t pointless_wchar_len(wchar_t* s)
	{ POINTLESS_STRING_LEN(s); }

void pointless_ucs4_cpy(uint32_t* dst, const uint32_t* src)
	{ POINTLESS_STRING_CPY(dst, src); }
void pointless_ucs2_cpy(uint16_t* dst, const uint16_t* src)
	{ POINTLESS_STRING_CPY(dst, src); }
void pointless_ascii_cpy(uint8_t* dst, const uint8_t* src)
	{ POINTLESS_STRING_CPY(dst, src); }

#undef POINTLESS_BELOW_RANGE
#undef POINTLESS_STRING_LEN
#undef POINTLESS_STRING_CPY

uint16_t* pointless_ucs4_to_ucs2(uint32_t* ucs4)
{
	assert(pointless_is_ucs4_ucs2(ucs4));
	size_t n = pointless_ucs4_len(ucs4);
	uint16_t* ucs2_ = (uint16_t*)pointless_malloc(sizeof(uint16_t) * (n + 1));

	if (ucs2_ == 0)
		return 0;

	uint16_t* ucs2 = ucs2_;

	while (*ucs4)
		*ucs2++ = (uint16_t)*ucs4++;

	*ucs2 = 0;
	return ucs2_;
}

uint8_t* pointless_ucs4_to_ascii(uint32_t* ucs4)
{
	assert(pointless_is_ucs4_ascii(ucs4));
	size_t n = pointless_ucs4_len(ucs4);
	uint8_t* ascii_ = (uint8_t*)pointless_malloc(sizeof(uint8_t) * (n + 1));

	if (ascii_ == 0)
		return 0;

	uint8_t* ascii = ascii_;

	while (*ucs4)
		*ascii++ = (uint8_t)*ucs4++;

	*ascii = 0;
	return ascii_;
}

uint32_t* pointless_ucs2_to_ucs4(uint16_t* ucs2)
{
	size_t n = pointless_ucs2_len(ucs2);

	// watch out for overflow
	intop_sizet_t c = intop_sizet_mult(intop_sizet_init(n + 1), intop_sizet_init(sizeof(uint32_t)));

	if (c.is_overflow)
		return 0;

	uint32_t* ucs4_ = (uint32_t*)pointless_malloc(c.value);

	if (ucs4_ == 0)
		return 0;

	uint32_t* ucs4 = ucs4_;

	while (*ucs2)
		*ucs4++ = (uint32_t)*ucs2++;

	*ucs4 = 0;
	return ucs4_;
}

uint8_t* pointless_ucs2_to_ascii(uint16_t* ucs2)
{
	assert(pointless_is_ucs2_ascii(ucs2));
	size_t n = pointless_ucs2_len(ucs2);
	uint8_t* ascii_ = (uint8_t*)pointless_malloc(sizeof(uint8_t) * (n + 1));

	if (ascii_ == 0)
		return 0;

	uint8_t* ascii = ascii_;

	while (*ucs2)
		*ascii++ = (uint16_t)*ucs2++;

	*ascii = 0;
	return ascii_;
}

uint32_t* pointless_ascii_to_ucs4(uint8_t* ascii)
{
	size_t n = pointless_ascii_len(ascii);

	// watch out for overflow
	intop_sizet_t c = intop_sizet_mult(intop_sizet_init(n + 1), intop_sizet_init(sizeof(uint32_t)));

	if (c.is_overflow)
		return 0;

	uint32_t* ucs4_ = (uint32_t*)pointless_malloc(c.value);

	if (ucs4_ == 0)
		return 0;

	uint32_t* ucs4 = ucs4_;

	while (*ascii)
		*ucs4++ = (uint32_t)*ascii++;

	*ucs4 = 0;
	return ucs4_;
}
