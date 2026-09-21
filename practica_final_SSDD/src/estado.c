/*Solo la logica de datos del servidor:
estructura de usuario
estructura de mensaje pendiente
buscar usuario
insertar usuario
borrar usuario
marcar conectado/desconectado
anadir/quitar mensajes pendientes

estado.c no implementa el comando completo del protocolo.
estado.c implementa solo la manipulacion interna de los datos del servidor.
*/
#include "estado.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static struct Usuario *lista_head = NULL;
/* Un unico mutex protege toda la estructura enlazada del estado. */
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

/* Funcion auxiliar para buscar usuarios */
static struct Usuario *buscar_usuario(const char *nombre) {
    struct Usuario *aux = lista_head;

    while (aux != NULL) {
        if (strcmp(aux->nombre, nombre) == 0) {
            return aux;
        }
        aux = aux->next;
    }

    return NULL;
}

int registro(const char *nombre) {
    struct Usuario *nuevo;

    if (nombre == NULL || strlen(nombre) >= MAX_NOMBRE) {
        return 2;
    }

    pthread_mutex_lock(&m);

    if (buscar_usuario(nombre) != NULL) {
        pthread_mutex_unlock(&m);
        return 1;
    }

    nuevo = malloc(sizeof(*nuevo));
    if (nuevo == NULL) {
        pthread_mutex_unlock(&m);
        return 2;
    }

    strncpy(nuevo->nombre, nombre, MAX_NOMBRE - 1);
    nuevo->nombre[MAX_NOMBRE - 1] = '\0';
    nuevo->conectado = false;
    nuevo->ip.s_addr = 0;
    nuevo->puerto = 0;
    nuevo->ultimo_identificador = 0;
    nuevo->mensajes_pendientes = NULL;
    nuevo->mensajes_pendientes_attach = NULL;

    /* Insertamos al principio porque para registro no importa el orden. */
    nuevo->next = lista_head;
    lista_head = nuevo;

    pthread_mutex_unlock(&m);
    return 0;
}

int eliminar_registro(const char *nombre) {
    struct Usuario *aux;
    struct Usuario *anterior;

    if (nombre == NULL) {
        return 2;
    }

    pthread_mutex_lock(&m);

    aux = lista_head;
    anterior = NULL;
    while (aux != NULL) {
        if (strcmp(aux->nombre, nombre) == 0) {
            if (anterior == NULL) {
                lista_head = aux->next;
            } else {
                anterior->next = aux->next;
            }

            /* Al borrar un usuario se eliminan tambien sus pendientes. */
            liberar_mensajes_pendientes(aux->mensajes_pendientes);
            liberar_mensajes_pendientes_attach(aux->mensajes_pendientes_attach);
            free(aux);
            pthread_mutex_unlock(&m);
            return 0;
        }

        anterior = aux;
        aux = aux->next;
    }

    pthread_mutex_unlock(&m);
    return 1;
}

int conectar(const char *nombre, struct in_addr ip, in_port_t puerto) {
    struct Usuario *aux;

    if (nombre == NULL) {
        return 3;
    }

    pthread_mutex_lock(&m);

    aux = buscar_usuario(nombre);
    if (aux == NULL) {
        pthread_mutex_unlock(&m);
        return 1;
    }

    if (aux->conectado) {
        pthread_mutex_unlock(&m);
        return 2;
    }

    aux->conectado = true;
    aux->ip = ip;
    aux->puerto = puerto;

    pthread_mutex_unlock(&m);
    return 0;
}

int desconectar(const char *nombre) {
    struct Usuario *aux;

    if (nombre == NULL) {
        return 3;
    }

    pthread_mutex_lock(&m);

    aux = buscar_usuario(nombre);
    if (aux == NULL) {
        pthread_mutex_unlock(&m);
        return 1;
    }

    if (!aux->conectado) {
        pthread_mutex_unlock(&m);
        return 2;
    }

    aux->conectado = false;
    aux->ip.s_addr = 0;
    aux->puerto = 0;

    pthread_mutex_unlock(&m);
    return 0;
}

