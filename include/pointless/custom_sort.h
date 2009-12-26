#ifndef __CUSTOM__SORT__H__
#define __CUSTOM__SORT__H__

#include <assert.h>

#include <pointless/util.h>

typedef int (*qsort_cmp)(int a, int b, int* c, void* user);
typedef void (*qsort_swap)(int a, int b, void* user);

int bentley_sort(int n, qsort_cmp cmp, qsort_swap swap, void* user);

#endif
