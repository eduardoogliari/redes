#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "logger.h"
#include "defs.h"

static void grow_log( logger_data_t ** log );

//####################################################################
void init_logger( logger_data_t * log ) {
    if( !log ) { return; }

    log->log_arr      = NULL;
    log->log_capacity = 0;
    log->offset       = 0;
    pthread_mutex_init( &log->mutex, NULL );
}

//####################################################################
void add_log_entry( logger_data_t * log, const char * str ) {
    if( !log ) { return; }
    if( !str ) { return; }
    if( str[0] == '\0' || str[0] == '\n' || str[0] == '\r' ) { return; }

    pthread_mutex_lock( &log->mutex );
    // LOCK
    {
        const int offset = log->offset;
        if( offset == log->log_capacity ) {
            grow_log( &log );
        }

        {
            const time_t time_value = time(NULL);
            const struct tm local_tm = *localtime(&time_value);
            snprintf( log->log_arr[offset].buffer, LOG_ENTRY_MAX_LEN-1, "[%02d/%02d/%d  %02d:%02d:%02d]: %s", local_tm.tm_mday, local_tm.tm_mon+1, local_tm.tm_year+1900, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec, str );
        }
        ++log->offset;
    }
    // UNLOCK
    pthread_mutex_unlock( &log->mutex );
}

//####################################################################
void grow_log( logger_data_t ** log ) {
    if( !log ) { return; }

    int new_capacity = 0;
    {
        if( (*log)->log_capacity == 0 ) {
            new_capacity = 32;
        } else {
            new_capacity = (*log)->log_capacity * 2;
        }
    }

    log_entry_t * new_arr = malloc(sizeof(log_entry_t) * new_capacity);
    memset(new_arr, 0, sizeof(log_entry_t) * new_capacity);

    for( int i = 0; i < (*log)->offset; ++i ) {
        strcpy( new_arr[i].buffer, (*log)->log_arr[i].buffer );
    }
    SAFE_FREE( (*log)->log_arr );

    (*log)->log_arr = new_arr;
    (*log)->log_capacity = new_capacity;
}

//####################################################################
void clear_log( logger_data_t * log ) {
    if( !log ) { return; }

    int res = pthread_mutex_lock( &log->mutex );
    if( res != 0 ) { ENSURE(0, "clear_log(): failed at \'pthread_mutex_lock()\'\n%s", strerror(errno)); exit(1); }

    // LOCK
    {
        for( int i = 0; i < log->log_capacity; ++i ) {
            memset( log->log_arr[i].buffer, 0, LOG_ENTRY_MAX_LEN );
        }
        log->offset = 0;
    }
    // UNLOCK

    res = pthread_mutex_unlock( &log->mutex );
    if( res != 0 ) { ENSURE(0, "clear_log(): failed at \'pthread_mutex_unlock()\'\n%s", strerror(errno)); exit(1); }

    // Importante: Nao setar para zero 'capacity' e nem liberar memoria de 'log_arr'
    // Isto e intencional para reaproveitar a memoria
}

