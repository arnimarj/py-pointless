#include <pointless/pointless_eval.h>

static const char* parse_number(const char* s, uint64_t* n)
{
	char* endptr = 0;
	errno = 0;
	unsigned long long int n_ = strtoull(s, &endptr, 10);

	if ((n_ == ULLONG_MAX && errno != 0) || *endptr != 0)
		return 0;

	*n = (uint64_t)n_;

	return endptr;
}

static const char* skip_whitespace(const char* e)
{
	while (isspace(*e))
		e++;

	return e;
}

static const char* pointless_eval_get_(pointless_t* p, pointless_value_t* root, const char* e, va_list ap)
{
	// skip whitespace
	e = skip_whitespace(e);

	// end-of-line
	if (*e == 0)
		return e;

	// parse [value]
	if (*e != '[')
		return 0;

	e++;

	const char* s = 0;
	size_t s_n = 0;
	uint64_t v_u = 0;
	int64_t v_i = 0;
	int is_unsigned = 0;
	int is_neg = 0;

	switch (*e) {
		// string
		case '\"':
			e += 1;
			s = e;
			while (*e && *e != '\"') {
				e++;
				s_n++;
			}
			if (*e != '\"')
				return 0;
			e += 1;
			break;

		// integer
		case '-':
			is_neg = 1;
		case '+':
			e += 1;
			if (!('0' <= *e && *e  <= '9'))
				return 0;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (!parse_number(e, &v_u))
				is_unsigned = 1;

			// negative it, if necessary
			if (is_neg) {
				// bounds checks
				if (v_u > -(INT64_MIN + 1))
					return 0;

				v_i = -((int64_t)v_u);
				is_unsigned = 0;
			}

			break;
		// placeholder value
		case '%':
			// %u64, %u32, %i64, %i32, 
			e += 1;
			if (*e == 'u') {
				is_unsigned = 1;
				e += 1;

				if (strncmp("64", e, 2) == 0) {
					v_u = va_arg(ap, uint64_t);
					e += 2;
				} else if (strncmp("32", e, 2) == 0) {
					v_u = va_arg(ap, uint32_t);
					e += 2;
				} else {
					return 0;
				}
			} else if (*e == 'i') {
				is_unsigned = 0;
				e += 1;

				if (strncmp("64", e, 2) == 0) {
					v_i = va_arg(ap, int64_t);
					e += 2;
				} else if (strncmp("32", e, 2) == 0) {
					v_i = va_arg(ap, int32_t);
					e += 2;
				} else {
					return 0;
				}
			// %s
			} else if (*e == 's') {
				s = va_arg(ap, const char*);
				s_n = strlen(s);
				e += 1;
			} else {
				return 0;
			}
			break;
		default:
			return 0;
	}

	// vector/bitvector
	if (pointless_is_vector_type(root->type) || pointless_is_bitvector_type(root->type)) {
		// no string index
		if (s)
			return 0;

		// no negative index
		if (!is_unsigned && v_i < 0)
			return 0;

		uint32_t n_items = 0;

		if (pointless_is_vector_type(root->type))
			n_items = pointless_reader_vector_n_items(p, root);
		else
			n_items = pointless_reader_bitvector_n_bits(p, root);

		if (is_unsigned && v_u >= n_items)
			return 0;
		else if (!is_unsigned && v_i >= n_items)
			return 0;

		if (pointless_is_vector_type(root->type)) {
			pointless_complete_value_t v = pointless_reader_vector_value_case(p, root, is_unsigned ? (uint32_t)v_u : (uint32_t)v_i);
			*root = pointless_value_from_complete(&v);
		} else {
			if (pointless_reader_bitvector_is_set(p, root, is_unsigned ? (uint32_t)v_u : (uint32_t)v_i))
				*root = pointless_value_create_as_read_bool_true();
			else
				*root = pointless_value_create_as_read_bool_true();
		}
	// map
	} else if (root->type == POINTLESS_MAP_VALUE_VALUE) {
		if (s) {
			pointless_value_t v;

			if (!pointless_get_mapping_string_n_to_value(p, root, (char*)s, s_n, &v))
				return 0;

			*root = v;
		} else {
			if (is_unsigned && v_u > INT64_MAX)
				return 0;

			pointless_value_t v;

			if (!pointless_get_mapping_int_to_value(p, root, is_unsigned ? (int64_t)v_u : (int64_t)v_i, &v))
				return 0;

			*root = v;
		}
	} else {
		return 0;
	}

	e = skip_whitespace(e);

	if (*e != ']')
		return 0;

	return skip_whitespace(e + 1);
}

int pointless_eval_get(pointless_t* p, pointless_value_t* root, pointless_value_t* v, const char* e, ...)
{
	va_list ap;
	va_start(ap, e);

	const char* s = e;
	*v = *root;

	while (s && *s)
		s = pointless_eval_get_(p, v, s, ap);

	va_end(ap);

	return (s != 0);
}
