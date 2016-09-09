#ifndef __POINTLESS__DEFS__H__
#define __POINTLESS__DEFS__H__

#include <stdio.h>
#include <sys/stat.h>

#ifndef __cplusplus
#include <limits.h>
#include <stdint.h>
#else
#include <climits>
#include <cstdint>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <Judy.h>

#ifdef __cplusplus
}
#endif

#include <pointless/pointless_dynarray.h>
#include <pointless/pointless_create_cache.h>

#define POINTLESS_FILE_FORMAT_OLDEST_VERSION_ 0
#define POINTLESS_FILE_FORMAT_LATEST_VERSION_ 2

#define POINTLESS_FF_VERSION_OFFSET_32_OLDHASH 0
#define POINTLESS_FF_VERSION_OFFSET_32_NEWHASH 1
#define POINTLESS_FF_VERSION_OFFSET_64_NEWHASH 2

#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
/* These can't be used after statements in c89. */
#ifdef __COUNTER__
  /* microsoft */
  #define STATIC_ASSERT(e,m) \
    enum { ASSERT_CONCAT(static_assert_, __COUNTER__) = 1/(!!(e)) }
#else
  /* This can't be used twice on the same line so ensure if using in headers
   * that the headers are not included twice (by wrapping in #ifndef...#endif)
   * Note it doesn't cause an issue when used on same line of separate modules
   * compiled with gcc -combine -fwhole-program.  */
  #define STATIC_ASSERT(e,m) \
    enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
#endif

// POINTLESS_WCHAR_T_IS_4_BYTES
#if 2147483647 <= WCHAR_MAX && WCHAR_MAX <= 4294967295
#define POINTLESS_WCHAR_T_IS_4_BYTES
#endif

// we use 32-bits for unicodes, and 8-bits for those who can
#define pointless_string_char_t uint8_t
#define pointless_unicode_char_t uint32_t

// returned from create functions which fail
#define POINTLESS_CREATE_VALUE_FAIL UINT32_MAX

// maximum depth for recursive data structures, just some big-enough arbitrary value
#define POINTLESS_MAX_DEPTH (512)

// NOTE: the creation code depends on the relative ordering here, do not change without a careful code-review

// general/specialized vectors
#define POINTLESS_VECTOR_VALUE          0
#define POINTLESS_VECTOR_VALUE_HASHABLE 1
#define POINTLESS_VECTOR_I8             2
#define POINTLESS_VECTOR_U8             3
#define POINTLESS_VECTOR_I16            4
#define POINTLESS_VECTOR_U16            5
#define POINTLESS_VECTOR_I32            6
#define POINTLESS_VECTOR_U32            7
#define POINTLESS_VECTOR_I64            25
#define POINTLESS_VECTOR_U64            26
#define POINTLESS_VECTOR_FLOAT          8
#define POINTLESS_VECTOR_EMPTY          9

// unicode strings
#define POINTLESS_UNICODE_ 10
#define POINTLESS_STRING_ 29

// bitvector
#define POINTLESS_BITVECTOR        11
#define POINTLESS_BITVECTOR_0      12
#define POINTLESS_BITVECTOR_1      13
#define POINTLESS_BITVECTOR_01     14
#define POINTLESS_BITVECTOR_10     15
#define POINTLESS_BITVECTOR_PACKED 16

// general set/map and empty-slot marker
#define POINTLESS_SET_VALUE        17
#define POINTLESS_MAP_VALUE_VALUE  18
#define POINTLESS_EMPTY_SLOT       19

// primitve (in-line) types
#define POINTLESS_I32     20
#define POINTLESS_U32     21
#define POINTLESS_FLOAT   22
#define POINTLESS_BOOLEAN 23
#define POINTLESS_NULL    24

// these two are not really supported yet, but are used internally
#define POINTLESS_I64     27
#define POINTLESS_U64     28


#define PC_HEAP_OFFSET(p, offsets, i) ((char*)((p)->heap_ptr) + ((p)->is_32_offset ? ((p)->offsets##_32[i]) : ((p)->offsets##_64[i])))
#define PC_OFFSET(p, offsets, i)      (                         ((p)->is_32_offset ? ((p)->offsets##_32[i]) : ((p)->offsets##_64[i])))

typedef union {
	int32_t data_i32;
	uint32_t data_u32;
	float data_f;

	// bitvector compression schemes
	struct {
		unsigned int n_bits:5;
		unsigned int bits:27;
	} __attribute__ ((packed)) bitvector_packed;

	struct {
		uint16_t n_bits_a;
		uint16_t n_bits_b;
	} __attribute__ ((packed)) bitvector_01_or_10;
} __attribute__ ((packed)) pointless_value_data_t;

