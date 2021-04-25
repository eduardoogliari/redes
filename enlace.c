#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "enlace.h"
#include "defs.h"

//####################################################################
void print_enlace_table( const enlace_table_t * table ) {
    if( !table ) { return; }
    if( table->enlace_count <= 0 ) { return; }

    puts("\n=========================");
    puts(       "ID  ID    CUSTO" );
    puts("=========================");
    for( int i = 0; i < table->enlace_count; ++i ) {
        const enlace_info_t * info = &table->enlace_arr[i];
        printf( "%d   %d   %d\n", info->left_id, info->right_id, info->cost );
    }
    puts("");
}

//####################################################################
void print_tabela_vizinho( const vizinho_table_t * table ) {
    if( !table ) { return; }

    puts("\n=========================");
    puts(       "VIZINHO  CUSTO" );
    puts("=========================");
    for( int i = 0; i < table->vizinho_count; ++i ) {
        const vizinho_t * info = &table->vizinhos_arr[i];
        printf( "%d        %d\n", info->vizinho_id, info->cost );
    }
    puts("");
}

//####################################################################
void criar_tabela_vizinhos( const enlace_table_t * enlace_table, const int router_id, vizinho_table_t * out_vizinho ) {
    if( !enlace_table ) { ENSURE(0, "criar_tabela_vizinhos(): \'enlace_table\' is NULL") return; }
    if( !out_vizinho ) { ENSURE(0, "criar_tabela_vizinhos(): \'out_vizinho\' is NULL") return; }

    int count = 0;
    {
        // Verifica a contagem de ocorrências na tabela (lado esquerdo e direito)
        for( int i = 0; i < enlace_table->enlace_count; ++i ) {
            const enlace_info_t * info = &enlace_table->enlace_arr[i];
            if( (info->left_id == router_id) || (info->right_id == router_id) ) {
                ++count;
            }
        }
    }

    if( count > 0 ) {
        const int byte_count = sizeof(vizinho_t) * count;
        vizinho_t * vizinhos_arr = malloc( byte_count );
        memset( vizinhos_arr, 0, byte_count );


        int index = 0;
        for( int i = 0; i < enlace_table->enlace_count; ++i ) {
            const enlace_info_t * info = &enlace_table->enlace_arr[i];
            if( info->left_id == router_id ) {
                vizinhos_arr[index].vizinho_id = info->right_id;
                vizinhos_arr[index].cost = info->cost;
                ++index;

            } else if( info->right_id == router_id ) {
                vizinhos_arr[index].vizinho_id = info->left_id;
                vizinhos_arr[index].cost = info->cost;
                ++index;
            }
        }

        out_vizinho->router_id     = router_id;
        out_vizinho->vizinhos_arr  = vizinhos_arr;
        out_vizinho->vizinho_count = count;
    }
}

//####################################################################
vizinho_t const * find_vizinho( const vizinho_table_t * table, const int router_id ) {
    if( !table ) { return NULL; }

    for( int i = 0; i < table->vizinho_count; ++i ) {
        if( router_id == table->vizinhos_arr[i].vizinho_id ) {
            return &table->vizinhos_arr[i];
        }
    }
    return NULL;
}