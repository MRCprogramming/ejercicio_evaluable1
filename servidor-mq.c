#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <pthread.h>
#include "mq_common.h"


// Cada hilo recibe un req, puntero a request_t (alocado en el hilo principal),
// Procesa la petición, envía la respuesta y libera la memoria.

void *handle_request(void *arg) // Función que cada hilo ejecuta para procesar su petición
{   
    // request_t *req para acceder a los datos de la petición, resp para preparar la respuesta
    // (request_t *)arg: cast de arg a request_t*
    request_t  *req  = (request_t *)arg; // para paramétro para handle_request
    response_t  resp = {0}; // struct para respuesta

    const char *op_names[] = {"UNKNOWN", "DESTROY", "SET_VALUE", "GET_VALUE", "MODIFY_VALUE", "DELETE_KEY", "EXIST"};
    // Si op entre 1 y 6, op_name es el nombre de la operación, sino "UNKNOWN"
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

    // Enviar respuesta a la cola del cliente
    mqd_t client_mq = mq_open(req->client_queue, O_WRONLY);  // Abrir cola del cliente para escribir respuesta
    if (client_mq == (mqd_t)-1) { // convierte -1 a mqd_t (if client_mq == -1)
        perror("servidor: mq_open cliente");
    } else { // enviar respuesta a cliente
        if (mq_send(client_mq, (char *)&resp, sizeof(response_t), 0) == -1)
            perror("servidor: mq_send"); //  si falla, imprimir error
        mq_close(client_mq);
    }

    free(req);
    return NULL;
}

int main(void) // Comienza ejecución del servidor (programa que actúa como servidor)
{
    // Eliminar la cola por si quedaron colas de ejecución anterior 
    // Ejemplo: si servidor se cerró inesperadamente sin eliminar cola
    // Útil para evitar errores al crear cola (O_CREAT)
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
    if (server_mq == (mqd_t)-1) { // convierte -1 a mqd_t (if server_mq == -1)
        perror("servidor: mq_open");
        return 1;
    }

    printf("Servidor escuchando en %s...\n", SERVER_QUEUE);
    fflush(stdout);

    while (1) { // Hasta Ctrl+C
        printf("[SERVIDOR] Esperando peticion...\n");
        fflush(stdout); // fflush(stdout) imprime mensaje inmediatamente sin esperar a llenar buffer
                        // stdout: buffer de salida estándar, printf escribe en stdout
    
        // request_t (struct que contiene datos de una petición)
        // malloc devuelve dir de memoria con espacio para un request_t
        request_t *req = malloc(sizeof(request_t)); // req: puntero a struct request_t, para recibir msj de cola
        if (req == NULL) { // No se pudo encontrar espacio para request_t
            perror("servidor: malloc");
            printf("[SERVIDOR] ERROR: No se pudo alocar memoria\n");
            fflush(stdout);
            continue; // Salir del bucle
        }

        // mq_receive intenta recibir mensaje de la cola server_mq, lo guarda en req
        // Devuelve bytes recibidos o -1 si error
        ssize_t bytes = mq_receive(server_mq, (char *)req, sizeof(request_t), NULL);
        if (bytes == -1) { // Si no se pudo recibir mensaje (mq_receive devuelve -1)
            perror("servidor: mq_receive");
            printf("[SERVIDOR] ERROR: No se recibio peticion\n");
            fflush(stdout);
            free(req); // se limpia memoria alocada para req, evitar espacio alocado sin usar (memory leak)
            continue;
        }

        // \u2713 es ✓ (queda bonito)
        printf("[SERVIDOR] \u2713 Peticion recibida desde '%s' (%ld bytes)\n", req->client_queue, bytes);
        printf("[SERVIDOR] Creando hilo para procesar peticion...\n");
        fflush(stdout);

        
        // Crear hilo desvinculado para atender petición
        // Cada hilo desvinculado creado procesa, responde una petición, luego se destruye
        pthread_t tid;             // pthread_t es tipo de dato para identificador de hilo
        pthread_attr_t tattr;      // pthread_attr_t es tipo de dato para atributos del hilo
        pthread_attr_init(&tattr); // Inicializar atributos del hilo
        
        // Declarar estado desvinculación
        // Destrucción automática al terminar handle_request
        pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);

        // AQUÍ se CREA el hilo
        if (pthread_create(&tid,           // identificador hilo a usar
                           &tattr,         // atributos del hilo a usar (desvinculado)
                           handle_request, // funcion a ejecutar por el hilo
                           req) != 0) {    // req: el parámetro que usa handle_request
            perror("servidor: pthread_create");
            printf("[SERVIDOR] ERROR: No se pudo crear hilo\n");
            fflush(stdout);
            free(req); // if (1) -> Fallo al crear hilo
        } else { // Éxito al crear hilo
            printf("[SERVIDOR] Hilo creado exitosamente (TID: %ld)\n\n", (long)tid);
            fflush(stdout);
        }
        pthread_attr_destroy(&tattr); // Borrar atributos, ya no necesitados
    }

    // YA NO SIRVE PARA NADA, PERO no lo quito porque me da miedo fastidiar algo (¿y si sí sirve?)
    mq_close(server_mq);     // Cerrar la cola de mensajes del servidor
    mq_unlink(SERVER_QUEUE); // Eliminar la cola de mensajes del servidor
    return 0;
}
