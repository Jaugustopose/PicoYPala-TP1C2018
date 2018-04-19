#include "comunicacion.h"

int conectar_a_server(char* ip, int puerto) {

	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    // Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;  // Indica que usaremos el protocolo TCP

	getaddrinfo(ip, puerto, &hints, &server_info);  // Carga en server_info los datos de la conexion

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
		printf("Error al recibir mensaje! El cliente conectado en socket %d ha cerrado la conexión", socket); //TODO Aplicar logger
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
int crear_socket_escucha() {

	struct sockaddr_in direccionServidor; // Información sobre mi dirección
	struct sockaddr_in direccionCliente; // Información sobre la dirección del cliente
	socklen_t addrlen; // El tamaño de la direccion del cliente
	int sockServ; // Socket de nueva conexion aceptada
	int sockClie; // Socket a la escucha

	//Crear socket. Dejar reutilizable. Crear direccion del servidor. Bind. Listen.
	sockServ = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	reusarSocket(sockServ, 1);
	direccionServidor = crearDireccionServidor(config.puerto);
	bind_w(sockServ, &direccionServidor);
	listen_w(sockServ);

}

int bind_socket(int socket, const struct sockaddr_in* mi_addr) {

}