int obtener_usuarios_conectados(const char *solicitante,char usuarios[][MAX_NOMBRE],int max_usuarios,int *num_usuarios) {
    struct Usuario *aux;
    struct Usuario *usuario_solicitante;
    int contador = 0;
    int i = 0;

    if (solicitante == NULL || usuarios == NULL || num_usuarios == NULL || max_usuarios < 0) {
        return 2;
    }

    pthread_mutex_lock(&m);

    usuario_solicitante = buscar_usuario(solicitante);
    if (usuario_solicitante == NULL || !usuario_solicitante->conectado) {
        pthread_mutex_unlock(&m);
        return 1;
    }

    aux = lista_head;
    while (aux != NULL) {
        if (aux->conectado) {
            contador++;
        }
        aux = aux->next;
    }

    if (contador > max_usuarios) {
        pthread_mutex_unlock(&m);
        return 2;
    }

    aux = lista_head;
    while (aux != NULL) {
        if (aux->conectado) {
            strncpy(usuarios[i], aux->nombre, MAX_NOMBRE - 1);
            usuarios[i][MAX_NOMBRE - 1] = '\0';
            i++;
        }
        aux = aux->next;
    }

    *num_usuarios = contador;

    pthread_mutex_unlock(&m);
    return 0;
}

int obtener_datos_conexion(const char *nombre, struct DatosConexion *datos) {
    struct Usuario *usuario;

    if (nombre == NULL || datos == NULL) {
        return 2;
    }

    pthread_mutex_lock(&m);

    usuario = buscar_usuario(nombre);
    if (usuario == NULL) {
        pthread_mutex_unlock(&m);
        return 1;
    }

    datos->conectado = usuario->conectado;
    datos->ip = usuario->ip;
    datos->puerto = usuario->puerto;

    pthread_mutex_unlock(&m);
    return 0;
}

int generar_id_mensaje(const char *remitente, unsigned int *id) {
    struct Usuario *usuario_remitente;

    if (remitente == NULL || id == NULL) {
        return 2;
    }
    if (strlen(remitente) >= MAX_NOMBRE) {
        return 2;
    }

    pthread_mutex_lock(&m);

    usuario_remitente = buscar_usuario(remitente);
    if (usuario_remitente == NULL || !usuario_remitente->conectado) {
        pthread_mutex_unlock(&m);
        return 2;
    }

    usuario_remitente->ultimo_identificador++;
    if (usuario_remitente->ultimo_identificador == 0) {
        usuario_remitente->ultimo_identificador = 1;
    }

    *id = usuario_remitente->ultimo_identificador;

    pthread_mutex_unlock(&m);
    return 0;
}

int generar_id_mensaje_attach(const char *remitente, unsigned int *id) {
    struct Usuario *usuario_remitente;

    if (remitente == NULL || id == NULL) {
        return 2;
    }
    if (strlen(remitente) >= MAX_NOMBRE) {
        return 2;
    }

    pthread_mutex_lock(&m);

    usuario_remitente = buscar_usuario(remitente);
    if (usuario_remitente == NULL || !usuario_remitente->conectado) {
        pthread_mutex_unlock(&m);
        return 2;
    }

    usuario_remitente->ultimo_identificador++;
    if (usuario_remitente->ultimo_identificador == 0) {
        usuario_remitente->ultimo_identificador = 1;
    }

    *id = usuario_remitente->ultimo_identificador;

    pthread_mutex_unlock(&m);
    return 0;
}

