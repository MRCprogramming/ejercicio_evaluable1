# gcc compilador Linux
CC = gcc

# CFLAGS: ayuda con el compilador
# -Wall activar muchos avisos del compilador (detectar posibles errores)
# -fPIC: -f prefijo para varias instrucciones de compilación
# PIC: Position Independent Code, para que código sea adecuado para bibliotecas compartidas (no me dé error)
CFLAGS = -Wall -fPIC

# Ejecutable busca bibliotecas compartidas TAMBIÉN durante ejecución (en directorio actual (.))
# -W indica que la variable que sigue es para el linker (l), en este caso -rpath
# -rpath,. indica linker buscar bibliotecas compartidas en directorio actual (.) durante ejecución programa
RPATH = -Wl,-rpath,.

# ---------------------------------------------------------------------------------------

# TARGETS GENERALES

# all va el primero para que sea el target por defecto al usar "make" sin argumentos
all: libclaves.so app_cliente libproxyclaves.so servidor_mq app_cliente_mq

parte_a: libclaves.so app_cliente

parte_b: libproxyclaves.so servidor_mq app_cliente_mq

clean:
	rm -f libclaves.so libproxyclaves.so app_cliente servidor_mq app_cliente_mq


# ---------------------------------------------------------------------------------------

# PARTE A: versión no distribuida

# Se busca crear libclaves.so, requiriendo claves.c (su dependencia)
# CC indica compilador a utilizar
# $(CC) indica "sustituye CC por valor de variable CC
# $(CFLAGS) lo mismo
# .so indica shared object (convención bibliotecas compartidas en Linux)
# -shared indica que será un objeto compartido enlazable
# -o indica el nombre del archivo de salida a gcc
# -lpthread indica "link (l) a biblioteca pthread (para hilos)"
# claves.c escrito 1a vez para indicar dependencia, 2a vez para compilarlo por gcc
libclaves.so: claves.c
	$(CC) $(CFLAGS) -shared -o libclaves.so claves.c -lpthread

# -L.: -L indica que linker debe buscar bibliotecas en el directorio actual (.)
# -lclaves indica que se debe linkar contra la biblioteca "claves" (libclaves.so)
# $(RPATH) asegura que ejecutable encuentre libclaves.so
app_cliente: app-cliente.c libclaves.so
	$(CC) -o app_cliente app-cliente.c -L. -lclaves $(RPATH)

# ---------------------------------------------------------------------------------------

# PARTE B: versión distribuida con colas de mensajes POSIX

# -lrt: -l link a biblioteca rt (real-time extensions) para funciones de colas de mensajes
libproxyclaves.so: proxy-mq.c
	$(CC) $(CFLAGS) -shared -o libproxyclaves.so proxy-mq.c -lrt

servidor_mq: servidor-mq.c libclaves.so
	$(CC) -o servidor_mq servidor-mq.c -L. -lclaves $(RPATH) -lpthread -lrt

# app_cliente_mq no requiere -lrt, ya tiene de dependencia a libproxyclaves.so que ya la incluye
app_cliente_mq: app-cliente.c libproxyclaves.so
	$(CC) -o app_cliente_mq app-cliente.c -L. -lproxyclaves $(RPATH)