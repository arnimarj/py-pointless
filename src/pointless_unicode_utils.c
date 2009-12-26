#include <pointless/pointless_unicode_utils.h>

// are converters initialized
static int is_init = 0;

// is destructor registered
static int is_destructor_registered = 0;

// unicode conversion handles
static iconv_t ascii_to_ucs4 = (iconv_t)-1;
static iconv_t ucs2_to_ucs4 = (iconv_t)-1;
static iconv_t ucs4_to_utf7 = (iconv_t)-1;
static iconv_t ucs4_to_utf8 = (iconv_t)-1;
static iconv_t ucs4_to_ucs2 = (iconv_t)-1;

#define ICONV_CLOSE_WRAPPER(var) if ((var) != (iconv_t)-1) {iconv_close(var); var = (iconv_t)-1;}

static void pointless_unicode_close()
{
	ICONV_CLOSE_WRAPPER(ascii_to_ucs4);
	ICONV_CLOSE_WRAPPER(ucs2_to_ucs4);
	ICONV_CLOSE_WRAPPER(ucs4_to_utf7);
	ICONV_CLOSE_WRAPPER(ucs4_to_utf8);
	ICONV_CLOSE_WRAPPER(ucs4_to_ucs2);
	is_init = 0;
}

#define ICONV_OPEN_WRAPPER(var, a, b) (var) = iconv_open(b, a); if ((var) == (iconv_t)-1) {*error = "iconv_open(\"" b "\", \"" a "\") failed"; pointless_unicode_close(); return 0;}

static int pointless_unicode_open(const char** error)
{
	if (!is_init) {
		ICONV_OPEN_WRAPPER(ascii_to_ucs4, "ASCII", "UCS-4LE");
		ICONV_OPEN_WRAPPER(ucs2_to_ucs4,  "UCS-2LE",  "UCS-4LE");
		ICONV_OPEN_WRAPPER(ucs4_to_utf7,  "UCS-4LE",  "UTF-7");
		ICONV_OPEN_WRAPPER(ucs4_to_utf8,  "UCS-4LE",  "UTF-8");
		ICONV_OPEN_WRAPPER(ucs4_to_ucs2,  "UCS-4LE",  "UCS-2LE");
		is_init = 1;
	}

	if (is_init && !is_destructor_registered) {
		if (atexit(pointless_unicode_close) != 0) {
			pointless_unicode_close();
			*error = "atexit() failure";
			return 0;
		}

		is_destructor_registered = 1;
	}

	return 1;
}

#define ICONV_INIT_WRAPPER() if (!pointless_unicode_open(error)) { return 0; }

static int pointless_unicode_conv_n_bytes(iconv_t conv, uint32_t* in, uint32_t n_bytes_in, size_t* result, const char** error)
{
	// 32 byte big enough for any single unicode datapoint, under any encoding
	char outbuf_buf[8192];
	char* outbuf;
	size_t outbytesleft;

	char* inbuf = (char*)in;
	size_t inbytesleft = n_bytes_in;

	// reset converter state
	iconv(conv, 0, 0, 0, 0);
	size_t iconv_res;

	*result = 0;

	while (inbytesleft > 0) {
		outbuf = outbuf_buf;
		outbytesleft = sizeof(outbuf_buf);

		size_t n_bytes_to_convert = 1024;

		if (n_bytes_to_convert > inbytesleft)
			n_bytes_to_convert = inbytesleft;

		size_t pre = n_bytes_to_convert;
		iconv_res = iconv(conv, &inbuf, &n_bytes_to_convert, &outbuf, &outbytesleft);
		inbytesleft -= (pre - n_bytes_to_convert);

		if (iconv_res == (size_t)-1) {
			switch (errno) {
				case E2BIG:
					*error = "pointless_unicode_conv_n_bytes(): There is not sufficient room at *outbuf";
					return 0;
				case EILSEQ:
					*error = "pointless_unicode_conv_n_bytes(): An invalid multibyte sequence has been encountered in the input";
					return 0;
				case EINVAL:
					*error = "pointless_unicode_conv_n_bytes(): An incomplete multibyte sequence has been encountered in the input";
					return 0;
				default:
					*error = "pointless_unicode_conv_n_bytes(): unknown error from iconv()";
					return 0;
			}
		}

		*result += (sizeof(outbuf_buf) - outbytesleft);
	}

	return 1;
}