int anadir_mensaje_pendiente(const char *destinatario, unsigned int id,
                             const char *remitente, const char *texto) {
    struct Usuario *usuario;
    struct MensajePendiente *nuevo;
    struct MensajePendiente *aux;

    if (destinatario == NULL || remitente == NULL || texto == NULL) {
        return 2;
    }
    if (strlen(destinatario) >= MAX_NOMBRE ||
        strlen(remitente) >= MAX_NOMBRE ||
        strlen(texto) >= MAX_TEXTO) {
        return 2;
    }

    pthread_mutex_lock(&m);

    usuario = buscar_usuario(destinatario);
    if (usuario == NULL) {
        pthread_mutex_unlock(&m);
        return 1;
    }

    nuevo = malloc(sizeof(*nuevo));
    if (nuevo == NULL) {
        pthread_mutex_unlock(&m);
        return 2;
    }

    nuevo->id = id;
    strncpy(nuevo->remitente, remitente, MAX_NOMBRE - 1);
    nuevo->remitente[MAX_NOMBRE - 1] = '\0';
    strncpy(nuevo->texto, texto, MAX_TEXTO - 1);
    nuevo->texto[MAX_TEXTO - 1] = '\0';
    nuevo->next = NULL;

    /* Encolamos al final para conservar orden FIFO de entrega. */
    if (usuario->mensajes_pendientes == NULL) {
        usuario->mensajes_pendientes = nuevo;
    } else {
        aux = usuario->mensajes_pendientes;
        while (aux->next != NULL) {
            aux = aux->next;
        }
        aux->next = nuevo;
    }

    pthread_mutex_unlock(&m);
    return 0;
}

int anadir_mensaje_pendiente_attach(const char *destinatario, unsigned int id,
                                    const char *remitente, const char *texto, const char *file) {
    struct Usuario *usuario;
    struct MensajePendienteAttach *nuevo;
    struct MensajePendienteAttach *aux;

    if (destinatario == NULL || remitente == NULL || texto == NULL || file == NULL) {
        return 2;
    }
    if (strlen(destinatario) >= MAX_NOMBRE ||
        strlen(remitente) >= MAX_NOMBRE ||
        strlen(texto) >= MAX_TEXTO ||
        strlen(file) >= MAX_TEXTO) {
        return 2;
    }

    pthread_mutex_lock(&m);

    usuario = buscar_usuario(destinatario);
    if (usuario == NULL) {
        pthread_mutex_unlock(&m);
        return 1;
    }

    nuevo = malloc(sizeof(*nuevo));
    if (nuevo == NULL) {
        pthread_mutex_unlock(&m);
        return 2;
    }

    nuevo->id_attach = id;
    strncpy(nuevo->remitente_attach, remitente, MAX_NOMBRE - 1);
    nuevo->remitente_attach[MAX_NOMBRE - 1] = '\0';
    strncpy(nuevo->texto_attach, texto, MAX_TEXTO - 1);
    nuevo->texto_attach[MAX_TEXTO - 1] = '\0';
    strncpy(nuevo->file, file, MAX_TEXTO - 1);
    nuevo->file[MAX_TEXTO - 1] = '\0';
    nuevo->next_attach = NULL;

    /* Encolamos al final para conservar orden FIFO de entrega. */
    if (usuario->mensajes_pendientes_attach == NULL) {
        usuario->mensajes_pendientes_attach = nuevo;
    } else {
        aux = usuario->mensajes_pendientes_attach;
        while (aux->next_attach != NULL) {
            aux = aux->next_attach;
        }
        aux->next_attach = nuevo;
    }

    pthread_mutex_unlock(&m);
    return 0;
}

int obtener_primer_mensaje_pendiente(const char *nombre,
                                     struct MensajePendiente *destino) {
    struct Usuario *usuario;
    struct MensajePendiente *primero;

    if (nombre == NULL || destino == NULL) {
        return 2;
    }

    pthread_mutex_lock(&m);

    usuario = buscar_usuario(nombre);
    if (usuario == NULL) {
        pthread_mutex_unlock(&m);
        return 1;
    }

    primero = usuario->mensajes_pendientes;
    if (primero == NULL) {
        pthread_mutex_unlock(&m);
        return 2;
    }

    /* Copiamos el contenido para no exponer el puntero interno fuera del mutex. */
    destino->id = primero->id;
    strncpy(destino->remitente, primero->remitente, MAX_NOMBRE - 1);
    destino->remitente[MAX_NOMBRE - 1] = '\0';
    strncpy(destino->texto, primero->texto, MAX_TEXTO - 1);
    destino->texto[MAX_TEXTO - 1] = '\0';
    destino->next = NULL;

    pthread_mutex_unlock(&m);
    return 0;
}

