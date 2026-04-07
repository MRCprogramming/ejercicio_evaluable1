#include "mq_common.h"

/*
 * sendMessage: Envía exactamente 'len' bytes por una cola de mensajes
 * Usa mq_send() iterativamente hasta enviar todos los bytes
 * Devuelve 0 si éxito, -1 si error
 */
int sendMessage(mqd_t mqdes, char *buffer, size_t len)
{
    ssize_t sent;
    
    /* mq_send envía todo el mensaje de una vez (no parcial como write) */
    sent = mq_send(mqdes, buffer, len, 0);
    
    if (sent == -1)
        return -1;  /* Error */
    else
        return 0;   /* Éxito */
}

/*
 * recvMessage: Recibe exactamente 'len' bytes de una cola de mensajes
 * Devuelve 0 si éxito, -1 si error
 */
int recvMessage(mqd_t mqdes, char *buffer, size_t len)
{
    ssize_t received;
    
    /* mq_receive recibe todo el mensaje disponible */
    received = mq_receive(mqdes, buffer, len, NULL);
    
    if (received == -1)
        return -1;  /* Error */
    else
        return 0;   /* Éxito */
}
