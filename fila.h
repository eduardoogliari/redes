#ifndef FILA_H
#define FILA_H

#define CAPACIDADE_FILA_MAX 64

#include <pthread.h>
#include "message.h"

typedef struct fila_t {
    message_t           mensagens[CAPACIDADE_FILA_MAX];
    pthread_mutex_t     mutex;
    int                 tamanho;
} fila_t;


// Inicia a fila com valores padrão (AVISO: não utilizar para limpar a fila, pois não é thread-safe. Utilize 'limpar_fila()')
void init_fila( fila_t * fila );

// Coloca mensagem no fim da fila
int add_fila( fila_t * fila, message_t * msg );

// Retira primeiro da fila
message_t * retira_fila( fila_t * fila );

// Limpa a fila inteira
void limpar_fila( fila_t * fila );

// Imprime a fila
void print_fila( fila_t * fila );


#endif