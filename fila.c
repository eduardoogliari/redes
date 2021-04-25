#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "fila.h"
#include "defs.h"
#include "message.h"

// static pthread_mutex_t g_fila_mutex = PTHREAD_MUTEX_INITIALIZER;

//####################################################################
void init_fila( fila_t * fila ) {
    if( !fila ) { return; }

    for( int i = 0; i < fila->tamanho; ++i ) {
        clear_message( &fila->mensagens[i] );
    }
    pthread_mutex_init( &fila->mutex, NULL );
    fila->tamanho = 0;
}

//####################################################################
int add_fila( fila_t * fila, message_t * msg ) {
    if( !fila ) { ENSURE(0, "add_fila(): \'fila\' is NULL ") return 0; }

    int res = pthread_mutex_lock( &fila->mutex );
    if( res != 0 ) { ENSURE(0, "add_fila(): failed at \'pthread_mutex_lock()\'\n%s", strerror(errno)); exit(1); }

    // LOCK
    {
        if( fila->tamanho >= CAPACIDADE_FILA_MAX ) {
            pthread_mutex_unlock( &fila->mutex ); // UNLOCK
            return 0;
        }

        const int fim = fila->tamanho;
        if( copy_message( &fila->mensagens[fim], msg ) == 0 ) {
            ENSURE(0, "push(): fail at copy_message()")
            pthread_mutex_unlock( &fila->mutex ); // UNLOCK
            return 0;
        }

        ++fila->tamanho;
    }
    // UNLOCK

    res = pthread_mutex_unlock( &fila->mutex );
    if( res != 0 ) { ENSURE(0, "add_fila(): failed at \'pthread_mutex_unlock()\'\n%s", strerror(errno)); exit(1); }

    return 1;
}

//####################################################################
message_t * retira_fila( fila_t * fila ) {
    if( !fila ) { ENSURE(0, "retira_fila(): \'fila\' is NULL ") return NULL; }

    int res = pthread_mutex_lock( &fila->mutex );
    if( res != 0 ) { ENSURE(0, "retira_fila(): failed at \'pthread_mutex_lock()\'\n%s", strerror(errno)); exit(1); }


    message_t * msg = malloc( sizeof(message_t) );
    memset( msg, 0, sizeof(message_t) );

    // LOCK
    {
        if( fila->tamanho <= 0 ) {
            // puts( "FILA VAZIA" );
            pthread_mutex_unlock( &fila->mutex ); // UNLOCK
            SAFE_FREE( msg );
            return NULL;
        }


        // Copia primeira mensagem da fila para 'msg'
        if( copy_message( msg, &fila->mensagens[0] ) == 0 ) {
            ENSURE(0, "retira_fila(): fail at copy_message()")
            pthread_mutex_unlock( &fila->mutex ); // UNLOCK
            SAFE_FREE( msg );
            return NULL;
        }

        const int fim = fila->tamanho;

        // Avanca a fila
        for( int i = 0; (i + 1) < fim; ++i ) {
            // fila->mensagens[i] = fila->mensagens[i+1];
            copy_message( &fila->mensagens[i], &fila->mensagens[i+1] );
        }

        --fila->tamanho;
    }
    // UNLOCK

    res = pthread_mutex_unlock( &fila->mutex );
    if( res != 0 ) { ENSURE(0, "retira_fila(): failed at \'pthread_mutex_unlock()\'\n%s", strerror(errno)); SAFE_FREE( msg ); exit(1); }

    return msg;
}

//####################################################################
void limpar_fila( fila_t * fila ) {
    if( !fila ) { ENSURE(0, "limpar_fila(): \'fila\' is NULL ") return; }

    int res = pthread_mutex_lock( &fila->mutex );
    if( res != 0 ) { ENSURE(0, "limpar_fila(): failed at \'pthread_mutex_lock()\'\n%s", strerror(errno)); exit(1); }

    // LOCK
    {
        for( int i = 0; i < CAPACIDADE_FILA_MAX; ++i ) {
            clear_message( &fila->mensagens[i] );
        }
        fila->tamanho = 0;
    }
    // UNLOCK

    res = pthread_mutex_unlock( &fila->mutex );
    if( res != 0 ) { ENSURE(0, "limpar_fila(): failed at \'pthread_mutex_unlock()\'\n%s", strerror(errno)); exit(1); }
}

