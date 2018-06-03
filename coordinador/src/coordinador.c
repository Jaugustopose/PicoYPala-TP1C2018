/*
 * coordinador.c
 *
 *  Created on: 26 abr. 2018
 *      Author: Maximo Cozetti
 */

//INCLUDES
#include <stdio.h>
#include <stdlib.h>
#include <comunicacion/comunicacion.h>
#include <commons/config.h>
#include <commons/string.h>
#include <unistd.h>

//STRUCTS
typedef struct config_t {
	int PUERTO;
	int PUERTO_PLANIFICADOR;
	char* IP_PLANIFICADOR;
	char* ALGORITMO_DISTRIBUCION;
	int CANT_ENTRADAS;
	int ENTRADA_SIZE;
	int RETARDO;
} config_t;

//VARIABLES
int socket_coordinador = 0;
int socket_planificador = 0;
fd_set master; // Bolsa donde voy a registrar todas las conexiones realizadas al coordinador.
fd_set read_fds; //Bolsa que utilizo para leer informacion de los fd_set actuales.
fd_set bolsa_esis; //Bolsa donde voy a ingresar cada esi que se conecte.
fd_set bolsa_instancias; //Bolsa donde voy a ingresar cada instancia que se conecte.
fd_set bolsa_planificador;
int fdCliente = 0; //Para recorrer conjunto de fd.
int maxfd = 0; //Numero del ultimo fd creado.
t_config* config_coordinador; //Para cuando tenga que traer cosas del .cfg
config_t config; //Para cuando tenga que traer cosas del .cfg
int planificador_conectado = 0;



//FUNCIONES

void establecer_configuracion(int puerto_escucha, int puerto_servidor, char* algoritmo, int entradas, int tamanio_entrada, int retard) {
		char* pat = string_new();
		char cwd[1024]; // Variable donde voy a guardar el path absoluto hasta el /Debug
		string_append(&pat, getcwd(cwd, sizeof(cwd)));
		if (string_contains(pat, "/Debug")){
			string_append(&pat,"/coordinador.cfg");
		}else{
		string_append(&pat, "/Debug/coordinador.cfg");
		}
		t_config* config_coordinador = config_create(pat);
		free(pat);

		if (config_has_property(config_coordinador, "PUERTO")) {
			config.PUERTO = config_get_int_value(config_coordinador, "PUERTO");
			printf("PUERTO_COORDINADOR: %d\n", config.PUERTO);
		}
//		if (config_has_property(config_coordinador, "PUERTO_PLANIFICADOR")) {
//			config.PUERTO_PLANIFICADOR = config_get_int_value(config_coordinador, "PUERTO_PLANIFICADOR");
//			printf("PUERTO_PLANIFICADOR: %d\n", config.PUERTO_PLANIFICADOR);
//		} Por si el planificador llega a ser servidor
//		if (config_has_property(config_coordinador, "IP_PLANIFICADOR")) {
//			config.IP_PLANIFICADOR = config_get_string_value(config_coordinador, "IP_PLANIFICADOR");
//			printf("IP_PLANIFICADOR: %s\n", config.IP_PLANIFICADOR);
//		} Por si el planificador llega a ser servidor
		if (config_has_property(config_coordinador, "ALGORITMO_DISTRIBUCION")) {
			config.ALGORITMO_DISTRIBUCION = config_get_string_value(config_coordinador, "ALGORITMO_DISTRIBUCION");
			printf("ALGORITMO_DISTRIBUCION: %s\n", config.ALGORITMO_DISTRIBUCION);
		}
		if (config_has_property(config_coordinador, "CANT_ENTRADAS")) {
			config.CANT_ENTRADAS = config_get_int_value(config_coordinador, "CANT_ENTRADAS");
			printf("CANT_ENTRADAS: %d\n", config.CANT_ENTRADAS);
		}
		if (config_has_property(config_coordinador, "ENTRADA_SIZE")) {
			config.ENTRADA_SIZE = config_get_int_value(config_coordinador, "ENTRADA_SIZE");
			printf("ENTRADA_SIZE: %d\n", config.ENTRADA_SIZE);
		}
		if (config_has_property(config_coordinador, "RETARDO")) {
			config.RETARDO = config_get_int_value(config_coordinador, "RETARDO");
			printf("RETARDO: %d\n", config.RETARDO);
		}
}

