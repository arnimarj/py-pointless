#if 0

	#define JEMALLOC_MANGLE
	#include <jemalloc/jemalloc.h>

	void* pointless_calloc(size_t nmemb, size_t size)
		{ return je_calloc(nmemb, size); }
	void* pointless_malloc(size_t size)
		{ return je_malloc(size); }
	void pointless_free(void* ptr)
		{ je_free(ptr); }
	void* pointless_realloc(void* ptr, size_t size)
		{ return je_realloc(ptr, size); }

#else

	#include <stdlib.h>

	void* pointless_calloc(size_t nmemb, size_t size)
		{ return calloc(nmemb, size); }
	void* pointless_malloc(size_t size)
		{ return malloc(size); }
	void pointless_free(void* ptr)
		{ free(ptr); }
	void* pointless_realloc(void* ptr, size_t size)
		{ return realloc(ptr, size); }

#endif
