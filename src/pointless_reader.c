#include <pointless/pointless_reader.h>

static int pointless_init(pointless_t* p, void* buf, uint64_t buflen, int force_ucs2, int do_validate, const char** error)
{
	// our header
	if (buflen < sizeof(pointless_header_t)) {
		*error = "header missing";
		return 0;
	}

	p->header = (pointless_header_t*)buf;

	// check for version
	p->is_32_offset = 0;
	p->is_64_offset = 0;

	switch (p->header->version) {
		case POINTLESS_FF_VERSION_OFFSET_32_OLDHASH:
			*error = "old-hash file version not supported";
			return 0;
		case POINTLESS_FF_VERSION_OFFSET_32_NEWHASH:
			p->is_32_offset = 1;
			break;
		case POINTLESS_FF_VERSION_OFFSET_64_NEWHASH:
			p->is_64_offset = 1;
			break;
		default:
			*error = "file version not supported";
			return 0;
	}

	// right, we need some number of bytes for the offset vectors
	uint64_t mandatory_size = sizeof(pointless_header_t);

	if (p->is_32_offset) {
		mandatory_size += p->header->n_string_unicode * sizeof(uint32_t);
		mandatory_size += p->header->n_vector * sizeof(uint32_t);
		mandatory_size += p->header->n_bitvector * sizeof(uint32_t);
		mandatory_size += p->header->n_set * sizeof(uint32_t);
		mandatory_size += p->header->n_map * sizeof(uint32_t);
	} else {
		mandatory_size += p->header->n_string_unicode * sizeof(uint64_t);
		mandatory_size += p->header->n_vector * sizeof(uint64_t);
		mandatory_size += p->header->n_bitvector * sizeof(uint64_t);
		mandatory_size += p->header->n_set * sizeof(uint64_t);
		mandatory_size += p->header->n_map * sizeof(uint64_t);
	}

	if (buflen < mandatory_size) {
		*error = "file is too small to hold offset vectors";
		return 0;
	}

	// offset vectors
	p->string_unicode_offsets_32   = (uint32_t*)(p->header + 1);
	p->vector_offsets_32           = (uint32_t*)(p->string_unicode_offsets_32   + p->header->n_string_unicode);
	p->bitvector_offsets_32        = (uint32_t*)(p->vector_offsets_32           + p->header->n_vector);
	p->set_offsets_32              = (uint32_t*)(p->bitvector_offsets_32        + p->header->n_bitvector);
	p->map_offsets_32              = (uint32_t*)(p->set_offsets_32              + p->header->n_set);

	p->string_unicode_offsets_64   = (uint64_t*)(p->header + 1);
	p->vector_offsets_64           = (uint64_t*)(p->string_unicode_offsets_64   + p->header->n_string_unicode);
	p->bitvector_offsets_64        = (uint64_t*)(p->vector_offsets_64           + p->header->n_vector);
	p->set_offsets_64              = (uint64_t*)(p->bitvector_offsets_64        + p->header->n_bitvector);
	p->map_offsets_64              = (uint64_t*)(p->set_offsets_64              + p->header->n_set);

	// our heap
	p->heap_len = (buflen - mandatory_size);
	p->heap_ptr = 0;

	if (p->is_32_offset)
		p->heap_ptr = (void*)(p->map_offsets_32 + p->header->n_map);
	else
		p->heap_ptr = (void*)(p->map_offsets_64 + p->header->n_map);

	// let us validate the damn thing
	pointless_validate_context_t context;
	context.p = p;
	context.force_ucs2 = force_ucs2;

	if (do_validate)
		return pointless_validate(&context, error);
	else
		return 1;
}

