#ifndef __POINTLESS__CYCLE__MARKER__H__
#define __POINTLESS__CYCLE__MARKER__H__

#include <pointless/pointless_defs.h>
#include <pointless/bitutils.h>

typedef uint32_t (*pointless_cycle_n_nodes)(void* user);
typedef uint64_t (*pointless_cycle_get_root)(void* user);
typedef int      (*pointless_cycle_is_container)(void* user, uint64_t node);
typedef uint32_t (*pointless_cycle_container_id)(void* user, uint64_t node);
typedef uint32_t (*pointless_cycle_n_children)(void* user, uint64_t node);
typedef uint64_t (*pointless_cycle_child_at)(void* user, uint64_t node, uint32_t i);

typedef struct {
	void* user;
	pointless_cycle_n_nodes fn_n_nodes;
	pointless_cycle_get_root fn_get_root;
	pointless_cycle_is_container fn_is_container;
	pointless_cycle_container_id fn_container_id;
	pointless_cycle_n_children fn_n_children;
	pointless_cycle_child_at fn_child_at;
} cycle_marker_info_t;

// returned buffer needs to be free()'d
void* pointless_cycle_marker(cycle_marker_info_t* info, const char** error);

#endif
