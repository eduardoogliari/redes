#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "defs.h"
#include "enlace.h"
#include "vetor_dist.h"

static void grow_dist_table( vetor_dist_table_t ** table );


//####################################################################
void init_tabela_vetores( vetor_dist_table_t * table ) {
    if( !table ) { return; }

    table->vetor_arr = NULL;
    table->vetor_count = 0;
    table->capacity = 0;
    pthread_mutex_init( &table->mutex, NULL );
}

//####################################################################
void init_vetor_dist( vetor_dist_t * vetor, const vizinho_table_t * vizinhos_table, const router_info_table_t * router_info_table ) {
    if( !vetor ) { return; }
    if( !vizinhos_table ) { return; }
    if( !router_info_table ) { return; }

    // ID do roteador
    vetor->router_id = vizinhos_table->router_id;

    // Quantos componentes o vetor terá
    {
        vetor->router_count = router_info_table->router_count;
        ENSURE( (router_info_table->router_count < ROUTER_MAX_COUNT), "init_vetor_dist(): quantidade de roteadores excede ROUTER_MAX_COUNT (%d)", ROUTER_MAX_COUNT );
    }

    // Zera array
    memset( vetor->router_dist_arr, 0, sizeof(par_dist_t) * ROUTER_MAX_COUNT );

    // Inicializa os pares <id destino, distância>
    for( int i = 0; i < vetor->router_count; ++i ) {
        // ID destino
        const int id_destino = router_info_table->routers_arr[i].id;
        vetor->router_dist_arr[i].router_dest_id = id_destino;

        // Distancia
        if( id_destino == vetor->router_id ) {
            // Ele mesmo, distancia igual a zero
            vetor->router_dist_arr[i].distance = 0;

        } else {
            int distancia = VETOR_DIST_INFINITE;
            {
                vizinho_t const * vizinho = find_vizinho( vizinhos_table, id_destino );
                if( vizinho ) {
                    // É vizinho
                    distancia = vizinho->cost;
                } else {
                    // Não é vizinho, distancia inicial inifinita
                }
            }
            vetor->router_dist_arr[i].distance = distancia;
        }
    }

    vetor->timestamp = time(NULL);
}

//####################################################################
void add_or_replace_vetor_dist( vetor_dist_table_t * table, vetor_dist_t * vetor ) {
    if( !table ) { return; }
    if( !vetor ) { return; }

    int res = -1;

    res = pthread_mutex_lock( &table->mutex );
    if( res != 0 ) { ENSURE(0, "add_vetor_dist(): failed at \'pthread_mutex_lock()\'\n%s", strerror(errno)); exit(1); }

    // LOCK
    {
        int existe_index = -1;
        for( int i = 0; i < table->vetor_count; ++i ) {
            if( table->vetor_arr[i].router_id == vetor->router_id ) {
                existe_index = i;
                break;
            }
        }

        if( existe_index >= 0 ) {
            // Ja existe, simplemente sobreescreve
            table->vetor_arr[existe_index] = *vetor;

        } else {
            // Não existe, adiciona
            if( table->vetor_count == table->capacity) {
                grow_dist_table( &table );
            }

            const int index = table->vetor_count;
            table->vetor_arr[index] = *vetor;

            ++table->vetor_count;
        }
    }
    // UNLOCK

    res = pthread_mutex_unlock( &table->mutex );
    if( res != 0 ) { ENSURE(0, "add_vetor_dist(): failed at \'pthread_mutex_unlock()\'\n%s", strerror(errno)); exit(1); }
}

//####################################################################
void remove_vetor_dist( vetor_dist_table_t * table, const int id_to_remove ) {
    if( !table ) { return; }


    int res = -1;
    res = pthread_mutex_lock( &table->mutex );
    if( res != 0 ) { ENSURE(0, "remove_vetor_dist(): failed at \'pthread_mutex_lock()\'\n%s", strerror(errno)); exit(1); }

    // LOCK
    {
        const int count = table->vetor_count;
        if( count > 0 ) {

            for( int i = 0; i < table->vetor_count; ++i ) {
                if( table->vetor_arr[i].router_id == id_to_remove ) {
                    if( count > 1 ) {
                        // Swap with last
                        table->vetor_arr[i] = table->vetor_arr[count-1];
                    }
                }
            }
            --table->vetor_count;
        }
    }
    // UNLOCK

    res = pthread_mutex_unlock( &table->mutex );
    if( res != 0 ) { ENSURE(0, "remove_vetor_dist(): failed at \'pthread_mutex_unlock()\'\n%s", strerror(errno)); exit(1); }
}

//####################################################################
int get_vector( vetor_dist_table_t * table, const int id_to_find, vetor_dist_t * out_vector ) {
    if( !table ) { return 0; }
    if( !out_vector ) { return 0; }

    int found = 0;

    int res = pthread_mutex_lock( &table->mutex );
    if( res != 0 ) { ENSURE(0, "remove_vetor_dist (): failed at \'pthread_mutex_lock()\'\n%s", strerror(errno)); exit(1); }

    // LOCK
    {
        for( int i = 0; i < table->vetor_count; ++i ) {
            if( table->vetor_arr[i].router_id == id_to_find ) {
                *out_vector = table->vetor_arr[i];
                found = 1;
                break;
            }
        }
    }
    // UNLOCK

    res = pthread_mutex_unlock( &table->mutex );
    if( res != 0 ) { ENSURE(0, "remove_vetor_dist(): failed at \'pthread_mutex_unlock()\'\n%s", strerror(errno)); exit(1); }

    return found;
}

//####################################################################
int get_custo( const vetor_dist_t * vetor, const int router_id ) {
    if( !vetor ) { return VETOR_DIST_INFINITE; }

    for( int i = 0; i < vetor->router_count; ++i ) {
        if( vetor->router_dist_arr[i].router_dest_id == router_id ) {
            return vetor->router_dist_arr[i].distance;
        }
    }
    return VETOR_DIST_INFINITE;
}

//####################################################################
void grow_dist_table( vetor_dist_table_t ** table ) {
    if( !table ) { return; }

    int new_capacity = 0;
    {
        if( (*table)->capacity == 0 ) {
            new_capacity = 16;
        } else {
            new_capacity = (*table)->capacity * 2;
        }
    }

    const int byte_count = sizeof(vetor_dist_t) * new_capacity;
    vetor_dist_t * arr = malloc( byte_count );
    memset( arr, 0, byte_count );

    memcpy( arr, (*table)->vetor_arr, sizeof(vetor_dist_t) * (*table)->capacity );
    SAFE_FREE((*table)->vetor_arr);

    (*table)->vetor_arr = arr;
    (*table)->capacity = new_capacity;
}