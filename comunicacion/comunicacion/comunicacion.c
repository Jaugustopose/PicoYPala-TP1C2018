#include "comunicacion.h"
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
#include "commons/string.h"

int conectar_a_server(char* ip, int puerto) {

	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    // Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;  // Indica que usaremos el protocolo TCP

	char* puerto_s = string_itoa(puerto);

	getaddrinfo(ip, puerto_s, &hints, &server_info);  // Carga en server_info los datos de la conexion

	int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if (server_socket == -1) {
		printf("Error al abrir socket contra el server! Detalle: %d - %s\n", errno, strerror(errno));
		return ERROR_SOCKET_CONNECT_SERVER;
	}

	int retorno = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);  // No lo necesitamos mas

	if (retorno != 0) {
	  printf("Error al conectar a server! Detalle: %d - %s\n", errno, strerror(errno)); //TODO Aplicar logger
	  return ERROR_CONNECT_SERVER;
	}

	return server_socket;

}

int recibir_mensaje(int socket, void* buffer, int tamanio) {
	/** Del Man del Recv:
	 * RETURN VALUE
       These  calls  return  the  number  of bytes received, or -1 if an error
       occurred.  In the event of an error,  errno  is  set  to  indicate  the
       error.   The  return  value  will  be  0 when the peer has performed an
       orderly shutdown.
	 */

	int retorno = recv(socket, buffer, tamanio, MSG_WAITALL);

	if (retorno == 0) {
		printf("Error al recibir mensaje! El cliente conectado en socket %d ha cerrado la conexión\n", socket); //TODO Aplicar logger
		close(socket);
		return ERROR_RECV_DISCONNECTED;
	} else if (retorno < 0) {
		printf("Error al recibir! Detalle: %d - %s\n", errno, strerror(errno)); //TODO Aplicar logger
		close(socket);
		return ERROR_RECV;
	}

	return retorno;
}

int enviar_mensaje(int socket, void* buffer, int tamanio) {

	int retorno = send(socket, buffer, tamanio, 0);

	if (retorno < 0) {
		printf("Error al enviar mensaje! Detalle: %d - %s\n", errno, strerror(errno)); //TODO Aplicar logger
		close(socket);
		return ERROR_SEND;
	}

	return retorno;

}

int crear_socket_escucha(int puerto) {

	int socket_escucha = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_escucha == -1) {
		puts("Error al crear socket\n");
		exit(EXIT_FAILURE);
	} else {

		struct sockaddr_in direccionServidor = crear_direccion_servidor(puerto);

		int retorno = bind(socket_escucha, (struct sockaddr*)&direccionServidor, sizeof(struct sockaddr));
		if (retorno < 0) {
			printf("Error al bindear socket! Detalle: %d - %s\n", errno, strerror(errno)); //TODO Aplicar logger
			close(socket_escucha);
			return ERROR_BIND;
		}

		retorno = listen(socket_escucha, 10);
		if (socket_escucha < 0) {
			printf("Error al intentar hacer listen sobre el socket! Detalle: %d - %s\n", errno, strerror(errno)); //TODO Aplicar logger
			close(socket_escucha);
			return ERROR_BIND;
		}

		return socket_escucha;
	}
}

struct sockaddr_in crear_direccion_servidor(int puerto) {

	struct sockaddr_in direccionServer;
	direccionServer.sin_family = AF_UNSPEC;
	direccionServer.sin_port = htons(puerto); // short, Ordenación de bytes de la red
	direccionServer.sin_addr.s_addr = INADDR_ANY;
	memset(&(direccionServer.sin_zero), '\0', 8); // Poner ceros para rellenar el resto de la estructura

	return direccionServer;

}

int aceptar_conexion(int socket_server) {

	struct sockaddr_in direccion_cliente;
	socklen_t addrlen = sizeof(direccion_cliente);
	int socket_cliente = accept(socket_server, (struct sockaddr*) &direccion_cliente, &addrlen);

	if (socket_cliente < 0) {
		printf("Error al aceptar conexión sobre el socket %d! Detalle: %d - %s\n", socket_server, errno, strerror(errno)); //TODO Aplicar logger
		return ERROR_ACCEPT;
	} else {
		printf("Server: nueva conexion de %s en socket %d\n", inet_ntoa(direccion_cliente.sin_addr), socket_server); //TODO Aplicar logger
		return socket_cliente;
	}
}
int conectarConProceso(char* ip, int puerto,int proceso){
	int socketProceso = conectar_a_server(ip, puerto);
	//Preparo mensaje handshake
	header_t header;
	header.comando = handshake;
	header.tamanio = sizeof(int);
	int cuerpo = proceso;
	int tamanio = sizeof(header_t) + sizeof(header.tamanio);
	void* buff = malloc(tamanio);
	memcpy(buff, &header, sizeof(header_t));
	memcpy(buff + sizeof(header_t), &cuerpo, sizeof(int));

	//Envio mensaje handshake
	enviar_mensaje(socketProceso,buff,tamanio);
	//Libero Buffer
	free(buff);
	//Recibo Respuesta del Handshake
	paquete_t paquete = recibirPaquete(socketProceso);
	if(paquete.header.comando == handshake && *(int*)paquete.cuerpo == proceso)
		printf("Conectado correctamente al Proceso %d\n",proceso);

	return socketProceso;

}
paquete_t recibirPaquete(int socket) {
	//TODO Manejar error que puede devolver recibir_mensaje
	paquete_t paquete;
	recibir_mensaje(socket, &paquete.header, sizeof(header_t));
	paquete.cuerpo = malloc(paquete.header.tamanio);
	recibir_mensaje(socket,paquete.cuerpo,paquete.header.tamanio);
	return paquete;
}

void responder_ok_handshake(int identificacion, int socket_destinatario) {
	//Preparación para responder OK Handshake al proceso conectado recientemente.
	int id = identificacion;
	header_t header;
	header.comando = handshake;
	header.tamanio = sizeof(id);

	//Serialización
	void* bufferOkHandshake = serializar(header, &id);
	//Enviamos OK al Planificador
	enviar_mensaje(socket_destinatario, bufferOkHandshake,sizeof(header) + header.tamanio);
}

void* serializar(header_t header, void* payload) {

	void* buffer = malloc(sizeof(header) + header.tamanio);
	memcpy(buffer, &header, sizeof(header)); //Primero el header (con comando/operación y tamanio payload)
	memcpy(buffer + sizeof(header), payload, header.tamanio); // Luego el payload

	return buffer;
}