typedef union {
	int64_t data_i64;
	uint64_t data_u64;
	int32_t data_i32;
	uint32_t data_u32;
	float data_f;

	// bitvector compression schemes
	struct {
		unsigned int n_bits:5;
		unsigned int bits:27;
	} __attribute__ ((packed)) bitvector_packed;

	struct {
		uint16_t n_bits_a;
		uint16_t n_bits_b;
	} __attribute__ ((packed)) bitvector_01_or_10;
} __attribute__ ((packed)) pointless_complete_value_data_t;

// a base value (64-bits)
typedef struct {
	uint32_t type;                  // 32
	pointless_value_data_t data;    // 32
} __attribute__ ((packed)) pointless_value_t;

// a catch-all base value (96-bits)
typedef struct {
	uint32_t type;                                  // 32
	pointless_complete_value_data_t complete_data;  // 64
} __attribute__ ((packed)) pointless_complete_value_t;

typedef struct {
	uint32_t type_29:29;
	uint32_t is_compressed_vector:1; // true iff: vector was normal, but allows for compression
	uint32_t is_set_map_vector:1;    // true iff: buffer owned by caller
	uint32_t is_outside_vector:1;    // true iff: vector is used to hold set/map keys or values
} pointless_create_value_header_t;

// a base value
typedef struct {
	pointless_create_value_header_t header;
	pointless_value_data_t data;
} __attribute__ ((packed)) pointless_create_value_t;

// a base value
typedef struct {
	pointless_create_value_header_t header;
	pointless_complete_value_data_t complete_data;
} __attribute__ ((packed)) pointless_complete_create_value_t;

// convenience macros
#define ICEIL(a, div) (((a) % (div)) ? (((a) / (div)) + 1) : ((a) / (div)))
#define SIMPLE_CMP(a, b) (((a) == (b)) ? 0 : ((a) < (b) ? -1 : +1))
#define SIMPLE_MIN(a, b) ((a) < (b) ? (a) : (b))
#define SIMPLE_MAX(a, b) ((a) > (b) ? (a) : (b))


/*
FILE FORMAT
-----------

pointless_value_t: root
uint32_t: n_string_unicode
uint32_t: n_vectors
uint32_t: n_bitvectors
uint32_t: n_sets
uint32_t: n_maps

pointless_value_t[n_inline_values_and_refs]

uint32/64_t string_offsets[n_unicode + n_string]
uint32/64_t vector_offsets[n_vector]
uint32/64_t bitvector_offsets[n_bitvectors]
uint32/64_t set_offsets[n_sets]
uint32/64_t map_offsets[n_maps]

<HEAP>
*/

typedef struct {
	pointless_value_t root;
	uint32_t n_string_unicode;
	uint32_t n_vector;
	uint32_t n_bitvector;
	uint32_t n_set;
	uint32_t n_map;
	uint32_t version;
} __attribute__ ((aligned (4))) pointless_header_t;

typedef struct {
	// mmap, library owned
	FILE* fd;
	uint64_t fd_len;
	void* fd_ptr;

	// regular memory, library owned
	void* buf;
	size_t buflen;

	// header
	pointless_header_t* header;

	// offset vectors
	uint32_t* string_unicode_offsets_32;
	uint32_t* vector_offsets_32;
	uint32_t* bitvector_offsets_32;
	uint32_t* set_offsets_32;
	uint32_t* map_offsets_32;

	uint64_t* string_unicode_offsets_64;
	uint64_t* vector_offsets_64;
	uint64_t* bitvector_offsets_64;
	uint64_t* set_offsets_64;
	uint64_t* map_offsets_64;

	int is_32_offset;
	int is_64_offset;

	// base heap pointer
	void* heap_ptr;
	uint64_t heap_len;
} pointless_t;

typedef struct {
	uint32_t n_items;
	uint32_t padding;
	pointless_value_t hash_vector;
	pointless_value_t key_vector;
} __attribute__ ((aligned (4))) pointless_set_header_t;

typedef struct {
	uint32_t n_items;
	uint32_t padding;
	pointless_value_t hash_vector;
	pointless_value_t key_vector;
	pointless_value_t value_vector;
} __attribute__ ((aligned (4))) pointless_map_header_t;

STATIC_ASSERT(sizeof(pointless_value_data_t)            == 4,  "pointless_value_data_t must be 4 bytes");
STATIC_ASSERT(sizeof(pointless_complete_value_data_t)   == 8,  "pointless_complete_value_data_t must be 8 bytes");
STATIC_ASSERT(sizeof(pointless_value_t)                 == 8,  "pointless_value_t must be 8 bytes");
STATIC_ASSERT(sizeof(pointless_complete_value_t)        == 12, "pointless_complete_value_t must be 12 bytes");
STATIC_ASSERT(sizeof(pointless_create_value_t)          == 8,  "pointless_create_value_t must be 8 bytes");
STATIC_ASSERT(sizeof(pointless_complete_create_value_t) == 12, "pointless_complete_create_value_t must be 12 bytes");
STATIC_ASSERT(sizeof(pointless_header_t)                == 32, "pointless_header_t must be 32 bytes");
STATIC_ASSERT(sizeof(pointless_set_header_t)            == 24, "pointless_set_header_t must be 24 bytes");
STATIC_ASSERT(sizeof(pointless_map_header_t)            == 32, "pointless_map_header_t must be 32 bytes");

