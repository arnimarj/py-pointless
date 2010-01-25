#ifndef __POINTLESS_CUSTOM__SORT__H__
#define __POINTLESS_CUSTOM__SORT__H__

#include <assert.h>

#include <pointless/pointless_defs.h>

typedef int (*qsort_cmp_)(int a, int b, int* c, void* user);
typedef void (*qsort_swap_)(int a, int b, void* user);

int bentley_sort_(int n, qsort_cmp_ cmp, qsort_swap_ swap, void* user);

#endif
