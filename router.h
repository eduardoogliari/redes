#ifndef ROUTER_H
#define ROUTER_H

#include <arpa/inet.h>

// Estrutura com informações do roteador
typedef struct router_info_t {
    int     id;
    int     port_number;
    char    ip_address[INET6_ADDRSTRLEN];
} router_info_t;


// Tabela com informação dos roteadores
typedef struct router_info_table_t {
    router_info_t *     routers_arr;
    int                 router_count;
} router_info_table_t;


// Imprime a tabela de roteadores
void print_router_info_table( const router_info_table_t * table );

// Encontra dados do roteador com id 'id_to_find'
router_info_t const * find_router_info( const router_info_table_t * table, const int id_to_find );

#endif