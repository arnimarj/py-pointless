#ifndef __POINTLESS__RECREATE__H__
#define __POINTLESS__RECREATE__H__

#include <stdlib.h>

#include <pointless/pointless_reader.h>
#include <pointless/pointless_create.h>

uint32_t pointless_recreate_value(pointless_t* p_in, pointless_value_t* v_in, pointless_create_t* c_out, const char** error);

int pointless_recreate_32(const char* fname_in, const char* fname_out, const char** error);
int pointless_recreate_64(const char* fname_in, const char* fname_out, const char** error);

#endif
