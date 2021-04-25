#ifndef VETOR_DIST_H
#define VETOR_DIST_H

#define VETOR_DIST_INFINITE 100
#define ROUTER_MAX_COUNT 32

#include <pthread.h>
#include <time.h>
#include "enlace.h"
#include "router.h"

typedef struct par_dist_t {
    int router_dest_id; // ID
    int distance;       // Custo
} par_dist_t;

typedef struct vetor_dist_t {
    int             router_id; // Linha
    par_dist_t      router_dist_arr[ROUTER_MAX_COUNT]; // Colunas
    int             router_count; // Quantidade de colunas (igual a quantidade total de ROTEADORES na rede)
    time_t          timestamp;
} vetor_dist_t;

typedef struct vetor_dist_table_t {
    vetor_dist_t *  vetor_arr;      // Array com vetores distancia dos vizinhos
    int             capacity;       // Tamanho total alocado
    int             vetor_count;    // Contagem de elementos
    pthread_mutex_t mutex;
} vetor_dist_table_t;

void init_tabela_vetores( vetor_dist_table_t * table );
void init_vetor_dist( vetor_dist_t * vetor, const vizinho_table_t * vizinhos_table, const router_info_table_t * router_info_table );
void add_or_replace_vetor_dist( vetor_dist_table_t * table, vetor_dist_t * vetor );
void remove_vetor_dist( vetor_dist_table_t * table, const int id_to_remove );
int get_vector( vetor_dist_table_t * table, const int id_to_find, vetor_dist_t * out_vector );
int get_custo( const vetor_dist_t * vetor, const int router_id );

#endif