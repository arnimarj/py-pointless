#include <pointless/pointless_cycle_marker.h>

/*
The cycle detection algorithm is an implementation of Tarjan's algorithm: http://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm

Páll Melsted suggested this, his Python implementation follows:

def strongly_connected_components_recursive(G):
	def visit(v, cnt):
		root[v] = cnt
		visited[v] = cnt
		cnt += 1
		stack.append(v)

		for w in G[v]:
			if w not in visited:
				visit(w, cnt)
			if w not in component:
				root[v] = min(root[v],root[w])

		if root[v] == visited[v]:
			component[v] = root[v]
			tmpc = [v]

			while stack[-1] != v:
				w = stack.pop()
				component[w] = root[v]
				tmpc.append(w)

			stack.remove(v)
			scc.append(tmpc)

	scc = []
	visited = {}
	component = {}
	root = {}
	cnt = 0
	stack = []

	for source in G:
		if source not in visited:
			visit(source, cnt)

	scc.sort(key = len)
	return scc
*/

typedef struct {
	pointless_t* p;
	const char* error;
	void* cycle_marker;

	Pvoid_t visited_judy;
	Pvoid_t component_judy;
	Pvoid_t root_judy;

	pointless_dynarray_t stack;
} pointless_cycle_marker_state_t;

static uint32_t pointless_is_container(pointless_value_t* v)
{
	switch (v->type) {
		case POINTLESS_VECTOR_VALUE:
		case POINTLESS_VECTOR_VALUE_HASHABLE:
		case POINTLESS_SET_VALUE:
		case POINTLESS_MAP_VALUE_VALUE:
			return 1;
	}

	return 0;
}

static void pointless_cycle_marker_visit(pointless_cycle_marker_state_t* state, pointless_value_t* v, Word_t count, uint32_t depth);

//static void print_depth(uint32_t depth)
//{
//	uint32_t i;
//	for (i = 0; i < depth; i++)
//		printf("   ");
//}

static void process_child(pointless_cycle_marker_state_t* state, uint32_t v_id, pointless_value_t* w, Word_t count, uint32_t depth)
{
	if (!pointless_is_container(w))
		return;

	// if w not in visited: visit(w, cnt)
	uint32_t w_id = pointless_container_id(state->p, w);
	//print_depth(depth); printf("process_child(w = %u, count = %llu)\n", w_id, (unsigned long long)count);

	Pvoid_t PValue = (Pvoid_t)JudyLGet(state->visited_judy, (Word_t)w_id, 0);

	if (PValue == 0) {
		//print_depth(depth); printf(" w is not in visited\n");
		//print_depth(depth); printf("  visit(w, %llu)\n", (unsigned long long)count);

		pointless_cycle_marker_visit(state, w, count, depth + 1);

		if (state->error)
			return;
	} else {
		//print_depth(depth); printf(" w is in visited\n");
	}

	// if w not in component:
	PValue = (Pvoid_t)JudyLGet(state->component_judy, (Word_t)w_id, 0);

	if (PValue == 0) {
		//print_depth(depth); printf(" w is not in component");

		// root[v]
		Pvoid_t root_v = (Pvoid_t)JudyLGet(state->root_judy, (Word_t)v_id, 0);
		// root[w]
		Pvoid_t root_w = (Pvoid_t)JudyLGet(state->root_judy, (Word_t)w_id, 0);

		if (root_v == 0 || root_w == 0) {
			state->error = "internal error, root[v]/root[w] missing";
			return;
		}

		// root[v] = min(root[v], root[w])
		if (*((Word_t*)(root_w)) < *((Word_t*)root_v)) {
			//print_depth(depth); printf("root[v] = min(%u, %u)\n", *root_v, *root_w);
			PValue = JudyLIns(&state->root_judy, (Word_t)v_id, 0);

			if (PValue == 0) {
				state->error = "out of memory";
				return;
			}

			*((Word_t*)PValue) = *((Word_t*)root_w);
		}
	} else {
		//print_depth(depth); printf(" w is in component\n");
	}
}

