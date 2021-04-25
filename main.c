#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "file_helper.h"
#include "fila.h"
#include "message.h"
#include "router.h"
#include "enlace.h"
#include "helper.h"
#include "logger.h"
#include "defs.h"
#include "vetor_dist.h"

#define ROTEADOR_CONFIG "roteador.config"
#define ENLACE_CONFIG   "enlace.config"

#define INTERVALO_ENVIO_DIST 5 // (em segundos)

// Limite maximo de atraso (em segundos) de recebimento do vetor distancia antes de considerar vizinho como desconectado
#define TOLERANCIA_ATRASO_DIST (INTERVALO_ENVIO_DIST * 2)

int                     g_ID;                   // ID deste roteador
int                     g_quit;                 // Sair do programa?
int                     g_waiting_input;        // Utilizado para aguardar entrada de usuario em telas redesenhadas continuamente
fila_t                  g_fila_in;              // Fila com mensagens recebidas de outros roteadores
fila_t                  g_fila_out;             // Fila com mensagens a serem enviadas a outros roteadores
fila_t                  g_fila_history;         // Historico de mensagens recebidas
fila_t                  g_fila_process;         // Fila com mensagens a serem processadas
router_info_table_t     g_router_info_table;    // Armazena informações do arquivo "roteador.config"
enlace_table_t          g_enlace_table;         // Armazena informações do arquivo "enlace.config"
vizinho_table_t         g_vizinho_table;        // Estrutura para armazenar informações dos vizinhos
logger_data_t           g_logger;               // Estrutura para armazenar informações de log
vetor_dist_table_t      g_vetores_dist;         // Tabela com vetores distancia

pthread_t g_receiver_thread;            // Thread que escura e coloca mensagens na pilha para processamento
pthread_t g_sender_thread;              // Thread que envia mensagens a outros roteadores
pthread_t g_packet_handler_thread;      // Thread que processa as mensagens/vetores
pthread_t g_distance_thread;            // Thread que envia vetores distancia
pthread_t g_wait_input_thread;          // Thread que aguarda entrada do usuario para telas redesenhadas

int g_socket; // Socket

void *  handle_incoming_messages( void * args );
void *  handle_outgoing_messages( void * args );
void *  process_messages( void * args );
void *  send_distance_messages( void * args );
void *  wait_input( void * args );

void    print_header();     // Imprime cabeçario com ID do roteador
void    message_form();     // Formulario para envio de mensagens de dados
void    print_historico();  // Imprime historico de mensagens
void    print_log();        // Imprime log de mensagens
void    print_vetores();    // Imprime tabela de vetores distancia
void    recalcular_dist();  // Recalcula vetor distancia para este roteador


