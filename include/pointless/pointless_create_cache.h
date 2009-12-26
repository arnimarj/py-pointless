#ifndef __POINTLESS__CREATE__CACHE__H__
#define __POINTLESS__CREATE__CACHE__H__

#include <pointless/pointless_defs.h>

// initialize cache
void pointless_create_cache_init(pointless_create_cache_t* cache);

// U32
uint32_t pointless_create_cache_get_u32(pointless_create_cache_t* cache, uint32_t u32);
void pointless_create_cache_set_u32(pointless_create_cache_t* cache, uint32_t u32, uint32_t handle);

// I32
uint32_t pointless_create_cache_get_i32(pointless_create_cache_t* cache, int32_t i32);
void pointless_create_cache_set_i32(pointless_create_cache_t* cache, int32_t i32, uint32_t handle);

// NULL
uint32_t pointless_create_cache_get_null(pointless_create_cache_t* cache);
void pointless_create_cache_set_null(pointless_create_cache_t* cache, uint32_t handle);

// EMPTY_SLOT
uint32_t pointless_create_cache_get_empty_slot(pointless_create_cache_t* cache);
void pointless_create_cache_set_empty_slot(pointless_create_cache_t* cache, uint32_t handle);

// TRUE
uint32_t pointless_create_cache_get_true(pointless_create_cache_t* cache);
void pointless_create_cache_set_true(pointless_create_cache_t* cache, uint32_t handle);

// FALSE
uint32_t pointless_create_cache_get_false(pointless_create_cache_t* cache);
void pointless_create_cache_set_false(pointless_create_cache_t* cache, uint32_t handle);

#endif