static int _pointless_open_f(pointless_t* p, const char* fname, int force_ucs2, int do_validate, const char** error)
{
	p->fd = 0;
	p->fd_len = 0;
	p->fd_ptr = 0;

	p->buf = 0;
	p->buflen = 0;

	p->fd = fopen(fname, "rb");

	if (p->fd == 0) {
		switch (errno) {
			case EACCES:       *error = "fopen(): EACCES";       break;
			case EINTR:        *error = "fopen(): EINTR";        break;
			case EINVAL:       *error = "fopen(): EINVAL";       break;
			case EISDIR:       *error = "fopen(): EISDIR";       break;
			case ELOOP:        *error = "fopen(): ELOOP";        break;
			case EMFILE:       *error = "fopen(): EMFILE";       break;
			case ENAMETOOLONG: *error = "fopen(): ENAMETOOLONG"; break;
			case ENFILE:       *error = "fopen(): ENFILE";       break;
			case ENOENT:       *error = "fopen(): ENOENT";       break;
			case ENOMEM:       *error = "fopen(): ENOMEM";       break;
			case ENOSPC:       *error = "fopen(): ENOSPC";       break;
			case ENOTDIR:      *error = "fopen(): ENOTDIR";      break;
			case ENXIO:        *error = "fopen(): ENXIO";        break;
			case EOVERFLOW:    *error = "fopen(): EOVERFLOW";    break;
			case EROFS:        *error = "fopen(): EROFS";        break;
			case ETXTBSY:      *error = "fopen(): ETXTBSY";      break;
			default:           *error = "fopen(): error";        break;
		}

		pointless_close(p);
		return 0;
	}

	struct stat s;

	if (fstat(fileno(p->fd), &s) != 0) {
		*error = "fstat error";
		pointless_close(p);
		return 0;
	}

	if (s.st_size == 0) {
		*error = "file is empty";
		pointless_close(p);
		return 0;
	}

	p->fd_len = s.st_size;
	p->fd_ptr = mmap(0, p->fd_len, PROT_READ, MAP_SHARED, fileno(p->fd), 0);

	if (p->fd_ptr == MAP_FAILED) {
		*error = "mmap error";
		pointless_close(p);
		return 0;
	}

	if (!pointless_init(p, p->fd_ptr, p->fd_len, force_ucs2, do_validate, error)) {
		pointless_close(p);
		return 0;
	}

	return 1;
}

int pointless_open_f(pointless_t* p, const char* fname, int force_ucs2, const char** error)
{
	return _pointless_open_f(p, fname, force_ucs2, 1, error);
}

int pointless_open_f_skip_validate(pointless_t* p, const char* fname, int force_ucs2, const char** error)
{
	return _pointless_open_f(p, fname, force_ucs2, 0, error);
}

void pointless_close(pointless_t* p)
{
	if (p->fd_ptr)
		munmap(p->fd_ptr, p->fd_len);

	if (p->fd)
		fclose(p->fd);

	pointless_free(p->buf);
}

static int _pointless_open_b(pointless_t* p, const void* buffer, size_t n_buffer, int force_ucs2, int do_validate, const char** error)
{
	p->fd = 0;
	p->fd_len = 0;
	p->fd_ptr = 0;

	p->buf = pointless_malloc(n_buffer);
	p->buflen = n_buffer;

	if (p->buf == 0) {
		*error = "out of memory";
		return 0;
	}

	memcpy(p->buf, buffer, n_buffer);

	if (!pointless_init(p, p->buf, p->buflen, force_ucs2, do_validate, error)) {
		pointless_close(p);
		return 0;
	}

	return 1;
}

int pointless_open_b(pointless_t* p, const void* buffer, size_t n_buffer, int force_ucs2, const char** error)
{
	return _pointless_open_b(p, buffer, n_buffer, force_ucs2, 1, error);
}

int pointless_open_b_skip_validate(pointless_t* p, const void* buffer, size_t n_buffer, int force_ucs2, const char** error)
{
	return _pointless_open_b(p, buffer, n_buffer, force_ucs2, 0, error);
}

pointless_value_t* pointless_root(pointless_t* p)
{
	return &p->header->root;
}

uint32_t pointless_reader_unicode_len(pointless_t* p, pointless_value_t* v)
{
	assert(v->data.data_u32 < p->header->n_string_unicode);
	uint32_t* u_len = (uint32_t*)PC_HEAP_OFFSET(p, string_unicode_offsets, v->data.data_u32);
	return *u_len;
}

static pointless_unicode_char_t* pointless_reader_unicode_value(pointless_t* p, pointless_value_t* v)
{
	assert(v->data.data_u32 < p->header->n_string_unicode);
	uint32_t* u_len = (uint32_t*)PC_HEAP_OFFSET(p, string_unicode_offsets, v->data.data_u32);
	return (pointless_unicode_char_t*)(u_len + 1);
}

