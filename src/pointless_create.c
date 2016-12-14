#include <pointless/pointless_create.h>

typedef struct {
	int (*write)(void* data, size_t datalen, void* user, const char** error);
	int (*align_4)(void* user, const char** error);
	void* user;
} pointless_create_cb_t;

static size_t align_next_4_size_t(size_t v)
{
	static size_t lookup[4] = {0, 3, 2, 1};
	return v + lookup[v%4];
}

static uint32_t align_next_4_32(uint32_t v)
{
	static uint32_t lookup[4] = {0, 3, 2, 1};
	return v + lookup[v%4];
}

static uint64_t align_next_4_64(uint64_t v)
{
	static uint64_t lookup[4] = {0, 3, 2, 1};
	return v + lookup[v%4];
}

// convert create-time value to serializable value, with offset correction
// for private/outside vectors
static pointless_value_t pointless_create_to_read_value(pointless_create_t* c, uint32_t v, uint32_t n_priv_vectors)
{
	uint32_t type = cv_value_type(v);
	pointless_value_data_t data = cv_value_at(v)->data;

	if (cv_is_outside_vector(v))
		data.data_u32 += n_priv_vectors;

	pointless_value_t r;
	r.type = type;
	r.data = data;

	return r;
}

static int pointless_hash_table_create(pointless_create_t* c, uint32_t hash_table, const char** error)
{
	// return value
	int retval = 0;

	// output vectors
	uint32_t* hash_serialize = 0;
	uint32_t* keys_serialize = 0;
	uint32_t* values_serialize = 0;
	uint32_t* hash_vector = 0;

	// serialized vector handles
	uint32_t sh = 0, sk = 0, sv = 0;

	uint32_t i, n_buckets, empty_slot_handle;

	// WARNING: we are using a direct pointer to dynamic array, but we
	//          make sure that it can't grow/shrink inside this function
	uint32_t n_keys = 0;
	uint32_t* keys_vector_ptr = 0;
	uint32_t* values_vector_ptr = 0;

	switch (cv_value_type(hash_table)) {
		case POINTLESS_SET_VALUE:
			n_keys = pointless_dynarray_n_items(&cv_set_at(hash_table)->keys);
			keys_vector_ptr = (uint32_t*)(cv_set_at(hash_table)->keys._data);
			break;
		case POINTLESS_MAP_VALUE_VALUE:
			n_keys = pointless_dynarray_n_items(&cv_map_at(hash_table)->keys);
			keys_vector_ptr = (uint32_t*)(cv_map_at(hash_table)->keys._data);
			values_vector_ptr = (uint32_t*)(cv_map_at(hash_table)->values._data);
			break;
		default:
			*error = "pointless_hash_table_create() only support POINTLESS_SET_VALUE and POINTLESS_MAP_VALUE_VALUE";
			goto cleanup;
	}

	// number of buckets
	n_buckets = pointless_hash_compute_n_buckets(n_keys);

	// allocate output vectors
	hash_serialize = (uint32_t*)pointless_malloc(sizeof(uint32_t) * n_buckets);
	keys_serialize = (uint32_t*)pointless_malloc(sizeof(uint32_t) * n_buckets);
	hash_vector = (uint32_t*)pointless_malloc(sizeof(uint32_t) * n_keys);

	if (hash_serialize == 0 || keys_serialize == 0 || hash_vector == 0) {
		*error = "out of memory B";
		goto cleanup;
	}

	// ...and one for values if this is a map
	if (cv_value_type(hash_table) == POINTLESS_MAP_VALUE_VALUE) {
		values_serialize = (uint32_t*)pointless_malloc(sizeof(uint32_t) * n_buckets);

		if (values_serialize == 0) {
			*error = "out of memory C";
			goto cleanup;
		}
	}

	// create an "empty slot" value
	empty_slot_handle = pointless_create_empty_slot(c);

	if (empty_slot_handle == POINTLESS_CREATE_VALUE_FAIL) {
		*error = "out of memory D";
		goto cleanup;
	}

	// initialize all vectors
	for (i = 0; i < n_buckets; i++) {
		hash_serialize[i] = 0;
		keys_serialize[i] = empty_slot_handle;

		if (values_serialize)
			values_serialize[i] = empty_slot_handle;
	}

	// compute all the key hashes
	for (i = 0; i < n_keys; i++) {
		if (!pointless_is_hashable(cv_value_type(keys_vector_ptr[i]))) {
			*error = "pointless_hash_table_create(): internal error: key not hashable";
			goto cleanup;
		}

		// there must be no empty slot values in key vector
		if (cv_value_type(keys_vector_ptr[i]) == POINTLESS_EMPTY_SLOT) {
			*error = "key in set/map is of type POINTLESS_EMPTY_SLOT";
			goto cleanup;
		}

		hash_vector[i] = pointless_hash_create_32(c, cv_value_at(keys_vector_ptr[i]));
	}

	// populate the arrays
	if (!pointless_hash_table_populate(c, hash_vector, keys_vector_ptr, values_vector_ptr, n_keys, hash_serialize, keys_serialize, values_serialize, n_buckets, empty_slot_handle, error))
		goto cleanup;

	// hash vector no longer needed
	pointless_free(hash_vector);
	hash_vector = 0;

	// our serialize vector handles
	switch (cv_value_type(hash_table)) {
		case POINTLESS_SET_VALUE:
			sh = cv_set_at(hash_table)->serialize_hash;
			sk = cv_set_at(hash_table)->serialize_keys;
			break;
		case POINTLESS_MAP_VALUE_VALUE:
			sh = cv_map_at(hash_table)->serialize_hash;
			sk = cv_map_at(hash_table)->serialize_keys;
			sv = cv_map_at(hash_table)->serialize_values;
			break;
		default:
			assert(0);
			*error = "pointless_hash_table_create(): internal error: what is this type?";
			goto cleanup;
	}

	// transfer hash vector over
	if (pointless_create_vector_u32_transfer(c, sh, hash_serialize, n_buckets) == POINTLESS_CREATE_VALUE_FAIL) {
		*error = "unable to transfer hash_serialize vector";
		goto cleanup;
	}

	// the vector has been transferred, so it being pointless_free()'d is somebody elses problem
	hash_serialize = 0;

	// transfer key vector over
	if (pointless_create_vector_value_transfer(c, sk, keys_serialize, n_buckets) == POINTLESS_CREATE_VALUE_FAIL) {
		*error = "unable to transfer keys_serialize vector";
		goto cleanup;
	}

	// somebody elses problem now
	keys_serialize = 0;

	// transfer value vector over
	if (cv_value_type(hash_table) == POINTLESS_MAP_VALUE_VALUE) {
		if (pointless_create_vector_value_transfer(c, sv, values_serialize, n_buckets) == POINTLESS_CREATE_VALUE_FAIL) {
			*error = "unable to transfer values_serialize_vector";
			goto cleanup;
		}

		// somebody elses problem
		values_serialize = 0;
	}

	retval = 1;

cleanup:

	pointless_free(hash_serialize);
	pointless_free(keys_serialize);
	pointless_free(hash_vector);
	pointless_free(values_serialize);

	return retval;
}

static void pointless_create_begin_(pointless_create_t* c, uint32_t version)
{
	c->root = UINT32_MAX;

	pointless_create_cache_init(&c->cache, POINTLESS_CREATE_VALUE_FAIL);

	pointless_dynarray_init(&c->values, sizeof(pointless_create_value_t));
	pointless_dynarray_init(&c->priv_vector_values, sizeof(pointless_create_vector_priv_t));
	pointless_dynarray_init(&c->outside_vector_values, sizeof(pointless_create_vector_outside_t));
	pointless_dynarray_init(&c->set_values, sizeof(pointless_create_set_t));
	pointless_dynarray_init(&c->map_values, sizeof(pointless_create_map_t));
	pointless_dynarray_init(&c->string_unicode_values, sizeof(void*));
	pointless_dynarray_init(&c->bitvector_values, sizeof(void*));

	c->string_unicode_map_judy = 0;
	c->bitvector_map_judy = 0;

	c->string_unicode_map_judy_count = 0;
	c->bitvector_map_judy_count = 0;

	c->version = version;
}

void pointless_create_begin_32(pointless_create_t* c)
{
	pointless_create_begin_(c, POINTLESS_FF_VERSION_OFFSET_32_NEWHASH);
}

void pointless_create_begin_64(pointless_create_t* c)
{
	pointless_create_begin_(c, POINTLESS_FF_VERSION_OFFSET_64_NEWHASH);
}

static void pointless_create_value_free(pointless_create_t* c, uint32_t i)
{
	switch (cv_value_type(i)) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
			assert(cv_is_outside_vector(i) == 0);
			pointless_dynarray_destroy(&cv_priv_vector_at(i)->vector);
			break;
		case POINTLESS_VECTOR_I8:
		case POINTLESS_VECTOR_U8:
		case POINTLESS_VECTOR_I16:
		case POINTLESS_VECTOR_U16:
		case POINTLESS_VECTOR_I32:
		case POINTLESS_VECTOR_U64:
		case POINTLESS_VECTOR_I64:
		case POINTLESS_VECTOR_U32:
		case POINTLESS_VECTOR_FLOAT:
			if (cv_is_outside_vector(i) == 0)
				pointless_dynarray_destroy(&cv_priv_vector_at(i)->vector);
			break;
		case POINTLESS_BITVECTOR:
			pointless_free(cv_bitvector_at(i));
			break;
		case POINTLESS_UNICODE_:
			pointless_free(cv_unicode_at(i));
			break;
		case POINTLESS_STRING_:
			pointless_free(cv_string_at(i));
			break;
		case POINTLESS_SET_VALUE:
			pointless_dynarray_destroy(&cv_set_at(i)->keys);
			break;
		case POINTLESS_MAP_VALUE_VALUE:
			pointless_dynarray_destroy(&cv_map_at(i)->keys);
			pointless_dynarray_destroy(&cv_map_at(i)->values);
			break;
		default:
			break;
	}
}

void pointless_create_end(pointless_create_t* c)
{
	uint32_t i, n_values = pointless_dynarray_n_items(&c->values);

	for (i = 0; i < n_values; i++)
		pointless_create_value_free(c, i);

	pointless_dynarray_destroy(&c->values);
	pointless_dynarray_destroy(&c->priv_vector_values);
	pointless_dynarray_destroy(&c->outside_vector_values);
	pointless_dynarray_destroy(&c->set_values);
	pointless_dynarray_destroy(&c->map_values);
	pointless_dynarray_destroy(&c->string_unicode_values);
	pointless_dynarray_destroy(&c->bitvector_values);

	JudyHSFreeArray(&c->string_unicode_map_judy, 0);
	JudyHSFreeArray(&c->bitvector_map_judy, 0);

	c->string_unicode_map_judy = 0;
	c->bitvector_map_judy = 0;
}

