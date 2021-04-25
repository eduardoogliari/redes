#ifndef MESSAGE_H
#define MESSAGE_H

#define MESSAGE_TYPE_UNKNOWN 0
#define MESSAGE_TYPE_DATA    1
#define MESSAGE_TYPE_CONTROL 2

#define MESSAGE_MAX_LENGHT 100

#include <arpa/inet.h>
#include "vetor_dist.h"

// Dados de roteamento
typedef struct roteamento_t {
    vetor_dist_t vetor;
} roteamento_t;


// Estrutura que representa a mensagem a ser enviada entre os roteadores
// 'payload' pode assumir dois tipos: 'char*' caso o tipo seja '_DATA', ou roteamento_t caso o tipo seja '_CONTROL'
typedef struct message_t {
    int         source_id;          // Identificador do roteador de origem
    int         destination_id;     // Identificador do roteador de destino
    int         type;               // Tipo da mensagem

    union {
        char            payload_str[MESSAGE_MAX_LENGHT];
        roteamento_t    payload_roteamento;
    };

    // void *      payload;    // Dados da mensagem (roteamento_t ou char*)
    // char        ip_source[INET6_ADDRSTRLEN];        // IP do roteador origem
    // char        ip_destination[INET6_ADDRSTRLEN];   // IP do roteador destino
} message_t;



//####################################################################

// Copia a mensagem de 'src' para 'dst'
// Retorna 1 em caso de sucesso, e 0 caso contrário
int copy_message( message_t * dst, const message_t * src );

// Limpa a estrutura em \'msg\'
void clear_message( message_t * msg );

// Converte MESSAGE_TYPE_* para um literal de string
char const * tipo_mensagem_para_str( const int tipo );

// Serializa mensagem para \'out_buffer\'
int serialize_message( const message_t * msg, char * out_buffer, const int out_buffer_size );

// Deserializa buffer para \'out_msg\'
int deserialize_message( const char * buffer, const int buffer_size, message_t * out_msg );

// Imprime dados contidos na estrutura de roteamento
void print_roteamento( const roteamento_t * roteamento );


#endif