int obtener_primer_mensaje_pendiente_attach(const char *nombre,
                                     struct MensajePendienteAttach *destino) {
    struct Usuario *usuario;
    struct MensajePendienteAttach *primero;

    if (nombre == NULL || destino == NULL) {
        return 2;
    }

    pthread_mutex_lock(&m);

    usuario = buscar_usuario(nombre);
    if (usuario == NULL) {
        pthread_mutex_unlock(&m);
        return 1;
    }

    primero = usuario->mensajes_pendientes_attach;
    if (primero == NULL) {
        pthread_mutex_unlock(&m);
        return 2;
    }

    /* Copiamos el contenido para no exponer el puntero interno fuera del mutex. */
    destino->id_attach = primero->id_attach;
    strncpy(destino->remitente_attach, primero->remitente_attach, MAX_NOMBRE - 1);
    destino->remitente_attach[MAX_NOMBRE - 1] = '\0';
    strncpy(destino->texto_attach, primero->texto_attach, MAX_TEXTO - 1);
    destino->texto_attach[MAX_TEXTO - 1] = '\0';
    strncpy(destino->file, primero->file, MAX_TEXTO - 1);
    destino->file[MAX_TEXTO - 1] = '\0';
    destino->next_attach = NULL;

    pthread_mutex_unlock(&m);
    return 0;
}

int eliminar_primer_mensaje_pendiente(const char *nombre) {
    struct Usuario *usuario;
    struct MensajePendiente *primero;

    if (nombre == NULL) {
        return 2;
    }

    pthread_mutex_lock(&m);

    usuario = buscar_usuario(nombre);
    if (usuario == NULL) {
        pthread_mutex_unlock(&m);
        return 1;
    }

    primero = usuario->mensajes_pendientes;
    if (primero == NULL) {
        pthread_mutex_unlock(&m);
        return 2;
    }

    /* Desencolamos la cabeza tras una entrega correcta. */
    usuario->mensajes_pendientes = primero->next;
    free(primero);

    pthread_mutex_unlock(&m);
    return 0;
}

int eliminar_primer_mensaje_pendiente_attach(const char *nombre) {
    struct Usuario *usuario;
    struct MensajePendienteAttach *primero;

    if (nombre == NULL) {
        return 2;
    }

    pthread_mutex_lock(&m);

    usuario = buscar_usuario(nombre);
    if (usuario == NULL) {
        pthread_mutex_unlock(&m);
        return 1;
    }

    primero = usuario->mensajes_pendientes_attach;
    if (primero == NULL) {
        pthread_mutex_unlock(&m);
        return 2;
    }

    /* Desencolamos la cabeza tras una entrega correcta. */
    usuario->mensajes_pendientes_attach = primero->next_attach;
    free(primero);

    pthread_mutex_unlock(&m);
    return 0;
}

void liberar_mensajes_pendientes(struct MensajePendiente *head) {
    struct MensajePendiente *actual = head;

    while (actual != NULL) {
        /* Guardamos el siguiente antes de liberar el nodo actual. */
        struct MensajePendiente *siguiente = actual->next;
        free(actual);
        actual = siguiente;
    }
}

void liberar_mensajes_pendientes_attach(struct MensajePendienteAttach *head) {
    struct MensajePendienteAttach *actual = head;

    while (actual != NULL) {
        /* Guardamos el siguiente antes de liberar el nodo actual. */
        struct MensajePendienteAttach *siguiente = actual->next_attach;
        free(actual);
        actual = siguiente;
    }
}

void finalizar_ejecucion(void) {
    struct Usuario *actual;

    pthread_mutex_lock(&m);

    actual = lista_head;
    lista_head = NULL;

    /* Liberamos toda la memoria del estado del servidor al cerrar. */
    while (actual != NULL) {
        struct Usuario *siguiente = actual->next;
        liberar_mensajes_pendientes(actual->mensajes_pendientes);
        liberar_mensajes_pendientes_attach(actual->mensajes_pendientes_attach);
        free(actual);
        actual = siguiente;
    }

    pthread_mutex_unlock(&m);
}
