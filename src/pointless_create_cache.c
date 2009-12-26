#include <pointless/pointless_create_cache.h>

// initialize cache
void pointless_create_cache_init(pointless_create_cache_t* cache)
{
	uint32_t i;

	for (i = 0; i < POINTLESS_CREATE_CACHE_N_U32; i++)
		cache->u32_cache[i] = POINTLESS_CREATE_VALUE_FAIL;

	for (i = 0; i < (POINTLESS_CREATE_CACHE_MAX_I32 - POINTLESS_CREATE_CACHE_MIN_I32 + 1); i++)
		cache->i32_cache[i] = POINTLESS_CREATE_VALUE_FAIL;

	cache->null_handle = POINTLESS_CREATE_VALUE_FAIL;
	cache->empty_slot_handle = POINTLESS_CREATE_VALUE_FAIL;
	cache->true_handle = POINTLESS_CREATE_VALUE_FAIL;
	cache->false_handle = POINTLESS_CREATE_VALUE_FAIL;
}

// U32
uint32_t pointless_create_cache_get_u32(pointless_create_cache_t* cache, uint32_t u32)
{
	uint32_t handle = POINTLESS_CREATE_VALUE_FAIL;

	if (u32 < POINTLESS_CREATE_CACHE_N_U32)
		handle = cache->u32_cache[u32];

	return handle;
}

void pointless_create_cache_set_u32(pointless_create_cache_t* cache, uint32_t u32, uint32_t handle)
{
	if (u32 < POINTLESS_CREATE_CACHE_N_U32)
		cache->u32_cache[u32] = handle;
}

// I32
uint32_t pointless_create_cache_get_i32(pointless_create_cache_t* cache, int32_t i32)
{
	uint32_t handle = POINTLESS_CREATE_VALUE_FAIL;

	if (POINTLESS_CREATE_CACHE_MIN_I32 <= i32 && i32 <= POINTLESS_CREATE_CACHE_MAX_I32)
		handle = cache->i32_cache[i32 - POINTLESS_CREATE_CACHE_MIN_I32];

	return handle;
}

void pointless_create_cache_set_i32(pointless_create_cache_t* cache, int32_t i32, uint32_t handle)
{
	if (POINTLESS_CREATE_CACHE_MIN_I32 <= i32 && i32 <= POINTLESS_CREATE_CACHE_MAX_I32)
		cache->i32_cache[i32 - POINTLESS_CREATE_CACHE_MIN_I32] = handle;
}

// NULL
uint32_t pointless_create_cache_get_null(pointless_create_cache_t* cache)
{
	return cache->null_handle;
}

void pointless_create_cache_set_null(pointless_create_cache_t* cache, uint32_t handle)
{
	cache->null_handle = handle;
}


// EMPTY_SLOT
uint32_t pointless_create_cache_get_empty_slot(pointless_create_cache_t* cache)
{
	return cache->empty_slot_handle;
}

void pointless_create_cache_set_empty_slot(pointless_create_cache_t* cache, uint32_t handle)
{
	cache->empty_slot_handle = handle;
}

// TRUE
uint32_t pointless_create_cache_get_true(pointless_create_cache_t* cache)
{
	return cache->true_handle;
}

void pointless_create_cache_set_true(pointless_create_cache_t* cache, uint32_t handle)
{
	cache->true_handle = handle;
}

// FALSE
uint32_t pointless_create_cache_get_false(pointless_create_cache_t* cache)
{
	return cache->false_handle;
}

void pointless_create_cache_set_false(pointless_create_cache_t* cache, uint32_t handle)
{
	cache->false_handle = handle;
}
