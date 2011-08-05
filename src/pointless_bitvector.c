#include <pointless/pointless_bitvector.h>

static uint32_t pointless_bitvector_is_set_bits(uint32_t t, pointless_value_data_t* v, void* bits, uint32_t bit)
{
	switch (t) {
		case POINTLESS_BITVECTOR:
			return (bm_is_set_(bits, bit) != 0);
		case POINTLESS_BITVECTOR_0:
			return 0;
		case POINTLESS_BITVECTOR_1:
			return 1;
		case POINTLESS_BITVECTOR_01:
			return (v->bitvector_01_or_10.n_bits_a <= bit);
		case POINTLESS_BITVECTOR_10:
			return (bit < v->bitvector_01_or_10.n_bits_a);
		case POINTLESS_BITVECTOR_PACKED:
			// this is quite hacky
			return (bm_is_set_((void*)&v->data_u32, bit + 5) != 0);
	}

	assert(0);
	return 0;
}

uint32_t pointless_bitvector_is_any_set(uint32_t t, pointless_value_data_t* v, void* bits)
{
	// easy stuff
	switch (t) {
		case POINTLESS_BITVECTOR_0:
			return 0;
		case POINTLESS_BITVECTOR_1:
			return (v->data_u32 > 0);
		case POINTLESS_BITVECTOR_01:
			return (v->bitvector_01_or_10.n_bits_b > 0);
		case POINTLESS_BITVECTOR_10:
			return (v->bitvector_01_or_10.n_bits_a > 0);
	}

	// otherwise, just do bitwise test
	uint64_t i;
	uint32_t n = pointless_bitvector_n_bits(t, v, bits);

	for (i = 0; i < n; i++) {
		if (pointless_bitvector_is_set_bits(t, v, bits, i))
			return 1;
	}

	return 0;
}

static void* pointless_bitvector_bits(void* buffer)
{
	return (void*)((uint32_t*)buffer + 1);
}

uint32_t pointless_bitvector_n_bits(uint32_t t, pointless_value_data_t* v, void* buffer)
{
	switch (t) {
		case POINTLESS_BITVECTOR:
			return *((uint32_t*)((char*)buffer));
		case POINTLESS_BITVECTOR_0:
		case POINTLESS_BITVECTOR_1:
			return v->data_u32;
		case POINTLESS_BITVECTOR_01:
		case POINTLESS_BITVECTOR_10:
			return (v->bitvector_01_or_10.n_bits_a + v->bitvector_01_or_10.n_bits_b);
		case POINTLESS_BITVECTOR_PACKED:
			return (v->bitvector_packed.n_bits);
	}

	assert(0);
	return 0;
}

#define HASH_BITVECTOR_SEED 1000000001L

uint32_t pointless_bitvector_hash_32_priv(uint32_t t, pointless_value_data_t* v, uint32_t n_bits, void* bits)
{
	uint64_t i = 0;
	uint32_t b = 0, h = 1, j;

	while (i < n_bits) {
		b = 0;
		j = 0;

		while (j < 8 && i < n_bits) {
			if (pointless_bitvector_is_set_bits(t, v, bits, i))
				b |= (1 << j);

			j += 1;
			i += 1;
		}

		h = h * HASH_BITVECTOR_SEED + b;
	}

	return h;
}

uint64_t pointless_bitvector_hash_64_priv(uint32_t t, pointless_value_data_t* v, uint32_t n_bits, void* bits)
{
	uint64_t b = 0, i = 0, h = 1, j;

	while (i < n_bits) {
		b = 0;
		j = 0;

		while (j < 8 && i < n_bits) {
			if (pointless_bitvector_is_set_bits(t, v, bits, i))
				b |= (1 << j);

			j += 1;
			i += 1;
		}

		h = h * HASH_BITVECTOR_SEED + b;
	}

	return h;
}


uint32_t pointless_bitvector_is_set(uint32_t t, pointless_value_data_t* v, void* buffer, uint32_t bit)
{
	void* bits = 0;

	if (t == POINTLESS_BITVECTOR)
		bits = pointless_bitvector_bits(buffer);

	return pointless_bitvector_is_set_bits(t, v, bits, bit);
}

uint32_t pointless_bitvector_hash_32(uint32_t t, pointless_value_data_t* v, void* buffer)
{
	void* bits = 0;
	uint32_t n_bits = pointless_bitvector_n_bits(t, v, buffer);

	if (t == POINTLESS_BITVECTOR)
		bits = pointless_bitvector_bits(buffer);

	return pointless_bitvector_hash_32_priv(t, v, n_bits, bits);
}

