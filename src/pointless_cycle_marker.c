#include <pointless/pointless_cycle_marker.h>

/*
The cycle detection algorithm is an implementation of Tarjan's algorithm: http://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm

Páll Melsted suggested this, his Python implementation follows:

def strongly_connected_components_recursive(G):
	def visit(v, count):
		root[v] = count
		visited[v] = count
		count += 1
		stack.append(v)

		for w in G[v]:
			if w not in visited:
				visit(w, count)
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
	count = 0
	stack = []

	for source in G:
		if source not in visited:
			visit(source, count)

	scc.sort(key = len)
	return scc
*/

typedef struct {
	cycle_marker_info_t* cb_info;
	const char* error;
	void* cycle_marker;

	Pvoid_t visited_judy;
	Pvoid_t component_judy;
	Pvoid_t root_judy;

	pointless_dynarray_t stack;
} pointless_cycle_marker_state_t;

static void pointless_cycle_marker_visit(pointless_cycle_marker_state_t* state, uint64_t v, Word_t count, uint32_t depth);

//static void print_depth(uint32_t depth)
//{
//	uint32_t i;
//	for (i = 0; i < depth; i++)
//		printf("   ");
//}

static void process_child(pointless_cycle_marker_state_t* state, uint32_t v_id, uint64_t w, Word_t count, uint32_t depth)
{
	if (!(*state->cb_info->fn_is_container)(state->cb_info->user, w))
		return;

	// if w not in visited: visit(w, count)
	uint32_t w_id = (*state->cb_info->fn_container_id)(state->cb_info->user, w);
	//print_depth(depth); printf("process_child(w = %u, count = %llu)\n", (unsigned int)w_id, (unsigned long long)count);

	Word_t* PValue = (Word_t*)(Pvoid_t)JudyLGet(state->visited_judy, (Word_t)w_id, 0);

	if (PValue == 0) {
		//print_depth(depth); printf(" w is not in visited, so visit(w = %u, count = %llu)\n", (unsigned int)w_id, (unsigned long long)count);

		pointless_cycle_marker_visit(state, w, count, depth + 1);

		if (state->error)
			return;
	} else {
		//print_depth(depth); printf(" w is in visited\n");
	}

	// if w not in component:
	PValue = (Word_t*)(Pvoid_t)JudyLGet(state->component_judy, (Word_t)w_id, 0);

	if (PValue == 0) {
		//print_depth(depth); printf(" w is not in component\n");

		// root[v]
		Word_t* root_v = (Word_t*)(Pvoid_t)JudyLGet(state->root_judy, (Word_t)v_id, 0);
		// root[w]
		Word_t* root_w = (Word_t*)(Pvoid_t)JudyLGet(state->root_judy, (Word_t)w_id, 0);

		if (root_v == 0 || root_w == 0) {
			state->error = "internal error, root[v]/root[w] missing";
			return;
		}

		// root[v] = min(root[v], root[w])
		if (*root_w < *root_v) {
			//print_depth(depth); printf("root[v] = min(%u, %u)\n", *(unsigned int*)root_v, *(unsigned int*)root_w);
			PValue = (Word_t*)(Pvoid_t)JudyLIns(&state->root_judy, (Word_t)v_id, 0);

			if (PValue == 0) {
				state->error = "out of memory";
				return;
			}

			*PValue = *root_w;
		}
	} else {
		//print_depth(depth); printf(" w is in component\n");
	}
}

static void _unpack_map_and_vector(uint64_t v, uint32_t* owner, uint32_t* value)
{
	*owner = (uint32_t)(v >> 32);
	*value = (uint32_t)(v & UINT32_MAX);
}

//static char buffer[1000];
//static char* depth_buffer(uint32_t depth)
//{
//	uint32_t i;
//	for (i = 0; i < depth * 3; i++)
//		buffer[i] = ' ';
//	buffer[i] = 0;
//	return buffer;
//}