//####################################################################
int main( int argc, char ** argv ) {
    srand((unsigned int)time(NULL));

    g_quit = 0;

    if( argc > 1 ) {
        // Lê ID
        {
            g_ID = strtol( argv[1], NULL, 10 );
        }

        // Lê arquivo de roteador
        {
            if( !read_router_config( ROTEADOR_CONFIG, &g_router_info_table ) ) {
                ENSURE(0, "Falha ao ler arquivo de configuracao de roteadores");
                return -1;
            }
        }

        // Lê arquivo de enlace
        {
            if( !read_enlace_config( ENLACE_CONFIG, &g_enlace_table ) ) {
                ENSURE(0, "Falha ao ler arquivo de configuracao de enlaces");
                return -1;
            }
        }

        // Cria socket
        {
            int s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
            if( s < 0 ) {
                ENSURE(0, "Falha ao criar socket\n%s", strerror(errno));
                return -1;
            }
            g_socket = s;
        }

        // Inicializa tabela de vizinhos imediatos
        {
            memset( &g_vizinho_table, 0, sizeof(vizinho_t) );
            criar_tabela_vizinhos( &g_enlace_table, g_ID, &g_vizinho_table );
        }

        // Inicializa tabela de vetores distancia
        {
            init_tabela_vetores( &g_vetores_dist );

            vetor_dist_t vetor;
            memset( &vetor, 0, sizeof(vetor_dist_t) );

            init_vetor_dist( &vetor, &g_vizinho_table, &g_router_info_table );
            add_or_replace_vetor_dist( &g_vetores_dist, &vetor );
        }

        // Inicializa filas
        {
            init_fila( &g_fila_in );
            init_fila( &g_fila_out );
            init_fila( &g_fila_history );
            init_fila( &g_fila_process );
        }

        // Inicializa logger
        {
            init_logger( &g_logger );
        }

        // Cria e executa threads
        {
            int res = -1;

            res = pthread_create( &g_receiver_thread, NULL, handle_incoming_messages, NULL );
            if( res != 0 ) { ENSURE(0, "Failed at \'pthread_create()\' for g_receiver_thread\n%s", strerror(errno)) return -1; }

            res = pthread_create( &g_sender_thread, NULL, handle_outgoing_messages, NULL );
            if( res != 0 ) { ENSURE(0, "Failed at \'pthread_create()\' for g_sender_thread\n%s", strerror(errno)) return -1; }

            res = pthread_create( &g_packet_handler_thread, NULL, process_messages, NULL );
            if( res != 0 ) { ENSURE(0, "Failed at \'pthread_create()\' for g_packet_handler_thread\n%s", strerror(errno)) return -1; }

            res = pthread_create( &g_distance_thread, NULL, send_distance_messages, NULL );
            if( res != 0 ) { ENSURE(0, "Failed at \'pthread_create()\' for g_distance_thread\n%s", strerror(errno)) return -1; }

            res = pthread_create( &g_wait_input_thread, NULL, wait_input, NULL );
            if( res != 0 ) { ENSURE(0, "Failed at \'pthread_create()\' for g_wait_input_thread\n%s", strerror(errno)) return -1; }
        }

        // Terminal (main thread)
        {
            while(1) {
                if( g_quit ) { break; }

                CLS();
                print_header();
                puts( "1. Enviar mensagem" );
                puts( "2. Visualizar log" );
                puts( "3. Historico de mensagens" );
                puts( "4. Visualizar tabela de vetores" );
                puts( "5. Sair" );
                puts( "------------------------" );
                printf( "> " );

                int option = 0;
                scanf( "%d", &option );
                getchar();

                switch( option ) {
                    case 1: // Enviar
                    {
                        message_form();
                        break;
                    }

                    case 2: // Visualizar log
                    {
                        print_log();
                        break;
                    }

                    case 3: // Historico de mensagens
                    {
                        print_historico();
                        break;
                    }

                    case 4: // Visualizar tabela de vetores
                    {
                        print_vetores();
                        break;
                    }

                    case 5: // Sair
                    {
                        g_quit = 1;
                        break;
                    }
                }
            }
        }


        // Cancela threads
        {
            pthread_cancel(g_packet_handler_thread);
            pthread_cancel(g_receiver_thread);
            pthread_cancel(g_sender_thread);
            pthread_cancel(g_distance_thread);
            pthread_cancel(g_wait_input_thread);

            int res = -1;

            res = pthread_join( g_packet_handler_thread, NULL );
            if( res != 0 ) { ENSURE(0, "Failed at \'pthread_join()\' for g_packet_handler_thread\n%s", strerror(errno)) return -1; }

            res = pthread_join( g_receiver_thread, NULL );
            if( res != 0 ) { ENSURE(0, "Failed at \'pthread_join()\' for g_receiver_thread\n%s", strerror(errno)) return -1; }

            res = pthread_join( g_sender_thread, NULL );
            if( res != 0 ) { ENSURE(0, "Failed at \'pthread_join()\' for g_sender_thread\n%s", strerror(errno)) return -1; }

            res = pthread_join( g_distance_thread, NULL );
            if( res != 0 ) { ENSURE(0, "Failed at \'pthread_join()\' for g_distance_thread\n%s", strerror(errno)) return -1; }

            res = pthread_join( g_wait_input_thread, NULL );
            if( res != 0 ) { ENSURE(0, "Failed at \'pthread_join()\' for g_wait_input_thread\n%s", strerror(errno)) return -1; }
        }


    } else {
        puts( "\nNenhum argumento fornecido." );
        puts( "O primeiro argumento corresponde ao ID do roteador sendo instanciado\n" );
    }

    return 0;
}