void responder_no_OK_handshake(int socket_cliente) {
	//Preparación para responder IMPOSIBILIDAD DE CONEXION al segundo planificador.
	header_t header;
	header.comando = imposibilidad_conexion;
	header.tamanio = 0; // Solo envia header. Payload va a estar vacio.
	//Enviamos OK al Planificador. No hace falta serializar dado que payload esta vacio.
	enviar_mensaje(socket_cliente, &header, sizeof(header) + header.tamanio); // header.tamanio se podria borrar pero lo dejo para mayor entendimiento.
	printf("Se ha impedido la conexion de un segundo planificador al sistema\n");
}

void identificar_proceso_e_ingresar_en_bolsa(int socket_cliente) {

	//Recibo identidad y coloco en la bolsa correspondiente.
	header_t cabecera;
	int identificacion;
	int resultado = recibir_mensaje(socket_cliente, &cabecera, sizeof(header_t));
	if(resultado == ERROR_RECV){
		printf("Error en el recv para socket %d!!!\n", socket_cliente); //TODO Manejar el error de cierta forma si queremos.
	}

	switch (cabecera.comando) {
	case msj_handshake:
		resultado = recibir_mensaje(socket_cliente, &identificacion, cabecera.tamanio);
		if(resultado == ERROR_RECV){
			printf("Error en el recv para socket %d al hacer handshake!!!\n", socket_cliente); //TODO Manejar el error de cierta forma si queremos.
		} else {
			switch (identificacion){
			case ESI:
				FD_SET(socket_cliente, &master);
				FD_SET(socket_cliente, &bolsa_esis); //Agrego un nuevo esi a la bolsa de esis.
				printf("Se ha conectado un nuevo esi \n");
				responder_ok_handshake(ESI, socket_cliente);
				break;

			case Instancia:
				FD_SET(socket_cliente, &master);
				FD_SET(socket_cliente, &bolsa_instancias); //Agrego una nueva instancia a la bolsa de instancias.
				printf("Se ha conectado una nueva instancia de ReDis \n");
				responder_ok_handshake(Instancia, socket_cliente);
				break;

			case Planificador:
				if (planificador_conectado == 0){
					FD_SET(socket_cliente, &master);
					FD_SET(socket_cliente, &bolsa_planificador);
					printf("Se ha conectado el planificador al sistema\n");
					planificador_conectado = 1; //Para que no se conecte mas de un planificador.
					responder_ok_handshake(Planificador, socket_cliente);
					break;

				} else {
					//Responder IMPOSIBILIDAD DE CONEXION al segundo planificador.
					responder_no_OK_handshake(socket_cliente);
					close(fdCliente);
				}
			}

		}

		if (socket_cliente > maxfd) {
			maxfd = socket_cliente;
		}

		break;

	default:
		printf("Comando/operacion %d no implementada!!!\n", cabecera.comando);
	}
}

void conexion_de_cliente_finalizada() {
	// conexión cerrada.
	printf("Server: socket %d termino la conexion\n", fdCliente);
	// Eliminar del conjunto maestro y su respectiva bolsa.
	FD_CLR(fdCliente, &master);
	if (FD_ISSET(fdCliente, &bolsa_esis)) {
		FD_CLR(fdCliente, &bolsa_esis);
		printf("Se desconecto esi del socket %d\n", fdCliente); //TODO: Probablemente haya que hacer algo mas, hay que ver el enunciado.

	} else if (FD_ISSET(fdCliente, &bolsa_instancias)){
		FD_CLR(fdCliente, &bolsa_instancias);
		printf("Se desconecto instancia del socket %d\n", fdCliente); //TODO: Reorganizar la distribucion para las claves.
	} else {
		FD_CLR(fdCliente, &bolsa_planificador);
		printf("Se desconecto el planificador\n");
	}

	close(fdCliente); // Si se perdio la conexion, la cierro.
}

