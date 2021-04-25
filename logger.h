#ifndef LOGGER_H
#define LOGGER_H

#define LOG_ENTRY_MAX_LEN 256

#include <pthread.h>

typedef struct log_entry_t {
    char buffer[LOG_ENTRY_MAX_LEN];
} log_entry_t;


typedef struct logger_data_t {
    log_entry_t *   log_arr;
    int             log_capacity;
    int             offset;
    pthread_mutex_t mutex;
} logger_data_t;


void init_logger( logger_data_t * log );
void add_log_entry( logger_data_t * log, const char * str );
void clear_log( logger_data_t * log );
// void print_log( logger_data_t * log );

#endif