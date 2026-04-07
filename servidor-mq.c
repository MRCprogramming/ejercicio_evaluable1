#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <pthread.h>
#include "mq_common.h"

/*
Helper interno: deserializa floats de string "f1,f2,f3"
*/
static int deserialize_floats(char *input, float *floats, int max_count)
{
    int count = 0;
    char *copy = strdup(input);
    char *token = strtok(copy, ",");
    
    while (token && count < max_count) {
        floats[count] = atof(token);
        token = strtok(NULL, ",");
        count++;
    }
    
    free(copy);
    return count;
}

/*
Helper interno: serializa floats a string "f1,f2,f3"
*/
static void serialize_floats(float *floats, int count, char *output)
{
    output[0] = '\0';
    for (int i = 0; i < count; i++) {
        if (i > 0) strcat(output, ",");
        char temp[32];
        snprintf(temp, sizeof(temp), "%.6f", floats[i]);
        strcat(output, temp);
    }
}

/*
Helper interno: deserializa Paquete de string "x,y,z"
*/
static struct Paquete deserialize_paquete(char *input)
{
    struct Paquete p = {0};
    sscanf(input, "%d,%d,%d", &p.x, &p.y, &p.z);
    return p;
}

/*
Helper interno: serializa Paquete a string "x,y,z"
*/
static void serialize_paquete(struct Paquete p, char *output)
{
    snprintf(output, 64, "%d,%d,%d", p.x, p.y, p.z);
}

/*
Estructura temporal para pasar los datos del request parseado al hilo
*/
typedef struct {
    int operation;
    char client_queue[64];
    char key[256];
    char value1[256];
    int N_value2;
    float V_value2[32];
    struct Paquete value3;
} request_data_t;

// Cada hilo recibe los datos parseados de una petición, la procesa, 
// y envía la respuesta serializada

void *handle_request(void *arg)
{
    request_data_t *req = (request_data_t *)arg;
    
    const char *op_names[] = {"UNKNOWN", "DESTROY", "SET_VALUE", "GET_VALUE", "MODIFY_VALUE", "DELETE_KEY", "EXIST"};
    const char *op_name = (req->operation >= 1 && req->operation <= 6) ? op_names[req->operation] : "UNKNOWN";
    printf("[HILO] Procesando operacion %d (%s) para clave '%s'\n", req->operation, op_name, req->key);
    fflush(stdout);

    int result = -1;
    char resp_value1[256] = "";
    int resp_N_value2 = 0;
    float resp_V_value2[32] = {0};
    struct Paquete resp_value3 = {0};

    switch (req->operation) {
        case OP_DESTROY:
            result = destroy();
            break;

        case OP_SET_VALUE:
            result = set_value(req->key, req->value1,
                              req->N_value2, req->V_value2, req->value3);
            break;

        case OP_GET_VALUE:
            result = get_value(req->key, resp_value1,
                              &resp_N_value2, resp_V_value2, &resp_value3);
            break;

        case OP_MODIFY_VALUE:
            result = modify_value(req->key, req->value1,
                                 req->N_value2, req->V_value2, req->value3);
            break;

        case OP_DELETE_KEY:
            result = delete_key(req->key);
            break;

        case OP_EXIST:
            result = exist(req->key);
            break;

        default:
            result = -1;
            break;
    }

    /* Serializar respuesta: result|value1|N_value2|float1,float2,...|x,y,z */
    char floats_str[512] = "";
    char paquete_str[64];
    
    serialize_floats(resp_V_value2, resp_N_value2, floats_str);
    serialize_paquete(resp_value3, paquete_str);
    
    char response_msg[MAX_MESSAGE_SIZE];
    snprintf(response_msg, MAX_MESSAGE_SIZE, "%d|%s|%d|%s|%s",
             result, resp_value1, resp_N_value2, floats_str, paquete_str);
    
    int msg_len = strlen(response_msg) + 1;

    /* Enviar respuesta a la cola del cliente */
    mqd_t client_mq = mq_open(req->client_queue, O_WRONLY);
    if (client_mq == (mqd_t)-1) {
        perror("servidor: mq_open cliente");
    } else {
        if (sendMessage(client_mq, response_msg, msg_len) == -1)
            perror("servidor: sendMessage");
        mq_close(client_mq);
    }

    free(req);
    return NULL;
}

int main(void)
{
    /* Eliminar la cola por si quedaron colas de ejecución anterior */
    mq_unlink(SERVER_QUEUE);

    struct mq_attr attr;
    attr.mq_flags   = 0;                 /* Modo bloqueante */
    attr.mq_maxmsg  = 10;                /* Número máximo de mensajes en cola */
    attr.mq_msgsize = MAX_MESSAGE_SIZE;  /* Tamaño máximo de cada mensaje */
    attr.mq_curmsgs = 0;                 /* Número actual de mensajes */

    mqd_t server_mq = mq_open(SERVER_QUEUE, O_CREAT | O_RDONLY, QUEUE_PERMS, &attr);
    if (server_mq == (mqd_t)-1) {
        perror("servidor: mq_open");
        return 1;
    }

    printf("Servidor escuchando en %s...\n", SERVER_QUEUE);
    fflush(stdout);

    while (1) {
        printf("[SERVIDOR] Esperando peticion...\n");
        fflush(stdout);

        /* Recibir petición serializada */
        char request_msg[MAX_MESSAGE_SIZE] = {0};
        if (recvMessage(server_mq, request_msg, MAX_MESSAGE_SIZE) == -1) {
            perror("servidor: recvMessage");
            printf("[SERVIDOR] ERROR: No se recibio peticion\n");
            fflush(stdout);
            continue;
        }

        printf("[SERVIDOR] ✓ Peticion recibida (%zu bytes)\n", strlen(request_msg));
        fflush(stdout);

        /* Parsear petición: operation|client_queue|key|value1|N_value2|float1,float2,...|x,y,z */
        request_data_t *req = malloc(sizeof(request_data_t));
        if (req == NULL) {
            perror("servidor: malloc");
            printf("[SERVIDOR] ERROR: No se pudo alocar memoria\n");
            fflush(stdout);
            continue;
        }

        char *copy = strdup(request_msg);
        char *token = strtok(copy, "|");

        req->operation = token ? atoi(token) : -1;              /* operation */
        token = strtok(NULL, "|");
        if (token) strncpy(req->client_queue, token, 63);       /* client_queue */
        token = strtok(NULL, "|");
        if (token) strncpy(req->key, token, 255);               /* key */
        token = strtok(NULL, "|");
        if (token) strncpy(req->value1, token, 255);            /* value1 */
        token = strtok(NULL, "|");
        req->N_value2 = token ? atoi(token) : 0;                /* N_value2 */
        token = strtok(NULL, "|");
        if (token) {                                            /* floats */
            req->N_value2 = deserialize_floats(token, req->V_value2, 32);
        }
        token = strtok(NULL, "|");
        if (token) {                                            /* Paquete */
            req->value3 = deserialize_paquete(token);
        }

        free(copy);

        printf("[SERVIDOR] Creando hilo para procesar peticion...\n");
        fflush(stdout);

        /* Crear hilo desvinculado */
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