//####################################################################
void * handle_incoming_messages( void * args ) {
    router_info_t const * info = find_router_info( &g_router_info_table, g_ID );
    if( !info ) {
        ENSURE( 0, "handle_incoming_messages(): nao foi possivel encontrar informacoes do roteador de id %d\n", g_ID );
        exit(1);
    }

    struct sockaddr_in sock_in;
    {
        memset( &sock_in, 0, sizeof(struct sockaddr_in) );
        sock_in.sin_family = AF_INET;
        sock_in.sin_port = htons(info->port_number);

        if( inet_aton(info->ip_address , &sock_in.sin_addr) == 0) {
            ENSURE( 0, "handle_incoming_messages(): falha em \'inet_aton()\'\n%s", strerror(errno) );
            exit(1);
        }

        if( bind( g_socket, (struct sockaddr*)&sock_in, sizeof(sock_in) ) == -1 ) {
            ENSURE( 0, "handle_incoming_messages(): falha em \'bind()\'\n%s", strerror(errno) );
            exit(1);
        }
    }


    message_t msg;
    memset( &msg, 0, sizeof(message_t) );

    while(1) {
        if( g_quit ) { break; }

        const int buffer_size = sizeof(message_t) + sizeof(int);
        char buffer[buffer_size];
        memset( buffer, 0, buffer_size );

        socklen_t slen = sizeof(sock_in);
        if( recvfrom(g_socket, (char*)buffer, buffer_size, 0, (struct sockaddr *)&sock_in, &slen) == -1 ) {
            ENSURE(0, "handle_incoming_messages(): failed at \'recvfrom()\'\n%s", strerror(errno));
            exit(1);

        } else {
            // Le ID do roteador que enviou a mensagem do inicio do buffer
            int sender_id = -1;
            {
                memcpy( &sender_id, &buffer[0], sizeof(int) );
                sender_id = htonl(sender_id);
            }

            if( deserialize_message( buffer + sizeof(int), sizeof(message_t), &msg ) ) {
                add_fila( &g_fila_process, &msg );

                // LOG
                {
                    char log_buffer[128];
                    memset(log_buffer, 0, 128);
                    snprintf( log_buffer, 127, "Mensagem recebida de [%d]", sender_id );

                    add_log_entry( &g_logger, log_buffer );
                }

            } else {
                ENSURE(0, "Failed to deserialize message");
            }
        }
    }
    return NULL;
}

//####################################################################
void * handle_outgoing_messages( void * args ) {
    while(1) {
        if( g_quit ) { break; }

        message_t * msg = retira_fila( &g_fila_out );
        if( msg ) {
            router_info_t const * info = find_router_info( &g_router_info_table, msg->destination_id );
            if( info ) {

                struct sockaddr_in sock_in;
                memset( &sock_in, 0, sizeof(sock_in) );
                sock_in.sin_family = AF_INET;
                sock_in.sin_port = htons(info->port_number);

                if( inet_aton(info->ip_address , &sock_in.sin_addr) != 0) {

                    const int buffer_size = sizeof(message_t) + sizeof(int);
                    char buffer[buffer_size];
                    memset( buffer, 0, buffer_size );

                    // Coloca o ID do roteador que esta enviando a mensagem no inicio do buffer
                    {
                        int value = htonl( g_ID );
                        memcpy( &buffer[0], &value, sizeof(int) );
                    }

                    if( serialize_message( msg, buffer + sizeof(int), sizeof(message_t) ) ) {
                        if( sendto(g_socket, (char*)buffer, buffer_size, 0, (struct sockaddr *)&sock_in, sizeof(sock_in)) == -1 ) {
                            ENSURE(0, "handle_outgoing_messages(): falha em \'sendto()\'" );
                            return NULL;
                        }

                        // LOG
                        {
                            char log_buffer[128];
                            memset(log_buffer, 0, 128);

                            if( msg->source_id == g_ID ) {
                                if( msg->type == MESSAGE_TYPE_DATA ) {
                                    snprintf( log_buffer, 127, "Mensagem enviada para [%d]", info->id );
                                } else if( msg->type == MESSAGE_TYPE_CONTROL ) {
                                    snprintf( log_buffer, 127, "Vetor distancia enviado para [%d]", info->id );
                                }
                            } else {
                                snprintf( log_buffer, 127, "Mensagem com origem [%d] encaminhada para [%d]", msg->source_id, info->id );
                            }

                            add_log_entry( &g_logger, log_buffer );
                        }
                    } else {
                        ENSURE(0, "Failed to serialize message");
                    }

                } else {
                    ENSURE( 0, "handle_outgoing_messages(): falha em \'inet_aton()\' com info->ip_address:%s \n%s", info->ip_address, strerror(errno) );
                    exit(1);
                }
            } else {
                ENSURE( 0, "handle_outgoing_messages(): nao foi possivel encontrar informacoes do roteador de id %d\n", msg->destination_id );
            }
            SAFE_FREE(msg);
        }
    }
    return NULL;
}

