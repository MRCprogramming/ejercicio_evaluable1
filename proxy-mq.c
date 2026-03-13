#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <unistd.h>
#include "mq_common.h"

/*
 * Función auxiliar interna: envía una petición al servidor y espera respuesta.
 * Devuelve 0 si la comunicación fue correcta, -2 si hubo error en las colas.
 */
static int send_and_receive(request_t *req, response_t *resp)
{
    /* Abrir la cola del servidor */
    mqd_t server_mq = mq_open(SERVER_QUEUE, O_WRONLY);
    if (server_mq == (mqd_t)-1) {
        perror("proxy: mq_open servidor");
        return -2;
    }

    /* Crear la cola de respuesta del cliente usando el PID del proceso */
    snprintf(req->client_queue, sizeof(req->client_queue), "/cliente_%d", getpid());

    struct mq_attr attr;
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = 10;
    attr.mq_msgsize = sizeof(response_t);
    attr.mq_curmsgs = 0;

    mqd_t client_mq = mq_open(req->client_queue, O_CREAT | O_RDONLY, QUEUE_PERMS, &attr);
    if (client_mq == (mqd_t)-1) {
        perror("proxy: mq_open cliente");
        mq_close(server_mq);
        return -2;
    }

    /* Enviar petición al servidor */
    if (mq_send(server_mq, (char *)req, sizeof(request_t), 0) == -1) {
        perror("proxy: mq_send");
        mq_close(server_mq);
        mq_close(client_mq);
        mq_unlink(req->client_queue);
        return -2;
    }
    mq_close(server_mq);

    /* Recibir respuesta del servidor */
    if (mq_receive(client_mq, (char *)resp, sizeof(response_t), NULL) == -1) {
        perror("proxy: mq_receive");
        mq_close(client_mq);
        mq_unlink(req->client_queue);
        return -2;
    }

    mq_close(client_mq);
    mq_unlink(req->client_queue);
    return 0;
}

/* ── Implementación de la API (lado cliente) ─────────────────────────────── */

int destroy(void)
{
    request_t  req  = {0};
    response_t resp = {0};

    req.operation = OP_DESTROY;

    if (send_and_receive(&req, &resp) == -2) return -2;
    return resp.result;
}

int set_value(char *key, char *value1, int N_value2, float *V_value2,
              struct Paquete value3)
{
    if (strlen(value1) > 255)           return -1;
    if (N_value2 < 1 || N_value2 > 32) return -1;

    request_t  req  = {0};
    response_t resp = {0};

    req.operation = OP_SET_VALUE;
    strncpy(req.key,    key,    255);
    strncpy(req.value1, value1, 255);
    req.N_value2 = N_value2;
    memcpy(req.V_value2, V_value2, N_value2 * sizeof(float));
    req.value3 = value3;

    if (send_and_receive(&req, &resp) == -2) return -2;
    return resp.result;
}

int get_value(char *key, char *value1, int *N_value2, float *V_value2,
              struct Paquete *value3)
{
    request_t  req  = {0};
    response_t resp = {0};

    req.operation = OP_GET_VALUE;
    strncpy(req.key, key, 255);

    if (send_and_receive(&req, &resp) == -2) return -2;

    if (resp.result == 0) {
        strncpy(value1, resp.value1, 256);
        *N_value2 = resp.N_value2;
        memcpy(V_value2, resp.V_value2, resp.N_value2 * sizeof(float));
        *value3 = resp.value3;
    }
    return resp.result;
}

int modify_value(char *key, char *value1, int N_value2, float *V_value2,
                 struct Paquete value3)
{
    if (strlen(value1) > 255)           return -1;
    if (N_value2 < 1 || N_value2 > 32) return -1;

    request_t  req  = {0};
    response_t resp = {0};

    req.operation = OP_MODIFY_VALUE;
    strncpy(req.key,    key,    255);
    strncpy(req.value1, value1, 255);
    req.N_value2 = N_value2;
    memcpy(req.V_value2, V_value2, N_value2 * sizeof(float));
    req.value3 = value3;

    if (send_and_receive(&req, &resp) == -2) return -2;
    return resp.result;
}

int delete_key(char *key)
{
    request_t  req  = {0};
    response_t resp = {0};

    req.operation = OP_DELETE_KEY;
    strncpy(req.key, key, 255);

    if (send_and_receive(&req, &resp) == -2) return -2;
    return resp.result;
}

int exist(char *key)
{
    request_t  req  = {0};
    response_t resp = {0};

    req.operation = OP_EXIST;
    strncpy(req.key, key, 255);

    if (send_and_receive(&req, &resp) == -2) return -2;
    return resp.result;
}
