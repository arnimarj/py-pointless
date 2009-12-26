#ifndef __POINTLESS__WALK__H__
#define __POINTLESS__WALK__H__

#include <assert.h>

#include <pointless/pointless_defs.h>
#include <pointless/pointless_reader_utils.h>

typedef uint32_t (*pointless_walk_cb)(pointless_t* p, pointless_value_t* v, uint32_t depth, void* user);

#define POINTLESS_WALK_VISIT_CHILDREN 0
#define POINTLESS_WALK_MOVE_UP 1
#define POINTLESS_WALK_STOP 2

void pointless_walk(pointless_t* p, pointless_walk_cb cb, void* user);

#endif