static int pointless_serialize_string(pointless_create_cb_t* cb, void* string_buffer, const char** error)
{
	uint32_t* len = (uint32_t*)string_buffer;
	pointless_string_char_t* s = (pointless_string_char_t*)(len + 1);

	assert(pointless_ascii_len(s) == *len);

	if (!(*cb->write)(len, sizeof(*len), cb->user, error))
		return 0;

	if (!(*cb->write)(s, (*len + 1) * sizeof(uint8_t), cb->user, error))
		return 0;

	if (!(*cb->align_4)(cb->user, error))
		return 0;

	return 1;
}

static int pointless_serialize_unicode(pointless_create_cb_t* cb, void* unicode_buffer, const char** error)
{
	uint32_t* len = (uint32_t*)unicode_buffer;
	pointless_unicode_char_t* s = (pointless_unicode_char_t*)(len + 1);

	if (!(*cb->write)(len, sizeof(*len), cb->user, error))
		return 0;

	if (!(*cb->write)(s, (*len + 1) * sizeof(pointless_unicode_char_t), cb->user, error))
		return 0;

	if (!(*cb->align_4)(cb->user, error))
		return 0;

	return 1;
}

static int pointless_serialize_vector_outside(pointless_create_t* c, uint32_t vector, pointless_create_cb_t* cb, const char** error)
{
	assert(cv_is_outside_vector(vector) == 1);
	uint32_t n_items = cv_outside_vector_at(vector)->n_items;
	void* items = cv_outside_vector_at(vector)->items;
	size_t w_len = 0;

	if (!(cb->write)(&n_items, sizeof(n_items), cb->user, error))
		return 0;

	switch (cv_value_type(vector)) {
		case POINTLESS_VECTOR_I8:
			w_len = sizeof(int8_t);
			break;
		case POINTLESS_VECTOR_U8:
			w_len = sizeof(uint8_t);
			break;
		case POINTLESS_VECTOR_I16:
			w_len = sizeof(int16_t);
			break;
		case POINTLESS_VECTOR_U16:
			w_len = sizeof(uint16_t);
			break;
		case POINTLESS_VECTOR_I32:
			w_len = sizeof(int32_t);
			break;
		case POINTLESS_VECTOR_U32:
			w_len = sizeof(uint32_t);
			break;
		case POINTLESS_VECTOR_I64:
			w_len = sizeof(int64_t);
			break;
		case POINTLESS_VECTOR_U64:
			w_len = sizeof(uint64_t);
			break;
		case POINTLESS_VECTOR_FLOAT:
			w_len = sizeof(float);
			break;
		default:
			assert(0);
			w_len = 0;
			*error = "pointless_serialize_vector_outside(): internal error";
			return 0;
	}

	// TBD: multiply overflow
	if (!(cb->write)(items, w_len * n_items, cb->user, error))
		return 0;

	if (!(cb->align_4)(cb->user, error))
		return 0;

	return 1;
}

static int pointless_serialize_vector_priv(pointless_create_t* c, uint32_t vector, pointless_create_cb_t* cb, uint32_t n_priv_vectors, const char** error)
{
	assert(cv_is_outside_vector(vector) == 0);
	uint32_t i, n_items = pointless_dynarray_n_items(&cv_priv_vector_at(vector)->vector);

	union {
		int8_t i8;
		uint8_t u8;
		int16_t i16;
		uint16_t u16;
		int32_t i32;
		uint32_t u32;
		float f;
		pointless_value_t v;
	} value;

	size_t w_len = 0;

	if (!(cb->write)(&n_items, sizeof(n_items), cb->user, error))
		return 0;

	// classify vector
	int is_uncompressed = cv_value_type(vector) == POINTLESS_VECTOR_VALUE || cv_value_type(vector) == POINTLESS_VECTOR_VALUE_HASHABLE;
	int is_compressed   = !is_uncompressed && cv_is_compressed_vector(vector);
	int is_native       = !is_uncompressed && !is_compressed;

	assert(is_uncompressed + is_compressed + is_native == 1);

	// if we have a native vector, we can write it in a single write call
	if (is_native) {
		switch (cv_value_type(vector)) {
			case POINTLESS_VECTOR_I8:
				w_len = sizeof(int8_t);
				break;
			case POINTLESS_VECTOR_U8:
				w_len = sizeof(uint8_t);
				break;
			case POINTLESS_VECTOR_I16:
				w_len = sizeof(int16_t);
				break;
			case POINTLESS_VECTOR_U16:
				w_len = sizeof(uint16_t);
				break;
			case POINTLESS_VECTOR_I32:
				w_len = sizeof(int32_t);
				break;
			case POINTLESS_VECTOR_U32:
				w_len = sizeof(uint32_t);
				break;
			case POINTLESS_VECTOR_I64:
				w_len = sizeof(int64_t);
				break;
			case POINTLESS_VECTOR_U64:
				w_len = sizeof(uint64_t);
				break;
			case POINTLESS_VECTOR_FLOAT:
				w_len = sizeof(float);
				break;
			default:
				assert(0);
				break;
		}

		if (!(cb->write)(cv_priv_vector_at(vector)->vector._data, w_len * n_items, cb->user, error))
			return 0;
	}

	uint32_t* items = (uint32_t*)cv_priv_vector_at(vector)->vector._data;

	for (i = 0; i < n_items && !is_native; i++) {
		// uncompressed value vector
		if (!is_compressed) {//is_uncompressed) {
			// WARNING: we are using a pointer to a dynamic array, so during its scope, we must
			//          not touch the original array, c->values in this case
			value.v = pointless_create_to_read_value(c, items[i], n_priv_vectors);
			w_len = sizeof(value.v);
		} else {
			// if vector was value based, but is now compressed, we must typecast all values
			assert(is_compressed);

			// WARNING: we are using a pointer to a dynamic array, so during its scope, we must
			//          not touch the original array, c->values in this case

			// translate the value
			switch (cv_value_type(vector)) {
				case POINTLESS_VECTOR_I8:
					assert(cv_value_type(items[i]) == POINTLESS_I32 || cv_value_type(items[i]) == POINTLESS_U32);
					value.i8 = (int8_t)pointless_create_get_int_as_int64(cv_value_at(items[i]));
					w_len = sizeof(value.i8);
					break;
				case POINTLESS_VECTOR_U8:
					assert(cv_value_type(items[i]) == POINTLESS_I32 || cv_value_type(items[i]) == POINTLESS_U32);
					value.u8 = (uint8_t)pointless_create_get_int_as_int64(cv_value_at(items[i]));
					w_len = sizeof(value.u8);
					break;
				case POINTLESS_VECTOR_I16:
					assert(cv_value_type(items[i]) == POINTLESS_I32 || cv_value_type(items[i]) == POINTLESS_U32);
					value.i16 = (int16_t)pointless_create_get_int_as_int64(cv_value_at(items[i]));
					w_len = sizeof(value.i16);
					break;
				case POINTLESS_VECTOR_U16:
					assert(cv_value_type(items[i]) == POINTLESS_I32 || cv_value_type(items[i]) == POINTLESS_U32);
					value.u16 = (uint16_t)pointless_create_get_int_as_int64(cv_value_at(items[i]));
					w_len = sizeof(value.u16);
					break;
				case POINTLESS_VECTOR_I32:
					assert(cv_value_type(items[i]) == POINTLESS_I32 || cv_value_type(items[i]) == POINTLESS_U32);
					value.i32 = (int32_t)pointless_create_get_int_as_int64(cv_value_at(items[i]));
					w_len = sizeof(value.i32);
					break;
				case POINTLESS_VECTOR_U32:
					assert(cv_value_type(items[i]) == POINTLESS_I32 || cv_value_type(items[i]) == POINTLESS_U32);
					value.u32 = (uint32_t)pointless_create_get_int_as_int64(cv_value_at(items[i]));
					w_len = sizeof(value.u32);
					break;
				case POINTLESS_VECTOR_FLOAT:
					assert(cv_value_type(items[i]) == POINTLESS_FLOAT);
					value.f = cv_float_at(items[i]);
					w_len = sizeof(value.f);
					break;
				default:
					assert(0);
					w_len = 0;
					break;
			}
		}

		if (!(cb->write)(&value, w_len, cb->user, error))
			return 0;
	}

	if (!(cb->align_4)(cb->user, error))
		return 0;

	return 1;
}

static int pointless_serialize_bitvector(pointless_create_cb_t* cb, void* bitvector_buffer, const char** error)
{
	uint32_t* len = (uint32_t*)bitvector_buffer;
	void* bitvector = (void*)(len + 1);
	size_t n_bytes = ICEIL(*len, 8);

	if (!(cb->write)(len, sizeof(*len), cb->user, error))
		return 0;

	if (!(cb->write)(bitvector, n_bytes, cb->user, error))
		return 0;

	if (!(cb->align_4)(cb->user, error))
		return 0;

	return 1;
}

static int pointless_serialize_set(pointless_create_cb_t* cb, pointless_create_t* c, uint32_t s, uint32_t n_priv_vectors, const char** error)
{
	uint32_t hash_vector_handle = cv_set_at(s)->serialize_hash;
	uint32_t keys_vector_handle = cv_set_at(s)->serialize_keys;

	pointless_set_header_t header;
	header.n_items = pointless_dynarray_n_items(&cv_set_at(s)->keys);
	header.padding = 0;
	header.hash_vector = pointless_create_to_read_value(c, hash_vector_handle, n_priv_vectors);
	header.key_vector = pointless_create_to_read_value(c, keys_vector_handle, n_priv_vectors);

	assert(header.hash_vector.type == POINTLESS_VECTOR_U32);
	assert(header.key_vector.type == POINTLESS_VECTOR_VALUE_HASHABLE);

	if (!(cb->write)(&header, sizeof(header), cb->user, error))
		return 0;

	if (!(cb->align_4)(cb->user, error))
		return 0;

	return 1;
}

