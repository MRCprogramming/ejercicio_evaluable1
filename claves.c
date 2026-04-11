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
    // crear LISTA ENLAZADA (tal que no haya límte de almacenamiento)
    // puntero a la siguiente clave/tupla
    // [ node1 ] --- .next ---> [ node2 ] --- .next ---> NULL (fin de la lista)
    struct claves *siguiente; // = NULL;
};

// declarar head de la lista enlazada
// puntero a la primera clave/tupla, inicialmente NULL (lista vacía)
struct claves *head = NULL;

// inicializar mutex, atomicidad
pthread_mutex_t candado = PTHREAD_MUTEX_INITIALIZER;

// FUNCIONES
int destroy(void) {
    pthread_mutex_lock(&candado); // sólo este hilo actúa ahora
    struct claves *actual = head; // nombrar el nodo actual como el head
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
    while (actual != NULL) {                 // mirar todo nodo
        // string compare, 0 si son iguales
        if (strcmp(actual->key, key) == 0) { // clave encontrada
            pthread_mutex_unlock(&candado);
            return -1;                       // error: clave ya existe
        }
        actual = actual->siguiente;
    }

    // Crear nuevo nodo para la nueva clave
    // crear puntero a nodo, se le asigna puntero a mem reservada, convertida a tipo struct claves
    struct claves *nuevo = (struct claves *)malloc(sizeof(struct claves));
    if (nuevo == NULL) {
        pthread_mutex_unlock(&candado);
        return -1; // error: fallo en malloc
    }

    // Copiar datos al nuevo nodo
    // 1er param: donde copiar, 2do param: qué copiar, 3ro: tam máx copiar (evitar overflow)
    strncpy(nuevo->key, key, 255);
    nuevo->key[255] = '\0';
    strncpy(nuevo->value1, value1, 255);
    nuevo->value1[255] = '\0';
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

// Guarda los valores sacados en los parámetros, tal que luego los puedas acceder
int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3) {
    pthread_mutex_lock(&candado);
    
    struct claves *actual = head;
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) { // clave encontrada
            // copiar datos al output
            // 1er param: donde copiar, 2do param: qué copiar, 3ro: tam máx copiar (evitar overflow)
            strncpy(value1, actual->value1, 256);
            *N_value2 = actual->N_value2;
            for (int i = 0; i < actual->N_value2; i++) {
                V_value2[i] = actual->value2[i]; // copiar valores del vector
            }
            *value3 = actual->value3; // copiar dir estructura Paquete
            
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
            // Crear puntero a decimal, poner valor de dir de mem reservada para dicho decimal
            float *nuevo_vector = (float *)malloc(N_value2 * sizeof(float)); // puntero al PRIMER elem del vector
            if (nuevo_vector == NULL) { // Sale NULL si no se encuentra mem suficiente
                // Si falla, se sale sin haber roto la tupla original
                pthread_mutex_unlock(&candado);
                return -1; 
            }
            // SÍ hay mem
            // Copiamos los números nuevos
            for (int i = 0; i < N_value2; i++) {
                nuevo_vector[i] = V_value2[i]; 
            }
            
            // Datos nuevos seguros, borramos los antiguos
            free(actual->value2); 
            
            // Conectar nuevo vector, actualizar textos y números simples
            actual->value2 = nuevo_vector;
            // longitud de 256 para value1
            strncpy(actual->value1, value1, 255); // 255 porque el 256 es para el \0 al final
            actual->value1[255] = '\0';
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
            if (anterior == NULL) { // si el nodo a eliminar es el head
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