//####################################################################
void * process_messages( void * args ) {
    while(1) {
        if( g_quit ) { break; }

        message_t * msg = retira_fila( &g_fila_process );
        if( msg ) {

            if( msg->destination_id == g_ID ) {
                // Fila cheia do historico, remove a entrada mais antiga
                {
                    if( g_fila_history.tamanho == CAPACIDADE_FILA_MAX ) {
                        message_t * temp = retira_fila( &g_fila_history );
                        SAFE_FREE(temp);
                    }
                }

                if( msg->type == MESSAGE_TYPE_CONTROL ) {
                    // Atualiza valor distancia na memoria
                    msg->payload_roteamento.vetor.timestamp = time(NULL);
                    add_or_replace_vetor_dist( &g_vetores_dist, &msg->payload_roteamento.vetor );

                    // Recalcula vetor distancia
                    recalcular_dist();
                }

                // LOG
                {
                    char log_buffer[MESSAGE_MAX_LENGHT + 128];
                    memset( log_buffer, 0, MESSAGE_MAX_LENGHT + 128 );

                    if( msg->type == MESSAGE_TYPE_CONTROL ) {
                        msg->payload_roteamento.vetor.timestamp = time(NULL);
                        snprintf( log_buffer, (MESSAGE_MAX_LENGHT + 128), "Vetor distancia de [%d] armazenado no historico", msg->source_id );
                    } else {
                        snprintf( log_buffer, (MESSAGE_MAX_LENGHT + 128), "Mensagem de origem [%d] armazenada no historico", msg->source_id );
                    }
                    add_log_entry( &g_logger, log_buffer );
                }

                // Adiciona ao histórico de mensagens
                add_fila( &g_fila_history, msg );

            } else {
                // Encaminhar para algum roteador vizinho
                // Passar para a fila de saída, a thread de processamento irá encaminhar para outro roteador
                add_fila( &g_fila_out, msg );
            }
            SAFE_FREE(msg);

        } else {
            int ids_to_remove[ROUTER_MAX_COUNT];
            memset( &ids_to_remove, 0, sizeof(int) * ROUTER_MAX_COUNT  );

            int remove_count = 0;

            // Nenhuma mensagem, chegou. Veja se algum vetor distancia expirou
            {
                for( int i = 0; i < g_vetores_dist.vetor_count; ++i ) {
                    vetor_dist_t * vetor = &g_vetores_dist.vetor_arr[i];

                    if( vetor->router_id == g_ID ) {
                        // Desconsidera a remoção do proprio vetor distancia
                        continue;
                    }


                    const int tolerancia_max = TOLERANCIA_ATRASO_DIST + (rand() % INTERVALO_ENVIO_DIST) + 1;

                    double diff = difftime( time(NULL), vetor->timestamp );
                    if( diff >= tolerancia_max ) {
                        // Marcar vetor para ser removido
                        ids_to_remove[remove_count] = vetor->router_id;
                        ++remove_count;
                    }
                }
            }

            if( remove_count > 0 ) {
                // Remover vetores distancia
                for( int i = 0; i < remove_count; ++i ){
                    const int id = ids_to_remove[i];
                    remove_vetor_dist( &g_vetores_dist, id );

                    // LOG
                    {
                        char log_buffer[128];
                        snprintf( log_buffer, 128, "Removido vetor expirado do roteador [%d]", id );
                        add_log_entry( &g_logger, log_buffer );
                    }
                }
                recalcular_dist();
            }

        }
    }
    return NULL;
}