static int pointless_serialize_map(pointless_create_cb_t* cb, pointless_create_t* c, uint32_t m, uint32_t n_priv_vectors, const char** error)
{
	uint32_t hash_vector_handle = cv_map_at(m)->serialize_hash;
	uint32_t keys_vector_handle = cv_map_at(m)->serialize_keys;
	uint32_t values_vector_handle = cv_map_at(m)->serialize_values;

	assert(pointless_dynarray_n_items(&cv_map_at(m)->keys) == pointless_dynarray_n_items(&cv_map_at(m)->values));

	pointless_map_header_t header;
	header.n_items = pointless_dynarray_n_items(&cv_map_at(m)->keys);
	header.padding = 0;
	header.hash_vector = pointless_create_to_read_value(c, hash_vector_handle, n_priv_vectors);
	header.key_vector = pointless_create_to_read_value(c, keys_vector_handle, n_priv_vectors);
	header.value_vector = pointless_create_to_read_value(c, values_vector_handle, n_priv_vectors);

	assert(header.hash_vector.type == POINTLESS_VECTOR_U32);
	assert(header.key_vector.type == POINTLESS_VECTOR_VALUE_HASHABLE);
	assert(header.value_vector.type == POINTLESS_VECTOR_VALUE_HASHABLE || header.value_vector.type == POINTLESS_VECTOR_VALUE);
	assert(pointless_is_vector_type(header.value_vector.type));

	if (!(cb->write)(&header, sizeof(header), cb->user, error))
		return 0;

	if (!(cb->align_4)(cb->user, error))
		return 0;

	return 1;
}

static uint32_t pointless_create_vector_compression(pointless_create_t* c, uint32_t vector)
{
	// no empty vectors here, caller should take care of those
	assert(cv_is_outside_vector(vector) == 0);
	assert(pointless_dynarray_n_items(&cv_priv_vector_at(vector)->vector) > 0);

	// no compression possibilites found yet
	uint32_t compression = cv_value_type(vector);
	assert(compression == POINTLESS_VECTOR_VALUE || compression == POINTLESS_VECTOR_VALUE_HASHABLE);
	size_t i;

	// value ranges we've found, we do not support 64 bit integers, only 8, 16 and 32
	// so these should hold them just fine
	int64_t min_int = 0, max_int = 0, cur_int = 0;
	int is_int = 0, init_int = 0, init_float = 0;

	// create-time IDs for this vector
	size_t n_items = pointless_dynarray_n_items(&cv_priv_vector_at(vector)->vector);
	uint32_t* items = (uint32_t*)cv_priv_vector_at(vector)->vector._data;

	for (i = 0; i < n_items; i++) {
		is_int = 0;

		switch (cv_value_type(items[i])) {
			// compressible types
			case POINTLESS_I32:
				is_int = 1;
				cur_int = (int64_t)cv_i32_at(items[i]);
				break;
			case POINTLESS_U32:
				cur_int = (int64_t)cv_u32_at(items[i]);
				is_int = 1;
				break;
			case POINTLESS_FLOAT:
				init_float = 1;
				break;
			// not a compressible vector type
			default:
				return compression;
		}

		assert(is_int || init_float);

		// we can't mix ints and floats
		if ((is_int || init_int) && init_float)
			return compression;

		// expand the current integer range
		if (is_int) {
			if (!init_int) {
				min_int = max_int = cur_int;
				init_int = 1;
			} else {
				min_int = SIMPLE_MIN(min_int, cur_int);
				max_int = SIMPLE_MAX(max_int, cur_int);
			}
		}
	}

	// we must have some floats or some integers, but not both
	assert(init_int || init_float);
	assert(!(init_int && init_float));

	// all floats, simple case
	if (init_float)
		return POINTLESS_VECTOR_FLOAT;

	// right, now check for range, first for unsigned ones
	assert(min_int <= max_int);

	if (min_int >= 0) {
		if (max_int <= UINT8_MAX)
			return POINTLESS_VECTOR_U8;
		else if (max_int <= UINT16_MAX)
			return POINTLESS_VECTOR_U16;
		else if (max_int <= UINT32_MAX)
			return POINTLESS_VECTOR_U32;
		else
			assert(0);
	}

	if (INT8_MIN <= min_int && max_int <= INT8_MAX)
		return POINTLESS_VECTOR_I8;
	else if (INT16_MIN <= min_int && max_int <= INT16_MAX)
		return POINTLESS_VECTOR_I16;
	else if (INT32_MIN <= min_int && max_int <= INT32_MAX)
		return POINTLESS_VECTOR_I32;

	// we must have hit something here already
	assert(0);
	return 0;
}

