#include <stdio.h>
#include <string.h>
#include "message.h"
#include "defs.h"
#include "helper.h"
#include "vetor_dist.h"

//####################################################################
int copy_message( message_t * dst, const message_t * src ) {
    if( !src ) { ENSURE(0, "copy_message(): \'src\' is NULL") return 0; }
    if( !dst ) { ENSURE(0, "copy_message(): \'dst\' is NULL") return 0; }
    if( src->type == MESSAGE_TYPE_UNKNOWN ) { ENSURE(0, "copy_message(): \'src->type\' is MESSAGE_TYPE_UNKNOWN") return 0; }

    // ip_destination
    {
        dst->destination_id = src->destination_id;
    }

    // ip_source
    {
        dst->source_id = src->source_id;
    }

    // type
    {
        dst->type = src->type;
    }

    // payload
    {
        if( src->type == MESSAGE_TYPE_CONTROL ) {
            dst->payload_roteamento = src->payload_roteamento;

        } else if( src->type == MESSAGE_TYPE_DATA ) {
            strcpy( dst->payload_str, src->payload_str );
        }
    }

    return 1;
}

//####################################################################
void clear_message( message_t * msg ) {
    if( !msg ) { ENSURE(0, "clear_message(): \'msg\' is NULL") return; }

    msg->source_id      = -1;
    msg->destination_id = -1;
    msg->type           = MESSAGE_TYPE_UNKNOWN;
    memset( msg->payload_str, 0, MESSAGE_MAX_LENGHT );
}

//####################################################################
char const * tipo_mensagem_para_str( const int tipo ) {
    switch( tipo ) {
        case MESSAGE_TYPE_CONTROL:
        {
            return "MESSAGE_TYPE_CONTROL";
        }

        case MESSAGE_TYPE_DATA:
        {
            return "MESSAGE_TYPE_DATA";
        }
    }
    return "MESSAGE_TYPE_UNKNOWN";
}

//####################################################################
int serialize_message( const message_t * msg, char * out_buffer, const int out_buffer_size ) {
    if( !msg ) { return 0; }
    if( !out_buffer ) { return 0; }

    const int byte_count = sizeof(message_t);
    if( out_buffer_size < byte_count ) {
        ENSURE( 0, "serialize_message(): buffer not big enough" );
        return 0;
    }

    int offset = 0;

    // source_id
    {
        const int value = htonl(msg->source_id);
        memcpy( &out_buffer[offset], &value, sizeof(msg->source_id) );

        offset += sizeof(msg->source_id);
    }

    // destination_id
    {
        const int value = htonl(msg->destination_id);
        memcpy( &out_buffer[offset], &value, sizeof(msg->destination_id) );

        offset += sizeof(msg->destination_id);
    }

    // type
    {
        const int value = htonl(msg->type);
        memcpy( &out_buffer[offset], &value, sizeof(msg->type) );

        offset += sizeof(msg->type);
    }

    // payload
    if( msg->type == MESSAGE_TYPE_CONTROL ) {
        const vetor_dist_t * vetor = &msg->payload_roteamento.vetor;

        // vetor->router_id
        {
            const int value = htonl(vetor->router_id);
            memcpy( &out_buffer[offset], &value, sizeof(vetor->router_id) );

            offset += sizeof(vetor->router_id);
        }

        // vetor->timestamp
        {
            const long int value = htonl(vetor->timestamp);
            memcpy( &out_buffer[offset], &value, sizeof(vetor->timestamp) );

            offset += sizeof(vetor->timestamp);
        }

        // vetor->router_count
        {
            const int value = htonl(vetor->router_count);
            memcpy( &out_buffer[offset], &value, sizeof(vetor->router_count) );

            offset += sizeof(vetor->router_count);
        }

        for( int i = 0; i < vetor->router_count; ++i ) {
            par_dist_t const * dist = &vetor->router_dist_arr[i];

            // router_dest_id
            {
                const int value = htonl(dist->router_dest_id);
                memcpy( &out_buffer[offset], &value, sizeof(dist->router_dest_id) );

                offset += sizeof(dist->router_dest_id);
            }

            // distance
            {
                const int value = htonl(dist->distance);
                memcpy( &out_buffer[offset], &value, sizeof(dist->distance) );

                offset += sizeof(dist->distance);
            }
        }

    } else if( msg->type == MESSAGE_TYPE_DATA ) {
        strcpy( &out_buffer[offset], msg->payload_str );
        offset += strlen(msg->payload_str);
    }

    return 1;
}

