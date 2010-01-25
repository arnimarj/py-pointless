#ifndef __POINTLESS__CYCLE__MARKER__H__
#define __POINTLESS__CYCLE__MARKER__H__

#include <pointless/bitutils.h>
#include <pointless/pointless_defs.h>
#include <pointless/pointless_reader_utils.h>

// returned buffer needs to be free()'d
void* pointless_cycle_marker(pointless_t* p, const char** error);

#endif