static int pointless_create_output_and_end_(pointless_create_t* c, pointless_create_cb_t* cb, const char** error)
{
	// return value
	int retval = 1;

	// different version of offset
	int is_32_offset = 0;
	int is_64_offset = 0;

	switch (c->version) {
		case POINTLESS_FF_VERSION_OFFSET_32_OLDHASH:
		case POINTLESS_FF_VERSION_OFFSET_32_NEWHASH:
			is_32_offset = 1;
			break;
		case POINTLESS_FF_VERSION_OFFSET_64_NEWHASH:
			is_64_offset = 1;
			break;
		default:
			*error = "unsupported version";
			return 0;
	}

	uint32_t debug_n_maps, debug_n_sets, debug_n_bitvectors, debug_n_outside_vectors, debug_n_priv_vectors, debug_n_string_unicode;
	uint32_t n_priv_vectors, n_outside_vectors, n_sets, n_maps;
	uint32_t i, n_values;

	uint32_t current_offset_32;
	uint64_t current_offset_64;

	pointless_dynarray_t temp;

	// bitmask for each container, used in cycle-detection
	void* cycle_marker = 0;

	// since we're removing some vectors from c->priv_vector_values, references to it change, so we need
	// a new c->priv_vector_values
	pointless_dynarray_t new_priv_vector_values;
	pointless_dynarray_init(&new_priv_vector_values, sizeof(pointless_create_vector_priv_t));

	// we must have a root
	if (c->root == UINT32_MAX) {
		*error = "root has not been set";
		goto error_cleanup;
	}

	// NOTE: value vector will grow, but the first 'n_values' items are the ones we want to serialize
	n_values = pointless_dynarray_n_items(&c->values);

	// count number of non-empty vectors, sets and maps, and perform work for empty vectors
	// we are not allowed to do this for vectors used to hold set/map keys and hashes, so we
	// ignore those
	n_priv_vectors = 0, n_outside_vectors = 0, n_sets = 0, n_maps = 0;

	for (i = 0; i < n_values; i++) {
		switch (cv_value_type(i)) {
			case POINTLESS_VECTOR_I8:
			case POINTLESS_VECTOR_U8:
			case POINTLESS_VECTOR_I16:
			case POINTLESS_VECTOR_U16:
			case POINTLESS_VECTOR_I32:
			case POINTLESS_VECTOR_U32:
			case POINTLESS_VECTOR_I64:
			case POINTLESS_VECTOR_U64:
			case POINTLESS_VECTOR_FLOAT:
			case POINTLESS_VECTOR_VALUE:
				// we only do this for private vector
				if (cv_is_outside_vector(i)) {
					n_outside_vectors += 1;
					break;
				}
				// set/map vectors can not be emptied
				if (pointless_dynarray_n_items(&cv_priv_vector_at(i)->vector) == 0 && cv_is_set_map_vector(i) == 0) {
					cv_value_at(i)->header.type_29 = POINTLESS_VECTOR_EMPTY;
					cv_value_at(i)->data.data_u32 = 0;
				} else {
					if (!pointless_dynarray_push(&new_priv_vector_values, cv_priv_vector_at(i))) {
						*error = "out of memory I";
						goto error_cleanup;
					}

					cv_value_at(i)->data.data_u32 = n_priv_vectors++;
				}

				break;
			case POINTLESS_SET_VALUE:
				cv_value_at(i)->data.data_u32 = n_sets++;
				break;
			case POINTLESS_MAP_VALUE_VALUE:
				cv_value_at(i)->data.data_u32 = n_maps++;
				break;
			case POINTLESS_VECTOR_VALUE_HASHABLE:
				assert(0);
				break;
		}
	}

	// now, c->priv_vector_values should become new_priv_vector_values, but we must not forget to
	// cleanup c->priv_vector_values, so we just swap them, and c->priv_vector_values will be
	// cleaned up as new_priv_vector_values, later on
	temp = new_priv_vector_values;
	new_priv_vector_values = c->priv_vector_values;
	c->priv_vector_values = temp;


	// check which vectors are hashable
	cycle_marker = pointless_cycle_marker_create(c, error);

	if (cycle_marker == 0) {
		goto error_cleanup;
	}

	// ...and mark the ones which are
	for (i = 0; i < n_values; i++) {
		//printf("value type %i is %i\n", (int)i, (int)cv_value_type(i));
		if (cv_value_type(i) == POINTLESS_VECTOR_VALUE && !cv_is_outside_vector(i)) {
			if (!bm_is_set_(cycle_marker, i)) {
				cv_value_at(i)->header.type_29 = POINTLESS_VECTOR_VALUE_HASHABLE;
				//printf("hashable %i\n", (int)i);
			}
		}
	}

	// check vectors for compressability
	for (i = 0; i < n_values; i++) {
		// we can not compress outside vector
		switch (cv_value_type(i)) {
			case POINTLESS_VECTOR_VALUE:
			case POINTLESS_VECTOR_VALUE_HASHABLE:
				// we're not allowed to compress outside or set/map vectors
				if (cv_is_outside_vector(i) == 0 && cv_is_set_map_vector(i) == 0) {
					cv_value_at(i)->header.type_29 = pointless_create_vector_compression(c, i);

					if (cv_value_at(i)->header.type_29 != POINTLESS_VECTOR_VALUE && cv_value_at(i)->header.type_29 != POINTLESS_VECTOR_VALUE_HASHABLE) {
						//printf("compressed %i to %i\n", (int)i, (int)cv_value_at(i)->header.type_29);
						cv_value_at(i)->header.is_compressed_vector = 1;
					}
				}

				break;
		}
	}


	// right, now populate the hash, key and value vectors for sets and maps
	for (i = 0; i < n_values; i++) {
		if (cv_value_type(i) == POINTLESS_SET_VALUE) {
			// serialize vectors must have been initialized
			assert(cv_set_at(i)->serialize_hash != POINTLESS_CREATE_VALUE_FAIL);
			assert(cv_set_at(i)->serialize_keys != POINTLESS_CREATE_VALUE_FAIL);

			// they must be legal values
			assert(cv_set_at(i)->serialize_hash < pointless_dynarray_n_items(&c->values));
			assert(cv_set_at(i)->serialize_keys < pointless_dynarray_n_items(&c->values));

			// the must be of the expected type
			assert(cv_value_type(cv_set_at(i)->serialize_hash) == POINTLESS_VECTOR_U32);
			assert(cv_value_type(cv_set_at(i)->serialize_keys) == POINTLESS_VECTOR_VALUE_HASHABLE);

			// ..and they must be empty
			assert(pointless_dynarray_n_items(&cv_priv_vector_at(cv_set_at(i)->serialize_hash)->vector) == 0);
			assert(pointless_dynarray_n_items(&cv_priv_vector_at(cv_set_at(i)->serialize_keys)->vector) == 0);

			// now we can populate these
			if (!pointless_hash_table_create(c, i, error))
				goto error_cleanup;
		}
	}

	// there is at least one more memory optimization available: it is possible to generate the serialize vectors
	// for sets/maps on the fly, when their respective vectors are serialized. i'm not sure on how to implement
	// this exactly, but if the need arose, this is fairly easy, methinks.

	for (i = 0; i < n_values; i++) {
		if (cv_value_type(i) == POINTLESS_MAP_VALUE_VALUE) {
			// serialize vectors must have been initalized
			assert(cv_map_at(i)->serialize_hash != POINTLESS_CREATE_VALUE_FAIL);
			assert(cv_map_at(i)->serialize_keys != POINTLESS_CREATE_VALUE_FAIL);
			assert(cv_map_at(i)->serialize_values != POINTLESS_CREATE_VALUE_FAIL);

			// they must be legal values
			assert(cv_map_at(i)->serialize_hash < pointless_dynarray_n_items(&c->values));
			assert(cv_map_at(i)->serialize_keys < pointless_dynarray_n_items(&c->values));
			assert(cv_map_at(i)->serialize_values < pointless_dynarray_n_items(&c->values));

			// they must be of the expected type
			assert(cv_value_type(cv_map_at(i)->serialize_hash) == POINTLESS_VECTOR_U32);
			assert(cv_value_type(cv_map_at(i)->serialize_keys) == POINTLESS_VECTOR_VALUE_HASHABLE);
			assert(cv_value_type(cv_map_at(i)->serialize_values) == POINTLESS_VECTOR_VALUE || cv_value_type(cv_map_at(i)->serialize_values) == POINTLESS_VECTOR_VALUE_HASHABLE);

			// ..and they must be empty
			assert(pointless_dynarray_n_items(&cv_priv_vector_at(cv_map_at(i)->serialize_hash)->vector) == 0);
			assert(pointless_dynarray_n_items(&cv_priv_vector_at(cv_map_at(i)->serialize_keys)->vector) == 0);
			assert(pointless_dynarray_n_items(&cv_priv_vector_at(cv_map_at(i)->serialize_values)->vector) == 0);

			// now we can populate these
			if (!pointless_hash_table_create(c, i, error))
				goto error_cleanup;
		}
	}

	// header
	pointless_header_t header;
	header.root = pointless_create_to_read_value(c, c->root, n_priv_vectors);
	header.n_string_unicode = c->string_unicode_map_judy_count;
	header.n_vector = n_priv_vectors + n_outside_vectors;
	header.n_bitvector = c->bitvector_map_judy_count;
	header.n_set = n_sets;
	header.n_map = n_maps;
	header.version = c->version;

	// write it out
	if (!(*cb->write)(&header, sizeof(header), cb->user, error))
		goto error_cleanup;

	// current offset value, refs are relative to heap base
	current_offset_32 = 0;
	current_offset_64 = 0;

	// write out offset vectors, first unicodes
	debug_n_string_unicode = 0;

	#define PC_WRITE_OFFSET() if (is_32_offset && !(*cb->write)(&current_offset_32, sizeof(current_offset_32), cb->user, error)) {goto error_cleanup;} if (is_64_offset && !(*cb->write)(&current_offset_64, sizeof(current_offset_64), cb->user, error)) {goto error_cleanup;}
	#define PC_INCREMENT_OFFSET(f) {current_offset_32 += (f); current_offset_64 += (f);}
	#define PC_ALIGN_OFFSET() {current_offset_32 = align_next_4_32(current_offset_32); current_offset_64 = align_next_4_64(current_offset_64);}

	for (i = 0; i < n_values; i++) {
		if (cv_value_type(i) == POINTLESS_UNICODE_) {
			assert(cv_value_data_u32(i) == debug_n_string_unicode);

			PC_WRITE_OFFSET();
			PC_INCREMENT_OFFSET(sizeof(uint32_t) + (*((uint32_t*)cv_unicode_at(i)) + 1) * sizeof(pointless_unicode_char_t));
			PC_ALIGN_OFFSET();
			debug_n_string_unicode += 1;
		}

		if (cv_value_type(i) == POINTLESS_STRING_) {
			assert(cv_value_data_u32(i) == debug_n_string_unicode);

			PC_WRITE_OFFSET();
			PC_INCREMENT_OFFSET(sizeof(uint32_t) + (*((uint32_t*)cv_string_at(i)) + 1) * sizeof(uint8_t));
			PC_ALIGN_OFFSET();
			debug_n_string_unicode += 1;
		}
	}

	assert(debug_n_string_unicode == c->string_unicode_map_judy_count);

	// then private vectors
	debug_n_priv_vectors = 0;

	for (i = 0; i < n_values; i++) {
		if (!pointless_is_vector_type(cv_value_type(i)))
			continue;

		if (cv_is_outside_vector(i))
			continue;

		if (cv_value_type(i) == POINTLESS_VECTOR_EMPTY)
			continue;

		uint32_t vector_heap_size = 0;
		uint32_t n_items = pointless_dynarray_n_items(&cv_priv_vector_at(i)->vector);

		switch (cv_value_type(i)) {
			case POINTLESS_VECTOR_VALUE:
			case POINTLESS_VECTOR_VALUE_HASHABLE:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(pointless_value_t);
				break;
			case POINTLESS_VECTOR_I8:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(int8_t);
				break;
			case POINTLESS_VECTOR_U8:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(uint8_t);
				break;
			case POINTLESS_VECTOR_I16:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(int16_t);
				break;
			case POINTLESS_VECTOR_U16:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(uint16_t);
				break;
			case POINTLESS_VECTOR_I32:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(int32_t);
				break;
			case POINTLESS_VECTOR_U32:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(uint32_t);
				break;
			case POINTLESS_VECTOR_I64:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(int64_t);
				break;
			case POINTLESS_VECTOR_U64:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(uint64_t);
				break;
			case POINTLESS_VECTOR_FLOAT:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(float);
				break;
			default:
				assert(0);
				break;
		}

		PC_WRITE_OFFSET();
		PC_INCREMENT_OFFSET(vector_heap_size);
		PC_ALIGN_OFFSET();
		debug_n_priv_vectors += 1;
	}

	assert(debug_n_priv_vectors == n_priv_vectors);

	// then outside vectors
	debug_n_outside_vectors = 0;

	for (i = 0; i < n_values; i++) {
		if (!pointless_is_vector_type(cv_value_type(i)))
			continue;

		if (!cv_is_outside_vector(i))
			continue;

		if (cv_value_type(i) == POINTLESS_VECTOR_EMPTY)
			continue;

		uint32_t vector_heap_size = 0;
		uint32_t n_items = cv_outside_vector_at(i)->n_items;

		switch (cv_value_type(i)) {
			case POINTLESS_VECTOR_I8:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(int8_t);
				break;
			case POINTLESS_VECTOR_U8:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(uint8_t);
				break;
			case POINTLESS_VECTOR_I16:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(int16_t);
				break;
			case POINTLESS_VECTOR_U16:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(uint16_t);
				break;
			case POINTLESS_VECTOR_I32:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(int32_t);
				break;
			case POINTLESS_VECTOR_U32:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(uint32_t);
				break;
			case POINTLESS_VECTOR_I64:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(int64_t);
				break;
			case POINTLESS_VECTOR_U64:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(uint64_t);
				break;
			case POINTLESS_VECTOR_FLOAT:
				vector_heap_size = sizeof(uint32_t) + n_items * sizeof(float);
				break;
			default:
				assert(0);
				break;
		}

		PC_WRITE_OFFSET();
		PC_INCREMENT_OFFSET(vector_heap_size);
		PC_ALIGN_OFFSET();
		debug_n_outside_vectors += 1;
	}

	assert(debug_n_outside_vectors == n_outside_vectors);

	// then uncompressed bitvectors
	debug_n_bitvectors = 0;

	for (i = 0; i < n_values; i++) {
		if (cv_value_type(i) == POINTLESS_BITVECTOR) {
			assert(cv_value_data_u32(i) == debug_n_bitvectors);

			PC_WRITE_OFFSET();
			PC_INCREMENT_OFFSET(sizeof(uint32_t) + ICEIL(*((uint32_t*)cv_bitvector_at(i)), 8));
			PC_ALIGN_OFFSET();
			debug_n_bitvectors += 1;
		}
	}

	assert(debug_n_bitvectors == c->bitvector_map_judy_count);

	// then sets
	debug_n_sets = 0;

	for (i = 0; i < n_values; i++) {
		if (cv_value_type(i) == POINTLESS_SET_VALUE) {
			assert(cv_value_data_u32(i) == debug_n_sets);

			PC_WRITE_OFFSET();
			PC_INCREMENT_OFFSET(sizeof(pointless_set_header_t));
			PC_ALIGN_OFFSET();
			debug_n_sets += 1;
		}
	}

	// then maps
	debug_n_maps = 0;

	for (i = 0; i < n_values; i++) {
		if (cv_value_type(i) == POINTLESS_MAP_VALUE_VALUE) {
			assert(cv_value_data_u32(i) == debug_n_maps);

			PC_WRITE_OFFSET();
			PC_INCREMENT_OFFSET(sizeof(pointless_map_header_t));
			PC_ALIGN_OFFSET();
			debug_n_maps += 1;
		}
	}

	// write out heap, unicodes first
	for (i = 0; i < n_values; i++) {
		if (cv_value_type(i) == POINTLESS_UNICODE_) {
			if (!pointless_serialize_unicode(cb, cv_unicode_at(i), error))
				goto error_cleanup;
		}

		if (cv_value_type(i) == POINTLESS_STRING_) {
			if (!pointless_serialize_string(cb, cv_string_at(i), error))
				goto error_cleanup;
		}
	}

	// private vectors
	for (i = 0; i < n_values; i++) {
		switch (cv_value_type(i)) {
			case POINTLESS_VECTOR_VALUE:
			case POINTLESS_VECTOR_VALUE_HASHABLE:
				assert(!cv_is_outside_vector(i));

				if (!pointless_serialize_vector_priv(c, i, cb, n_priv_vectors, error))
					goto error_cleanup;

				break;
			case POINTLESS_VECTOR_I8:
			case POINTLESS_VECTOR_U8:
			case POINTLESS_VECTOR_I16:
			case POINTLESS_VECTOR_U16:
			case POINTLESS_VECTOR_I32:
			case POINTLESS_VECTOR_U32:
			case POINTLESS_VECTOR_I64:
			case POINTLESS_VECTOR_U64:
			case POINTLESS_VECTOR_FLOAT:
				if (!cv_is_outside_vector(i)) {
					if (!pointless_serialize_vector_priv(c, i, cb, n_priv_vectors, error))
						goto error_cleanup;
				}
				break;
		}
	}

	// outside vectors
	for (i = 0; i < n_values; i++) {
		switch (cv_value_type(i)) {
			case POINTLESS_VECTOR_I8:
			case POINTLESS_VECTOR_U8:
			case POINTLESS_VECTOR_I16:
			case POINTLESS_VECTOR_U16:
			case POINTLESS_VECTOR_I32:
			case POINTLESS_VECTOR_U32:
			case POINTLESS_VECTOR_I64:
			case POINTLESS_VECTOR_U64:
			case POINTLESS_VECTOR_FLOAT:
				if (cv_is_outside_vector(i)) {
					if (!pointless_serialize_vector_outside(c, i, cb, error))
						goto error_cleanup;
				}

				break;
		}
	}

	// bitvectors
	for (i = 0; i < n_values; i++) {
		if (cv_value_type(i) == POINTLESS_BITVECTOR) {
			if (!pointless_serialize_bitvector(cb, cv_bitvector_at(i), error))
				goto error_cleanup;
		}
	}

	// sets
	for (i = 0; i < n_values; i++) {
		if (cv_value_type(i) == POINTLESS_SET_VALUE) {
			if (!pointless_serialize_set(cb, c, i, n_priv_vectors, error))
				goto error_cleanup;
		}
	}

	// maps
	for (i = 0; i < n_values; i++) {
		if (cv_value_type(i) == POINTLESS_MAP_VALUE_VALUE) {
			if (!pointless_serialize_map(cb, c, i, n_priv_vectors, error))
				goto error_cleanup;
		}
	}

	retval = 1;
	goto success_cleanup;

error_cleanup:
	retval = 0;
	goto success_cleanup;

success_cleanup:

	pointless_dynarray_destroy(&new_priv_vector_values);
	pointless_free(cycle_marker);

	pointless_create_end(c);

	return retval;
}

