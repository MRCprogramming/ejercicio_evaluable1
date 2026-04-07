#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <unistd.h>
#include "mq_common.h"

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
Helper interno: serializa Paquete a string "x,y,z"
*/
static void serialize_paquete(struct Paquete p, char *output)
{
    snprintf(output, 64, "%d,%d,%d", p.x, p.y, p.z);
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
Función auxiliar interna: envía una petición al servidor y espera respuesta.
Formato petición:  operation|client_queue|key|value1|N_value2|float1,float2,...|x,y,z
Formato respuesta: result|value1|N_value2|float1,float2,...|x,y,z
Devuelve 0 si la comunicación fue correcta, -2 si hubo error en las colas.
*/
static int send_and_receive(int operation, const char *key, const char *value1, 
                            int N_value2, float *V_value2, struct Paquete value3,
                            char *resp_value1, int *resp_N_value2, float *resp_V_value2, 
                            struct Paquete *resp_value3)
{
    /* Crear la cola de respuesta del cliente usando el PID del proceso */
    char client_queue[64];
    snprintf(client_queue, sizeof(client_queue), "/cliente_%d", getpid());

    struct mq_attr attr;
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = 10;
    attr.mq_msgsize = MAX_MESSAGE_SIZE;
    attr.mq_curmsgs = 0;

    mq_unlink(client_queue); /* limpiar cola residual de ejecución anterior */
    mqd_t client_mq = mq_open(client_queue, O_CREAT | O_RDONLY, QUEUE_PERMS, &attr);
    if (client_mq == (mqd_t)-1) {
        perror("proxy: mq_open cliente");
        return -2;
    }

    /* Abrir la cola del servidor */
    mqd_t server_mq = mq_open(SERVER_QUEUE, O_WRONLY);
    if (server_mq == (mqd_t)-1) {
        perror("proxy: mq_open servidor");
        mq_close(client_mq);
        mq_unlink(client_queue);
        return -2;
    }

    /* Serializar petición */
    char request_msg[MAX_MESSAGE_SIZE];
    char floats_str[512] = "";
    char paquete_str[64];
    
    serialize_floats(V_value2, N_value2, floats_str);
    serialize_paquete(value3, paquete_str);
    
    snprintf(request_msg, MAX_MESSAGE_SIZE, "%d|%s|%s|%s|%d|%s|%s",
             operation, client_queue, key, value1, N_value2, floats_str, paquete_str);
    
    int msg_len = strlen(request_msg) + 1;  /* +1 para el \0 */

    /* Enviar petición al servidor */
    if (sendMessage(server_mq, request_msg, msg_len) == -1) {
        perror("proxy: sendMessage");
        mq_close(server_mq);
        mq_close(client_mq);
        mq_unlink(client_queue);
        return -2;
    }
    mq_close(server_mq);

    /* Recibir respuesta del servidor */
    char response_msg[MAX_MESSAGE_SIZE] = {0};
    if (recvMessage(client_mq, response_msg, MAX_MESSAGE_SIZE) == -1) {
        perror("proxy: recvMessage");
        mq_close(client_mq);
        mq_unlink(client_queue);
        return -2;
    }

    /* Parsear respuesta: result|value1|N_value2|float1,float2,...|x,y,z */
    int result;
    
    char *copy = strdup(response_msg);
    char *token = strtok(copy, "|");
    
    result = atoi(token);                                   /* result */
    token = strtok(NULL, "|");
    if (token && resp_value1) strncpy(resp_value1, token, 255); /* value1 */
    token = strtok(NULL, "|");
    if (resp_N_value2) *resp_N_value2 = atoi(token);        /* N_value2 */
    token = strtok(NULL, "|");
    if (token && resp_V_value2) {                             /* floats */
        *resp_N_value2 = deserialize_floats(token, resp_V_value2, 32);
    }
    token = strtok(NULL, "|");
    if (token && resp_value3) {                              /* Paquete */
        *resp_value3 = deserialize_paquete(token);
    }
    
    free(copy);

    mq_close(client_mq);
    mq_unlink(client_queue);
    return result;
}


/* Implementación de la API (lado cliente) */

int destroy(void)
{
    return send_and_receive(OP_DESTROY, "", "", 0, NULL, (struct Paquete){0},
                           NULL, NULL, NULL, NULL);
}

int set_value(char *key, char *value1, int N_value2, float *V_value2,
              struct Paquete value3)
{
    if (strlen(value1) > 255)           return -1;
    if (N_value2 < 1 || N_value2 > 32) return -1;

    return send_and_receive(OP_SET_VALUE, key, value1, N_value2, V_value2, value3,
                           NULL, NULL, NULL, NULL);
}

int get_value(char *key, char *value1, int *N_value2, float *V_value2,
              struct Paquete *value3)
{
    return send_and_receive(OP_GET_VALUE, key, "", 0, NULL, (struct Paquete){0},
                           value1, N_value2, V_value2, value3);
}

int modify_value(char *key, char *value1, int N_value2, float *V_value2,
                 struct Paquete value3)
{
    if (strlen(value1) > 255)           return -1;
    if (N_value2 < 1 || N_value2 > 32) return -1;

    return send_and_receive(OP_MODIFY_VALUE, key, value1, N_value2, V_value2, value3,
                           NULL, NULL, NULL, NULL);
}

int delete_key(char *key)
{
    return send_and_receive(OP_DELETE_KEY, key, "", 0, NULL, (struct Paquete){0},
                           NULL, NULL, NULL, NULL);
}

int exist(char *key)
{
    return send_and_receive(OP_EXIST, key, "", 0, NULL, (struct Paquete){0},
                           NULL, NULL, NULL, NULL);
}
