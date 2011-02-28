#include <pointless/pointless_malloc.h>

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
	char* pointless_strdup(const char* s)
	{
		char* s_ = (char*)pointless_malloc(strlen(s) + 1);

		if (s_)
			strcpy(s_, s);

		return s_;
	}
	void pointless_malloc_stats()
	{
		je_malloc_stats_print();
	}

#elif 0

	static long page_size = -1;
	static size_t n_alloc = 0;
	static size_t b_alloc = 0;
	static size_t n_wasted = 0;

	#include <unistd.h>
	#include <strings.h>
	#include <sys/mman.h>

	static void get_page_size()
	{
		if (page_size == -1) {
			page_size = sysconf(_SC_PAGESIZE);
			printf("PAGESIZE: %li\n", page_size);
		}
	}

	static void update_stats_alloc(size_t n_bytes)
	{
		get_page_size();

		n_alloc += 1;
		b_alloc += n_bytes;

		if (n_bytes % page_size)
			n_wasted += page_size - n_bytes % page_size;
	}

	static void update_stats_free(size_t n_bytes)
	{
		get_page_size();

		n_alloc -= 1;
		b_alloc -= n_bytes;

		if (n_bytes % page_size)
			n_wasted -= page_size - n_bytes % page_size;
	}

	void* pointless_calloc(size_t nmemb, size_t size)
	{
		void* m = pointless_malloc(nmemb * size);

		if (m)
			bzero(m, nmemb * size);

		return m;
	}

	void* pointless_malloc(size_t size)
	{
		void* m = mmap(NULL, size + sizeof(size_t), PROT_READ | PROT_WRITE, MAP_PRIVATE |  MAP_ANONYMOUS, -1, 0);

		if (m == MAP_FAILED)
			return 0;

		update_stats_alloc(size + sizeof(size_t));

		*((size_t*)m) = size;
		return (char*)m + sizeof(size_t);
	}

	void pointless_free(void* ptr)
	{
		if (ptr == 0)
			return;

		ptr = (char*)ptr - sizeof(size_t);
		size_t s = *((size_t*)ptr);
		munmap(ptr, s + sizeof(size_t));
		update_stats_free(s + sizeof(size_t));
	}

	void* pointless_realloc(void* ptr, size_t size)
	{
		if (ptr == 0)
			return pointless_malloc(size);

		ptr = (char*)ptr - sizeof(size_t);
		size_t _s = *((size_t*)ptr);
		void* _ptr = mremap(ptr, _s + sizeof(size_t), size + sizeof(size_t), MREMAP_MAYMOVE);

		if (_ptr == MAP_FAILED)
			return 0;

		update_stats_free(_s + sizeof(size_t));
		update_stats_alloc(size + sizeof(size_t));

		*((size_t*)_ptr) = size;

		return (char*)_ptr + sizeof(size_t);
	}

	char* pointless_strdup(const char* s)
	{
		size_t n = strlen(s) + 1;
		char* m = pointless_malloc(n);
		if (m)
			strcpy(m, s);
		return m;
	}

	void pointless_malloc_stats()
	{
		get_page_size();
		printf("---------------------------------------------------------\n");
		printf("page_size: %lli\n", (long long int)page_size);
		printf("n_alloc:   %lli\n", (long long int)n_alloc);
		printf("b_alloc:   %lli\n", (long long int)b_alloc);
		printf("n_wasted:  %lli\n", (long long int)n_wasted);
		printf("---------------------------------------------------------\n");
		printf("b_alloc:   %.2lf MB\n", (double)b_alloc / (1024 * 1024));
		printf("n_wasted:  %.2lf MB\n", (double)n_wasted / (1024 * 1024));
		printf("---------------------------------------------------------\n");
	}

#else

	#include <malloc.h>

	void* pointless_calloc(size_t nmemb, size_t size)
		{ return calloc(nmemb, size); }
	void* pointless_malloc(size_t size)
		{ return malloc(size); }
	void pointless_free(void* ptr)
		{ free(ptr); }
	void* pointless_realloc(void* ptr, size_t size)
		{ return realloc(ptr, size); }
	char* pointless_strdup(const char* s)
		{ return strdup(s); }
	void pointless_malloc_stats()
	{
		malloc_stats();
	}

#endif
