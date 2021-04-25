#ifndef ENLACE_H
#define ENLACE_H

// Estrutura que armazena informação de custo entre dois roteadores
typedef struct enlace_info_t {
    int left_id;
    int right_id;
    int cost;
} enlace_info_t;


// Tabela com informações de enlace entre vários roteadores
typedef struct enlace_table_t {
    enlace_info_t *     enlace_arr;
    int                 enlace_count;
} enlace_table_t;


typedef struct vizinho_t {
    int vizinho_id;
    int cost;
} vizinho_t;

typedef struct vizinho_table_t {
    int         router_id;
    vizinho_t * vizinhos_arr;
    int         vizinho_count;
} vizinho_table_t;


// Imprime a tabela de enlace
void print_enlace_table( const enlace_table_t * table );

void print_tabela_vizinho( const vizinho_table_t * table );

// Cria tabela de vizinhos do roteador a partir da tabela de enlace
void criar_tabela_vizinhos( const enlace_table_t * enlace_table, const int router_id, vizinho_table_t * out_vizinho );

// Procura se roteador de id \'router_id\' esta presente na tabela de vizinhos
vizinho_t const * find_vizinho( const vizinho_table_t * table, const int router_id );


#endif