void atender_accion_esi(int fdEsi) {
	printf("Atendiendo acción esi en socket %d!!!\n", fdEsi);
}

void atender_accion_instancia(int fdInstancia) {
	printf("Atendiendo acción instancia en socket %d!!!\n", fdInstancia);
}

int main(void) {

	//Extraer informacion del archivo de configuracion.
	establecer_configuracion(config.PUERTO,config.PUERTO_PLANIFICADOR,config.ALGORITMO_DISTRIBUCION,config.CANT_ENTRADAS,
							 config.ENTRADA_SIZE,config.RETARDO);

	//Establecer conexiones con el sistema.
	//En caso que el plani sea server de coordinador socket_planificador = conectar_a_server(config.IP_PLANIFICADOR, config.PUERTO_PLANIFICADOR);
	socket_coordinador = crear_socket_escucha(config.PUERTO);

	//Acciones necesarias para identificar los esi o instancias entrantes.

	FD_SET(socket_coordinador, &master);
	maxfd = socket_coordinador;

	//Bucle principal
	for (;;) {
		read_fds = master;
		if (select(maxfd + 1, &read_fds, NULL, NULL, NULL) == -1) { //Compruebo si algun cliente quiere interactuar.
			perror("Se ha producido un error en el Select a la espera de actividad en sockets");
			exit(EXIT_FAILURE);
		}

		// Recorremos los file descriptors asignados a los sockets conectados para ver cuál presenta actividad
		for (fdCliente = 0; fdCliente <= maxfd; fdCliente++) {

			if (FD_ISSET(fdCliente, &read_fds)) { // Me fijo si tengo datos listos para leer
				if (fdCliente == socket_coordinador) { //Si entro en este "if", significa que tengo datos en socket escucha (Nueva conexión).

					//Gestionar nueva conexión
					int socket_cliente = aceptar_conexion(socket_coordinador);
					if (socket_cliente == ERROR_ACCEPT) {
						printf("Error en el accept\n"); //TODO Deberiamos tomar el error y armar un exit_gracefully como en el tp0.
					} else {
						identificar_proceso_e_ingresar_en_bolsa(socket_cliente);
					}

				} else {

					// Gestionar datos de un cliente.
					int id_operacion_mensaje;
					int cantBytes = recibir_mensaje(fdCliente,&id_operacion_mensaje,sizeof(int));
					//Handlear errores en el recibir
					if (cantBytes == ERROR_RECV_DISCONNECTED) {
						conexion_de_cliente_finalizada();
						break; //Salimos del for
					} else if (cantBytes == ERROR_RECV) {
						printf("Error inesperado al recibir datos del cliente!\n");
						//TODO Si el interlcutor esperaba respuesta hay que responderle con algún código de error
						// 		Por ejemplo: handlearError(cantBytes); el cual haría un send
						break; //Salimos del for.
					}
					//Se recibió OK. Atender de acuerdo a proceso.
					if (FD_ISSET(fdCliente, &bolsa_esis)) { // EN CASO DE QUE EL MENSAJE LO HAYA ENVIADO UN ESI.
						atender_accion_esi(fdCliente); //TODO: VER QUE PUEDE QUERER.
					} else if (FD_ISSET(fdCliente, &bolsa_instancias)) { // EN CASO DE QUE EL MENSAJE LO HAYA ENVIADO UNA INSTANCIA.
						atender_accion_instancia(fdCliente); //TODO:VER QUE PUEDE QUERER.
					} else {
						perror("El socket buscado no está en ningún set!!! Situación anómala. Finalizando...");
						exit(EXIT_FAILURE);
					}
				}
			}
		}
	}
}