uint32_t* pointless_reader_unicode_value_ucs4(pointless_t* p, pointless_value_t* v)
{
	assert(((size_t)pointless_reader_unicode_value(p, v) % 4) == 0);
	return pointless_reader_unicode_value(p, v);
}

#ifdef POINTLESS_WCHAR_T_IS_4_BYTES
wchar_t* pointless_reader_unicode_value_wchar(pointless_t* p, pointless_value_t* v)
{
	return (wchar_t*)pointless_reader_unicode_value_ucs4(p, v);
}
#endif

uint16_t* pointless_reader_unicode_value_ucs2_alloc(pointless_t* p, pointless_value_t* v, const char** error)
{
	uint16_t* s = pointless_ucs4_to_ucs2(pointless_reader_unicode_value_ucs4(p, v));

	if (s == 0)
		*error = "out of memory";

	return s;
}

uint32_t pointless_reader_string_len(pointless_t* p, pointless_value_t* v)
{
	assert(v->data.data_u32 < p->header->n_string_unicode);
	uint32_t* u_len = (uint32_t*)PC_HEAP_OFFSET(p, string_unicode_offsets, v->data.data_u32);
	return *u_len;
}

uint8_t* pointless_reader_string_value_ascii(pointless_t* p, pointless_value_t* v)
{
	assert(v->data.data_u32 < p->header->n_string_unicode);
	uint32_t* u_len = (uint32_t*)PC_HEAP_OFFSET(p, string_unicode_offsets, v->data.data_u32);
	return (uint8_t*)(u_len + 1);
}

uint32_t pointless_reader_vector_n_items(pointless_t* p, pointless_value_t* v)
{
	if (v->type == POINTLESS_VECTOR_EMPTY)
		return 0;

	assert(v->data.data_u32 < p->header->n_vector);
	uint32_t* v_len = (uint32_t*)PC_HEAP_OFFSET(p, vector_offsets, v->data.data_u32);

	return *v_len;
}

static void* pointless_reader_vector_base_ptr(pointless_t* p, pointless_value_t* v)
{
	if (v->type == POINTLESS_VECTOR_EMPTY)
		return 0;

	assert(v->data.data_u32 < p->header->n_vector);
	uint32_t* v_len = (uint32_t*)PC_HEAP_OFFSET(p, vector_offsets, v->data.data_u32);
	return (void*)(v_len + 1);
}

pointless_value_t* pointless_reader_vector_value(pointless_t* p, pointless_value_t* v)
{
	assert(v->type == POINTLESS_VECTOR_VALUE || v->type == POINTLESS_VECTOR_VALUE_HASHABLE || v->type == POINTLESS_VECTOR_EMPTY);
	assert((size_t)pointless_reader_vector_base_ptr(p, v) % 4 == 0);
	return (pointless_value_t*)pointless_reader_vector_base_ptr(p, v);
}

int8_t* pointless_reader_vector_i8(pointless_t* p, pointless_value_t* v)
{
	assert(v->type == POINTLESS_VECTOR_I8);
	assert((size_t)pointless_reader_vector_base_ptr(p, v) % 4 == 0);
	return (int8_t*)pointless_reader_vector_base_ptr(p, v);
}

uint8_t* pointless_reader_vector_u8(pointless_t* p, pointless_value_t* v)
{
	assert(v->type == POINTLESS_VECTOR_U8);
	assert((size_t)pointless_reader_vector_base_ptr(p, v) % 4 == 0);
	return (uint8_t*)pointless_reader_vector_base_ptr(p, v);
}

int16_t* pointless_reader_vector_i16(pointless_t* p, pointless_value_t* v)
{
	assert(v->type == POINTLESS_VECTOR_I16);
	assert((size_t)pointless_reader_vector_base_ptr(p, v) % 4 == 0);
	return (int16_t*)pointless_reader_vector_base_ptr(p, v);
}

uint16_t* pointless_reader_vector_u16(pointless_t* p, pointless_value_t* v)
{
	assert(v->type == POINTLESS_VECTOR_U16);
	assert((size_t)pointless_reader_vector_base_ptr(p, v) % 4 == 0);
	return (uint16_t*)pointless_reader_vector_base_ptr(p, v);
}

int32_t* pointless_reader_vector_i32(pointless_t* p, pointless_value_t* v)
{
	assert(v->type == POINTLESS_VECTOR_I32);
	assert((size_t)pointless_reader_vector_base_ptr(p, v) % 4 == 0);
	return (int32_t*)pointless_reader_vector_base_ptr(p, v);
}

