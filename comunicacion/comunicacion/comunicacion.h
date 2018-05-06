
#ifndef COMUNICACION_COMUNICACION_H_
#define COMUNICACION_COMUNICACION_H_

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


typedef enum id_mensaje { // A medida que se creen mensajes, aca ponemos el ID
	handshake = 1,
	imposibilidad_conexion = 2
}id_mensaje;

typedef struct{
	int comando;
	int tamanio;
}header_t;

typedef struct{
	header_t header;
	void* cuerpo;
} paquete_t;

int conectar_a_server(char* ip, int puerto);
int recibir_mensaje(int socket, void* buffer, int tamanio);
int enviar_mensaje(int socket, void* buffer, int tamanio);
int crear_socket_escucha(int puerto);
struct sockaddr_in crear_direccion_servidor(int puerto);
int cerrar_socket(int socket);
int listen_socket(int socket);
int aceptar_conexion(int socket);
paquete_t recibirPaquete(int socket);
void responder_ok_handshake(int identificacion, int socket_destinatario);
void* serializar(header_t paquete, void* payload);
int conectarConProceso(char* ip, int puerto,int identidad);
#endif /* COMUNICACION_COMUNICACION_H_ */
