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

// inicializar mutex, atomicidad
pthread_mutex_t candado = PTHREAD_MUTEX_INITIALIZER;

// FUNCIONES
int destroy(void) {
    pthread_mutex_lock(&candado);
    struct claves *actual = head; // comenzar en head
    while (actual != NULL) { // recorrer toda la lista
        struct claves *temporal = actual; // guardar el nodo actual
        
        // struct actual pasa a ser el siguiente nodo
        // -> es como . en Python, acceder a un campo de struct
        actual = actual->siguiente;

        // sin él, free(temporal) solo borraría la dir del 1er elem de value2
        // nos quedaría mem basura inaccesible
        free(temporal->value2); // borra todo elem del vector value2
        free(temporal); // borrar todo el nodo actual
    }
    head = NULL; // resetear head a NULL (lista vacía)
    pthread_mutex_unlock(&candado);
    return 0; // éxito
}

int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    // || OR
    if (N_value2 < 1 || N_value2 > 32) {
        return -1; // error: N_value2 fuera de rango
    }

    pthread_mutex_lock(&candado);
    
    // buscar si la clave ya existe
    struct claves *actual = head;
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) { // clave encontrada
            pthread_mutex_unlock(&candado);
            return -1; // error: clave ya existe
        }
        actual = actual->siguiente;
    }

    // crear nuevo nodo para la nueva clave
    struct claves *nuevo = (struct claves *)malloc(sizeof(struct claves));
    if (nuevo == NULL) {
        pthread_mutex_unlock(&candado);
        return -1; // error: fallo en malloc
    }

    // copiar datos al nuevo nodo
    strncpy(nuevo->key, key, 256);
    strncpy(nuevo->value1, value1, 256);
    nuevo->N_value2 = N_value2;
    
    nuevo->value2 = (float *)malloc(N_value2 * sizeof(float));
    if (nuevo->value2 == NULL) {
        free(nuevo); // liberar memoria del nodo antes de salir
        pthread_mutex_unlock(&candado);
        return -1; // error: fallo en malloc para value2
    }
    
    for (int i = 0; i < N_value2; i++) {
        nuevo->value2[i] = V_value2[i]; // copiar valores del vector
    }
    
    nuevo->value3 = value3; // copiar estructura Paquete

    // insertar el nuevo nodo al inicio de la lista enlazada
    nuevo->siguiente = head;
    head = nuevo;

    pthread_mutex_unlock(&candado);
    return 0; // éxito
}

int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3) {
    pthread_mutex_lock(&candado);
    
    struct claves *actual = head;
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) { // clave encontrada
            // copiar datos al output
            strncpy(value1, actual->value1, 256);
            *N_value2 = actual->N_value2;
            for (int i = 0; i < actual->N_value2; i++) {
                V_value2[i] = actual->value2[i]; // copiar valores del vector
            }
            *value3 = actual->value3; // copiar estructura Paquete
            
            pthread_mutex_unlock(&candado);
            return 0; // éxito
        }
        actual = actual->siguiente;
    }

    pthread_mutex_unlock(&candado);
    return -1; // error: clave no encontrada
}

int modify_value(char *key, char *value1, int N_value2, float *V_value2, 
struct Paquete value3) {
    if (N_value2 < 1 || N_value2 > 32) {
        return -1; // error: N_value2 fuera de rango
    }

    pthread_mutex_lock(&candado);
    
    struct claves *actual = head;
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) {// Pedir la memoria nueva usando un puntero temporal
            float *nuevo_vector = (float *)malloc(N_value2 * sizeof(float));
            if (nuevo_vector == NULL) {
                // Si falla, se sale sin haber roto la tupla original
                pthread_mutex_unlock(&candado);
                return -1; 
            }
            
            // Si sí hay mem, copiamos los números nuevos
            for (int i = 0; i < N_value2; i++) {
                nuevo_vector[i] = V_value2[i]; 
            }
            
            // Datos nuevos seguros, borramos los antiguos
            free(actual->value2); 
            
            // Conectar nuevo vector, actualizar textos y números simples
            actual->value2 = nuevo_vector;
            strncpy(actual->value1, value1, 256);
            actual->N_value2 = N_value2;
            actual->value3 = value3; 
            
            pthread_mutex_unlock(&candado);
            return 0; // éxito
        }
        actual = actual->siguiente;
    }

    pthread_mutex_unlock(&candado);
    return -1; // error: clave no encontrada
}

int delete_key(char *key) {
    pthread_mutex_lock(&candado);
    
    struct claves *actual = head;
    struct claves *anterior = NULL;
    
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) { // clave encontrada
            // eliminar el nodo actual
            if (anterior == NULL) { // el nodo a eliminar es el head
                head = actual->siguiente; // actualizar head
            } else {
                anterior->siguiente = actual->siguiente; // saltar el nodo actual
            }
            
            free(actual->value2); // liberar memoria del vector value2
            free(actual); // liberar memoria del nodo
            
            pthread_mutex_unlock(&candado);
            return 0; // éxito
        }
        anterior = actual;
        actual = actual->siguiente;
    }

    pthread_mutex_unlock(&candado);
    return -1; // error: clave no encontrada
}

int exist(char *key) {
    pthread_mutex_lock(&candado);
    
    struct claves *actual = head;
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) { // clave encontrada
            pthread_mutex_unlock(&candado);
            return 1; // clave existe
        }
        actual = actual->siguiente;
    }

    pthread_mutex_unlock(&candado);
    return 0; // clave no existe
}