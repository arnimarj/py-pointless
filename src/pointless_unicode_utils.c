#include <pointless/pointless_unicode_utils.h>

static size_t ucs2_strlen(uint16_t* s)
{
	size_t len = 0;

	while (*s) {
		len += 1;
		s += 1;
	}

	return len;
}

static size_t ucs4_strlen(uint32_t* s)
{
	size_t len = 0;

	while (*s) {
		len += 1;
		s += 1;
	}

	return len;
}

uint32_t* pointless_unicode_ascii_to_ucs4(char* ascii, const char** error)
{
	size_t len = strlen(ascii), i;
	uint32_t* buffer = (uint32_t*)malloc(sizeof(uint32_t) * (len + 1));

	if (buffer == 0) {
		*error = "out of memory";
		return 0;
	}

	for (i = 0; i < len; i++)
		buffer[i] = (uint32_t)ascii[i];

	buffer[i] = 0;

	return buffer;
}

uint32_t* pointless_unicode_ucs2_to_ucs4(uint16_t* ucs2)
{
	size_t len = ucs2_strlen(ucs2), i;
	uint32_t* buffer = (uint32_t*)malloc(sizeof(uint32_t) * (len + 1));

	if (buffer == 0)
		return 0;

	for (i = 0; i < len; i++)
		buffer[i] = (uint32_t)ucs2[i];

	buffer[i] = 0;

	return buffer;
}

uint16_t* pointless_unicode_ucs4_to_ucs2(uint32_t* ucs4, const char** error)
{
	size_t len = ucs4_strlen(ucs4), i;
	uint16_t* buffer = (uint16_t*)malloc(sizeof(uint16_t) * (len + 1));

	if (buffer == 0) {
		*error = "out of memory";
		return 0;
	}

	for (i = 0; i < len; i++) {
		if (ucs4[i] > 0xFFFF) {
			*error = "string character is larger than 0xFFFF";
			free(buffer);
			return 0;
		}

		buffer[i] = (uint16_t)ucs4[i];
	}

	buffer[i] = 0;

	return buffer;
}
