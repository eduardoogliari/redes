#include <stdio.h>
#include "router.h"

//####################################################################
void print_router_info_table( const router_info_table_t * table ) {
    if( !table ) { return; }
    if( table->router_count <= 0 ) { return; }

    puts("\n=========================");
    puts(       "ID  PORT    IP" );
    puts("=========================");
    for( int i = 0; i < table->router_count; ++i ) {
        const router_info_t * info = &table->routers_arr[i];
        printf( "%d   %d   %s\n", info->id, info->port_number, info->ip_address );
    }
    puts("");
}

//####################################################################
router_info_t const * find_router_info( const router_info_table_t * table, const int id_to_find ) {
    if( !table ) { return NULL; }

    for( int i = 0; i < table->router_count; ++i ) {
        router_info_t const * info = &table->routers_arr[i];
        if( info->id == id_to_find ) {
            return info;
        }
    }
    return NULL;
}