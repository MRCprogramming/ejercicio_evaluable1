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
    /* Eliminar la cola por si quedó de una ejecución anterior */
    mq_unlink(SERVER_QUEUE);

    struct mq_attr attr;
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = 10;
    attr.mq_msgsize = sizeof(request_t);
    attr.mq_curmsgs = 0;

    mqd_t server_mq = mq_open(SERVER_QUEUE, O_CREAT | O_RDONLY, QUEUE_PERMS, &attr);
    if (server_mq == (mqd_t)-1) {
        perror("servidor: mq_open");
        return 1;
    }

    printf("Servidor escuchando en %s...\n", SERVER_QUEUE);

    while (1) {
        request_t *req = malloc(sizeof(request_t));
        if (req == NULL) {
            perror("servidor: malloc");
            continue;
        }

        ssize_t bytes = mq_receive(server_mq, (char *)req, sizeof(request_t), NULL);
        if (bytes == -1) {
            perror("servidor: mq_receive");
            free(req);
            continue;
        }

        /* Crear hilo desvinculado para atender la petición */
        pthread_t tid;
        pthread_attr_t tattr;
        pthread_attr_init(&tattr);
        pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&tid, &tattr, handle_request, req) != 0) {
            perror("servidor: pthread_create");
            free(req);
        }
        pthread_attr_destroy(&tattr);
    }

    mq_close(server_mq);
    mq_unlink(SERVER_QUEUE);
    return 0;
}