//####################################################################
void * send_distance_messages( void * args ) {
    while( 1 ) {
        sleep( INTERVALO_ENVIO_DIST );
        if( g_quit ) { break; }

        vetor_dist_t vetor;
        memset( &vetor, 0, sizeof(vetor_dist_t) );

        int res = pthread_mutex_lock( &g_vetores_dist.mutex );
        if( res != 0 ) { ENSURE(0, "send_distance_messages(): failed at \'pthread_mutex_lock()\'\n%s", strerror(errno)); exit(1); }

        // LOCK
        {
            // Obtem vetor distancia deste roteador
            {
                for( int i = 0; i < g_vetores_dist.vetor_count; ++i ) {
                    if( g_vetores_dist.vetor_arr[i].router_id == g_ID ) {
                        vetor = g_vetores_dist.vetor_arr[i];
                        break;
                    }
                }
            }

            message_t msg = {};
            msg.source_id                = g_ID;
            msg.type                     = MESSAGE_TYPE_CONTROL;
            msg.payload_roteamento.vetor = vetor;

            // Envia para todos os vizinhoss
            for( int i = 0; i < g_vizinho_table.vizinho_count; ++i ) {
                const int vizinho_id = g_vizinho_table.vizinhos_arr[i].vizinho_id;
                msg.destination_id = vizinho_id;

                add_fila( &g_fila_out, &msg );
            }
        }
        // UNLOCK

        res = pthread_mutex_unlock( &g_vetores_dist.mutex );
        if( res != 0 ) { ENSURE(0, "send_distance_messages(): failed at \'pthread_mutex_unlock()\'\n%s", strerror(errno)); exit(1); }
    }
    return NULL;
}

//####################################################################
void * wait_input( void * args ) {
    while(1) {
        if( g_quit ) { break; }

        if( g_waiting_input ) {
            getchar();
            g_waiting_input = 0;
        }
        sleep(1);
    }
    return NULL;
}

//####################################################################
void print_header() {
    router_info_t const * router_info = find_router_info( &g_router_info_table, g_ID );
    if( !router_info ) { ENSURE(0, "print_header(): nao foi encontrado roteador de id \'%d\' na tabela de roteadores", g_ID ); return; }

    puts( "------------------------------------------------" );
    printf( "ROTEADOR [ID:%d   PORT:%d   IP:%s]\n", g_ID, router_info->port_number, router_info->ip_address );
    puts( "------------------------------------------------" );
}

//####################################################################
void message_form() {
    // Procura informações deste roteador a partir da tabela
    router_info_t const * router_info = find_router_info( &g_router_info_table, g_ID );
    if( !router_info ) { ENSURE(0, "Nao foi encontrado roteador de id \'%d\' na tabela de roteadores", g_ID ); return; }

    if( g_vizinho_table.vizinho_count <= 0 ) {
        CLS();
        puts( "Este roteador nao possui vizinhos. Pressione ENTER para continuar..." );
        getchar();
        return;
    }

    message_t msg;
    memset( &msg, 0, sizeof(message_t) );
    msg.source_id = g_ID;
    msg.type = MESSAGE_TYPE_DATA;

    {
        int opt_roteador_id = -1;

        while( 1 ) {
            CLS();
            print_header();
            puts( "Digite o ID de um dos roteadores vizinhos listados abaixo" ); // Provisorio
            for( int i = 0; i < g_vizinho_table.vizinho_count; ++i ) {
                printf( "ID: [%d]\n", g_vizinho_table.vizinhos_arr[i].vizinho_id );
            }
            printf( "> " );

            scanf( "%d", &opt_roteador_id );
            getchar();

            if( find_vizinho( &g_vizinho_table, opt_roteador_id ) ) {
                msg.destination_id = opt_roteador_id;
                break;
            } else {
                CLS();
                puts( "Por favor, digite o ID de apenas um dos roteadores listados. Pressione ENTER para continuar" );
                getchar();
            }
        }
    }

    while(1) {
        char buffer[MESSAGE_MAX_LENGHT];
        memset( buffer, 0, MESSAGE_MAX_LENGHT );
        {
            CLS();
            print_header();
            printf( "Digite a mensagem (%d caracteres max)\n", MESSAGE_MAX_LENGHT );
            printf( " > " );
            fgets( buffer, MESSAGE_MAX_LENGHT-1, stdin );
        }

        int len_index = strlen(buffer)-1;
        if( buffer[len_index] == '\n') { buffer[len_index] = '\0'; }


        if( buffer[0] == '\0' ) {
            CLS();
            puts( "Mensagem nao pode ser vazia. Pressione ENTER e digite sua mensagem novamente..." );
            getchar();
        } else {

            int apenas_espacos = 1;
            for( int i = 0; i < strlen(buffer); ++i ) {
                if( buffer[i] != ' ' ) {
                    apenas_espacos = 0;
                    break;
                }
            }

            if( apenas_espacos ) {
                CLS();
                puts( "Mensagem nao pode conter apenas espacos em branco. Pressione ENTER e digite sua mensagem novamente..." );
                getchar();
            } else {
                // Ok, mensagem valida
                strcpy( msg.payload_str, buffer );
                break;
            }
        }
    }

    const int resultado = add_fila( &g_fila_process, &msg );
    if( resultado == 0 ) {
        CLS();
        puts( "Erro ao enviar mensagem a fila de saida, e possivel que esteja cheia. Pressione ENTER para retornar ao menu\n" );
        getchar();
    } else {
        CLS();
        puts( "Mensagem enviada com sucesso!. Pressione ENTER para continuar" );
        getchar();
    }
}

