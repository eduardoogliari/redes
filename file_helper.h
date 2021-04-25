#ifndef FILE_HELPER_H
#define FILE_HELPER_H

#include "router.h"
#include "enlace.h"

int read_router_config( const char * filename, router_info_table_t * out_table );
int read_enlace_config( const char * filename, enlace_table_t * out_table );

int remove_spaces( const char * src, char * dst, const int dst_size );

#endif