static int file_align_4(void* user, const char** error)
{
	FILE* f = (FILE*)user;
	long pos = ftell(f);

	if (pos == -1) {
		*error = "ftell() failure";
		return 0;
	}

	// good alignment, nothing to do
	if (pos % 4 == 0)
		return 1;

	uint32_t v = 0;
	size_t n = 4 - pos % 4;

	if (fwrite(&v, n, 1, f) != 1) {
		*error = "fwrite() failure A";
		return 0;
	}

	return 1;
}

static int file_write(void* buf, size_t buflen, void* user, const char** error)
{
	if (buflen == 0)
		return 1;

	FILE* f = (FILE*)user;

	if (fwrite(buf, buflen, 1, f) != 1) {
		*error = "fwrite() failure B";
		return 0;
	}

	return 1;
}

int pointless_create_output_and_end_f(pointless_create_t* c, const char* fname, const char** error)
{
	// our file descriptors
	int fd = -1;
	FILE* f = 0;
	char* temp_fname = 0;
	const char* unlink_fname = 0;

	// create and open a unique file
	temp_fname = (char*)pointless_malloc(strlen(fname) + 32);

	if (temp_fname == 0) {
		*error = "out of memory";
		goto cleanup;
	}

	sprintf(temp_fname, "%s.XXXXXX", fname);

	fd = mkstemp(temp_fname);

	if (fd == -1) {
		fprintf(stderr, "temp_fname: %s\n", temp_fname);
		*error = "error creating temporary file";
		goto cleanup;
	}

	unlink_fname = temp_fname;

	f = fdopen(fd, "w");

	if (f == 0) {
		*error = "error attaching to temporary file";
		return 0;
	}

	pointless_create_cb_t cb;
	cb.write = file_write;
	cb.align_4 = file_align_4;
	cb.user = (void*)f;

	if (!pointless_create_output_and_end_(c, &cb, error))
		goto cleanup;

	// fflush
	if (fflush(f) != 0) {
		*error = "fflush() failure";
		goto cleanup;
	}

	// fsync
	if (fsync(fd) != 0) {
		*error = "fsync failure";
		goto cleanup;
	}

	// change file permission
	if (fchmod(fd, S_IRUSR) != 0) {
		*error = "fchmod failure";
		goto cleanup;
	}

	fd = -1;

	// rename
	if (rename(temp_fname, fname) != 0) {
		*error = "error renaming file";
		goto cleanup;
	}

	unlink_fname = fname;

	// fclose
	if (fclose(f) == EOF) {
		f = 0;
		*error = "error closing file";
		goto cleanup;
	}

	f = 0;

	pointless_free(temp_fname);
	temp_fname = 0;

	return 1;

cleanup:

	pointless_create_end(c);

	if (f) {
		fclose(f);
		fd = -1;
	}

	if (fd != -1)
		close(fd);

	if (unlink_fname)
		unlink(unlink_fname);

	pointless_free(temp_fname);
	temp_fname = 0;

	return 0;
}

static int dynarray_write(void* buf, size_t buflen, void* user, const char** error)
{
	unsigned char* cbuf = (unsigned char*)buf;
	pointless_dynarray_t* a = (pointless_dynarray_t*)user;

	//printf("writing %i bytes to buffer\n", (int)buflen);

	if (!pointless_dynarray_push_bulk(a, cbuf, buflen)) {
		*error = "out of memory";
		return 0;
	}

	return 1;
}

static int dynarray_align_4(void* user, const char** error)
{
	pointless_dynarray_t* a = (pointless_dynarray_t*)user;
	uint8_t zero = 0;
	size_t i, n = pointless_dynarray_n_items(a);
	size_t n_next = align_next_4_size_t(n);

	for (i = n; i < n_next; i++) {
		//printf(" ...aligning X\n");
		if (!pointless_dynarray_push(a, &zero)) {
			*error = "out of memory";
			return 0;
		}
	}

	return 1;
}

int pointless_create_output_and_end_b(pointless_create_t* c, void** buf, size_t* buflen, const char** error)
{
	pointless_dynarray_t a;
	pointless_dynarray_init(&a, 1);

	pointless_create_cb_t cb;
	cb.write = dynarray_write;
	cb.align_4 = dynarray_align_4;
	cb.user = (void*)&a;

	if (!pointless_create_output_and_end_(c, &cb, error)) {
		pointless_dynarray_destroy(&a);
		return 0;
	}

	*buf = pointless_dynarray_buffer(&a);
	*buflen = pointless_dynarray_n_items(&a);

	pointless_create_end(c);

	return 1;
}

void pointless_create_set_root(pointless_create_t* c, uint32_t root)
{
	assert(root < pointless_dynarray_n_items(&c->values));
	c->root = root;
}

#define pointless_create_and_return_inline_value_1(c, v, func) pointless_create_value_t cv = func(v); return pointless_dynarray_push(&c->values, &cv) ? (pointless_dynarray_n_items(&c->values) - 1) : POINTLESS_CREATE_VALUE_FAIL;
#define pointless_create_and_return_inline_value_2(c, func)    pointless_create_value_t cv = func();  return pointless_dynarray_push(&c->values, &cv) ? (pointless_dynarray_n_items(&c->values) - 1) : POINTLESS_CREATE_VALUE_FAIL;

static uint32_t pointless_create_i32_priv(pointless_create_t* c, int32_t v)
{
	pointless_create_and_return_inline_value_1(c, v, pointless_value_create_i32);
}

uint32_t pointless_create_i32(pointless_create_t* c, int32_t v)
{
	uint32_t handle = pointless_create_cache_get_i32(&c->cache, v);

	if (handle != POINTLESS_CREATE_VALUE_FAIL)
		return handle;

	handle = pointless_create_i32_priv(c, v);
	pointless_create_cache_set_i32(&c->cache, v, handle);

	return handle;
}

static uint32_t pointless_create_u32_priv(pointless_create_t* c, uint32_t v)
{
	pointless_create_and_return_inline_value_1(c, v, pointless_value_create_u32);
}

uint32_t pointless_create_u32(pointless_create_t* c, uint32_t v)
{
	uint32_t handle = pointless_create_cache_get_u32(&c->cache, v);

	if (handle != POINTLESS_CREATE_VALUE_FAIL)
		return handle;

	handle = pointless_create_u32_priv(c, v);
	pointless_create_cache_set_u32(&c->cache, v, handle);

	return handle;
}

uint32_t pointless_create_float(pointless_create_t* c, float v)
{
	pointless_create_and_return_inline_value_1(c, v, pointless_value_create_float);
}

static uint32_t pointless_create_boolean_false_priv(pointless_create_t* c)
{
	pointless_create_and_return_inline_value_2(c, pointless_value_create_bool_false);
}

uint32_t pointless_create_boolean_false(pointless_create_t* c)
{
	uint32_t handle = pointless_create_cache_get_false(&c->cache);

	if (handle != POINTLESS_CREATE_VALUE_FAIL)
		return handle;

	handle = pointless_create_boolean_false_priv(c);
	pointless_create_cache_set_false(&c->cache, handle);

	return handle;
}

static uint32_t pointless_create_boolean_true_priv(pointless_create_t* c)
{
	pointless_create_and_return_inline_value_2(c, pointless_value_create_bool_true);
}

uint32_t pointless_create_boolean_true(pointless_create_t* c)
{
	uint32_t handle = pointless_create_cache_get_true(&c->cache);

	if (handle != POINTLESS_CREATE_VALUE_FAIL)
		return handle;

	handle = pointless_create_boolean_true_priv(c);
	pointless_create_cache_set_true(&c->cache, handle);

	return handle;
}

