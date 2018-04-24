
#ifndef COMUNICACION_COMUNICACION_H_
#define COMUNICACION_COMUNICACION_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
//#include <commons/string.h>

int conectar_a_server(char* ip, int puerto);
int recibir_mensaje(int socket, void* buffer, int tamanio);
int enviar_mensaje(int socket, void* buffer, int tamanio);
int crear_socket_escucha(int puerto);
struct sockaddr_in crear_direccion_servidor(int puerto);
int cerrar_socket(int socket);
int listen_socket(int socket);
int aceptar_conexion(int socket);

typedef enum comm_err {
	ERROR_CONNECT_SERVER = -10,
	ERROR_SOCKET_CONNECT_SERVER = -11,
	ERROR_RECV = -12,
	ERROR_RECV_DISCONNECTED = -13,
	ERROR_SEND = -14,
	ERROR_BIND = -15,
	ERROR_LISTEN = -16,
	ERROR_ACCEPT = -17
} comm_err ;

typedef enum id_proceso {
	ESI = 1,
	Planificador = 2,
	Coordinador = 3,
	Instancia = 4
} id_proceso;


#endif /* COMUNICACION_COMUNICACION_H_ */
