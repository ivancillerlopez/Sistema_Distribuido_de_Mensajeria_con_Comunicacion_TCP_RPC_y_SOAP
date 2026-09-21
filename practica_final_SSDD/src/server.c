/*Solo la parte general:
main
parseo del puerto
creación del socket servidor
bind
listen
accept
creación de hilo por conexión
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "estado.h"
#include "protocolo.h"
#include "lines.h"
#include "fichero.h"

void reportar_log(char *usuario, char *operacion, char *fichero);

/*Estructura auxiliar para almacenar los argumentos que pasaremos a traves del hilo*/
struct ThreadArgs {
    int sc;
    /*Accept ya nos da la IP del usuario*/
    struct sockaddr_in cliente_addr;
};

static void entregar_pendientes(const char *destinatario) {
    struct MensajePendiente mensaje;
    struct DatosConexion datos_destinatario;
    struct DatosConexion datos_remitente;
    int sd_destinatario;
    int sd_remitente;

    /* Procesamos la cola en orden, empezando siempre por el primer pendiente. */
    while (obtener_primer_mensaje_pendiente(destinatario, &mensaje) == 0) {
        /* Si el destinatario ya no esta conectado, se conserva el mensaje. */
        if (obtener_datos_conexion(destinatario, &datos_destinatario) != 0 ||
            !datos_destinatario.conectado) {
            break;
        }

        /* Abrimos una conexion nueva al hilo de escucha del destinatario. */
        sd_destinatario = conectar_a_cliente(datos_destinatario.ip, datos_destinatario.puerto);
        if (sd_destinatario < 0) {
            desconectar(destinatario);
            break;
        }

        /* Si falla el envio, no se elimina de pendientes para reintentarlo despues. */
        if (enviar_send_message(sd_destinatario, mensaje.remitente, mensaje.id, mensaje.texto) < 0) {
            close(sd_destinatario);
            desconectar(destinatario);
            break;
        }
        close(sd_destinatario);

        printf("SEND MESSAGE %u FROM %s TO %s OK\n", mensaje.id, mensaje.remitente, destinatario);
        printf("s> ");
        fflush(stdout);

        /* Solo borramos el pendiente cuando el envio al destinatario ha salido bien. */
        if (eliminar_primer_mensaje_pendiente(destinatario) != 0) {
            break;
        }

        /* Avisamos al remitente de la entrega si sigue conectado. */
        if (obtener_datos_conexion(mensaje.remitente, &datos_remitente) == 0 &&
            datos_remitente.conectado) {
            sd_remitente = conectar_a_cliente(datos_remitente.ip, datos_remitente.puerto);
            if (sd_remitente >= 0) {
                enviar_send_message_ack(sd_remitente, mensaje.id);
                close(sd_remitente);
            }
        }
    }
}

static void entregar_pendientes_attach(const char *destinatario) {
    struct MensajePendienteAttach mensaje;
    struct DatosConexion datos_destinatario;
    struct DatosConexion datos_remitente;
    int sd_destinatario;
    int sd_remitente;

    /* Procesamos la cola en orden, empezando siempre por el primer pendiente. */
    while (obtener_primer_mensaje_pendiente_attach(destinatario, &mensaje) == 0) {
        /* Si el destinatario ya no esta conectado, se conserva el mensaje. */
        if (obtener_datos_conexion(destinatario, &datos_destinatario) != 0 ||
            !datos_destinatario.conectado) {
            break;
        }

        /* Abrimos una conexion nueva al hilo de escucha del destinatario. */
        sd_destinatario = conectar_a_cliente(datos_destinatario.ip, datos_destinatario.puerto);
        if (sd_destinatario < 0) {
            desconectar(destinatario);
            break;
        }

        /* Si falla el envio, no se elimina de pendientes para reintentarlo despues. */
        if (enviar_send_message_attach(sd_destinatario, mensaje.remitente_attach, mensaje.id_attach, mensaje.texto_attach, mensaje.file) < 0) {
            close(sd_destinatario);
            desconectar(destinatario);
            break;
        }
        close(sd_destinatario);

        printf("SENDATTACH MESSAGE %u FROM %s TO %s OK\n", mensaje.id_attach, mensaje.remitente_attach, destinatario);
        printf("s> ");
        fflush(stdout);

        /* Solo borramos el pendiente cuando el envio al destinatario ha salido bien. */
        if (eliminar_primer_mensaje_pendiente_attach(destinatario) != 0) {
            break;
        }

        /* Avisamos al remitente de la entrega si sigue conectado. */
        if (obtener_datos_conexion(mensaje.remitente_attach, &datos_remitente) == 0 &&
            datos_remitente.conectado) {
            sd_remitente = conectar_a_cliente(datos_remitente.ip, datos_remitente.puerto);
            if (sd_remitente >= 0) {
                enviar_send_message_ack_attach(sd_remitente, mensaje.id_attach, mensaje.file);
                close(sd_remitente);
            }
        }
    }
}

