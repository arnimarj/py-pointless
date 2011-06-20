#ifndef __POINTLESS__TEST__H__
#define __POINTLESS__TEST__H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <pointless/pointless.h>
#include <pointless/pointless_recreate.h>

#define CHECK_HANDLE(handle) if (handle == POINTLESS_CREATE_VALUE_FAIL) { fprintf(stderr, #handle " creation failure"); exit(EXIT_FAILURE); }

// hash validator
void validate_hash_semantics();

// create/query test-cases
typedef void (*create_begin_cb)(pointless_create_t* c);
typedef void (*create_cb)(pointless_create_t* c);
typedef void (*query_cb)(pointless_t* p);

void create_wrapper(const char* fname, create_begin_cb begin_cb, create_cb cb);
void query_wrapper(const char* fname, query_cb cb);

void create_tuple(pointless_create_t* c);
void create_very_simple(pointless_create_t* c);
void create_simple(pointless_create_t* c);
void create_set(pointless_create_t* c);
void query_set(pointless_t* p);
void create_special_a(pointless_create_t* c);
void create_special_b(pointless_create_t* c);
void create_special_c(pointless_create_t* c);
void create_special_d(pointless_create_t* c);
void query_special_d(pointless_t* p);

// performance tests
void create_1M_set(pointless_create_t* c);
void query_1M_set(pointless_t* p);

#endif
