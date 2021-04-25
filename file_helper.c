#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_helper.h"
#include "defs.h"

static const char separator[] = " ";


//####################################################################
int read_router_config( const char * filename, router_info_table_t * out_table ) {
    if( !filename ) { ENSURE(0, "read_router_config(): \'filename\' is NULL") return 0; }
    if( !out_table ) { ENSURE(0, "read_router_config(): \'out_table\' is NULL") return 0; }


    FILE * file = fopen( filename, "r" );
    if( !file ) {
        ENSURE(0, "read_router_config(): error when opening file \'%s\'", filename );
        return 0;
    }

    // Le contagem de linhas
    int line_count = 0;
    {
        char line[256];
        while( fgets( line, 256, file ) != NULL ) {
            // Desconsidera linhas vazias
            if( line[0] != '\0' && line[0] != ' ' && line[0] != '\r' && line[0] != '\t' && line[0] != '\n' ) {
                ++line_count;
            }
        }

        if( line_count == 0 ) {
            ENSURE(0, "read_router_config(): file \'%s\' has zero line count", filename );
            return 0;
        }
    }
    fseek( file, 0, SEEK_SET );


    // Aloca memoria para 'line_count' roteadores
    const unsigned long byte_count = sizeof(router_info_t) * line_count;
    router_info_t * info_arr = malloc( byte_count );
    memset( info_arr, 0, byte_count );


    int index = 0;
    char line[256];
    while( fgets( line, 256, file ) != NULL ) {
        const int len_index = strlen(line)-1;
        if( line[len_index] == '\n' ) { line[len_index] = '\0'; }

        char values[3][256];
        memset( values[0], 0, 256 );
        memset( values[1], 0, 256 );
        memset( values[2], 0, 256 );

        int parameter_count = 0;

        // Le 3 tokens separados por espaço
        {
            char * token = strtok( line, separator );
            while( token != NULL ) {
                if( !remove_spaces( token, values[parameter_count], 256 ) ) {
                    ENSURE(0, "read_router_config(): on remove_spaces()");
                    return 0;
                }

                token = strtok(NULL, separator);
                ++parameter_count;
            }
        }

        if( parameter_count != 3 ) {
            ENSURE(0, "read_router_config(): invalid parameter count of %d when reading file \'%s\'", parameter_count, filename );
            SAFE_FREE(info_arr);
            return 0;
        }

        // Copia router_info
        {
            info_arr[index].id = strtol( values[0], NULL, 10 );
            info_arr[index].port_number = strtol( values[1], NULL, 10 );
            strcpy( info_arr[index].ip_address, values[2] );
        }
        ++index;

        memset(line, 0, 256);
    }

    out_table->routers_arr = info_arr;
    out_table->router_count = line_count;
    return 1;
}

//####################################################################
int read_enlace_config( const char * filename, enlace_table_t * out_table ) {
    if( !filename ) { ENSURE(0, "read_enlace_config(): \'filename\' is NULL") return 0; }
    if( !out_table ) { ENSURE(0, "read_enlace_config(): \'out_table\' is NULL") return 0; }

    FILE * file = fopen( filename, "r" );
    if( !file ) {
        ENSURE(0, "read_enlace_config(): error when opening file \'%s\'", filename );
        return 0;
    }

    // Read line count
    int line_count = 0;
    {
        char line[256];
        while( fgets( line, 256, file ) != NULL ) { ++line_count; }

        if( line_count == 0 ) {
            ENSURE(0, "read_enlace_config(): file \'%s\' has zero line count", filename );
            return 0;
        }
    }
    fseek( file, 0, SEEK_SET );


    // Allocate memory for 'line_count' routers
    const unsigned long byte_count = sizeof(enlace_info_t) * line_count;
    enlace_info_t * info_arr = malloc( byte_count );
    memset( info_arr, 0, byte_count );


    int index = 0;
    char line[256];
    while( fgets( line, 256, file ) != NULL ) {

        char values[3][256];
        memset( values[0], 0, 256 );
        memset( values[1], 0, 256 );
        memset( values[2], 0, 256 );

        int parameter_count = 0;

        // Reads 3 tokens separated by space
        {
            char * token = strtok( line, separator );
            while( token != NULL ) {
                if( !remove_spaces( token, values[parameter_count], 256 ) ) {
                    ENSURE(0, "read_enlace_config(): on remove_spaces()");
                    return 0;
                }

                token = strtok(NULL, separator);
                ++parameter_count;
            }
        }

        if( parameter_count != 3 ) {
            ENSURE(0, "read_enlace_config(): invalid parameter count of %d when reading file \'%s\'", parameter_count, filename );
            SAFE_FREE(info_arr);
            return 0;
        }

        // Copy router info
        {
            info_arr[index].left_id = strtol( values[0], NULL, 10 );
            info_arr[index].right_id = strtol( values[1], NULL, 10 );
            info_arr[index].cost = strtol( values[2], NULL, 10 );
        }
        ++index;
    }

    out_table->enlace_arr = info_arr;
    out_table->enlace_count = line_count;
    return 1;
}

//####################################################################
int remove_spaces( const char * src, char * dst, const int dst_size ) {
    if( !src ) { return 0; }
    if( !dst ) { return 0; }
    if( dst_size <= 0 ) { return 0; }

    const int len = strlen(src) + 1;
    if( dst_size < len ) { return 0; }

    int y = 0;
    for( int i = 0; i < len; ++i ) {
        if( src[i] == ' ' ) { continue; }
        dst[y] = src[i];
        ++y;
    }

    return 1;
}