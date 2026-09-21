CC = gcc

# Includes necesarios para RPC con tirpc
CPPFLAGS = -Iinclude -I. -I/usr/include/tirpc -D_REENTRANT
CFLAGS = -Wall -Wextra -pthread -g
RPCFLAGS = -Wno-unused-parameter -Wno-unused-variable -Wno-cast-function-type
LDLIBS = -lnsl -lpthread -ltirpc

TARGET = server
LOG_SERVER = servidor_log

# Archivos fuente principales
SRCS = src/server.c src/estado.c src/lines.c src/protocolo.c

# Archivos RPC generados automáticamente
RPC_GEN_SRCS = fichero_clnt.c fichero_xdr.c
RPC_GEN_HDR = fichero.h

# Objetos
OBJS = $(SRCS:.c=.o) $(RPC_GEN_SRCS:.c=.o)

.PHONY: all clean

# Regla principal
all: $(TARGET) $(LOG_SERVER)

# Generación de RPC (CLAVE)
$(RPC_GEN_SRCS) $(RPC_GEN_HDR): fichero.x
	rpcgen -NM fichero.x

# Servidor principal
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDLIBS)

# Servidor de logs
$(LOG_SERVER): fichero_svc.c fichero_server.c fichero_xdr.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(RPCFLAGS) $^ -o $@ $(LDLIBS)

# Compilación src/
src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Compilación RPC en raíz
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(RPCFLAGS) -c $< -o $@

# Limpieza
clean:
	rm -f $(SRCS:.c=.o) $(RPC_GEN_SRCS:.c=.o)
	rm -f $(TARGET) $(LOG_SERVER)
	rm -f fichero_clnt.c fichero_svc.c fichero_xdr.c