static int pointless_unicode_conv(iconv_t conv, uint32_t* in, uint32_t n_bytes_in, char* out, uint32_t n_bytes_out, const char** error)
{
	// input buffer
	char* inbuf = (char*)in;
	size_t inbytesleft = n_bytes_in;

	// output buffer
	char* outbuf = (char*)out;
	size_t outbytesleft = n_bytes_out;

	// reset converter state
	iconv(conv, 0, 0, 0, 0);
	size_t iconv_res;

	// start the conversion
	iconv_res = iconv(conv, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

	if (iconv_res == (size_t)-1) {
		switch (errno) {
			case E2BIG:
				*error = "pointless_unicode_conv(): There is not sufficient room at *outbuf";
				return 0;
			case EILSEQ:
				*error = "pointless_unicode_conv(): An invalid multibyte sequence has been encountered in the input";
				return 0;
			case EINVAL:
				*error = "pointless_unicode_conv(): An incomplete multibyte sequence has been encountered in the input";
				return 0;
			default:
				*error = "pointless_unicode_conv(): unknown error from iconv()";
				return 0;
		}
	}

	// we should have converted everything
	if (outbytesleft != 0) {
		*error = "pointless_unicode_decode(): not all characters converted";
		return 0;
	}

	return 1;
}

static char* pointless_unicode_encode_wrapper(void* unicode_ptr, uint32_t unicode_n_bytes, iconv_t conv, size_t tz_bytes, const char** error)
{
	// a buffer to hold conversion results
	char* buffer = 0;
	size_t buffer_len = 0;

	// compute its length
	if (!pointless_unicode_conv_n_bytes(conv, unicode_ptr, unicode_n_bytes, &buffer_len, error))
		return 0;

	// allcate it
	buffer = (char*)malloc(buffer_len + tz_bytes);

	if (buffer == 0) {
		*error = "out of memory";
		return 0;
	}

	if (!pointless_unicode_conv(conv, unicode_ptr, unicode_n_bytes, buffer, buffer_len, error)) {
		free(buffer);
		return 0;
	}

	// add terminating zero
	size_t i;

	for (i = 0; i < tz_bytes; i++)
		buffer[buffer_len + i] = 0;

	// return it
	return buffer;
}

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
	ICONV_INIT_WRAPPER();
	return (uint32_t*)pointless_unicode_encode_wrapper(ascii, strlen(ascii) * sizeof(char), ascii_to_ucs4, sizeof(uint32_t), error);
}

uint32_t* pointless_unicode_ucs2_to_ucs4(uint16_t* ucs2, const char** error)
{
	ICONV_INIT_WRAPPER();
	return (uint32_t*)pointless_unicode_encode_wrapper(ucs2, ucs2_strlen(ucs2) * sizeof(uint16_t), ucs2_to_ucs4, sizeof(uint32_t), error);
}

char* pointless_unicode_ucs4_to_utf7(uint32_t* ucs4, const char** error)
{
	ICONV_INIT_WRAPPER();
	return pointless_unicode_encode_wrapper(ucs4, ucs4_strlen(ucs4) * sizeof(uint32_t), ucs4_to_utf7, sizeof(char), error);
}


char* pointless_unicode_ucs4_to_utf8(uint32_t* ucs4, const char** error)
{
	ICONV_INIT_WRAPPER();
	return pointless_unicode_encode_wrapper(ucs4, ucs4_strlen(ucs4) * sizeof(uint32_t), ucs4_to_utf8, sizeof(char), error);
}

uint16_t* pointless_unicode_ucs4_to_ucs2(uint32_t* ucs4, const char** error)
{
	ICONV_INIT_WRAPPER();
	return (uint16_t*)pointless_unicode_encode_wrapper(ucs4, ucs4_strlen(ucs4) * sizeof(uint32_t), ucs4_to_ucs2, sizeof(uint16_t), error);
}

/*
#include <stdio.h>

int main(int argc, char** argv)
{
	const char* error = 0;

	const char* ascii = "hello world";
	uint32_t* ucs4 = 0;

	if ((ucs4 = pointless_unicode_ascii_to_ucs4((char*)ascii, &error)) == 0) {
		fprintf(stderr, "pointless_unicode_ascii_to_ucs4(): %s\n", error);
		exit(EXIT_FAILURE);
	}

	char* ascii_post = 0;

	if ((ascii_post = pointless_unicode_ucs4_to_utf7(ucs4, &error)) == 0) {
		fprintf(stderr, "pointless_unicode_ucs4_to_utf7(): %s\n", error);
		exit(EXIT_FAILURE);
	}

	free(ascii_post);
	free(ucs4);
	exit(EXIT_FAILURE);
}
*/