uint32_t* pointless_reader_vector_u32(pointless_t* p, pointless_value_t* v)
{
	assert(v->type == POINTLESS_VECTOR_U32);
	assert((size_t)pointless_reader_vector_base_ptr(p, v) % 4 == 0);
	return (uint32_t*)pointless_reader_vector_base_ptr(p, v);
}

int64_t* pointless_reader_vector_i64(pointless_t* p, pointless_value_t* v)
{
	assert(v->type == POINTLESS_VECTOR_I64);
	assert((size_t)pointless_reader_vector_base_ptr(p, v) % 4 == 0);
	return (int64_t*)pointless_reader_vector_base_ptr(p, v);
}

uint64_t* pointless_reader_vector_u64(pointless_t* p, pointless_value_t* v)
{
	assert(v->type == POINTLESS_VECTOR_U64);
	assert((size_t)pointless_reader_vector_base_ptr(p, v) % 4 == 0);
	return (uint64_t*)pointless_reader_vector_base_ptr(p, v);
}

float* pointless_reader_vector_float(pointless_t* p, pointless_value_t* v)
{
	assert(v->type == POINTLESS_VECTOR_FLOAT);
	assert((size_t)pointless_reader_vector_base_ptr(p, v) % 4 == 0);
	return (float*)pointless_reader_vector_base_ptr(p, v);
}

pointless_complete_value_t pointless_reader_vector_value_case(pointless_t* p, pointless_value_t* v, uint32_t i)
{
	assert(pointless_is_vector_type(v->type));

	switch (v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			return pointless_value_to_complete(pointless_reader_vector_value(p, v) + i);
		case POINTLESS_VECTOR_I8:
			return pointless_complete_value_create_as_read_i32((int32_t)(pointless_reader_vector_i8(p, v)[i]));
		case POINTLESS_VECTOR_U8:
			return pointless_complete_value_create_as_read_u32((uint32_t)(pointless_reader_vector_u8(p, v)[i]));
		case POINTLESS_VECTOR_I16:
			return pointless_complete_value_create_as_read_i32((int32_t)(pointless_reader_vector_i16(p, v)[i]));
		case POINTLESS_VECTOR_U16:
			return pointless_complete_value_create_as_read_u32((uint32_t)(pointless_reader_vector_u16(p, v)[i]));
		case POINTLESS_VECTOR_I32:
			return pointless_complete_value_create_as_read_i32(pointless_reader_vector_i32(p, v)[i]);
		case POINTLESS_VECTOR_U32:
			return pointless_complete_value_create_as_read_u32(pointless_reader_vector_u32(p, v)[i]);
		case POINTLESS_VECTOR_I64:
			return pointless_complete_value_create_as_read_i64(pointless_reader_vector_i64(p, v)[i]);
		case POINTLESS_VECTOR_U64:
			return pointless_complete_value_create_as_read_u64(pointless_reader_vector_u64(p, v)[i]);
		case POINTLESS_VECTOR_FLOAT:
			return pointless_complete_value_create_as_read_float(pointless_reader_vector_float(p, v)[i]);
	}

	assert(0);
	return pointless_complete_value_create_as_read_null();
}

uint32_t pointless_reader_bitvector_n_bits(pointless_t* p, pointless_value_t* v)
{
	void* buffer = 0;

	if (v->type == POINTLESS_BITVECTOR) {
		assert(v->data.data_u32 < p->header->n_bitvector);
		buffer = (void*)PC_HEAP_OFFSET(p, bitvector_offsets, v->data.data_u32);
	}

	assert((size_t)buffer % 4 == 0);

	return pointless_bitvector_n_bits(v->type, &v->data, buffer);
}

uint32_t pointless_reader_bitvector_is_set(pointless_t* p, pointless_value_t* v, uint32_t bit)
{
	assert(bit < pointless_reader_bitvector_n_bits(p, v));

	void* buffer = 0;

	if (v->type == POINTLESS_BITVECTOR) {
		assert(v->data.data_u32 < p->header->n_bitvector);
		buffer = (void*)PC_HEAP_OFFSET(p, bitvector_offsets, v->data.data_u32);
	}

	assert((size_t)buffer % 4 == 0);

	return pointless_bitvector_is_set(v->type, &v->data, buffer, bit);
}