//####################################################################
void print_historico() {
    int res = -1;
    g_waiting_input = 1;

    while(1) {
        res = pthread_mutex_lock( &g_fila_history.mutex );
        if( res != 0 ) { ENSURE(0, "print_historico(): failed at \'pthread_mutex_lock()\'\n%s", strerror(errno)); exit(1); return; }

        // LOCK
        {
            CLS();
            puts( "=============================" );
            puts( "         HISTORICO           " );
            puts( "=============================" );

            if( g_fila_history.tamanho == 0 ) {
                puts( "Nenhuma mensagem no historico recente para exibir." );

            } else {
                for( int i = 0; i < g_fila_history.tamanho; ++i ) {
                    // print_message( &g_fila_history.mensagens[i] );
                    const message_t * msg = &g_fila_history.mensagens[i];
                    printf( "ORIGEM:%d   DESTINO:%d  TIPO:%s\n", msg->source_id, msg->destination_id, tipo_mensagem_para_str( msg->type ) );
                    printf( "PAYLOAD: " );

                    if( msg->type == MESSAGE_TYPE_CONTROL ) {
                        print_roteamento( (roteamento_t*)&msg->payload_roteamento );
                    } else if( msg->type == MESSAGE_TYPE_DATA ) {
                        printf( "%s\n", msg->payload_str );
                    }
                    puts("-----------------------------");
                }
            }
        }
        // UNLOCK
        res = pthread_mutex_unlock( &g_fila_history.mutex );
        if( res != 0 ) { ENSURE(0, "print_historico(): failed at \'pthread_mutex_unlock()\'\n%s", strerror(errno)); exit(1); return; }

        puts( "\n\nEsta tela e atualizada uma vez por segundo" );
        puts( "Pressione ENTER para retornar ao menu..." );

        if( !g_waiting_input ) { break; }
        sleep(1);
    }

}

//####################################################################
void print_log() {
    int res = -1;
    g_waiting_input = 1;

    while(1) {
        res = pthread_mutex_lock( &g_logger.mutex );
        if( res != 0 ) { ENSURE(0, "print_log(): failed at \'pthread_mutex_lock()\'\n%s", strerror(errno)); exit(1); return; }

        // LOCK
        {
            CLS();
            puts( "=============================" );
            puts( "           LOG               " );
            puts( "=============================" );

            if( g_logger.offset == 0 ) {
                puts( "Vazio..." );

            } else {
                for( int i = 0; i < g_logger.offset; ++i ) {
                    puts( g_logger.log_arr[i].buffer );
                }
            }
        }
        // UNLOCK

        res = pthread_mutex_unlock( &g_logger.mutex );
        if( res != 0 ) { ENSURE(0, "print_log(): failed at \'pthread_mutex_unlock()\'\n%s", strerror(errno)); exit(1); return; }

        puts( "\n\nEsta tela e atualizada uma vez por segundo" );
        puts( "Pressione ENTER para retornar ao menu..." );

        if( !g_waiting_input ) { break; }
        sleep(1);
    }
}