uint32_t pointless_create_boolean(pointless_create_t* c, int32_t v)
{
	if (v)
		return pointless_create_boolean_true(c);
	else
		return pointless_create_boolean_false(c);
}

static uint32_t pointless_create_empty_slot_priv(pointless_create_t* c)
{
	pointless_create_and_return_inline_value_2(c, pointless_value_create_empty_slot);
}

uint32_t pointless_create_empty_slot(pointless_create_t* c)
{
	uint32_t handle = pointless_create_cache_get_empty_slot(&c->cache);

	if (handle != POINTLESS_CREATE_VALUE_FAIL)
		return handle;

	handle = pointless_create_empty_slot_priv(c);
	pointless_create_cache_set_empty_slot(&c->cache, handle);

	return handle;
}

static uint32_t pointless_create_null_priv(pointless_create_t* c)
{
	pointless_create_and_return_inline_value_2(c, pointless_value_create_null);
}

uint32_t pointless_create_null(pointless_create_t* c)
{
	uint32_t handle = pointless_create_cache_get_null(&c->cache);

	if (handle != POINTLESS_CREATE_VALUE_FAIL)
		return handle;

	handle = pointless_create_null_priv(c);
	pointless_create_cache_set_null(&c->cache, handle);

	return handle;
}


// big-values
uint32_t pointless_create_unicode_ucs4(pointless_create_t* c, uint32_t* v)
{
	// resources we allocate
	int pop_value = 0;
	int pop_unicode = 0;

	Pvoid_t PValue;
	Pvoid_t prev_ref;

	pointless_unicode_char_t* vv;

	// create buffer to hold [uint32 + v]
	size_t unicode_len = pointless_ucs4_len(v);
	size_t buffer_len = sizeof(uint32_t) + sizeof(pointless_unicode_char_t) * (unicode_len + 1);
	void* unicode_buffer = pointless_malloc(buffer_len);

	if (unicode_buffer == 0)
		goto cleanup;

	// setup buffer data
	*((uint32_t*)unicode_buffer) = (uint32_t)unicode_len;
	vv = (pointless_unicode_char_t*)((uint32_t*)unicode_buffer + 1);
	pointless_ucs4_cpy(vv, v);

	// see if it already exists
	prev_ref = (Pvoid_t)JudyHSGet(c->string_unicode_map_judy, unicode_buffer, buffer_len);

	if (prev_ref) {
		pointless_free(unicode_buffer);
		return (uint32_t)(*((Word_t*)prev_ref));
	}

	// create an appropriate value
	pointless_create_value_t value;
	value.header.type_29 = POINTLESS_UNICODE_;
	value.header.is_outside_vector = 0;
	value.header.is_compressed_vector = 0;
	value.header.is_set_map_vector = 0;
	value.data.data_u32 = c->string_unicode_map_judy_count;

	// add to value vector
	if (!pointless_dynarray_push(&c->values, &value))
		goto cleanup;

	pop_value = 1;

	if (!pointless_dynarray_push(&c->string_unicode_values, &unicode_buffer))
		goto cleanup;

	pop_unicode = 1;

	// add to mapping
	PValue = (Pvoid_t)JudyHSIns(&c->string_unicode_map_judy, unicode_buffer, buffer_len, 0);

	if (PValue == 0)
		goto cleanup;

	*((Word_t*)PValue) = (Word_t)(pointless_dynarray_n_items(&c->values) - 1);

	c->string_unicode_map_judy_count += 1;

	assert(c->string_unicode_map_judy_count == pointless_dynarray_n_items(&c->string_unicode_values));

	// we're done
	return (pointless_dynarray_n_items(&c->values) - 1);

cleanup:

	pointless_free(unicode_buffer);

	if (pop_value)
		pointless_dynarray_pop(&c->values);

	if (pop_unicode)
		pointless_dynarray_pop(&c->string_unicode_values);

	return POINTLESS_CREATE_VALUE_FAIL;
}

// big-values
uint32_t pointless_create_string_ascii(pointless_create_t* c, uint8_t* v)
{
	// resources we allocate
	int pop_value = 0;
	int pop_string = 0;

	Pvoid_t PValue = 0;
	Pvoid_t prev_ref = 0;

	uint8_t* vv;

	// create buffer to hold [uint32 + v]
	size_t string_len = pointless_ascii_len(v);
	size_t buffer_len = sizeof(uint32_t) + sizeof(uint8_t) * (string_len + 1);
	void* string_buffer = pointless_malloc(buffer_len);

	if (string_buffer == 0)
		goto cleanup;

	// setup buffer data
	*((uint32_t*)string_buffer) = (uint32_t)string_len;
	vv = (uint8_t*)((uint32_t*)string_buffer + 1);
	pointless_ascii_cpy(vv, v);

	// see if it already exists
	prev_ref = JudyHSGet(c->string_unicode_map_judy, string_buffer, buffer_len);

	if (prev_ref) {
		pointless_free(string_buffer);
		return (uint32_t)(*((Word_t*)prev_ref));
	}

	// create an appropriate value
	pointless_create_value_t value;
	value.header.type_29 = POINTLESS_STRING_;
	value.header.is_outside_vector = 0;
	value.header.is_compressed_vector = 0;
	value.header.is_set_map_vector = 0;
	value.data.data_u32 = c->string_unicode_map_judy_count;

	// add to value vector
	if (!pointless_dynarray_push(&c->values, &value))
		goto cleanup;

	pop_value = 1;

	if (!pointless_dynarray_push(&c->string_unicode_values, &string_buffer))
		goto cleanup;

	pop_string = 1;

	// add to mapping
	PValue = JudyHSIns(&c->string_unicode_map_judy, string_buffer, buffer_len, PJE0);

	if (PValue == 0)
		goto cleanup;

	*((Word_t*)PValue) = (Word_t)(pointless_dynarray_n_items(&c->values) - 1);

	c->string_unicode_map_judy_count += 1;

	assert(c->string_unicode_map_judy_count == pointless_dynarray_n_items(&c->string_unicode_values));
	// we're done
	return (pointless_dynarray_n_items(&c->values) - 1);

cleanup:

	pointless_free(string_buffer);

	if (pop_value)
		pointless_dynarray_pop(&c->values);

	if (pop_string)
		pointless_dynarray_pop(&c->string_unicode_values);

	return POINTLESS_CREATE_VALUE_FAIL;
}

uint32_t pointless_create_string_ucs2(pointless_create_t* c, uint16_t* v)
{
	uint8_t* ascii = pointless_ucs2_to_ascii(v);

	if (ascii == 0)
		return POINTLESS_CREATE_VALUE_FAIL;

	uint32_t handle = pointless_create_string_ascii(c, ascii);

	pointless_free(ascii);

	return handle;
}

uint32_t pointless_create_string_ucs4(pointless_create_t* c, uint32_t* v)
{
	uint8_t* ascii = pointless_ucs4_to_ascii(v);

	if (ascii == 0)
		return POINTLESS_CREATE_VALUE_FAIL;

	uint32_t handle = pointless_create_string_ascii(c, ascii);

	pointless_free(ascii);

	return handle;
}

uint32_t pointless_create_unicode_ucs2(pointless_create_t* c, uint16_t* s)
{
	uint32_t* ucs4 = pointless_ucs2_to_ucs4(s);

	if (ucs4 == 0)
		return POINTLESS_CREATE_VALUE_FAIL;

	uint32_t handle = pointless_create_unicode_ucs4(c, ucs4);

	pointless_free(ucs4);

	return handle;
}

uint32_t pointless_create_unicode_ascii(pointless_create_t* c, const char* s, const char** error)
{
	uint32_t* ucs4 = pointless_ascii_to_ucs4((uint8_t*)s);

	if (ucs4 == 0) {
		*error = "out of memory";
		return POINTLESS_CREATE_VALUE_FAIL;
	}

	uint32_t handle = pointless_create_unicode_ucs4(c, ucs4);

	if (handle == POINTLESS_CREATE_VALUE_FAIL)
		*error = "out of memory";

	pointless_free(ucs4);

	return handle;
}

static int pointless_bitvector_is_all_1(void* v, uint32_t n_bits)
{
	uint32_t i;

	for (i = 0; i < n_bits; i++)
		if (!bm_is_set_(v, i))
			return 0;

	return 1;
}

static int pointless_bitvector_is_all_0(void* v, uint32_t n_bits)
{
	uint32_t i;

	for (i = 0; i < n_bits; i++)
		if (bm_is_set_(v, i))
			return 0;

	return 1;
}

static int pointless_bitvector_is_01(void* v, uint32_t n_bits, uint32_t* n_bits_0, uint32_t* n_bits_1)
{
	uint32_t n_0 = 0, n_1 = 0, i;

	for (i = 0; i < n_bits; i++) {
		// (0|1) -> 1 good
		if (bm_is_set_(v, i)) {
			n_1 += 1;
		// 0 -> 0 good
		// 1 -> 0 bad
		} else {
			if (n_1 > 0)
				return 0;
			n_0 += 1;
		}
	}

	*n_bits_0 = n_0;
	*n_bits_1 = n_1;

	return 1;
}

static int pointless_bitvector_is_10(void* v, uint32_t n_bits, uint32_t* n_bits_1, uint32_t* n_bits_0)
{
	uint32_t n_1 = 0, n_0 = 0, i;

	for (i = 0; i < n_bits; i++) {
		// (0|1) -> 0 good
		if (!bm_is_set_(v, i)) {
			n_0 += 1;
		// 1 -> 1 good
		// 0 -> 1 bad
		} else {
			if (n_0 > 0)
				return 0;

			n_1 += 1;
		}
	}

	*n_bits_1 = n_1;
	*n_bits_0 = n_0;

	return 1;
}

uint32_t pointless_create_bitvector_compressed(pointless_create_t* c, pointless_value_t* v)
{
	// first, check for compression options
	assert(
		v->type == POINTLESS_BITVECTOR_0 ||
		v->type == POINTLESS_BITVECTOR_1 ||
		v->type == POINTLESS_BITVECTOR_PACKED ||
		v->type == POINTLESS_BITVECTOR_01 ||
		v->type == POINTLESS_BITVECTOR_10
	);

	pointless_create_value_t value;
	value.header.type_29 = v->type;
	value.header.is_outside_vector = 0;
	value.header.is_compressed_vector = 0;
	value.header.is_set_map_vector = 0;
	value.data = v->data;

	if (!pointless_dynarray_push(&c->values, &value))
		return POINTLESS_CREATE_VALUE_FAIL;

	return (pointless_dynarray_n_items(&c->values) - 1);
}

