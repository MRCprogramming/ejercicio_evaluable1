#include "mq_common.h"
// Este archivo proporciona funciones comunes para enviar y recibir mensajes por colas de mensajes POSIX

// sendMessage envía 'len' bytes por una cola de mensajes
// Usa mq_send() iterativamente hasta enviar todos los bytes
// Devuelve 0 si éxito, -1 si error
int sendMessage(mqd_t mqdes,  // descriptor de la cola
                char *buffer, // datos a enviar
                size_t len)   // cantidad de bytes a enviar
{
    ssize_t sent; // cantidad de bytes enviados en cada llamada a mq_send
    
    // mq_send envía todo el mensaje de una vez
    sent = mq_send(mqdes, // descriptor de la cola
                   buffer, // datos a enviar
                   len,   // cantidad de bytes a enviar
                   0);    // prioridad del mensaje (0 la menor, y default)
    
    if (sent == -1)
        return -1;  // Error
    else
        return 0;   // Éxito
}

// recvMessage: Recibe exactamente 'len' bytes de una cola de mensajes
// Devuelve 0 si éxito, -1 si error
int recvMessage(mqd_t mqdes, // descriptor de la cola
                char *buffer, // buffer para recibir los datos
                size_t len) // cantidad de bytes a recibir
{
    ssize_t received;
    
    // mq_receive recibe todo el mensaje disponible
    received = mq_receive(mqdes, // descriptor de la cola
                          buffer, // buffer para recibir los datos
                          len,    // cantidad de bytes a recibir
                          NULL);  // prioridad del mensaje
    
    if (received == -1)
        return -1;  // Error
    else
        return 0;   // Éxito
}
