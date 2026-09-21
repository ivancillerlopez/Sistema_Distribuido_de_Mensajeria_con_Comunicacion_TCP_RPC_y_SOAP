/*Solo la logica de datos del servidor:
estructura de usuario
estructura de mensaje pendiente
buscar usuario
insertar usuario
borrar usuario
marcar conectado/desconectado
anadir/quitar mensajes pendientes
*/
/*Similar a mensaje.h pero lo usaremos simplemente como memoria del servidor*/
#ifndef ESTADO_H
#define ESTADO_H

#include <stdbool.h>
#include <netinet/in.h>

#define MAX_TEXTO 256
#define MAX_NOMBRE 256
#define MAX_LISTA 1000
/*Separamos enviar_mensaje, conectar, usuarios_conectados, etc. porque en esta practica:
las operaciones del protocolo (SEND, CONNECT, USERS) son mas grandes y mezclan varias cosas
y las funciones internas del servidor deben representar acciones concretas sobre el estado, no todo el flujo completo*/

struct MensajePendiente {
    unsigned int id;                /* identificador del mensaje */
    char remitente[MAX_NOMBRE];     /* usuario que envia */
    char texto[MAX_TEXTO];          /* mensaje */
    struct MensajePendiente *next;  /* siguiente mensaje pendiente */
};

struct MensajePendienteAttach {
    unsigned int id_attach;                /* identificador del mensaje */
    char remitente_attach[MAX_NOMBRE];     /* usuario que envia */
    char texto_attach[MAX_TEXTO];          /* mensaje */
    char file[MAX_TEXTO];           /* archivo adjunto */
    struct MensajePendienteAttach *next_attach;  /* siguiente mensaje pendiente */
};

struct DatosConexion {
    bool conectado;
    struct in_addr ip;
    in_port_t puerto;
};

/*Si lees el enunciado pone el usuario tiene un nombre o esta conectado etc.*/
struct Usuario {
    char nombre[MAX_NOMBRE];                      /* nombre del usuario */
    bool conectado;                               /* true si esta conectado, false si no */
    struct in_addr ip;                            /* IP del cliente cuando esta conectado */
    in_port_t puerto;                             /* puerto de escucha del cliente */
    unsigned int ultimo_identificador;            /* ultimo id generado para mensajes enviados */
    struct MensajePendiente *mensajes_pendientes; /* cabeza de la lista de pendientes */
    struct MensajePendienteAttach *mensajes_pendientes_attach; /* cabeza de la lista de pendientes con adjunto */
    struct Usuario *next;                         /* siguiente usuario de la lista */
};

int registro(const char *nombre);
int eliminar_registro(const char *nombre);
int conectar(const char *nombre, struct in_addr ip, in_port_t puerto);
int desconectar(const char *nombre);
int obtener_usuarios_conectados(const char *solicitante, char usuarios[][MAX_NOMBRE], int max_usuarios, int *num_usuarios);
int obtener_datos_conexion(const char *nombre, struct DatosConexion *datos);
int generar_id_mensaje(const char *remitente, unsigned int *id);
int anadir_mensaje_pendiente(const char *destinatario, unsigned int id,const char *remitente, const char *texto);
int obtener_primer_mensaje_pendiente(const char *nombre,struct MensajePendiente *destino);
int eliminar_primer_mensaje_pendiente(const char *nombre);
void liberar_mensajes_pendientes(struct MensajePendiente *head);
void finalizar_ejecucion(void);
int anadir_mensaje_pendiente_attach(const char *destinatario, unsigned int id, const char *remitente, const char *texto, const char *file);
int obtener_primer_mensaje_pendiente_attach(const char *nombre, struct MensajePendienteAttach *destino);
int eliminar_primer_mensaje_pendiente_attach(const char *nombre);
void liberar_mensajes_pendientes_attach(struct MensajePendienteAttach *head);

#endif