static uint32_t pointless_create_bitvector_(pointless_create_t* c, void* v, uint32_t n_bits, int normalize)
{
	// first, check for compression options
	pointless_create_value_t value;
	value.header.type_29 = POINTLESS_BITVECTOR;
	value.header.is_outside_vector = 0;
	value.header.is_compressed_vector = 0;
	value.header.is_set_map_vector = 0;

	// easy, all-0 or all-1
	if (pointless_bitvector_is_all_0(v, n_bits)) {
		value.header.type_29 = POINTLESS_BITVECTOR_0;
		value.data.data_u32 = n_bits;
	} else if (pointless_bitvector_is_all_1(v, n_bits)) {
		value.header.type_29 = POINTLESS_BITVECTOR_1;
		value.data.data_u32 = n_bits;
	// slightly worse, 27-bits or less
	} else if (n_bits <= 27) {
		value.header.type_29 = POINTLESS_BITVECTOR_PACKED;
		value.data.bitvector_packed.n_bits = n_bits;
		value.data.bitvector_packed.bits = 0;

		uint32_t i;

		// yes, this is hacky, might cause problems with bitfield semantics in GCC
		for (i = 0; i < n_bits; i++) {
			if (bm_is_set_(v, i))
				bm_set_((void*)&value.data.data_u32, i + 5);
		}
	// a bit worse, 01 or 10
	} else {
		uint32_t n_bits_a, n_bits_b;

		if (pointless_bitvector_is_01(v, n_bits, &n_bits_a, &n_bits_b)) {
			if (n_bits_a <= UINT16_MAX && n_bits_b <= UINT16_MAX) {
				value.header.type_29 = POINTLESS_BITVECTOR_01;
				value.data.bitvector_01_or_10.n_bits_a = (uint16_t)n_bits_a;
				value.data.bitvector_01_or_10.n_bits_b = (uint16_t)n_bits_b;
			}
		} else if (pointless_bitvector_is_10(v, n_bits, &n_bits_a, &n_bits_b)) {
			if (n_bits_a <= UINT16_MAX && n_bits_b <= UINT16_MAX) {
				value.header.type_29 = POINTLESS_BITVECTOR_10;
				value.data.bitvector_01_or_10.n_bits_a = (uint16_t)n_bits_a;
				value.data.bitvector_01_or_10.n_bits_b = (uint16_t)n_bits_b;
			}
		}
	}

	// if we compressed it, no need to involve the heap
	if (value.header.type_29 != POINTLESS_BITVECTOR) {
		if (!pointless_dynarray_push(&c->values, &value))
			return POINTLESS_CREATE_VALUE_FAIL;

		return (pointless_dynarray_n_items(&c->values) - 1);
	}

	// right, we have to allocate a buffer, find duplicates and some other stuff
	void* buffer = 0;
	int pop_value = 0;
	int pop_bitvector = 0;

	// create buffer to hold [uint32 + v]
	size_t buffer_len = sizeof(uint32_t) + ICEIL(n_bits, 8);
	buffer = pointless_malloc(buffer_len);

	if (buffer == 0)
		goto cleanup;

	*((uint32_t*)buffer) = n_bits;
	memcpy((uint32_t*)buffer + 1, v, ICEIL(n_bits, 8));

	// try to find if we already have it
	if (normalize) {
		Pvoid_t prev_ref = (Pvoid_t)JudyHSGet(c->bitvector_map_judy, buffer, buffer_len);

		if (prev_ref) {
			pointless_free(buffer);
			return (uint32_t)(*((Word_t*)prev_ref));
		}
	}

	// it doesn't, create a new value
	value.data.data_u32 = c->bitvector_map_judy_count;

	// add to vector list
	if (!pointless_dynarray_push(&c->values, &value))
		goto cleanup;

	pop_value = 1;

	if (!pointless_dynarray_push(&c->bitvector_values, &buffer))
		goto cleanup;

	pop_bitvector = 1;

	// add to mapping
	if (normalize) {
		Pvoid_t PValue = (Pvoid_t)JudyHSIns(&c->bitvector_map_judy, buffer, buffer_len, 0);

		if (PValue == 0)
			goto cleanup;

		*((Word_t*)PValue) = (Word_t)(pointless_dynarray_n_items(&c->values) - 1);
	}

	c->bitvector_map_judy_count += 1;

	// we're done
	return (pointless_dynarray_n_items(&c->values) - 1);

cleanup:
	pointless_free(buffer);

	if (pop_value)
		pointless_dynarray_pop(&c->values);

	if (pop_bitvector)
		pointless_dynarray_pop(&c->bitvector_values);

	return POINTLESS_CREATE_VALUE_FAIL;
}

uint32_t pointless_create_bitvector(pointless_create_t* c, void* v, uint32_t n_bits)
{
	return pointless_create_bitvector_(c, v, n_bits, 1);
}

uint32_t pointless_create_bitvector_no_normalize(pointless_create_t* c, void* v, uint32_t n_bits)
{
	return pointless_create_bitvector_(c, v, n_bits, 0);
}

// vectors, buffer owned by library
static uint32_t pointless_create_vector_priv(pointless_create_t* c, uint32_t vector_type, uint32_t item_size)
{
	int pop_value = 0;

	pointless_create_value_t value;
	value.header.type_29 = vector_type;
	value.header.is_outside_vector = 0;
	value.header.is_set_map_vector = 0;
	value.header.is_compressed_vector = 0 ;
	value.data.data_u32 = pointless_dynarray_n_items(&c->priv_vector_values);

	pointless_create_vector_priv_t vector;

	pointless_dynarray_init(&vector.vector, item_size);

	if (!pointless_dynarray_push(&c->values, &value))
		goto cleanup;

	pop_value = 1;

	if (!pointless_dynarray_push(&c->priv_vector_values, &vector))
		goto cleanup;

	return (pointless_dynarray_n_items(&c->values) - 1);

cleanup:

	if (pop_value)
		pointless_dynarray_pop(&c->values);

	return POINTLESS_CREATE_VALUE_FAIL;
}

uint32_t pointless_create_vector_value(pointless_create_t* c)
{
	return pointless_create_vector_priv(c, POINTLESS_VECTOR_VALUE, sizeof(uint32_t));
}

uint32_t pointless_create_vector_i8(pointless_create_t* c)
{
	return pointless_create_vector_priv(c, POINTLESS_VECTOR_I8, sizeof(int8_t));
}

uint32_t pointless_create_vector_u8(pointless_create_t* c)
{
	return pointless_create_vector_priv(c, POINTLESS_VECTOR_U8, sizeof(uint8_t));
}

uint32_t pointless_create_vector_i16(pointless_create_t* c)
{
	return pointless_create_vector_priv(c, POINTLESS_VECTOR_I16, sizeof(int16_t));
}

uint32_t pointless_create_vector_u16(pointless_create_t* c)
{
	return pointless_create_vector_priv(c, POINTLESS_VECTOR_U16, sizeof(uint16_t));
}

uint32_t pointless_create_vector_i32(pointless_create_t* c)
{
	return pointless_create_vector_priv(c, POINTLESS_VECTOR_I32, sizeof(int32_t));
}

uint32_t pointless_create_vector_u32(pointless_create_t* c)
{
	return pointless_create_vector_priv(c, POINTLESS_VECTOR_U32, sizeof(uint32_t));
}

uint32_t pointless_create_vector_i64(pointless_create_t* c)
{
	return pointless_create_vector_priv(c, POINTLESS_VECTOR_I64, sizeof(int64_t));
}

uint32_t pointless_create_vector_u64(pointless_create_t* c)
{
	return pointless_create_vector_priv(c, POINTLESS_VECTOR_U64, sizeof(uint64_t));
}

uint32_t pointless_create_vector_float(pointless_create_t* c)
{
	return pointless_create_vector_priv(c, POINTLESS_VECTOR_FLOAT, sizeof(float));
}

static uint32_t pointless_create_vector_append_priv(pointless_create_t* c, uint32_t vector, uint32_t vector_type, void* v)
{
	assert(vector < pointless_dynarray_n_items(&c->values));

	if (vector_type == POINTLESS_VECTOR_VALUE) {
		assert(cv_value_type(vector) == POINTLESS_VECTOR_VALUE || cv_value_type(vector) == POINTLESS_VECTOR_VALUE_HASHABLE);
	} else {
		assert(cv_value_type(vector) == vector_type);
	}

	assert(cv_is_outside_vector(vector) == 0);

	if (!pointless_dynarray_push(&cv_priv_vector_at(vector)->vector, v))
		return POINTLESS_CREATE_VALUE_FAIL;

	return vector;
}

uint32_t pointless_create_vector_value_append(pointless_create_t* c, uint32_t vector, uint32_t v)
{
	if (v >= pointless_dynarray_n_items(&c->values))
		return POINTLESS_CREATE_VALUE_FAIL;

	return pointless_create_vector_append_priv(c, vector, POINTLESS_VECTOR_VALUE, &v);
}

uint32_t pointless_create_vector_i8_append(pointless_create_t* c, uint32_t vector, int8_t v)
{
	return pointless_create_vector_append_priv(c, vector, POINTLESS_VECTOR_I8, &v);
}

uint32_t pointless_create_vector_u8_append(pointless_create_t* c, uint32_t vector, uint8_t v)
{
	return pointless_create_vector_append_priv(c, vector, POINTLESS_VECTOR_U8, &v);
}

uint32_t pointless_create_vector_i16_append(pointless_create_t* c, uint32_t vector, int16_t v)
{
	return pointless_create_vector_append_priv(c, vector, POINTLESS_VECTOR_I16, &v);
}

uint32_t pointless_create_vector_u16_append(pointless_create_t* c, uint32_t vector, uint16_t v)
{
	return pointless_create_vector_append_priv(c, vector, POINTLESS_VECTOR_U16, &v);
}

uint32_t pointless_create_vector_i32_append(pointless_create_t* c, uint32_t vector, int32_t v)
{
	return pointless_create_vector_append_priv(c, vector, POINTLESS_VECTOR_I32, &v);
}

uint32_t pointless_create_vector_u32_append(pointless_create_t* c, uint32_t vector, uint32_t v)
{
	return pointless_create_vector_append_priv(c, vector, POINTLESS_VECTOR_U32, &v);
}

