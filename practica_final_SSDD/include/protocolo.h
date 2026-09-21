/*Solo utilidades de comunicación:

enviar cadena terminada en '\0'
recibir cadena
enviar byte de estado
recibir byte
helpers para no repetir código de sockets
*/
#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <netinet/in.h>

typedef enum {
    REGISTER = 0,
    UNREGISTER = 1,
    CONNECT = 2,
    DISCONNECT = 3,
    SEND = 4,
    USERS = 5,
    SENDATTACH = 6,
    ERROR = -1
} codigo_operacion;

codigo_operacion string_a_int_codigo_operacion(char *operacion);
/* Abre una conexion TCP con el puerto de escucha de un cliente. */
int conectar_a_cliente(struct in_addr ip, in_port_t puerto);
/* Envia al cliente destinatario el mensaje completo con su remitente e identificador. */
int enviar_send_message(int socket, const char *remitente, unsigned int id, const char *texto);
/* Envia al emisor la confirmacion de que un mensaje concreto fue entregado. */
int enviar_send_message_ack(int socket, unsigned int id);

int enviar_send_message_attach(int socket, const char *remitente, unsigned int id, const char *texto, const char *archivo);
int enviar_send_message_ack_attach(int socket, unsigned int id, const char *archivo);
#endif