// pointless-owned vector
typedef struct {
	pointless_dynarray_t vector;   // POINTLESS_VECTOR_*
} pointless_create_vector_priv_t;

// user-owned vector
typedef struct {
	void* items;
	uint32_t n_items;
} pointless_create_vector_outside_t;

typedef struct {
	// used during creation phase
	pointless_dynarray_t keys;

	// used during serialization phase, no-one else touches these
	uint32_t serialize_hash;
	uint32_t serialize_keys;
} pointless_create_set_t;

typedef struct {
	// used during creation phase
	pointless_dynarray_t keys;
	pointless_dynarray_t values;

	// used during serialization phase, no-one else touches these
	uint32_t serialize_hash;
	uint32_t serialize_keys;
	uint32_t serialize_values;
} pointless_create_map_t;

typedef struct {
	// root, index into 'values', or UINT32_MAX
	uint32_t root;

	// primitive value cache
	pointless_create_cache_t cache;

	// value-create-id -> inline value or specific-value-id
	pointless_dynarray_t values;

	// priv-vector-create-id -> vector info
	pointless_dynarray_t priv_vector_values;

	// public-vector-create-id -> vector info
	pointless_dynarray_t outside_vector_values;

	// set-create-id -> set info
	pointless_dynarray_t set_values;

	// map-create-id -> map info
	pointless_dynarray_t map_values;

	// string/unicode-create-id -> string/unicode buffer (void*)
	pointless_dynarray_t string_unicode_values;

	// bitvector-create-id -> bitvector buffer (void*)
	pointless_dynarray_t bitvector_values;

	// string/unicode value -> unicode reference
	Pvoid_t string_unicode_map_judy;
	uint32_t string_unicode_map_judy_count;

	// bitvector value -> bitvector reference
	Pvoid_t bitvector_map_judy;
	uint32_t bitvector_map_judy_count;

	// file format version
	uint32_t version;
} pointless_create_t;

// create-time utility macros
#define cv_value_at(v) (&pointless_dynarray_ITEM_AT(pointless_create_value_t, &c->values, v))
#define cv_value_type(v) (&pointless_dynarray_ITEM_AT(pointless_create_value_t, &c->values, v))->header.type_29
#define cv_value_data_u32(v) (&pointless_dynarray_ITEM_AT(pointless_create_value_t, &c->values, v))->data.data_u32

#define cv_is_compressed_vector(v) (&pointless_dynarray_ITEM_AT(pointless_create_value_t, &c->values, v))->header.is_compressed_vector
#define cv_is_set_map_vector(v) (&pointless_dynarray_ITEM_AT(pointless_create_value_t, &c->values, v))->header.is_set_map_vector
#define cv_is_outside_vector(v) (&pointless_dynarray_ITEM_AT(pointless_create_value_t, &c->values, v))->header.is_outside_vector

#define cv_float_at(v) pointless_create_value_get_float(cv_value_at(v))
#define cv_i32_at(v) pointless_create_value_get_i32(cv_value_at(v))
#define cv_u32_at(v) pointless_create_value_get_u32(cv_value_at(v))

#define cv_priv_vector_at(v) (&pointless_dynarray_ITEM_AT(pointless_create_vector_priv_t, &c->priv_vector_values, cv_value_data_u32(v)))
#define cv_outside_vector_at(v) (&pointless_dynarray_ITEM_AT(pointless_create_vector_outside_t, &c->outside_vector_values, cv_value_data_u32(v)))
#define cv_set_at(v) (&pointless_dynarray_ITEM_AT(pointless_create_set_t, &c->set_values, cv_value_data_u32(v)))
#define cv_map_at(v) (&pointless_dynarray_ITEM_AT(pointless_create_map_t, &c->map_values, cv_value_data_u32(v)))
#define cv_bitvector_at(v) (*((void**)&pointless_dynarray_ITEM_AT(void*, &c->bitvector_values, cv_value_data_u32(v))))
#define cv_unicode_at(v) (*((void**)&pointless_dynarray_ITEM_AT(void*, &c->string_unicode_values, cv_value_data_u32(v))))
#define cv_string_at(v) (*((void**)&pointless_dynarray_ITEM_AT(void*, &c->string_unicode_values, cv_value_data_u32(v))))