void *tratar_peticion(void *arg){
    struct ThreadArgs *thread_args;
    char respuesta_str;
    int sc;
    int i;
    int num_usuarios;
    char *endptr;
    long puerto_largo;

    char nombre[MAX_NOMBRE];              /* nombre del usuario */
    struct sockaddr_in ip;                /* IP del cliente cuando esta conectado */
    char puerto_str[MAX_TEXTO];           /* puerto de escucha del cliente */
    int32_t respuesta;                    /* respuesta correspondiente */
    char operacion[MAX_TEXTO];            /* leer operacion con readLine */
    
    char lista_conectados[MAX_LISTA][MAX_NOMBRE]; /* lista con los usuarios conectados */
    char num_usuarios_str[MAX_TEXTO];
    
    char remitente[MAX_NOMBRE];
    unsigned int id;
    char destinatario[MAX_NOMBRE];
    char texto[MAX_TEXTO];
    char archivo[MAX_TEXTO];
    struct DatosConexion datos_destinatario;
    char id_str[32];

    struct DatosConexion datos;
    char buffer[512];


    codigo_operacion op;

    thread_args = (struct ThreadArgs *)arg;
    sc = thread_args->sc;
    ip = thread_args->cliente_addr;
    free(thread_args);

    /* Obtenemos la operacion mandada por el cliente. */
    if (readLine(sc, operacion, MAX_TEXTO) < 0) {
        close(sc);
        return NULL;
    }

    /* Convertimos la operacion recibida a un codigo manejable por switch. */
    op = string_a_int_codigo_operacion(operacion);

    switch (op){
        case REGISTER: {
            /* Leemos el nombre, registramos y enviamos el codigo de respuesta al cliente. */
            if (readLine(sc, nombre, MAX_NOMBRE) < 0){
                close(sc);
                return NULL;
            }
            respuesta = registro(nombre);
            sendMessage(sc, (char*)&respuesta, 1);

            /* Imprimimos el mensaje correspondiente. */
            if (respuesta == 0){
                printf("REGISTER %s OK\n", nombre);
                reportar_log(nombre, "REGISTER", NULL);
                printf("s> ");
                fflush(stdout);
            } else {
                printf("REGISTER %s FAIL\n", nombre);
                printf("s> ");
                fflush(stdout);
            }

            break; 
        }

        case UNREGISTER: {
            /* Leemos el nombre, eliminamos el registro y enviamos la respuesta. */
            if (readLine(sc, nombre, MAX_NOMBRE) < 0){
                close(sc);
                return NULL;
            }
            respuesta = eliminar_registro(nombre);
            sendMessage(sc, (char*)&respuesta, 1);

            /* Imprimimos el mensaje correspondiente. */
            if (respuesta == 0){
                printf("UNREGISTER %s OK\n", nombre);
                reportar_log(nombre, "UNREGISTER", NULL);
                printf("s> ");
                fflush(stdout);
            } else {
                printf("UNREGISTER %s FAIL\n", nombre);
                printf("s> ");
                fflush(stdout);
            }

            break;
        }

        case CONNECT: {
            /* Leemos el nombre. */
            if (readLine(sc, nombre, MAX_NOMBRE) < 0){
                close(sc);
                return NULL;
            }

            /* Leemos el puerto. */
            if (readLine(sc, puerto_str, MAX_TEXTO) < 0){
                close(sc);
                return NULL;
            }

            /* Obtenemos la IP. */
            /* La IP ya viene dada por accept en el hilo principal. */

            /* Convertimos el puerto de string a numero. */
            puerto_largo = strtol(puerto_str, &endptr, 10);
            if (*endptr != '\0' || puerto_largo < 1 || puerto_largo > 65535) {
                respuesta = 3; /* Error interno */
            } else {
                in_port_t puerto_cliente = (in_port_t)puerto_largo;
             
                /* Llamamos a conectar con la IP obtenida por accept. */
                respuesta = conectar(nombre, ip.sin_addr, puerto_cliente);
            }
            sendMessage(sc, (char*)&respuesta, 1);

            /* Imprimimos el mensaje correspondiente. */
            if (respuesta == 0){
                printf("CONNECT %s OK\n", nombre);
                reportar_log(nombre, "CONNECT", NULL);
                entregar_pendientes(nombre);
                entregar_pendientes_attach(nombre);
                printf("s> ");
                fflush(stdout);

            } else {
                printf("CONNECT %s FAIL\n", nombre);
                printf("s> ");
                fflush(stdout);
            }

            break;
        }

        case DISCONNECT: {
            /* Leemos el nombre, desconectamos y enviamos la respuesta. */
            if (readLine(sc, nombre, MAX_NOMBRE) < 0){
                close(sc);
                return NULL;
            }
            respuesta = desconectar(nombre);
            sendMessage(sc, (char*)&respuesta, 1);

            /* Imprimimos el mensaje correspondiente. */
            if (respuesta == 0){
                printf("DISCONNECT %s OK\n", nombre);
                reportar_log(nombre, "DISCONNECT", NULL);
                printf("s> ");
                fflush(stdout);
            } else {
                printf("DISCONNECT %s FAIL\n", nombre);
                printf("s> ");
                fflush(stdout);
            }

            break;
        }

        case USERS: {
            if (readLine(sc, nombre, MAX_NOMBRE) < 0){
                close(sc);
                return NULL;
            }

            respuesta = obtener_usuarios_conectados(nombre, lista_conectados, MAX_LISTA, &num_usuarios);
            if (sendMessage(sc, (char*)&respuesta, 1) < 0) {
                break;
            }

            if (respuesta == 0) {
                snprintf(num_usuarios_str, sizeof(num_usuarios_str), "%d", num_usuarios);
                if (sendMessage(sc, num_usuarios_str, strlen(num_usuarios_str) + 1) < 0) {
                    break;
                }

                printf("CONNECTEDUSERS OK\n");
                reportar_log(nombre, "USERS", NULL);
                printf("s> ");
                fflush(stdout);

                for (i = 0; i < num_usuarios; i++) {
                    if (obtener_datos_conexion(lista_conectados[i], &datos) != 0) {
                        continue; /* por seguridad */
                    }
                    snprintf(buffer, sizeof(buffer), "%s::%s::%d", lista_conectados[i], inet_ntoa(datos.ip), datos.puerto);
                    if (sendMessage(sc, buffer, strlen(buffer) + 1) < 0) {
                        perror("Error enviando usuario");
                        close(sc);
                        return NULL;
                    }
                }
            } else {
                printf("CONNECTEDUSERS FAIL\n");
                printf("s> ");
                fflush(stdout);
            }
            break;
        }

        case SEND: {
            if (readLine(sc, remitente, MAX_NOMBRE) < 0){
                close(sc);
                return NULL;
            }
            if (readLine(sc, destinatario, MAX_NOMBRE) < 0){
                close(sc);
                return NULL;
            }
            if (readLine(sc, texto, MAX_TEXTO) < 0){
                close(sc);
                return NULL;
            }

            respuesta = obtener_datos_conexion(destinatario, &datos_destinatario);
            if (respuesta == 0) {
                respuesta = generar_id_mensaje(remitente, &id);
            }
            if (respuesta == 0) {
                respuesta = anadir_mensaje_pendiente(destinatario, id, remitente, texto);
            }

            respuesta_str = (char)respuesta;
            if (sendMessage(sc, &respuesta_str, 1) < 0) {
                break;
            }

            if (respuesta == 0) {
                snprintf(id_str, sizeof(id_str), "%u", id);
                if (sendMessage(sc, id_str, strlen(id_str) + 1) < 0) {
                    break;
                }

                if (datos_destinatario.conectado) {
                    entregar_pendientes(destinatario);
                } else {
                    printf("MESSAGE %u FROM %s TO %s STORED\n", id, remitente, destinatario);
                    printf("s> ");
                    fflush(stdout);
                }
                reportar_log(remitente, "SEND", NULL);
            } else if (respuesta == 1) {
                printf("SEND %s FAIL, USER DOES NOT EXIST\n", destinatario);
                printf("s> ");
                fflush(stdout);
            } else {
                printf("SEND %s FAIL\n", destinatario);
                printf("s> ");
                fflush(stdout);
            }
            break;
        }

        case SENDATTACH :{
            if (readLine(sc, remitente, MAX_NOMBRE) < 0){
                close(sc);
                return NULL;
            }
            if (readLine(sc, destinatario, MAX_NOMBRE) < 0){
                close(sc);
                return NULL;
            }
            if (readLine(sc, texto, MAX_TEXTO) < 0){
                close(sc);
                return NULL;
            }

            if (readLine(sc, archivo, MAX_TEXTO) < 0){
                close(sc);
                return NULL;
            }

            respuesta = obtener_datos_conexion(destinatario, &datos_destinatario);
            if (respuesta == 0) {
                respuesta = generar_id_mensaje(remitente, &id);
            }
            if (respuesta == 0) {
                respuesta = anadir_mensaje_pendiente_attach(destinatario, id, remitente, texto, archivo);
            }

            respuesta_str = (char)respuesta;
            if (sendMessage(sc, &respuesta_str, 1) < 0) {
                break;
            }

            if (respuesta == 0) {
                snprintf(id_str, sizeof(id_str), "%u", id);
                if (sendMessage(sc, id_str, strlen(id_str) + 1) < 0) {
                    break;
                }

                if (datos_destinatario.conectado) {
                    entregar_pendientes_attach(destinatario);
                } else {
                    printf("MESSAGE %u FROM %s TO %s STORED\n", id, remitente, destinatario);
                    printf("s> ");
                    fflush(stdout);
                }
                reportar_log(remitente, "SENDATTACH", archivo);
            } else if (respuesta == 1) {
                printf("SENDATTACH %s FAIL, USER DOES NOT EXIST\n", destinatario);
                printf("s> ");
                fflush(stdout);
            } else {
                printf("SENDATTACH %s FAIL\n", destinatario);
                printf("s> ");
                fflush(stdout);
            }
            break;
        }

        case ERROR: {
                char respuesta_str = (char)2;
                printf("UNKNOWN OPERATION\n");
                printf("s> ");
                fflush(stdout);
                sendMessage(sc, &respuesta_str, 1);
                break;
            }
    }

    close(sc);
    return NULL;
}

