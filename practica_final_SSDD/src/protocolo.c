/*Solo utilidades de comunicación:

enviar cadena terminada en '\0'
recibir cadena
enviar byte de estado
 recibir byte
helpers para no repetir código de sockets
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "lines.h"
#include "protocolo.h"

codigo_operacion string_a_int_codigo_operacion(char *operacion){
    if (strcmp(operacion, "REGISTER") == 0){
        return REGISTER;
    } else if (strcmp(operacion, "UNREGISTER") == 0){
        return UNREGISTER;
    } else if (strcmp(operacion, "CONNECT") == 0){
        return CONNECT;
    } else if (strcmp(operacion, "DISCONNECT") == 0){
        return DISCONNECT;
    } else if (strcmp(operacion, "SEND") == 0){
        return SEND;
    } else if (strcmp(operacion, "USERS") == 0){
        return USERS;
    } else if (strcmp(operacion, "SENDATTACH") == 0){
        return SENDATTACH;
    } else {
        return ERROR;
    }
}

/*Establece la conexion nueva entre el servidor -> cliente destinatarios,es decir al que se le va a enviar el mensaje*/
int conectar_a_cliente(struct in_addr ip, in_port_t puerto) {
    int sd_cliente;
    struct sockaddr_in direccion_cliente;

    sd_cliente = socket(AF_INET, SOCK_STREAM, 0);
    if (sd_cliente < 0) {
        return -1;
    }

    memset((char *)&direccion_cliente, 0, sizeof(struct sockaddr_in));
    direccion_cliente.sin_family = AF_INET;
    direccion_cliente.sin_addr = ip;
    direccion_cliente.sin_port = htons(puerto);

    if (connect(sd_cliente, (struct sockaddr *)&direccion_cliente, sizeof(struct sockaddr_in)) < 0) {
        close(sd_cliente);
        return -1;
    }

    return sd_cliente;
}

/* Construye y envia el formato que espera el hilo receptor del cliente:
operacion, remitente, id y texto; todo terminado en '\0'. */
int enviar_send_message(int socket, const char *remitente, unsigned int id, const char *texto) {
    char operacion[] = "SEND_MESSAGE";
    char id_str[32];

    if (remitente == NULL || texto == NULL) {
        return -1;
    }

    /* El cliente lee el id como cadena, por eso aqui lo convertimos a texto. */
    snprintf(id_str, sizeof(id_str), "%u", id);

    if (sendMessage(socket, operacion, strlen(operacion) + 1) < 0) {
        return -1;
    }
    if (sendMessage(socket, (char *)remitente, strlen(remitente) + 1) < 0) {
        return -1;
    }
    if (sendMessage(socket, id_str, strlen(id_str) + 1) < 0) {
        return -1;
    }
    if (sendMessage(socket, (char *)texto, strlen(texto) + 1) < 0) {
        return -1;
    }

    return 0;
}

int enviar_send_message_attach(int socket, const char *remitente, unsigned int id, const char *texto, const char *archivo) {
    char operacion[] = "SEND_MESSAGE_ATTACH";
    char id_str[32];

    if (remitente == NULL || texto == NULL || archivo == NULL) {
        return -1;
    }

    /* El cliente lee el id como cadena, por eso aqui lo convertimos a texto. */
    snprintf(id_str, sizeof(id_str), "%u", id);

    if (sendMessage(socket, operacion, strlen(operacion) + 1) < 0) {
        return -1;
    }
    if (sendMessage(socket, (char *)remitente, strlen(remitente) + 1) < 0) {
        return -1;
    }
    if (sendMessage(socket, id_str, strlen(id_str) + 1) < 0) {
        return -1;
    }
    if (sendMessage(socket, (char *)texto, strlen(texto) + 1) < 0) {
        return -1;
    }
    if (sendMessage(socket, (char *)archivo, strlen(archivo) + 1) < 0) {
        return -1;
    }

    return 0;
}

/* Envia la confirmacion de entrega al emisor: operacion + id del mensaje. */
int enviar_send_message_ack(int socket, unsigned int id) {
    char operacion[] = "SEND_MESS_ACK";
    char id_str[32];

    /* Igual que en SEND_MESSAGE, el identificador viaja como texto. */
    snprintf(id_str, sizeof(id_str), "%u", id);

    if (sendMessage(socket, operacion, strlen(operacion) + 1) < 0) {
        return -1;
    }
    if (sendMessage(socket, id_str, strlen(id_str) + 1) < 0) {
        return -1;
    }

    return 0;
}

int enviar_send_message_ack_attach(int socket, unsigned int id, const char *archivo) {
    char operacion[] = "SEND_MESS_ATTACH_ACK";
    char id_str[32];

    /* Igual que en SEND_MESSAGE, el identificador viaja como texto. */
    snprintf(id_str, sizeof(id_str), "%u", id);

    if (sendMessage(socket, operacion, strlen(operacion) + 1) < 0) {
        return -1;
    }
    if (sendMessage(socket, id_str, strlen(id_str) + 1) < 0) {
        return -1;
    }
    if (sendMessage(socket, (char *)archivo, strlen(archivo) + 1) < 0) {
        return -1;
    }

    return 0;
}