static void pointless_cycle_marker_visit(pointless_cycle_marker_state_t* state, pointless_value_t* v, Word_t count, uint32_t depth)
{
	assert(pointless_is_container(v));

	if (depth >= POINTLESS_MAX_DEPTH) {
		state->error = "maximum recursion depth reached";
		return;
	}

	if (count >= (Word_t)pointless_n_containers(state->p)) {
		state->error = "internal error: pre-order count exceeds number of containers";
		return;
	}

	// if w not in visited: visit(w, cnt)
	//print_depth(depth); printf("pointless_cycle_marker_visit(v = %u, type %u, count = %llu):\n", pointless_container_id(state->p, v), v->type, (unsigned long long)count);

	uint32_t v_id = pointless_container_id(state->p, v);

	// root[v] = count
	Pvoid_t PValue = (Pvoid_t)JudyLIns(&state->root_judy, (Word_t)v_id, 0);

	if (PValue == 0) {
		state->error = "out of memory";
		return;
	}

	*((Word_t*)PValue) = count;

	//print_depth(depth); printf(" root[%u] = %llu\n", v_id, (unsigned long long)count);

	// visited[v] = count
	PValue = JudyLIns(&state->visited_judy, (Word_t)v_id, 0);

	if (PValue == 0) {
		state->error = "out of memory";
		return;
	}

	*((Word_t*)PValue) = count;

	//print_depth(depth); printf(" visited[%u] = %llu\n", v_id, (unsigned long long)count);

	// count += 1
	//print_depth(depth); printf(" count = %llu + 1\n", (unsigned long long)count);
	count += 1;

	if (count >= (Word_t)pointless_n_containers(state->p)) {
		state->error = "internal error: pre-order count exceeds number of containers";
		return;
	}

	// stack.append(v)
	if (!pointless_dynarray_push(&state->stack, &v_id)) {
		state->error = "out of memory";
		return;
	}

	//print_depth(depth); printf(" stack.append(%u)\n", v_id);

	// for each child container
	if (v->type == POINTLESS_VECTOR_VALUE || v->type == POINTLESS_VECTOR_VALUE_HASHABLE) {
		uint32_t i, n_items = pointless_reader_vector_n_items(state->p, v);
		pointless_value_t* children = pointless_reader_vector_value(state->p, v);

		for (i = 0; i < n_items; i++) {
			process_child(state, v_id, &children[i], count, depth);

			if (state->error)
				return;
		}
	} else if (v->type == POINTLESS_SET_VALUE) {
		pointless_value_t* hash_vector = pointless_set_hash_vector(state->p, v);
		pointless_value_t* key_vector = pointless_set_key_vector(state->p, v);

		process_child(state, v_id, hash_vector, count, depth);

		if (state->error)
			return;

		process_child(state, v_id, key_vector, count, depth);

		if (state->error)
			return;
	} else if (v->type == POINTLESS_MAP_VALUE_VALUE) {
		pointless_value_t* hash_vector = pointless_map_hash_vector(state->p, v);
		pointless_value_t* key_vector = pointless_map_key_vector(state->p, v);
		pointless_value_t* value_vector = pointless_map_value_vector(state->p, v);

		process_child(state, v_id, hash_vector, count, depth);

		if (state->error)
			return;

		process_child(state, v_id, key_vector, count, depth);

		if (state->error)
			return;

		process_child(state, v_id, value_vector, count, depth);

		if (state->error)
			return;
	}

	// if root[v] == visited[v]
	Pvoid_t root_v = (Pvoid_t)JudyLGet(state->root_judy, (Word_t)v_id, 0);
	Pvoid_t visited_v = (Pvoid_t)JudyLGet(state->visited_judy, (Word_t)v_id, 0);

	if (root_v == 0 || visited_v == 0) {
		state->error = "internal error: root[v]/visited[v] missing";
		return;
	}

	//print_depth(depth); printf(" if root[%u] (%u) == visited[%u] (%u)\n", v_id, *root_v, v_id, *visited_v);

	if (*((Word_t*)root_v) == *((Word_t*)visited_v)) {
		// component[v] = root[v]
		Pvoid_t PValue = (Pvoid_t)JudyLIns(&state->component_judy, (Word_t)v_id, 0);

		if (PValue == 0) {
			state->error = "out of memory";
			return;
		}

		*((Word_t*)PValue) = *((Word_t*)root_v);

		//print_depth(depth); printf("  component[%u] = root[%u] (%u)\n", v_id, v_id, *root_v);
		//print_depth(depth); printf("  while stack[-1] != %u\n", v_id);
		// while stack[-1] != v:
		while (1) {
			assert(pointless_dynarray_n_items(&state->stack) > 0);
			uint32_t* last = &pointless_dynarray_ITEM_AT(uint32_t, &state->stack, pointless_dynarray_n_items(&state->stack) - 1);

			if (*last == v_id)
				break;

			// w = stack.pop()
			uint32_t w_id = *last;
			pointless_dynarray_pop(&state->stack);
			//print_depth(depth); printf("  w = stack.pop() (%u)\n", *w_id);

			bm_set_(state->cycle_marker, w_id);

			// component[w] = root[v]
			Pvoid_t PValue = (Pvoid_t)JudyLIns(&state->component_judy, (Word_t)w_id, 0);

			if (PValue == 0) {
				state->error = "out of memory";
				return;
			}

			*((Word_t*)PValue) = *((Word_t*)root_v);

			//print_depth(depth); printf("  component[%u] = root[%u] (%u)\n", w_id, v_id, *root_v);
		}

		//print_depth(depth); printf("  len(stack) == %u\n", pointless_dynarray_n_items(&state->stack));
		//print_depth(depth); printf("  stack.pop()\n");

		// stack.remove(v) <=> stack.pop()
		pointless_dynarray_pop(&state->stack);
	}
}

void* pointless_cycle_marker(pointless_t* p, const char** error)
{
	pointless_value_t* root = 0;

	pointless_cycle_marker_state_t state;
	state.p = p;
	state.error = 0;
	state.cycle_marker = pointless_calloc(ICEIL(pointless_n_containers(p), 8), 1);
	state.visited_judy = 0;
	state.component_judy = 0;
	state.root_judy = 0;
	pointless_dynarray_init(&state.stack, sizeof(uint32_t));

	if (state.cycle_marker == 0) {
		state.error = "out of memory";
		goto error;
	}

	root = pointless_root(p);

	if (pointless_is_container(root))
		pointless_cycle_marker_visit(&state, root, 0, 0);

	if (state.error)
		goto error;

	goto cleanup;

error:

	assert(state.error != 0);

	pointless_free(state.cycle_marker);
	state.cycle_marker = 0;

	*error = state.error;

cleanup:

	JudyLFreeArray(&state.visited_judy, 0);
	state.visited_judy = 0;

	JudyLFreeArray(&state.component_judy, 0);
	state.component_judy = 0;

	JudyLFreeArray(&state.root_judy, 0);
	state.root_judy = 0;

	pointless_dynarray_destroy(&state.stack);

	return state.cycle_marker;
}