static void pointless_cycle_marker_visit(pointless_cycle_marker_state_t* state, uint64_t v, Word_t count, uint32_t depth)
{
	assert((*state->cb_info->fn_is_container)(state->cb_info->user, v));

	if (depth >= POINTLESS_MAX_DEPTH) {
		state->error = "maximum recursion depth reached";
		return;
	}

	if (count >= (Word_t)((*state->cb_info->fn_n_nodes)(state->cb_info->user))) {
		//printf("B depth %u, count %u\n", depth, (unsigned int)count);
		state->error = "internal error: pre-order count exceeds number of containers";
		return;
	}

	// if w not in visited: visit(w, count)
	uint32_t v_id = (*state->cb_info->fn_container_id)(state->cb_info->user, v);
	//print_depth(depth); printf("pointless_cycle_marker_visit(v = %u, count = %llu):\n", (unsigned int)v_id, (unsigned long long)count);

	// root[v] = count
	Word_t* PValue = (Word_t*)(Pvoid_t)JudyLIns(&state->root_judy, (Word_t)v_id, 0);

	if (PValue == 0) {
		state->error = "out of memory";
		return;
	}

	*PValue = count;
	//print_depth(depth); printf(" root[%u] = %llu\n", v_id, (unsigned long long)count);

	// visited[v] = count
	PValue = (Word_t*)(Pvoid_t)JudyLIns(&state->visited_judy, (Word_t)v_id, 0);

	if (PValue == 0) {
		state->error = "out of memory";
		return;
	}

	*((Word_t*)PValue) = count;
	//print_depth(depth); printf(" visited[%u] = %llu\n", v_id, (unsigned long long)count);

	//print_depth(depth); printf(" count = %llu + 1\n", (unsigned long long)count);

	// stack.append(v)
	//print_depth(depth); printf(" stack.append(v = %u)\n", (unsigned int)v_id);
	if (!pointless_dynarray_push(&state->stack, &v_id)) {
		state->error = "out of memory";
		return;
	}

	uint32_t i, n_children = (*state->cb_info->fn_n_children)(state->cb_info->user, v);
	uint32_t owner, value;
	_unpack_map_and_vector(v, &owner, &value);
	//printf("%s%u/%u n-children: %u\n", depth_buffer(depth), owner, value, n_children);

	for (i = 0; i < n_children; i++) {
		uint64_t child = (*state->cb_info->fn_child_at)(state->cb_info->user, v, i);
		_unpack_map_and_vector(child, &owner, &value);
		//printf(" %schild %u/%u\n", depth_buffer(depth), owner, value);

		// this algorithm does not support single-node cycles, so check for them manuall
		if ((*state->cb_info->fn_is_container)(state->cb_info->user, child)) {
			uint32_t parent_container_id = (*state->cb_info->fn_container_id)(state->cb_info->user, v);
			uint32_t child_container_id  =(*state->cb_info->fn_container_id)(state->cb_info->user, child);

			//printf("%s  and it is a container\n", depth_buffer(depth));
			//printf("%s  parent/child-ids: %i/%i\n", depth_buffer(depth), (int)parent_container_id, (int)child_container_id);

			if (parent_container_id == child_container_id) {
				//printf("zero cycle %u\n", (unsigned int)parent_container_id);
				bm_set_(state->cycle_marker, parent_container_id);
				continue;
			}
		}

		process_child(state, v_id, child, count + 1, depth + 1);

		if (state->error)
			return;
	}

	// if root[v] == visited[v]
	Word_t* root_v = (Word_t*)(Pvoid_t)JudyLGet(state->root_judy, (Word_t)v_id, 0);
	Word_t* visited_v = (Word_t*)(Pvoid_t)JudyLGet(state->visited_judy, (Word_t)v_id, 0);

	if (root_v == 0 || visited_v == 0) {
		state->error = "internal error: root[v]/visited[v] missing";
		return;
	}

	//print_depth(depth); printf(" if root[%u] (%u) == visited[%u] (%u)\n", (unsigned int)v_id, (unsigned int)(*root_v), (unsigned int)v_id, (unsigned int)(*visited_v));

	if (*root_v == *visited_v) {
		// component[v] = root[v]
		Word_t* PValue = (Word_t*)(Pvoid_t)JudyLIns(&state->component_judy, (Word_t)v_id, 0);

		if (PValue == 0) {
			state->error = "out of memory";
			return;
		}

		*PValue = *root_v;

		//print_depth(depth); printf("  component[%u] = root[%u] (%u)\n", (unsigned int)v_id, (unsigned int)v_id, (unsigned int)(*root_v));
		//print_depth(depth); printf("  while stack[-1] != %u\n", (unsigned int)v_id);

		// while stack[-1] != v:
		while (1) {
			assert(pointless_dynarray_n_items(&state->stack) > 0);
			uint32_t last = pointless_dynarray_ITEM_AT(uint32_t, &state->stack, pointless_dynarray_n_items(&state->stack) - 1);

			if (last == v_id)
				break;

			// w = stack.pop()
			uint32_t w_id = last;
			pointless_dynarray_pop(&state->stack);
			//print_depth(depth); printf("   w = stack.pop() (%u)\n", (unsigned int)w_id);

			//printf("B marking %u\n", w_id);
			bm_set_(state->cycle_marker, w_id);

			// component[w] = root[v]
			Word_t* PValue = (Word_t*)(Pvoid_t)JudyLIns(&state->component_judy, (Word_t)w_id, 0);

			if (PValue == 0) {
				state->error = "out of memory";
				return;
			}

			*PValue = *root_v;

			//print_depth(depth); printf("   component[%u] = root[%u] (%u)\n", (unsigned int)w_id, (unsigned int)v_id, *(unsigned int*)root_v);
		}

		//print_depth(depth); printf("  len(stack) == %u\n", (unsigned int)pointless_dynarray_n_items(&state->stack));
		//print_depth(depth); printf("  stack.pop()\n");

		// stack.remove(v) <=> stack.pop()
		pointless_dynarray_pop(&state->stack);
	}
}

void* pointless_cycle_marker(cycle_marker_info_t* info, const char** error)
{
	uint32_t n_nodes = (*info->fn_n_nodes)(info->user);
	uint64_t root;

	pointless_cycle_marker_state_t state;
	state.cb_info = info;
	state.error = 0;
	state.cycle_marker = pointless_calloc(ICEIL(n_nodes, 8), 1);
	state.visited_judy = 0;
	state.component_judy = 0;
	state.root_judy = 0;
	pointless_dynarray_init(&state.stack, sizeof(uint32_t));

	if (state.cycle_marker == 0) {
		state.error = "out of memory";
		goto error_cleanup;
	}

	root = (*info->fn_get_root)(state.cb_info->user);

	if ((*info->fn_is_container)(state.cb_info->user, root))
		pointless_cycle_marker_visit(&state, root, 0, 0);

	if (state.error)
		goto error_cleanup;

	for (uint32_t i = 0; i < n_nodes; i++) {
		//printf("cycle %i: %i\n", (int)i, bm_is_set_(state.cycle_marker, i) ? 1 : 0);
	}

	goto cleanup;

error_cleanup:

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
