#ifndef __POINTLESS__VALIDATE__H__
#define __POINTLESS__VALIDATE__H__

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <pointless/bitutils.h>
#include <pointless/pointless_int_ops.h>

/*
We have to perform exhaustive validation when opening a pointless file, since 
invalid files may crash the pointless API even when used correctly. Our 
guarantee is that correct use of the API will never produce invalid results or 
errors.

There are a few basic checks we do, and two structural checks. First the simple 
ones:

	a) there must be a header
	b) there must be offset vectors matching the number of types given in the header

Then, all values reachable from the root must have a place in the corresponding 
heaps, or if they are inline-values, they must be valid.

Then there are more complicated checks which check the following:

	a) the recursive depth does not exceed POINTLESS_MAX_DEPTH
	b) all keys in sets/maps are hashable
	c) all sets/maps are setup correctly w.r.t. probing strategy
	d) all hashable vectors are not part of a cycle

Each hashable data type is either a non-compressed vector, or unicodes or inline 
values. The only data type which is not allowed in a cycle is hashable vectors.

Each hashable value can only recurse down to hashable values.

Checking all this is pretty expensive, and is done after the first set of tests.
*/

#include <pointless/pointless_defs.h>
#include <pointless/pointless_reader_utils.h>
#include <pointless/pointless_hash_table.h>
#include <pointless/pointless_unicode_utils.h>
#include <pointless/pointless_cycle_marker_wrappers.h>
#include <pointless/pointless_walk.h>

typedef struct {
	pointless_t* p;
	int force_ucs2;
} pointless_validate_context_t;

int32_t pointless_validate(pointless_validate_context_t* context, const char** error);

// checks if the offset vector reference is good, and that the heap data is valid, without
// checking the children (if any)
int32_t pointless_validate_heap_ref(pointless_validate_context_t* context, pointless_value_t* v, const char** error);

// checks if the internal invariants of an inline-value hold
int32_t pointless_validate_inline_invariants(pointless_validate_context_t* context, pointless_value_t* v, const char** error);

// check heap data
int32_t pointless_validate_heap_value(pointless_validate_context_t* context, pointless_value_t* v, const char** error);

// validate hash table invariants
int32_t pointless_hash_table_validate(pointless_t* p, uint32_t n_items, uint32_t n_buckets, uint32_t* hash_vector, pointless_value_t* key_vector, pointless_value_t* value_vector, const char** error);

#endif
