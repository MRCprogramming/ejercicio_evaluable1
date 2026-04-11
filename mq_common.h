// Si MQ_COMMON_H no está definido, entonces (siguiente línea)
#ifndef MQ_COMMON_H
#define MQ_COMMON_H

#include "claves.h"
#include <unistd.h>
#include <errno.h>
#include <mqueue.h>

// Nombre de la cola del servidor (bien conocido por ambos lados)
#define SERVER_QUEUE "/servidor_mq"

// Códigos de operación
#define OP_DESTROY      1
#define OP_SET_VALUE    2
#define OP_GET_VALUE    3
#define OP_MODIFY_VALUE 4
#define OP_DELETE_KEY   5
#define OP_EXIST        6

// Permisos de las colas
#define QUEUE_PERMS 0666 // Lectura y escritura para todos

// Tamaño máximo de mensaje serializados
#define MAX_MESSAGE_SIZE 4096

// Protocolo de serialización:
// REQUEST:  operation|client_queue|key|value1|N_value2|float1,float2,...|x,y,z
// RESPONSE: result|value1|N_value2|float1,float2,...|x,y,z
// Los campos están separados por '|'
// Los floats en el vector están separados por ','
// El Paquete se serializa como x,y,z

// sendMessage: Envía exactamente 'len' bytes por una cola de mensajes
// Devuelve 0 si éxito, -1 si error
int sendMessage(mqd_t mqdes, char *buffer, size_t len);

// recvMessage: Recibe exactamente 'len' bytes de una cola de mensajes
// Devuelve 0 si éxito, -1 si error
int recvMessage(mqd_t mqdes, char *buffer, size_t len);

#endif