void* pointless_reader_bitvector_buffer(pointless_t* p, pointless_value_t* v)
{
	return (void*)PC_HEAP_OFFSET(p, bitvector_offsets, v->data.data_u32);
}

// sets
uint32_t pointless_reader_set_n_items(pointless_t* p, pointless_value_t* s)
{
	assert(s->type == POINTLESS_SET_VALUE);
	pointless_set_header_t* header = (pointless_set_header_t*)PC_HEAP_OFFSET(p, set_offsets, s->data.data_u32);
	assert((size_t)header % 4 == 0);
	return header->n_items;
}

uint32_t pointless_reader_set_n_buckets(pointless_t* p, pointless_value_t* s)
{
	assert(s->type == POINTLESS_SET_VALUE);
	pointless_set_header_t* header = (pointless_set_header_t*)PC_HEAP_OFFSET(p, set_offsets, s->data.data_u32);
	assert(pointless_reader_vector_n_items(p, &header->key_vector) == pointless_reader_vector_n_items(p, &header->hash_vector));
	assert((size_t)header % 4 == 0);
	return pointless_reader_vector_n_items(p, &header->key_vector);
}

uint32_t pointless_reader_set_iter(pointless_t* p, pointless_value_t* s, pointless_value_t** k, uint32_t* iter_state)
{
	assert(s->type == POINTLESS_SET_VALUE);
	pointless_set_header_t* header = (pointless_set_header_t*)PC_HEAP_OFFSET(p, set_offsets, s->data.data_u32);
	assert((size_t)header % 4 == 0);
	assert(header->key_vector.type == POINTLESS_VECTOR_VALUE_HASHABLE);
	uint32_t n_buckets = pointless_reader_vector_n_items(p, &header->key_vector);

	while (*iter_state < n_buckets) {
		*k = &pointless_reader_vector_value(p, &header->key_vector)[*iter_state];
		*iter_state += 1;

		if ((*k)->type != POINTLESS_EMPTY_SLOT)
			return 1;
	}

	return 0;
}

void pointless_reader_set_lookup(pointless_t* p, pointless_value_t* s, pointless_value_t* k, pointless_value_t** kk, const char** error)
{
	// value must be hashable
	if (!pointless_is_hashable(k->type)) {
		*error = "value is not hashable";
		return;
	}

	// this must be a set
	assert(s->type == POINTLESS_SET_VALUE);
	pointless_set_header_t* header = (pointless_set_header_t*)PC_HEAP_OFFSET(p, set_offsets, s->data.data_u32);
	assert((size_t)header % 4 == 0);

	// value hash
	uint32_t hash = pointless_hash_reader_32(p, k);

	// other info
	uint32_t* hash_vector = pointless_reader_vector_u32(p, &header->hash_vector);
	pointless_value_t* key_vector = pointless_reader_vector_value(p, &header->key_vector);

	uint32_t n_buckets = pointless_reader_vector_n_items(p, &header->key_vector);

	// do the probe
	uint32_t probe = pointless_hash_table_probe(p, hash, k, n_buckets, hash_vector, key_vector, error);

	if (probe == POINTLESS_HASH_TABLE_PROBE_ERROR || probe == POINTLESS_HASH_TABLE_PROBE_MISS)
		*kk = 0;
	else
		*kk = &key_vector[probe];
}

void pointless_reader_set_lookup_ext(pointless_t* p, pointless_value_t* s, uint32_t hash, pointless_eq_cb cb, void* user, pointless_value_t** kk, const char** error)
{
	// this must be a set
	assert(s->type == POINTLESS_SET_VALUE);
	pointless_set_header_t* header = (pointless_set_header_t*)PC_HEAP_OFFSET(p, set_offsets, s->data.data_u32);
	assert((size_t)header % 4 == 0);

	// other info
	uint32_t* hash_vector = pointless_reader_vector_u32(p, &header->hash_vector);
	pointless_value_t* key_vector = pointless_reader_vector_value(p, &header->key_vector);

	uint32_t n_buckets = pointless_reader_vector_n_items(p, &header->key_vector);

	// do the probe
	uint32_t probe = pointless_hash_table_probe_ext(p, hash, cb, user, n_buckets, hash_vector, key_vector, error);

	if (probe == POINTLESS_HASH_TABLE_PROBE_ERROR || probe == POINTLESS_HASH_TABLE_PROBE_MISS)
		*kk = 0;
	else
		*kk = &key_vector[probe];
}