#define cv_get_priv_vector(cv) (&pointless_dynarray_ITEM_AT(pointless_create_vector_priv_t, &c->priv_vector_values, (cv)->data.data_u32))
#define cv_get_outside_vector(cv) (&pointless_dynarray_ITEM_AT(pointless_create_vector_outside_t, &c->outside_vector_values, (cv)->data.data_u32))
#define cv_get_set(cv) (&pointless_dynarray_ITEM_AT(pointless_create_set_t, &c->set_values, (cv)->data.data_u32))
#define cv_get_map(cv) (&pointless_dynarray_ITEM_AT(pointless_create_map_t, &c->map_values, (cv)->data.data_u32))
#define cv_get_bitvector(cv) (*((void**)&pointless_dynarray_ITEM_AT(void*, &c->bitvector_values, (cv)->data.data_u32)))
#define cv_get_unicode(cv) (*((void**)&pointless_dynarray_ITEM_AT(void*, &c->string_unicode_values, (cv)->data.data_u32)))
#define cv_get_string(cv) (*((void**)&pointless_dynarray_ITEM_AT(void*, &c->string_unicode_values, (cv)->data.data_u32)))

// top-level type checkers
int32_t pointless_is_vector_type(uint32_t type);
int32_t pointless_is_bitvector_type(uint32_t type);
int32_t pointless_is_integer_type(uint32_t type);

// hash functions
typedef struct {
	uint32_t mult;
	uint32_t x;
	uint32_t len;
} pointless_vector_hash_state_32_t;

void pointless_vector_hash_init_32(pointless_vector_hash_state_32_t* state, uint32_t len);
void pointless_vector_hash_next_32(pointless_vector_hash_state_32_t* state, uint32_t hash);
uint32_t pointless_vector_hash_end_32(pointless_vector_hash_state_32_t* state);

uint32_t pointless_is_hashable(uint32_t type);

uint32_t pointless_hash_unicode_ucs4_v1_32(uint32_t* s);
uint32_t pointless_hash_unicode_ucs2_v1_32(uint16_t* s);
uint32_t pointless_hash_string_v1_32(uint8_t* s);
uint32_t pointless_hash_string_v1_32_(uint8_t* s, size_t n);

uint32_t pointless_hash_float_32(float f);
uint32_t pointless_hash_i32_32(int32_t i);
uint32_t pointless_hash_u32_32(uint32_t i);
uint32_t pointless_hash_i64_32(int64_t i);
uint32_t pointless_hash_u64_32(uint64_t i);
uint32_t pointless_hash_bool_true_32();
uint32_t pointless_hash_bool_false_32();
uint32_t pointless_hash_null_32();
uint32_t pointless_hash_reader_32(pointless_t* p, pointless_value_t* v);
uint32_t pointless_hash_reader_vector_32(pointless_t* p, pointless_value_t* v, uint32_t i, uint32_t n);
uint32_t pointless_hash_create_32(pointless_create_t* c, pointless_create_value_t* v);

// comparison functions
#ifdef POINTLESS_WCHAR_T_IS_4_BYTES
int32_t pointless_cmp_wchar_wchar(wchar_t* a, wchar_t* b);
#endif

int32_t pointless_cmp_string_8_8(uint8_t* a, uint8_t* b);
int32_t pointless_cmp_string_8_16(uint8_t* a, uint16_t* b);
int32_t pointless_cmp_string_8_32(uint8_t* a, uint32_t* b);
int32_t pointless_cmp_string_16_8(uint16_t* a, uint8_t* b);
int32_t pointless_cmp_string_16_16(uint16_t* a, uint16_t* b);
int32_t pointless_cmp_string_16_32(uint16_t* a, uint32_t* b);
int32_t pointless_cmp_string_32_8(uint32_t* a, uint8_t* b);
int32_t pointless_cmp_string_32_16(uint32_t* a, uint16_t* b);
int32_t pointless_cmp_string_32_32(uint32_t* a, uint32_t* b);

int32_t pointless_cmp_string_8_8_n(uint8_t* a, uint8_t* b, size_t n_b);
int32_t pointless_cmp_string_32_8_n(uint32_t* a, uint8_t* b, size_t n_b);

int32_t pointless_cmp_reader(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b, const char** error);
int32_t pointless_cmp_reader_acyclic(pointless_t* p_a, pointless_complete_value_t* a, pointless_t* p_b, pointless_complete_value_t* b);
int32_t pointless_cmp_create(pointless_create_t* c, uint32_t a, uint32_t b, const char** error);

// equality test callback
typedef uint32_t (*pointless_eq_cb)(pointless_t* p, pointless_complete_value_t* v, void* user, const char** error);

#endif
