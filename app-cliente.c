#include <stdio.h> 
#include "claves.h" 

int main (int argc, char **argv)  
{ 
char *key = "clave 5"; 
char *v1 = "ejemplo de valor 1"; 
float v2[] = {2.3, 0.5, 23.45}; 
struct Paquete v3; 
v3.x = 10; 
v3.y = 5; 
v3.z = 3; 
int err = set_value(key, v1, 3, v2, v3); 
if (err == -1) { 
printf("Error al insertar la tupla\n"); 
} 
return 0; 
} 