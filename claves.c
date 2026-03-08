#include "claves.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct claves {
    char key[256];
    char value1[256];
    int N_value2; // longitud del vector value2
    float *value2;
    struct Paquete value3;
    // crear lista enlazada (tal que no haya límte de almacenamiento)
    // puntero a la siguiente clave/tupla
    struct claves *siguiente;
};

// declarar head de la lista enlazada
// puntero a la primera clave/tupla, inicialmente NULL (lista vacía)
struct claves *head = NULL;

// mecanismo bloqueo (mutex) para operaciones atómicas.
pthread_mutex_t candado = PTHREAD_MUTEX_INITIALIZER;


// FALTA (en claves.c): implementar las funciones insert_key, get_value, modify_value, delete_key y exist.