//####################################################################
void print_vetores() {
    int res = -1;
    g_waiting_input = 1;

    while(1) {
        res = pthread_mutex_lock( &g_vetores_dist.mutex );
        if( res != 0 ) { ENSURE(0, "print_vetores(): failed at \'pthread_mutex_lock()\'\n%s", strerror(errno)); exit(1); return; }

        // LOCK
        {
            CLS();
            puts( "=============================" );
            printf( "         ROTEADOR (%d)       \n", g_ID );
            puts( "=============================" );

            if( g_vetores_dist.vetor_count == 0 ) {
                puts( "Vazio..." );

            } else {

                for( int i = 0; i < g_vetores_dist.vetor_count; ++i ) {
                    printf( "%d = { ", g_vetores_dist.vetor_arr[i].router_id );

                    int count = g_vetores_dist.vetor_arr[i].router_count;
                    for( int x = 0; x < count; ++x ) {
                        const int dest_id = g_vetores_dist.vetor_arr[i].router_dist_arr[x].router_dest_id;
                        const int distance = g_vetores_dist.vetor_arr[i].router_dist_arr[x].distance;

                        if( distance >= VETOR_DIST_INFINITE ) {
                            printf( "[#%d, inf]", dest_id );
                        } else {
                            printf( "[#%d, %d]", dest_id, distance );
                        }
                        if( x + 1 < count ) { printf( ", " ); }
                    }
                    printf(" }");


                    double diff = difftime( time(NULL), g_vetores_dist.vetor_arr[i].timestamp );
                    printf( "   (%d secs)\n", (int)diff );
                }
            }
        }
        // UNLOCK

        res = pthread_mutex_unlock( &g_vetores_dist.mutex );
        if( res != 0 ) { ENSURE(0, "print_vetores(): failed at \'pthread_mutex_unlock()\'\n%s", strerror(errno)); exit(1); return; }

        puts( "\n\nEsta tela e atualizada uma vez por segundo" );
        puts( "Pressione ENTER para retornar ao menu..." );

        if( !g_waiting_input ) { break; }
        sleep(1);
    }
}

//####################################################################
void recalcular_dist() {
    // Vetor que ira armazenar novo calculo de distancia
    vetor_dist_t temp;
    {
        memset( &temp, 0, sizeof(vetor_dist_t) );
        init_vetor_dist( &temp, &g_vizinho_table, &g_router_info_table);
    }


    int res = pthread_mutex_lock( &g_vetores_dist.mutex );
    if( res != 0 ) { ENSURE(0, "recalcular_dist(): failed at \'pthread_mutex_lock()\'\n%s", strerror(errno)); exit(1); return; }

    // LOCK
    {
        // Iterar vetores distancia
        for( int i = 0; i < g_vetores_dist.vetor_count; ++i ) {
            if( g_vetores_dist.vetor_arr[i].router_id == g_ID ) {
                continue;
            }

            const int custo = get_custo( &temp, g_vetores_dist.vetor_arr[i].router_id );

            int count = g_vetores_dist.vetor_arr[i].router_count;
            for( int x = 0; x < count; ++x ) {
                const int dest_id = g_vetores_dist.vetor_arr[i].router_dist_arr[x].router_dest_id;
                const int dest_custo = g_vetores_dist.vetor_arr[i].router_dist_arr[x].distance;

                if( dest_id == g_ID ) { continue; } // Desconsidera a si mesmo

                int current_custo = get_custo( &temp, dest_id );

                // Verifica se novo custo é menor
                if( dest_custo + custo < current_custo ) {
                    for( int y = 0; y < temp.router_count; ++y ) {
                        if( temp.router_dist_arr[y].router_dest_id == dest_id ) {
                            temp.router_dist_arr[y].distance = dest_custo + custo;
                            break;
                        }
                    }
                }
            }

        }
    }
    // UNLOCK

    res = pthread_mutex_unlock( &g_vetores_dist.mutex );
    if( res != 0 ) { ENSURE(0, "recalcular_dist(): failed at \'pthread_mutex_unlock()\'\n%s", strerror(errno)); exit(1); return; }


    // Atualiza a ultima vez que vetor foi atualizado e insere
    {
        temp.timestamp = time(NULL);
        add_or_replace_vetor_dist( &g_vetores_dist, &temp );
    }

    // LOG
    {
        char log_buffer[128];
        snprintf( log_buffer, 128, "Vetor distancia recalculado" );
        add_log_entry( &g_logger, log_buffer );
    }
}