//####################################################################
int deserialize_message( const char * buffer, const int buffer_size, message_t * out_msg ) {
    if( !buffer ) { return 0; }
    if( !out_msg ) { return 0; }
    if( buffer_size < sizeof(message_t) ) { return 0; }

    int offset = 0;

    // source_id
    {
        int value = 0;
        memcpy( &value, &buffer[offset], sizeof(int) );
        out_msg->source_id = htonl( value );

        offset += sizeof(int);
    }

    // destination_id
    {
        int value = 0;
        memcpy( &value, &buffer[offset], sizeof(int) );
        out_msg->destination_id = htonl( value );

        offset += sizeof(int);
    }

    // type
    {
        int value = 0;
        memcpy( &value, &buffer[offset], sizeof(int) );
        out_msg->type = htonl( value );

        offset += sizeof(int);
    }

    // payload
    {
        if( out_msg->type == MESSAGE_TYPE_CONTROL ) {
            // router_id
            {
                int value = 0;
                memcpy( &value, &buffer[offset], sizeof(int) );
                out_msg->payload_roteamento.vetor.router_id = htonl( value );

                offset += sizeof(int);
            }

            // timestamp
            {
                long int value = 0;
                memcpy( &value, &buffer[offset], sizeof(long int) );
                out_msg->payload_roteamento.vetor.timestamp = (time_t)htonl( value );

                offset += sizeof(long int);
            }

            // router_count
            {
                int value = 0;
                memcpy( &value, &buffer[offset], sizeof(int) );
                out_msg->payload_roteamento.vetor.router_count = htonl( value );

                offset += sizeof(int);
            }

            const int count = out_msg->payload_roteamento.vetor.router_count;
            for( int i = 0; i < count; ++i ) {

                // router_dest_id
                {
                    int value = 0;
                    memcpy( &value, &buffer[offset], sizeof(int) );
                    out_msg->payload_roteamento.vetor.router_dist_arr[i].router_dest_id = htonl( value );

                    offset += sizeof(int);
                }

                 // distance
                {
                    int value = 0;
                    memcpy( &value, &buffer[offset], sizeof(int) );
                    out_msg->payload_roteamento.vetor.router_dist_arr[i].distance = htonl( value );

                    offset += sizeof(int);
                }
            }

        } else if( out_msg->type == MESSAGE_TYPE_DATA ) {
            strcpy( out_msg->payload_str, &buffer[offset] );
            offset += strlen(&buffer[offset]);
        }
    }

    return 1;
}

//####################################################################
void print_roteamento( const roteamento_t * roteamento ) {
    if( !roteamento ) { return; }

    const vetor_dist_t * vetor = &roteamento->vetor;


    printf( "%d = {", vetor->router_id );

    for( int i = 0; i < vetor->router_count; ++i ) {
        if( vetor->router_dist_arr[i].distance >= VETOR_DIST_INFINITE ) {
            printf( " [#%d, inf]", vetor->router_dist_arr[i].router_dest_id );
        } else {
            printf( " [#%d, %d]", vetor->router_dist_arr[i].router_dest_id, vetor->router_dist_arr[i].distance );
        }

        if( i + 1 < vetor->router_count ) { printf( "," ); }
    }

    printf( " }\n" );
}