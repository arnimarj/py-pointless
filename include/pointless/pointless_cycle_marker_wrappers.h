#ifndef __POINTLESS__CYCLE__MARKER__WRAPPERS__H__
#define __POINTLESS__CYCLE__MARKER__WRAPPERS__H__

#include <pointless/pointless_defs.h>
#include <pointless/pointless_reader.h>

#include <pointless/pointless_cycle_marker.h>

// returned buffer needs to be free()'d
void* pointless_cycle_marker_read(pointless_t* p, const char** error);
void* pointless_cycle_marker_create(pointless_create_t* c, const char** error);

#endif