int main(int argc,char *argv[]){
    struct sockaddr_in servidor,cliente;
    long puerto;
    char *endptr;
    int sd,val;
    socklen_t size;
    int *sc;

    struct ThreadArgs *thread_args;
    pthread_t th;

    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Uso: ./server -p <PUERTO>\n");
        return 1;
    }

    puerto = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0' || puerto < 1 || puerto > 65535) {
        fprintf(stderr, "Puerto invalido\n");
        return(1);
    }

    /* Crear el socket (dominio, tipo, protocolo). */
    if ((sd=socket(AF_INET,SOCK_STREAM,0))<0){
        perror("Error creando el socket");
        return (1);
    }

    val=1;
    /*Modificamos las opciones del socket para reutilizar las direcciones y evitar errores futuros*/
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(int)) < 0) {
        perror("Error en setsockopt");
        close(sd);
        return(1);
    }
    /*Vaciamos la estructura antes de rellenarla*/
    memset((char *)&servidor, 0, sizeof(struct sockaddr_in));
    /*Ahora la completamos*/
    servidor.sin_family=AF_INET;
    servidor.sin_port=htons((uint16_t)puerto);
    servidor.sin_addr.s_addr=INADDR_ANY;

    /*Hacemos bind*/
    if(bind(sd,(struct sockaddr *) &servidor,sizeof(struct sockaddr_in))<0){
        perror("Error asignado direcciones al socket (bind)");
        close(sd);
        return(1);
    }
    if ((listen(sd,SOMAXCONN))<0){
        perror("Error en listen");
        close(sd);
        return(1);
    }

    char ip_str[INET_ADDRSTRLEN];

    /* Obtener la IP local asociada al socket */
    socklen_t len = sizeof(servidor);
    if (getsockname(sd, (struct sockaddr *)&servidor, &len) == 0) {
        inet_ntop(AF_INET, &servidor.sin_addr, ip_str, sizeof(ip_str));
    } else {
        strcpy(ip_str, "0.0.0.0"); /* fallback */
    }

    /* Mensaje de inicio */
    printf("s> init server %s::%ld\n", ip_str, puerto);

    /* Mensaje antes de aceptar peticiones */
    printf("s> ");
    fflush(stdout);

    /* Comenzamos la conexion cliente-servidor. */
    while (1){
        /*size sera en este caso el equivalente al parametro necesario (socklen_t *) size*/
        size=sizeof(struct sockaddr_in);
        /*Cada hilo necesita una copia independiente del descriptor del cliente por ello reservamos memoria*/
        sc=(int *) malloc(sizeof(int));
        if (sc==NULL){
            printf("Error reservando memoria\n");
            continue;
        }
        /*Al trabajar con hilos trataremos a sc como un puntero*/
        /*Cada vez que se quiera mandar una peticion entraremos en un bucle donde el servidor debera aceptar cada vez la conexion*/
        if((*sc=accept(sd,(struct sockaddr *) &cliente,&size))<0){
            printf("Error en accept\n");
            free(sc);
            continue;
        }

        thread_args = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs));
        if (thread_args == NULL){
            printf("Error reservando memoria\n");
            close(*sc);
            free(sc);
            continue;
        }
        thread_args->sc = *sc;
        thread_args->cliente_addr = cliente;
        free(sc);
        
        /*Como trabajremos con un servidor concurrente aqui comenzaremos con la creación de hilos*/
        if(pthread_create(&th,NULL,tratar_peticion, thread_args)!=0){
            printf("Error creando hilo\n");
            close(thread_args->sc);
            free(thread_args);
            continue;
        }
        pthread_detach(th);
    }
    close(sd);
    return (0);
}


void reportar_log(char *usuario, char *operacion, char *fichero) {
    CLIENT *clnt;
    enum clnt_stat retval;
    void *result; 
    char *host = getenv("LOG_RPC_IP");
    
    if (host == NULL) return;

    clnt = clnt_create(host, LOGPROG, LOGVERS, "tcp"); 
    if (clnt == NULL) return;

    entrada_log entrada; 

    entrada.usuario = usuario;
    entrada.operacion = operacion;
    entrada.fichero = (fichero != NULL) ? fichero : "";

    retval = enviar_log_1(entrada, &result, clnt);
    
    if (retval != RPC_SUCCESS) {
        clnt_perror(clnt, "Error RPC");
    }

    clnt_destroy(clnt);
}
