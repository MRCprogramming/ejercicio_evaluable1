#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <pthread.h>
#include "mq_common.h"

/*
 * Cada hilo recibe un puntero a request_t (alocado en el hilo principal),
 * procesa la petición, envía la respuesta y libera la memoria.
 */

void *handle_request(void *arg)
{
    request_t  *req  = (request_t *)arg;
    response_t  resp = {0};

    const char *op_names[] = {"UNKNOWN", "DESTROY", "SET_VALUE", "GET_VALUE", "MODIFY_VALUE", "DELETE_KEY", "EXIST"};
    const char *op_name = (req->operation >= 1 && req->operation <= 6) ? op_names[req->operation] : "UNKNOWN";
    printf("[HILO] Procesando operacion %d (%s) para clave '%s'\n", req->operation, op_name, req->key);
    fflush(stdout);

    switch (req->operation) {
        case OP_DESTROY:
            resp.result = destroy();
            break;

        case OP_SET_VALUE:
            resp.result = set_value(req->key, req->value1,
                                    req->N_value2, req->V_value2, req->value3);
            break;

        case OP_GET_VALUE:
            resp.result = get_value(req->key, resp.value1,
                                    &resp.N_value2, resp.V_value2, &resp.value3);
            break;

        case OP_MODIFY_VALUE:
            resp.result = modify_value(req->key, req->value1,
                                       req->N_value2, req->V_value2, req->value3);
            break;

        case OP_DELETE_KEY:
            resp.result = delete_key(req->key);
            break;

        case OP_EXIST:
            resp.result = exist(req->key);
            break;

        default:
            resp.result = -1;
            break;
    }

    /* Enviar respuesta a la cola del cliente */
    mqd_t client_mq = mq_open(req->client_queue, O_WRONLY);
    if (client_mq == (mqd_t)-1) {
        perror("servidor: mq_open cliente");
    } else {
        if (mq_send(client_mq, (char *)&resp, sizeof(response_t), 0) == -1)
            perror("servidor: mq_send");
        mq_close(client_mq);
    }

    free(req);
    return NULL;
}

int main(void)
{
    /* Eliminar la cola por si quedaron colas de ejecución anterior 
    Ejemplo: si servidor se cerró inesperadamente sin eliminar cola
    Útil para evitar errores al crear cola (O_CREAT) */
    mq_unlink(SERVER_QUEUE);

    // mq_attr es estructura que define los atributos de la cola
    // Se pasa attr como argumento a mq_open para crear la cola
    struct mq_attr attr;
    attr.mq_flags   = 0;                 // Modo bloqueante (0), si cola vacía, espera a nvo msj
    attr.mq_maxmsg  = 10;                // Número máximo de mensajes en cola
    attr.mq_msgsize = sizeof(request_t); // Tamaño máximo de cada mensaje (request_t)
    attr.mq_curmsgs = 0;                 // Número actual de mensajes en cola (inicialmente 0)

    // mqd_t server_mq es descriptor de la cola, para crearla con mq_open
    mqd_t server_mq = mq_open(SERVER_QUEUE, O_CREAT | O_RDONLY, QUEUE_PERMS, &attr);
    if (server_mq == (mqd_t)-1) { // Si server_mq == -1
        perror("servidor: mq_open");
        return 1;
    }

    printf("Servidor escuchando en %s...\n", SERVER_QUEUE);
    fflush(stdout);

    while (1) {
        printf("[SERVIDOR] Esperando peticion...\n");
        fflush(stdout);

        // Puntero a request_t (struct que contiene todo dato de una petición)
        // malloc devuelve dir de memoria con espacio para un request_t
        request_t *req = malloc(sizeof(request_t));
        if (req == NULL) { // No se pudo alocar memoria
            perror("servidor: malloc");
            printf("[SERVIDOR] ERROR: No se pudo alocar memoria\n");
            fflush(stdout);
            continue; // Salir del bucle
        }

        ssize_t bytes = mq_receive(server_mq, (char *)req, sizeof(request_t), NULL);
        if (bytes == -1) {
            perror("servidor: mq_receive");
            printf("[SERVIDOR] ERROR: No se recibio peticion\n");
            fflush(stdout);
            free(req);
            continue;
        }

        printf("[SERVIDOR] \u2713 Peticion recibida desde '%s' (%ld bytes)\n", req->client_queue, bytes);
        printf("[SERVIDOR] Creando hilo para procesar peticion...\n");
        fflush(stdout);

        /* Crear hilo desvinculado para atender la petición */
        pthread_t tid;
        pthread_attr_t tattr;
        pthread_attr_init(&tattr);
        pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&tid, &tattr, handle_request, req) != 0) {
            perror("servidor: pthread_create");
            printf("[SERVIDOR] ERROR: No se pudo crear hilo\n");
            fflush(stdout);
            free(req);
        } else {
            printf("[SERVIDOR] Hilo creado exitosamente (TID: %ld)\n\n", (long)tid);
            fflush(stdout);
        }
        pthread_attr_destroy(&tattr);
    }

    mq_close(server_mq);
    mq_unlink(SERVER_QUEUE);
    return 0;
}