uint32_t pointless_create_vector_i64_append(pointless_create_t* c, uint32_t vector, int64_t v)
{
	return pointless_create_vector_append_priv(c, vector, POINTLESS_VECTOR_I64, &v);
}

uint32_t pointless_create_vector_u64_append(pointless_create_t* c, uint32_t vector, uint64_t v)
{
	return pointless_create_vector_append_priv(c, vector, POINTLESS_VECTOR_U64, &v);
}

uint32_t pointless_create_vector_float_append(pointless_create_t* c, uint32_t vector, float v)
{
	return pointless_create_vector_append_priv(c, vector, POINTLESS_VECTOR_FLOAT, &v);
}

void pointless_create_vector_value_set(pointless_create_t* c, uint32_t vector, uint32_t i, uint32_t v)
{
	pointless_create_vector_priv_t* vp = cv_priv_vector_at(vector);
	pointless_dynarray_ITEM_AT(uint32_t, &vp->vector, i) = v;
}

static uint32_t pointless_create_vector_transfer_priv(pointless_create_t* c, uint32_t vector, void* value, uint32_t n_items)
{
	pointless_create_vector_priv_t* vp = cv_priv_vector_at(vector);

	if (pointless_dynarray_n_items(&vp->vector) != 0)
		return POINTLESS_CREATE_VALUE_FAIL;

	pointless_dynarray_give_data(&vp->vector, (void*)value, n_items);
	return vector;
}

uint32_t pointless_create_vector_u32_transfer(pointless_create_t* c, uint32_t vector, uint32_t* v, uint32_t n)
{
	assert(cv_value_type(vector) == POINTLESS_VECTOR_U32);
	return pointless_create_vector_transfer_priv(c, vector, v, n);
}

uint32_t pointless_create_vector_value_transfer(pointless_create_t* c, uint32_t vector, uint32_t* v, uint32_t n)
{
	assert(cv_value_type(vector) == POINTLESS_VECTOR_VALUE || cv_value_type(vector) == POINTLESS_VECTOR_VALUE_HASHABLE);

	uint32_t i;

	for (i = 0; i < n; i++) {
		if (v[i] >= pointless_dynarray_n_items(&c->values))
			return POINTLESS_CREATE_VALUE_FAIL;
	}

	return pointless_create_vector_transfer_priv(c, vector, v, n);
}

// vectors, buffer owned by caller
uint32_t pointless_create_vector_owner_priv(pointless_create_t* c, uint32_t vector_type, void* items, uint32_t n_items)
{
	assert(vector_type != POINTLESS_VECTOR_VALUE);

	pointless_create_value_t value;
	value.data.data_u32 = pointless_dynarray_n_items(&c->outside_vector_values);

	int pop_value = 0;
	int push_vector = 1;

	pointless_create_vector_outside_t vector;

	// special case, empty vector
	if (n_items == 0) {
		value.header.type_29 = POINTLESS_VECTOR_EMPTY;
		value.header.is_compressed_vector = 0;
		value.header.is_outside_vector = 0;
		value.header.is_set_map_vector = 0;
		value.data.data_u32 = 0;
		push_vector = 0;
	// normal case
	} else {
		value.header.type_29 = vector_type;
		value.header.is_compressed_vector = 0;
		value.header.is_outside_vector = 1;
		value.header.is_set_map_vector = 0;
		vector.items = items;
		vector.n_items = n_items;
	}

	if (!pointless_dynarray_push(&c->values, &value))
		goto cleanup;

	pop_value = 1;

	if (push_vector && !pointless_dynarray_push(&c->outside_vector_values, &vector))
		goto cleanup;

	return (pointless_dynarray_n_items(&c->values) - 1);

cleanup:

	if (pop_value)
		pointless_dynarray_pop(&c->values);

	return POINTLESS_CREATE_VALUE_FAIL;
}

uint32_t pointless_create_vector_i8_owner(pointless_create_t* c, int8_t* items, uint32_t n_items)
{
	return pointless_create_vector_owner_priv(c, POINTLESS_VECTOR_I8, items, n_items);
}

uint32_t pointless_create_vector_u8_owner(pointless_create_t* c, uint8_t* items, uint32_t n_items)
{
	return pointless_create_vector_owner_priv(c, POINTLESS_VECTOR_U8, items, n_items);
}

uint32_t pointless_create_vector_i16_owner(pointless_create_t* c, int16_t* items, uint32_t n_items)
{
	return pointless_create_vector_owner_priv(c, POINTLESS_VECTOR_I16, items, n_items);
}

uint32_t pointless_create_vector_u16_owner(pointless_create_t* c, uint16_t* items, uint32_t n_items)
{
	return pointless_create_vector_owner_priv(c, POINTLESS_VECTOR_U16, items, n_items);
}

uint32_t pointless_create_vector_i32_owner(pointless_create_t* c, int32_t* items, uint32_t n_items)
{
	return pointless_create_vector_owner_priv(c, POINTLESS_VECTOR_I32, items, n_items);
}

uint32_t pointless_create_vector_u32_owner(pointless_create_t* c, uint32_t* items, uint32_t n_items)
{
	return pointless_create_vector_owner_priv(c, POINTLESS_VECTOR_U32, items, n_items);
}

uint32_t pointless_create_vector_i64_owner(pointless_create_t* c, int64_t* items, uint32_t n_items)
{
	return pointless_create_vector_owner_priv(c, POINTLESS_VECTOR_I64, items, n_items);
}

uint32_t pointless_create_vector_u64_owner(pointless_create_t* c, uint64_t* items, uint32_t n_items)
{
	return pointless_create_vector_owner_priv(c, POINTLESS_VECTOR_U64, items, n_items);
}

uint32_t pointless_create_vector_float_owner(pointless_create_t* c, float* items, uint32_t n_items)
{
	return pointless_create_vector_owner_priv(c, POINTLESS_VECTOR_FLOAT, items, n_items);
}

uint32_t pointless_create_set(pointless_create_t* c)
{
	// create the value
	pointless_create_value_t value;
	value.header.type_29 = POINTLESS_SET_VALUE;
	value.header.is_outside_vector = 0;
	value.header.is_set_map_vector = 0;
	value.header.is_compressed_vector = 0;
	value.data.data_u32 = pointless_dynarray_n_items(&c->set_values);

	int pop_value = 0;

	// create the set value
	pointless_create_set_t set;
	pointless_dynarray_init(&set.keys, sizeof(uint32_t));
	set.serialize_hash = pointless_create_vector_u32(c);
	set.serialize_keys = pointless_create_vector_value(c);

	// NOTE: possible array leak here on failure
	if (set.serialize_hash == POINTLESS_CREATE_VALUE_FAIL)
		goto cleanup;

	if (set.serialize_keys == POINTLESS_CREATE_VALUE_FAIL)
		goto cleanup;

	// override the set/map vector flag
	cv_value_at(set.serialize_hash)->header.is_set_map_vector = 1;
	cv_value_at(set.serialize_keys)->header.is_set_map_vector = 1;

	// add to value vector (note: we leak a single vector)
	if (!pointless_dynarray_push(&c->values, &value))
		goto cleanup;

	pop_value = 1;

	if (!pointless_dynarray_push(&c->set_values, &set))
		goto cleanup;

	return (pointless_dynarray_n_items(&c->values) - 1);

cleanup:

	if (pop_value)
		pointless_dynarray_pop(&c->values);

	return POINTLESS_CREATE_VALUE_FAIL;
}

uint32_t pointless_create_set_add(pointless_create_t* c, uint32_t s, uint32_t k)
{
	assert(cv_value_type(s) == POINTLESS_SET_VALUE);

	if (!pointless_dynarray_push(&cv_set_at(s)->keys, &k))
		return POINTLESS_CREATE_VALUE_FAIL;

	return s;
}

// maps
uint32_t pointless_create_map(pointless_create_t* c)
{
	// create the value
	pointless_create_value_t value;
	value.header.type_29 = POINTLESS_MAP_VALUE_VALUE;
	value.header.is_outside_vector = 0;
	value.header.is_set_map_vector = 0;
	value.header.is_compressed_vector = 0;
	value.data.data_u32 = pointless_dynarray_n_items(&c->map_values);

	int pop_value = 0;

	// create the map value
	pointless_create_map_t map;
	pointless_dynarray_init(&map.keys, sizeof(uint32_t));
	pointless_dynarray_init(&map.values, sizeof(uint32_t));

	// allocate the final hash/key/value vectors
	map.serialize_hash = pointless_create_vector_u32(c);
	map.serialize_keys = pointless_create_vector_value(c);
	map.serialize_values = pointless_create_vector_value(c);

	// NOTE: possible array leak here on failure
	if (map.serialize_hash == POINTLESS_CREATE_VALUE_FAIL)
		goto cleanup;

	if (map.serialize_keys == POINTLESS_CREATE_VALUE_FAIL)
		goto cleanup;

	if (map.serialize_values == POINTLESS_CREATE_VALUE_FAIL)
		goto cleanup;

	// override the set/map vector flag
	cv_value_at(map.serialize_hash)->header.is_set_map_vector = 1;
	cv_value_at(map.serialize_keys)->header.is_set_map_vector = 1;
	cv_value_at(map.serialize_values)->header.is_set_map_vector = 1;

	// add to value vector (note: we leak a single vector)
	if (!pointless_dynarray_push(&c->values, &value))
		goto cleanup;

	pop_value = 1;

	if (!pointless_dynarray_push(&c->map_values, &map))
		goto cleanup;

	return (pointless_dynarray_n_items(&c->values) - 1);

cleanup:

	if (pop_value)
		pointless_dynarray_pop(&c->values);

	return POINTLESS_CREATE_VALUE_FAIL;
}

uint32_t pointless_create_map_add(pointless_create_t* c, uint32_t m, uint32_t k, uint32_t v)
{
	assert(cv_value_type(m) == POINTLESS_MAP_VALUE_VALUE);

//	printf("adding k/v: %u/%u to %u\n", k, v, m);
//	printf("types %i/%i\n", (int)cv_value_type(k), (int)cv_value_type(v));

	if (!pointless_dynarray_push(&cv_map_at(m)->keys, &k))
		return POINTLESS_CREATE_VALUE_FAIL;

	if (!pointless_dynarray_push(&cv_map_at(m)->values, &v)) {
		pointless_dynarray_pop(&cv_map_at(m)->keys);
		return POINTLESS_CREATE_VALUE_FAIL;
	}

	return m;
}