pointless_value_t* pointless_set_hash_vector(pointless_t* p, pointless_value_t* s)
{
	// this must be a set
	assert(s->type == POINTLESS_SET_VALUE);
	pointless_set_header_t* header = (pointless_set_header_t*)PC_HEAP_OFFSET(p, set_offsets, s->data.data_u32);
	assert((size_t)header % 4 == 0);

	return &header->hash_vector;
}

pointless_value_t* pointless_set_key_vector(pointless_t* p, pointless_value_t* s)
{
	// this must be a set
	assert(s->type == POINTLESS_SET_VALUE);
	pointless_set_header_t* header = (pointless_set_header_t*)PC_HEAP_OFFSET(p, set_offsets, s->data.data_u32);
	assert((size_t)header % 4 == 0);

	return &header->key_vector;
}

uint32_t pointless_reader_map_n_items(pointless_t* p, pointless_value_t* m)
{
	assert(m->type == POINTLESS_MAP_VALUE_VALUE);
	pointless_map_header_t* header = (pointless_map_header_t*)PC_HEAP_OFFSET(p, map_offsets, m->data.data_u32);
	assert((size_t)header % 4 == 0);
	return header->n_items;
}

uint32_t pointless_reader_map_n_buckets(pointless_t* p, pointless_value_t* m)
{
	assert(m->type == POINTLESS_MAP_VALUE_VALUE);
	pointless_map_header_t* header = (pointless_map_header_t*)PC_HEAP_OFFSET(p, map_offsets, m->data.data_u32);
	assert(pointless_reader_vector_n_items(p, &header->hash_vector) == pointless_reader_vector_n_items(p, &header->key_vector));
	assert(pointless_reader_vector_n_items(p, &header->key_vector) == pointless_reader_vector_n_items(p, &header->value_vector));
	assert((size_t)header % 4 == 0);
	return pointless_reader_vector_n_items(p, &header->key_vector);
}


uint32_t pointless_reader_map_iter(pointless_t* p, pointless_value_t* m, pointless_value_t** k, pointless_value_t** v, uint32_t* iter_state)
{
	assert(m->type == POINTLESS_MAP_VALUE_VALUE);
	pointless_map_header_t* header = (pointless_map_header_t*)PC_HEAP_OFFSET(p, map_offsets, m->data.data_u32);
	assert(header->key_vector.type == POINTLESS_VECTOR_VALUE_HASHABLE);
	assert(header->value_vector.type == POINTLESS_VECTOR_VALUE || header->value_vector.type == POINTLESS_VECTOR_VALUE_HASHABLE);
	uint32_t n_buckets = pointless_reader_vector_n_items(p, &header->key_vector);
	assert((size_t)header % 4 == 0);

	while (*iter_state < n_buckets) {
		*k = &pointless_reader_vector_value(p, &header->key_vector)[*iter_state];
		*v = &pointless_reader_vector_value(p, &header->value_vector)[*iter_state];
		*iter_state += 1;

		if ((*k)->type != POINTLESS_EMPTY_SLOT)
			return 1;
	}

	return 0;
}

void pointless_reader_map_iter_hash_init(pointless_t* p, pointless_value_t* m, uint32_t hash, pointless_hash_iter_state_t* iter_state)
{
	assert(m->type == POINTLESS_MAP_VALUE_VALUE);
	pointless_map_header_t* header = (pointless_map_header_t*)PC_HEAP_OFFSET(p, map_offsets, m->data.data_u32);
	assert(header->hash_vector.type == POINTLESS_VECTOR_U32);
	uint32_t n_buckets = pointless_reader_vector_n_items(p, &header->hash_vector);
	assert((size_t)header % 4 == 0);
	pointless_hash_table_probe_hash_init(p, hash, n_buckets, iter_state);
}

