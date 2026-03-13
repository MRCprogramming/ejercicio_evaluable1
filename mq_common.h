#ifndef MQ_COMMON_H
#define MQ_COMMON_H

#include "claves.h"

/* Nombre de la cola del servidor (bien conocido por ambos lados) */
#define SERVER_QUEUE "/servidor_mq"

/* Códigos de operación */
#define OP_DESTROY      1
#define OP_SET_VALUE    2
#define OP_GET_VALUE    3
#define OP_MODIFY_VALUE 4
#define OP_DELETE_KEY   5
#define OP_EXIST        6

/* Permisos de las colas */
#define QUEUE_PERMS 0666

/* Mensaje de petición (cliente → servidor) */
typedef struct {
    int   operation;
    char  client_queue[64]; /* nombre de la cola de respuesta del cliente */
    char  key[256];
    char  value1[256];
    int   N_value2;
    float V_value2[32];
    struct Paquete value3;
} request_t;

/* Mensaje de respuesta (servidor → cliente) */
typedef struct {
    int   result;
    char  value1[256];
    int   N_value2;
    float V_value2[32];
    struct Paquete value3;
} response_t;

#endif
