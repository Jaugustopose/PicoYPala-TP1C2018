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

	int PUERTO_COORDINADOR;
	int PUERTO_PLANIFICADOR;
	char* IP_PLANIFICADOR;
	char* ALGORITMO_DISTRIBUCION;
	int CANT_ENTRADAS;
	int ENTRADA_SIZE;
	int RETARDO;
}config_t;

//VARIABLES
int socket_coordinador = 0;
int socket_planificador = 0;
int socket_cliente = 0;
fd_set master; // Bolsa donde voy a registrar todas las conexiones realizadas al coordinador.
fd_set read_fds; //Bolsa que utilizo para leer informacion de los fd_set actuales.
fd_set bolsa_esis; //Bolsa donde voy a ingresar cada esi que se conecte.
fd_set bolsa_instancias; //Bolsa donde voy a ingresar cada instancia que se conecte.
fd_set bolsa_planificador;
int fdCliente = 0; //Para recorrer conjunto de fd.
int maxfd = 0; //Numero del ultimo fd creado.
int id_operacion_mensaje = 0;
int cantBytes = 0;
t_config* config_coordinador; //Para cuando tenga que traer cosas del .cfg
config_t config; //Para cuando tenga que traer cosas del .cfg
int planificador_conectado = 0;



//FUNCIONES

void establecer_configuracion (int puerto_escucha, int puerto_servidor, char* algoritmo, int entradas, int tamanio_entrada, int retard) {
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

		if (config_has_property(config_coordinador, "PUERTO_COORDINADOR")) {
			config.PUERTO_COORDINADOR = config_get_int_value(config_coordinador, "PUERTO_COORDINADOR");
			printf("PUERTO_COORDINADOR: %d\n", config.PUERTO_COORDINADOR);
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


void identificar_proceso () {

	//Recibo identidad y coloco en la bolsa correspondiente.
		int OK;
		int identificacion;
		if((OK = recibir_mensaje(socket_cliente,&id_operacion_mensaje,sizeof(int))) == ERROR_RECV){
			//Manejar el error de cierta forma si queremos.
		}else {
			if (id_operacion_mensaje == handshake){
				if((OK = recibir_mensaje(socket_cliente,&identificacion,sizeof(int))) == ERROR_RECV){
					//Manejar el error de cierta forma si queremos.
				}else {
					if ((OK =recibir_mensaje(socket_cliente,&identificacion,sizeof(int))) == ERROR_RECV){
						////Manejar el error de cierta forma si queremos.
					}else {

						switch (identificacion){

						case ESI:
							FD_SET(socket_cliente,&master);
							FD_SET(socket_cliente, &bolsa_esis); //agrego un nuevo esi a la bolsa de esis.
									printf("Se ha conectado un nuevo esi \n");
									break;

						case Instancia:
							FD_SET(socket_cliente,&master);
							FD_SET(socket_cliente, &bolsa_instancias); //agrego una nueva instancia a la bolsa de instancias.
									printf("Se ha conectado una nueva instancia de ReDis \n");
									break;

						case Planificador:

							if (planificador_conectado == 0){
								FD_SET(socket_cliente,&master);
								FD_SET(socket_cliente,&bolsa_planificador);
								printf("Se ha conectado el planificador al sistema\n");
								planificador_conectado = 1; //Para que no se conecte mas de un planificador.
								break;
								}
							}


						}
					}
				}
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
		printf("Se desconecto el planificador\n"); //TODO: Reorganizar la distribucion para las claves.
	}
	close(fdCliente); // Si se perdio la conexion, la cierro.
}

void atender_accion_esi(){

}
void atender_accion_instancia(){

}

int main(void) {

	//Extraer informacion del archivo de configuracion.

	establecer_configuracion(config.PUERTO_COORDINADOR,config.PUERTO_PLANIFICADOR,config.ALGORITMO_DISTRIBUCION,config.CANT_ENTRADAS,
							 config.ENTRADA_SIZE,config.RETARDO);

	//Establecer conexiones con el sistema.
	// En caso que el plani sea server de coordinador socket_planificador = conectar_a_server(config.IP_PLANIFICADOR, config.PUERTO_PLANIFICADOR);
	socket_coordinador = crear_socket_escucha(config.PUERTO_COORDINADOR);

	//Acciones necesarias para identificar los esi o instancias entrantes.

	FD_SET(socket_coordinador, &master);
	maxfd = socket_coordinador;

	//Bucle principal

	for (;;) {
		read_fds = master;
		if (select(maxfd + 1, &read_fds, NULL, NULL, NULL) == -1) { //Compruebo si algun cliente quiere interactuar.
			perror("select");
			exit(1);
		};

		for (fdCliente = 0; fdCliente <= maxfd; fdCliente++) {

			if (FD_ISSET(fdCliente, &read_fds)) { // Me fijo si tengo datos listos para leer
				if (fdCliente == socket_coordinador) { //si entro en este "if", significa que tengo datos.

					// gestionar nuevas conexiones.

					if ((socket_cliente = aceptar_conexion(socket_coordinador)) == ERROR_ACCEPT){
						printf("Error en el accept"); //Esto es cabeza, deberiamos tomar el error y armar un exit_gracefully como en el tp0.
					}else{
						identificar_proceso();

					}
				} else {

					// Gestionar datos de un cliente.

					int id_operacion_mensaje;

					if ((cantBytes = recibir_mensaje(fdCliente,&id_operacion_mensaje,sizeof(int)) == ERROR_RECV)) {

						conexion_de_cliente_finalizada();


					} else {

						if (FD_ISSET(fdCliente, &bolsa_esis)) { // EN CASO DE QUE EL MENSAJE LO HAYA ENVIADO UN ESI.

							atender_accion_esi(); //TODO: VER QUE PUEDE QUERER.
						}
						if (FD_ISSET(fdCliente, &bolsa_instancias)) { // EN CASO DE QUE EL MENSAJE LO HAYA ENVIADO UNA INSTANCIA.

							atender_accion_instancia(); //TODO:VER QUE PUEDE QUERER.
						}
					}
				}
			}
		}
	}
}
