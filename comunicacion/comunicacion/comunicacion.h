
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
	msj_handshake = 1,
	msj_imposibilidad_conexion = 2,
	msj_nombre_instancia = 3,
	msj_instruccion_esi = 4,
	msj_solicitud_get_clave = 5,
	msj_clave_permitida_para_operar = 6,
	msj_crear_clave_get = 7,
	msj_store_clave = 8,
	msj_requerimiento_ejecucion = 9,
	msj_sentencia_finalizada = 10,
	msj_esi_finalizado = 11,
	msj_esi_tiene_tomada_clave = 12,
	msj_ok_solicitud_operacion = 13,
	msj_fail_solicitud_operacion = 14,
	msj_abortar_esi = 15,
	msj_sentencia_get = 16,
	msj_sentencia_set = 17,
	msj_sentencia_store = 18,
	msj_error_tamanio_clave = 19,
	msj_error_clave_no_identificada = 20,
	msj_error_comunicacion = 21,
	msj_error_clave_inaccesible = 22,
	msj_error_clave_no_bloqueada = 23,
	msj_instancia_compactar = 24,
	msj_instancia_compactacion_finalizada = 25,
	msj_instancia_sustituyo_clave = 26,
	msj_compactacion_finalizada_continuar_planificacion = 27
}id_mensaje;

typedef struct{
	int comando;
	int tamanio;
}header_t;

typedef struct{
	header_t header;
	void* cuerpo;
} paquete_t;

typedef enum t_operacion{
	get,
	set,
	store
} operacion_t;

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