uint32_t pointless_reader_map_iter_hash(pointless_t* p, pointless_value_t* m, uint32_t hash, pointless_value_t** kk, pointless_value_t** vv, pointless_hash_iter_state_t* iter_state)
{
	assert(m->type == POINTLESS_MAP_VALUE_VALUE);
	pointless_map_header_t* header = (pointless_map_header_t*)PC_HEAP_OFFSET(p, map_offsets, m->data.data_u32);
	assert(header->hash_vector.type == POINTLESS_VECTOR_U32);
	assert(header->key_vector.type == POINTLESS_VECTOR_VALUE_HASHABLE);
	assert(header->value_vector.type == POINTLESS_VECTOR_VALUE || header->value_vector.type == POINTLESS_VECTOR_VALUE_HASHABLE);

	assert((size_t)header % 4 == 0);

	// get vector pointers
	uint32_t* hash_vector = pointless_reader_vector_u32(p, &header->hash_vector);
	pointless_value_t* key_vector = pointless_reader_vector_value(p, &header->key_vector);
	pointless_value_t* value_vector = pointless_reader_vector_value(p, &header->value_vector);

	// probe until we hit an empty bucket, or a matching hash(again)
	uint32_t bucket_out = 0;

	while (pointless_hash_table_probe_hash(p, hash_vector, key_vector, iter_state, &bucket_out)) {
		if (hash_vector[bucket_out] == hash) {
			*kk = &key_vector[bucket_out];
			*vv = &value_vector[bucket_out];
			return 1;
		}
	}

	return 0;
}

void pointless_reader_set_iter_hash_init(pointless_t* p, pointless_value_t* s, uint32_t hash, pointless_hash_iter_state_t* iter_state)
{
	assert(s->type == POINTLESS_SET_VALUE);
	pointless_set_header_t* header = (pointless_set_header_t*)PC_HEAP_OFFSET(p, set_offsets, s->data.data_u32);
	assert(header->hash_vector.type == POINTLESS_VECTOR_U32);
	uint32_t n_buckets = pointless_reader_vector_n_items(p, &header->hash_vector);
	assert((size_t)header % 4 == 0);
	pointless_hash_table_probe_hash_init(p, hash, n_buckets, iter_state);
}

uint32_t pointless_reader_set_iter_hash(pointless_t* p, pointless_value_t* s, uint32_t hash, pointless_value_t** kk, pointless_hash_iter_state_t* iter_state)
{
	assert(s->type == POINTLESS_SET_VALUE);
	pointless_set_header_t* header = (pointless_set_header_t*)PC_HEAP_OFFSET	(p, set_offsets, s->data.data_u32);
	assert(header->hash_vector.type == POINTLESS_VECTOR_U32);
	assert(header->key_vector.type == POINTLESS_VECTOR_VALUE_HASHABLE);

	assert((size_t)header % 4 == 0);

	// get vector pointers
	uint32_t* hash_vector = pointless_reader_vector_u32(p, &header->hash_vector);
	pointless_value_t* key_vector = pointless_reader_vector_value(p, &header->key_vector);

	// probe until we hit an empty bucket, or a matching hash(again)
	uint32_t bucket_out = 0;

	while (pointless_hash_table_probe_hash(p, hash_vector, key_vector, iter_state, &bucket_out)) {
		if (hash_vector[bucket_out] == hash) {
			*kk = &key_vector[bucket_out];
			return 1;
		}
	}

	return 0;
}

void pointless_reader_map_lookup(pointless_t* p, pointless_value_t* m, pointless_value_t* k, pointless_value_t** kk, pointless_value_t** vv, const char** error)
{
	// value must be hashable
	if (!pointless_is_hashable(k->type)) {
		*error = "value is not hashable";
		return;
	}

	// this must be a map
	assert(m->type == POINTLESS_MAP_VALUE_VALUE);
	pointless_map_header_t* header = (pointless_map_header_t*)PC_HEAP_OFFSET(p, map_offsets, m->data.data_u32);
	assert((size_t)header % 4 == 0);

	// value hash
	uint32_t hash = pointless_hash_reader_32(p, k);

	// other info
	uint32_t* hash_vector = pointless_reader_vector_u32(p, &header->hash_vector);
	pointless_value_t* key_vector = pointless_reader_vector_value(p, &header->key_vector);
	pointless_value_t* value_vector = pointless_reader_vector_value(p, &header->value_vector);

	uint32_t n_buckets = pointless_reader_vector_n_items(p, &header->key_vector);

	// do the probe
	uint32_t probe = pointless_hash_table_probe(p, hash, k, n_buckets, hash_vector, key_vector, error);

	if (probe == POINTLESS_HASH_TABLE_PROBE_ERROR || probe == POINTLESS_HASH_TABLE_PROBE_MISS) {
		*kk = 0;
		*vv = 0;
	} else {
		*kk = &key_vector[probe];
		*vv = &value_vector[probe];
	}
}