uint64_t pointless_bitvector_hash_64(uint32_t t, pointless_value_data_t* v, void* buffer)
{
	void* bits = 0;
	uint32_t n_bits = pointless_bitvector_n_bits(t, v, buffer);

	if (t == POINTLESS_BITVECTOR)
		bits = pointless_bitvector_bits(buffer);

	return pointless_bitvector_hash_64_priv(t, v, n_bits, bits);
}

int32_t pointless_bitvector_cmp_buffer_buffer(uint32_t t_a, pointless_value_data_t* v_a, void* buffer_a, uint32_t t_b, pointless_value_data_t* v_b, void* buffer_b)
{
	uint32_t n_bits_a = pointless_bitvector_n_bits(t_a, v_a, buffer_a);
	uint32_t n_bits_b = pointless_bitvector_n_bits(t_b, v_b, buffer_b);
	uint32_t n_bits = (n_bits_a < n_bits_b) ? n_bits_a : n_bits_b;

	uint64_t i;
	uint32_t ba, bb;

	for (i = 0; i < n_bits; i++) {
		ba = pointless_bitvector_is_set(t_a, v_a, buffer_a, i);
		bb = pointless_bitvector_is_set(t_a, v_b, buffer_b, i);

		if (ba != bb)
			return SIMPLE_CMP(ba, bb);
	}

	return SIMPLE_CMP(n_bits_a, n_bits_b);
}

int32_t pointless_bitvector_cmp_bits_buffer(uint32_t n_bits_a, void* bits_a, pointless_value_t* v_b, void* buffer_b)
{
	uint32_t n_bits_b = pointless_bitvector_n_bits(v_b->type, &v_b->data, buffer_b);
	uint32_t n_bits = (n_bits_a < n_bits_b) ? n_bits_a : n_bits_b;
	uint64_t i;
	uint32_t ba, bb;

	for (i = 0; i < n_bits; i++) {
		ba = (bm_is_set_(bits_a, i) != 0);
		bb = pointless_bitvector_is_set(v_b->type, &v_b->data, buffer_b, i);

		if (ba != bb)
			return SIMPLE_CMP(ba, bb);
	}

	return SIMPLE_CMP(n_bits_a, n_bits_b);
}

int32_t pointless_bitvector_cmp_buffer_bits(pointless_value_t* v_a, void* buffer_a, uint32_t n_bits_b, void* bits_b)
{
	uint32_t n_bits_a = pointless_bitvector_n_bits(v_a->type, &v_a->data, buffer_a);
	uint32_t n_bits = (n_bits_a < n_bits_b) ? n_bits_a : n_bits_b;
	uint64_t i;
	uint32_t ba, bb;

	for (i = 0; i < n_bits; i++) {
		ba = pointless_bitvector_is_set(v_a->type, &v_a->data, buffer_a, i);
		bb = (bm_is_set_(bits_b, i) != 0);

		if (ba != bb)
			return SIMPLE_CMP(ba, bb);
	}

	return SIMPLE_CMP(n_bits_a, n_bits_b);
}

uint32_t pointless_bitvector_hash_buffer_32(void* buffer)
{
	pointless_value_t v;
	v.type = POINTLESS_BITVECTOR;
	v.data.data_u32 = 0;

	uint32_t n_bits = pointless_bitvector_n_bits(v.type, &v.data, buffer);
	void* bits = pointless_bitvector_bits(buffer);

	return pointless_bitvector_hash_32_priv(v.type, &v.data, n_bits, bits);
}

uint64_t pointless_bitvector_hash_buffer_64(void* buffer)
{
	pointless_value_t v;
	v.type = POINTLESS_BITVECTOR;
	v.data.data_u32 = 0;

	uint32_t n_bits = pointless_bitvector_n_bits(v.type, &v.data, buffer);
	void* bits = pointless_bitvector_bits(buffer);

	return pointless_bitvector_hash_64_priv(v.type, &v.data, n_bits, bits);
}

uint32_t pointless_bitvector_hash_n_bits_bits_32(uint32_t n_bits, void* bits)
{
	pointless_value_t v;
	v.type = POINTLESS_BITVECTOR;
	v.data.data_u32 = 0;

	return pointless_bitvector_hash_32_priv(v.type, &v.data, n_bits, bits);
}

uint64_t pointless_bitvector_hash_n_bits_bits_64(uint32_t n_bits, void* bits)
{
	pointless_value_t v;
	v.type = POINTLESS_BITVECTOR;
	v.data.data_u32 = 0;

	return pointless_bitvector_hash_64_priv(v.type, &v.data, n_bits, bits);
}

int32_t pointless_bitvector_cmp_buffer(void* a, void* b)
{
	pointless_value_t v;
	v.type = POINTLESS_BITVECTOR;
	v.data.data_u32 = 0;
	return pointless_bitvector_cmp_buffer_buffer(v.type, &v.data, a, v.type, &v.data, b);
}
