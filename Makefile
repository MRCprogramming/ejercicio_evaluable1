CC = gcc
CFLAGS = -Wall -fPIC
RPATH = -Wl,-rpath,.

# ── Parte A: versión no distribuida ──────────────────────────────────────────

libclaves.so: claves.c
	$(CC) $(CFLAGS) -shared -o libclaves.so claves.c -lpthread

app_cliente: app-cliente.c libclaves.so
	$(CC) -o app_cliente app-cliente.c -L. -lclaves $(RPATH)

# ── Parte B: versión distribuida con colas de mensajes POSIX ─────────────────

libproxyclaves.so: proxy-mq.c
	$(CC) $(CFLAGS) -shared -o libproxyclaves.so proxy-mq.c -lrt

servidor_mq: servidor-mq.c libclaves.so
	$(CC) -o servidor_mq servidor-mq.c -L. -lclaves $(RPATH) -lpthread -lrt

app_cliente_mq: app-cliente.c libproxyclaves.so
	$(CC) -o app_cliente_mq app-cliente.c -L. -lproxyclaves $(RPATH)

# ── Targets generales ────────────────────────────────────────────────────────

all: libclaves.so app_cliente libproxyclaves.so servidor_mq app_cliente_mq

parte_a: libclaves.so app_cliente

parte_b: libproxyclaves.so servidor_mq app_cliente_mq

clean:
	rm -f libclaves.so libproxyclaves.so app_cliente servidor_mq app_cliente_mq