void pointless_reader_map_lookup_ext(pointless_t* p, pointless_value_t* m, uint32_t hash, pointless_eq_cb cb, void* user, pointless_value_t** kk, pointless_value_t** vv, const char** error)
{
	// this must be a map
	assert(m->type == POINTLESS_MAP_VALUE_VALUE);
	pointless_map_header_t* header = (pointless_map_header_t*)PC_HEAP_OFFSET(p, map_offsets, m->data.data_u32);
	assert((size_t)header % 4 == 0);

	// other info
	uint32_t* hash_vector = pointless_reader_vector_u32(p, &header->hash_vector);
	pointless_value_t* key_vector = pointless_reader_vector_value(p, &header->key_vector);
	pointless_value_t* value_vector = pointless_reader_vector_value(p, &header->value_vector);

	uint32_t n_buckets = pointless_reader_vector_n_items(p, &header->key_vector);

	// do the probe
	uint32_t probe = pointless_hash_table_probe_ext(p, hash, cb, user, n_buckets, hash_vector, key_vector, error);

	if (probe == POINTLESS_HASH_TABLE_PROBE_ERROR || probe == POINTLESS_HASH_TABLE_PROBE_MISS) {
		*kk = 0;
		*vv = 0;
	} else {
		*kk = &key_vector[probe];
		*vv = &value_vector[probe];
	}
}

pointless_value_t* pointless_map_hash_vector(pointless_t* p, pointless_value_t* m)
{
	// this must be a map
	assert(m->type == POINTLESS_MAP_VALUE_VALUE);
	pointless_map_header_t* header = (pointless_map_header_t*)PC_HEAP_OFFSET(p, map_offsets, m->data.data_u32);
	assert((size_t)header % 4 == 0);

	return &header->hash_vector;
}

pointless_value_t* pointless_map_key_vector(pointless_t* p, pointless_value_t* m)
{
	// this must be a map
	assert(m->type == POINTLESS_MAP_VALUE_VALUE);
	pointless_map_header_t* header = (pointless_map_header_t*)PC_HEAP_OFFSET(p, map_offsets, m->data.data_u32);
	assert((size_t)header % 4 == 0);

	return &header->key_vector;
}

pointless_value_t* pointless_map_value_vector(pointless_t* p, pointless_value_t* m)
{
	// this must be a map
	assert(m->type == POINTLESS_MAP_VALUE_VALUE);
	pointless_map_header_t* header = (pointless_map_header_t*)PC_HEAP_OFFSET(p, map_offsets, m->data.data_u32);
	assert((size_t)header % 4 == 0);

	return &header->value_vector;
}

// number of containers
uint32_t pointless_n_containers(pointless_t* p)
{
	// one extra for empty vectors
	uint32_t n = 1;

	n += p->header->n_vector;
	n += p->header->n_set;
	n += p->header->n_map;

	return n;
}

// get ID of container (non-empty vectors, sets and maps)
uint32_t pointless_container_id(pointless_t* p, pointless_value_t* c)
{
	switch (c->type) {
		case POINTLESS_VECTOR_EMPTY:
			return 0;
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
		case POINTLESS_VECTOR_I8:
		case POINTLESS_VECTOR_U8:
		case POINTLESS_VECTOR_I16:
		case POINTLESS_VECTOR_U16:
		case POINTLESS_VECTOR_I32:
		case POINTLESS_VECTOR_U32:
		case POINTLESS_VECTOR_I64:
		case POINTLESS_VECTOR_U64:
		case POINTLESS_VECTOR_FLOAT:
			return 1 + c->data.data_u32;
		case POINTLESS_SET_VALUE:
			return 1 + c->data.data_u32 + p->header->n_vector;
		case POINTLESS_MAP_VALUE_VALUE:
			return 1 + c->data.data_u32 + p->header->n_vector + p->header->n_set;
	}

	assert(0);
	return UINT32_MAX;
}
