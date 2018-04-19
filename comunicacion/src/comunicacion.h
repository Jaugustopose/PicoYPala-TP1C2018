
#ifndef COMUNICACION_H_
#define COMUNICACION_H_

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

int conectar_a_server(char* ip, int puerto);
int recibir_mensaje(int socket, void* buffer, int tamanio);
int enviar_mensaje(int socket, void* buffer, int tamanio);
int crear_socket_server();
int cerrar_socket(int socket);
int bind_socket(int socket, const struct sockaddr_in* mi_addr);
int listen_socket(int socket);
struct sockaddr_in get_direccion_server(int puerto);

typedef enum comm_err {
	ERROR_CONNECT_SERVER = -10,
	ERROR_SOCKET_CONNECT_SERVER = -11,
	ERROR_RECV = -12,
	ERROR_RECV_DISCONNECTED = -13,
	ERROR_SEND = -14
} comm_err ;


#endif /* COMUNICACION_H_ */
