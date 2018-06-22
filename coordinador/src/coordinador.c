/*
 * coordinador.c
 *
 *  Created on: 26 abr. 2018
 *      Author: Maximo Cozetti
 */

//INCLUDES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comunicacion/comunicacion.h>
#include <commons/config.h>
#include <commons/string.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdbool.h>
#include <signal.h>

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

typedef struct infoInstancia_t{
	char* nombre;
	int fd;
	int espacio_disponible;
	char letra_inicio;
	char letra_fin;
	int desconectada;
	t_list* claves;
	pthread_mutex_t semaforo;
}infoInstancia_t;

typedef struct {
		enum {
			GET,
			SET,
			STORE,
			COMPACTAR
		} keyword;
		union {
			struct {
				char* clave;
			} GET;
			struct {
				char* claveYValor;
			} SET;
			struct {
				char* clave;
			} STORE;
		} argumentos;
	} operacion_compartida_t;

typedef struct infoParaHilo_t{
	int fd;
}infoParaHilo_t;


//VARIABLES
int socket_coordinador = 0;
int socket_planificador = 0;
fd_set master; // Bolsa donde voy a registrar todas las conexiones realizadas al coordinador.
fd_set read_fds; //Bolsa que utilizo para leer informacion de los fd_set actuales.
fd_set bolsa_instancias; //Bolsa donde voy a ingresar cada instancia que se conecte.
int fdCliente = 0; //Para recorrer conjunto de fd.
int maxfd = 0; //Numero del ultimo fd creado.
t_config* config_coordinador; //Para cuando tenga que traer cosas del .cfg
config_t config; //Para cuando tenga que traer cosas del .cfg
int planificador_conectado = 0;
int max_instancias = 0;
int puntero_algoritmo_equitative = 0;
int contadorDeInstancias = 0;
t_list* lista_instancias_claves;
t_list* lista_esis_permisos_setear;
operacion_compartida_t* operacion; //Estructura que se compartira con todas las instancias. Se sincroniza para su ejecucion particular en una instancia.
int compactaciones = 0;
bool compactacionFinalizada = false;
int ultimaOperacion = 0;
infoInstancia_t* instanciaQuePidioCompactacion;
t_log* log_operaciones_esis;
int esiActual = 0;

//FUNCIONES

void sig_handler(int signo) {
  if (signo == SIGINT) {
  printf("SIGINT interceptado. Finalizando... \n");
  close(socket_coordinador);
  exit(EXIT_SUCCESS);
  }
}

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
	header.comando = msj_imposibilidad_conexion;
	header.tamanio = 0; // Solo envia header. Payload va a estar vacio.
	//Enviamos OK al Planificador. No hace falta serializar dado que payload esta vacio.
	enviar_mensaje(socket_cliente, &header, sizeof(header) + header.tamanio); // header.tamanio se podria borrar pero lo dejo para mayor entendimiento.
	printf("Se ha impedido la conexion de un segundo planificador al sistema\n");
}

void* instancia_conectada_anteriormente(char* unNombre){
	//Funcion de ayuda solo dentro de este scope
	int mismo_nombre (infoInstancia_t* p){
		return string_equals_ignore_case(p->nombre,unNombre);
	}

	if (list_any_satisfy(lista_instancias_claves,(void*) mismo_nombre)){
		return list_find(lista_instancias_claves,(void*) mismo_nombre); //Si el nombre estaba en la lista, devuelvo el nodo de esa instancia en la lista_instancias.

	}else{
	return 0; //Interpreto el 0 como que no habia coincidencia con el nombre de ninguna instancia dentro del sistema.
	}

}

void conexion_de_cliente_finalizada() {
	// conexión cerrada.
	printf("Server: socket %d termino la conexion\n", fdCliente);
	// Eliminar del conjunto maestro y su respectiva bolsa.
	FD_CLR(fdCliente, &master);

	if (FD_ISSET(fdCliente, &bolsa_instancias)){
		FD_CLR(fdCliente, &bolsa_instancias);
		printf("Se desconecto instancia del socket %d\n", fdCliente); //Reorganizar la distribucion para las claves no se hace. Nos quitan Key Explicit
	}

	close(fdCliente); // Si se perdio la conexion, la cierro.
}

infoInstancia_t* elegir_instancia_por_algoritmo(char* algoritmo){ //El warning sale porque no estan implementados LSU y KEY.

	if (string_equals_ignore_case(algoritmo,"EQUITATIVE")){
		if (puntero_algoritmo_equitative < max_instancias){
			return list_get(lista_instancias_claves,puntero_algoritmo_equitative);
			puntero_algoritmo_equitative++;
		}else if(puntero_algoritmo_equitative == max_instancias) {
			return list_get(lista_instancias_claves,puntero_algoritmo_equitative);
			puntero_algoritmo_equitative = 0;
		}
	}else if (string_equals_ignore_case(algoritmo, "LSU")){
		// Ordeno la lista_instancias_claves por espacio_disponible
	}else if (string_equals_ignore_case(algoritmo, "KEY")){
		// Hago resta con numeros de letras para saber en que instancia tiene que escribir/buscar/colgarse_de_esta/etc
	}
}

void* encontrar_instancia_por_clave (char* unaClave){

	//Funcion de ayuda solo dentro de este scope
	int misma_clave (char* p){
		return string_equals_ignore_case(p,unaClave);
	}

	int i;
	infoInstancia_t* instanciaConlistaDeClaves;
	void* instanciaConClave;

	for (i = 0; i < lista_instancias_claves->elements_count; ++i) {

		instanciaConlistaDeClaves = (infoInstancia_t*)list_get(lista_instancias_claves,i);
		instanciaConClave = list_find(instanciaConlistaDeClaves->claves,(void*) misma_clave);

		if(instanciaConClave != NULL){
			return list_get(lista_instancias_claves,i);
		}
	}
		return 0; // Devuelvo 0 si no tenia la clave en ninguna instancia
}

void* encontrar_instancia_por_fd (int unFd){

	//Funcion de ayuda solo dentro de este scope
	int mismo_fd (infoInstancia_t* p){
		return (p->fd == unFd);
	}

	void* instanciaConFd;

	instanciaConFd = list_find(lista_instancias_claves,(void*) mismo_fd);

	return instanciaConFd;
}

void* filtrar_instancias_conectadas(){

	//Funcion solo dentro de este scope
	int esta_conectada (infoInstancia_t* unaInstancia){
			return (unaInstancia->desconectada == false);
	}

	t_list* listaConectadas = list_create();
	listaConectadas = list_filter(lista_instancias_claves,(void*) esta_conectada);
	return listaConectadas;
}

void signal_a_todos_los_semaforos_hiloInstancia(t_list* listaInstancias){

	int i = 0;
	infoInstancia_t* unaInstancia;

	for (i = 0; i < listaInstancias->elements_count; ++i) {

		unaInstancia = list_get(listaInstancias,i);
		pthread_mutex_unlock(&unaInstancia->semaforo);
	}
}

int enviar_mensaje_planificador(int socket_planificador, header_t* header, void* buffer, int id_mensaje) {
	void* bufferAEnviar;
	int retornoPlanificador = 0;

	switch(id_mensaje){

		case msj_solicitud_get_clave:

			header->comando = msj_solicitud_get_clave;
			header->tamanio = 0;
			bufferAEnviar = serializar(*header, buffer);
			enviar_mensaje(socket_planificador, bufferAEnviar, sizeof(header_t) + header->tamanio);
			free(bufferAEnviar);
			printf("Se envió mensaje SOLICITUD CLAVE al planificador\n");
			recibir_mensaje(socket_planificador, &retornoPlanificador, sizeof(int)); //Posible que rompa aca al no esperarse un header y solo un numero (se rompe protocolo).
			printf("Se recibió respuesta del planificador\n"); //No necesito respuesta del planificador pero lo hago por forma generica de envio de OK en PLANI.
		break;

		case msj_error_clave_inaccesible:

			header->comando = msj_error_clave_inaccesible;
			header->tamanio = 0;
			bufferAEnviar = serializar(*header,buffer);
			enviar_mensaje(socket_planificador,bufferAEnviar,sizeof(header_t));
			free(bufferAEnviar);
			printf("Se envió mensaje ERROR CLAVE INACCESIBLE al planificador\n");
			recibir_mensaje(socket_planificador, &retornoPlanificador, sizeof(int));
			printf("Se recibió respuesta del planificador\n"); //No necesito respuesta del planificador pero lo hago por forma generica de envio de OK en PLANI.
		break;

		case msj_esi_tiene_tomada_clave:

			header->comando = msj_esi_tiene_tomada_clave;
			header->tamanio = 0;
			bufferAEnviar = serializar(*header, buffer);
			enviar_mensaje(socket_planificador, bufferAEnviar, sizeof(header_t) + header->tamanio); //Pregunto al PLANI si el ESI tiene la clave tomada como para operar.
			free(bufferAEnviar);
			printf("Se envió pregunta ESI_TIENE_TOMADA_CLAVE al planificador\n");
			recibir_mensaje(socket_planificador, &retornoPlanificador, sizeof(int));
			printf("Se recibió respuesta del planificador\n"); //No necesito respuesta del planificador pero lo hago por forma generica de envio de OK en PLANI.
		break;

		case msj_error_clave_no_identificada:

			header->comando = msj_error_clave_no_identificada;
			header->tamanio = 0;
			bufferAEnviar = serializar(*header, buffer);
			enviar_mensaje(socket_planificador, bufferAEnviar, sizeof(header_t) + header->tamanio);
			free(bufferAEnviar);
			printf("Se envió mensaje ERROR CLAVE NO IDENTIFICADA al planificador\n");
			recibir_mensaje(socket_planificador, &retornoPlanificador, sizeof(int));
			printf("Se recibió respuesta del planificador\n"); //No necesito respuesta del planificador pero lo hago por forma generica de envio de OK en PLANI.
		break;

		case msj_error_clave_no_bloqueada: //TODO: Hablarlo con los chicos, tal vez es innecesario que le mande este mensaje porque lo deducen solos cuando les pregunto si tiene bloqueada la clave.

			header->comando = msj_error_clave_no_identificada;
			header->tamanio = 0;
			bufferAEnviar = serializar(*header, buffer);
			enviar_mensaje(socket_planificador, bufferAEnviar, sizeof(header_t) + header->tamanio);
			free(bufferAEnviar);
			printf("Se envió mensaje ERROR CLAVE NO BLOQUEADA al planificador\n");
			recibir_mensaje(socket_planificador, &retornoPlanificador, sizeof(int));
			printf("Se recibió respuesta del planificador\n"); //No necesito respuesta del planificador pero lo hago por forma generica de envio de OK en PLANI.
		break;

		case msj_instancia_compactar:
			header->comando = msj_instancia_compactar;
			header->tamanio = 0;
			bufferAEnviar = serializar(*header,buffer);
			enviar_mensaje(socket_planificador,bufferAEnviar,sizeof(header_t) + header->tamanio);
			free(bufferAEnviar);
			printf("Se envió necesidad de COMPACTAR al planificador\n");
			recibir_mensaje(socket_planificador, &retornoPlanificador, sizeof(int));
			printf("Se recibió respuesta del planificador\n"); //No necesito respuesta del planificador pero lo hago por forma generica de envio de OK en PLANI.
		break;

		default:
			printf("Encabezado erroneo. No envie nada al planificador\n");
			break;
	}

	return retornoPlanificador;

}

operacion_compartida_t* preparar_operacion_compartida_GET(char* clave) {

	operacion_compartida_t* operacionX = malloc(sizeof(operacion_compartida_t));

	operacionX->keyword = GET;
	operacionX->argumentos.GET.clave = clave;

	return operacionX;
}

operacion_compartida_t* preparar_operacion_compartida_SET(char* clave) {

	operacion_compartida_t* operacionX = malloc(sizeof(operacion_compartida_t));

	operacionX->keyword = SET;
	operacionX->argumentos.SET.claveYValor = clave;

	return operacionX;
}

operacion_compartida_t* preparar_operacion_compartida_STORE(char* clave) {

	operacion_compartida_t* operacionX = malloc(sizeof(operacion_compartida_t));

	operacionX->keyword = STORE;
	operacionX->argumentos.STORE.clave = clave;

	return operacionX;
}

void* atender_accion_esi(void* fd) { //Hecho con void* para evitar casteo en creacion del hilo.

	int fdEsi = (int) fd;
	esiActual = (int) fd;
	header_t header;
	int resultado;

	while(1){ //Comienzo a atender el esi en el hilo hasta que el header sea msj_esi_finalizado

		printf("Atendiendo acción esi en socket %d!!!\n", fdEsi);

		resultado = recibir_mensaje(fdEsi,&header,sizeof(header_t));
		if ((resultado == ERROR_RECV) || (resultado == ERROR_RECV_DISCONNECTED)){
			printf("Error al recibir header del ESI \n");
			conexion_de_cliente_finalizada();
			exit(EXIT_FAILURE);
		}
		void* buffer = malloc(header.tamanio);
		resultado = recibir_mensaje(fdEsi,buffer,header.tamanio);
		if ((resultado == ERROR_RECV) || (resultado == ERROR_RECV_DISCONNECTED)){
			printf("Error al recibir payload del ESI \n");
			conexion_de_cliente_finalizada();
			exit(EXIT_FAILURE);
		}

		if(header.comando == msj_esi_finalizado){ //Si el ESI me indica que termino, cierro el hilo
			conexion_de_cliente_finalizada();
			printf("Se desconecto esi del socket %d\n", fdCliente); //TODO: Probablemente haya que hacer algo mas, hay que ver el enunciado.
			exit(EXIT_FAILURE);

		}else{ //Veo que sentencia me envio

			switch (header.comando) {
				infoInstancia_t* instanciaConClave;
				char* clave;

				case msj_sentencia_get:

					log_trace(log_operaciones_esis, "ESI %d realizo un GET clave %s",fdEsi,(char*) buffer);

					/* 1) Le pregunto al planificador si puede hacer el get de la clave indicada
					 *		1.1) Si puede, el PLANIFICADOR le avisa al ESI que ejecute la siguiente instruccion.
					 *		1.2) Si no puede, el planificador lo para y le dice a otro ESI que me mande una instruccion.
					 */
						clave = (char*)buffer;
						instanciaConClave = encontrar_instancia_por_clave(clave);

						if (instanciaConClave != 0){ //Distinto de cero indica que se encontro la clave

							if(instanciaConClave->desconectada == false){ //Si la instancia que tiene esa clave no esta desconectada

								//Envio mensaje a planificador con solicitud de GET clave por parte del ESI y recibo su respuesta.
								enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_solicitud_get_clave);

							}else {
								//Envio mensaje a planificador diciendo que el ESI debe abortar por tratar de ingresar a una clave en instancia desconectada y recibo su respuesta.
								enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_error_clave_inaccesible);

							}
						}else { //Entro aqui si no encontre la clave en mi sistema

							//Selecciono instancia victima segun algoritmo de distribucion
							instanciaConClave = elegir_instancia_por_algoritmo(config.ALGORITMO_DISTRIBUCION);

							//Agrego clave a la lista de claves que tiene la respectiva instancia.
							list_add(instanciaConClave->claves, clave);

							//Modifico estructuras en variable global operacion compartida para que tome la respectiva instancia.
							operacion = preparar_operacion_compartida_GET(clave);


							//Levanto el semaforo de la instancia seleccionada para que trabaje con variable global operacion compartida.
							pthread_mutex_unlock(&instanciaConClave->semaforo);
						}
				break;

				case msj_sentencia_set:

					log_trace(log_operaciones_esis, "ESI %d realizo un SET clave %s",fdEsi,(char*) buffer);
					clave = (char*)buffer; //Anda de una porque como separamos con /0

					instanciaConClave = encontrar_instancia_por_clave(clave);

					if (instanciaConClave != 0){ //Distinto de cero indica que se encontro la clave

						if(instanciaConClave->desconectada == false){ //Si la instancia que tiene esa clave no esta desconectada

							//Envio mensaje a planificador con pregunta si ESI tiene tomada la clave y recibo su respuesta.
							resultado = enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_esi_tiene_tomada_clave);

							if (resultado == msj_ok_solicitud_operacion){

								//Modifico estructuras en variable global operacion compartida para que tome la respectiva instancia.
								operacion = preparar_operacion_compartida_SET(clave);

								//Levanto el semaforo de la instancia seleccionada para que trabaje con variable global operacion compartida.
								pthread_mutex_unlock(&instanciaConClave->semaforo);

							}else{
								//El ESI no tiene geteada la clave para operar con SET o STORE. PLANI hace lo que tenga que hacer, COORDINADOR no toca nada, solo avisa al PLANI.
								enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_error_clave_no_bloqueada);
							}
						}else {
							//Envio mensaje a planificador diciendo que el ESI debe abortar por tratar de ingresar a una clave en instancia desconectada y recibo su respuesta.
							enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_error_clave_inaccesible);

						}
					}else {
						//Envio mensaje a planificador diciendo que el ESI debe abortar por tratar de ingresar a una clave no identificada y recibo su respuesta.
						enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_error_clave_no_identificada);
					}
				break;

				case msj_sentencia_store:

					log_trace(log_operaciones_esis, "ESI %d realizo un STORE clave %s",fdEsi,(char*) buffer);
					clave = (char*)buffer;

					instanciaConClave = encontrar_instancia_por_clave(clave);

					if (instanciaConClave != 0){ //Distinto de cero indica que se encontro la clave

						if(instanciaConClave->desconectada == false){ //Si la instancia que tiene esa clave no esta desconectada

							//Envio mensaje a planificador con pregunta si ESI tiene tomada la clave y recibo su respuesta.
							resultado = enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_esi_tiene_tomada_clave);

							if (resultado == msj_ok_solicitud_operacion){

								//Modifico estructuras en variable global operacion compartida para que tome la respectiva instancia.
								operacion = preparar_operacion_compartida_STORE(clave);

								//Levanto el semaforo de la instancia seleccionada para que trabaje con variable global operacion compartida.
								pthread_mutex_unlock(&instanciaConClave->semaforo);

								//TODO: Avisarle al planificador que se libero la clave con STORE.

							}else{
								//El ESI no tiene geteada la clave para operar con SET o STORE. PLANI hace lo que tenga que hacer, COORDINADOR no toca nada, solo avisa al PLANI.
								enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_error_clave_no_bloqueada);
							}
						}else {
							//Se envia al PLANI que el ESI debe abortar por tratar de ingresar a una clave en instancia desconectada.
							enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_error_clave_inaccesible);
						}
					}else {
						//Envio mensaje a planificador diciendo que el ESI debe abortar por tratar de ingresar a una clave no identificada y recibo su respuesta.
						enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_error_clave_no_identificada);
					}
				break;

				default:
					log_error(log_operaciones_esis,"Se recibio comando desconocido desde ESI");
				break;
			}
		}
	} //Fin del while
}

void* atender_accion_instancia(void* info) { // Puesto void* para evitar casteo en creacion de hilo.

	header_t* header = malloc(sizeof(header));
	void* buffer;
	void* bufferAEnviar;
	infoInstancia_t* miInstancia = malloc(sizeof(infoInstancia_t));
	infoParaHilo_t* informacionQueSeComparte = malloc(sizeof(infoParaHilo_t));
	informacionQueSeComparte = (infoParaHilo_t*) info;

	miInstancia = encontrar_instancia_por_fd(informacionQueSeComparte->fd);

	while(1){

		pthread_mutex_lock(&miInstancia->semaforo); //Hago el wait al mutex

		if(miInstancia->desconectada == true){ //Si la instancia que atendia este hilo se desconecto, el hilo muere.
			pthread_mutex_destroy(&miInstancia->semaforo);
			exit(EXIT_FAILURE);
		}else{

			printf("Atendiendo acción instancia en socket %d!!!\n", informacionQueSeComparte->fd);

			switch (operacion->keyword) {

				case GET:
					header->comando = msj_sentencia_get;
					header->tamanio = strlen(operacion->argumentos.GET.clave)+1;
					buffer = malloc(header->tamanio);
					memcpy(buffer,operacion->argumentos.GET.clave,header->tamanio);

					bufferAEnviar = serializar(*header,buffer);
					enviar_mensaje(informacionQueSeComparte->fd,bufferAEnviar,sizeof(header_t) + header->tamanio);
				break;

				case SET:
					header->comando = msj_sentencia_set;
					header->tamanio = strlen(operacion->argumentos.SET.claveYValor)+1; //Va +1 y no +2 porque  esto tiene un \0 en el medio.

					buffer = malloc(header->tamanio);
					memcpy(buffer,operacion->argumentos.SET.claveYValor,strlen(operacion->argumentos.SET.claveYValor)+1);

					bufferAEnviar = serializar(*header,buffer);
					enviar_mensaje(informacionQueSeComparte->fd,bufferAEnviar,sizeof(header_t) + header->tamanio);
					break;

				case STORE:
					header->comando = msj_sentencia_store;
					header->tamanio = strlen(operacion->argumentos.STORE.clave)+1;

					buffer = malloc(header->tamanio);
					memcpy(buffer,operacion->argumentos.STORE.clave,header->tamanio);

					bufferAEnviar = serializar(*header,buffer);
					enviar_mensaje(informacionQueSeComparte->fd,bufferAEnviar,sizeof(header_t) + header->tamanio);
				break;

				case COMPACTAR:
					header->comando = msj_instancia_compactar;
					header->tamanio = 0;

					buffer = malloc(header->tamanio);

					bufferAEnviar = serializar(*header,buffer);
					enviar_mensaje(informacionQueSeComparte->fd,bufferAEnviar,sizeof(header_t));
				break;

				default:
					printf("Le metieron cualquier cosa al keyword de la operacion compartida\n");
				break;
			}
		}
	}
}//Fin del for

void* crear_nueva_instancia(int socket_cliente, const config_t* config, char* nombre_instancia) {
	//No existia anteriormente en el sistema. La creo

	infoInstancia_t* nueva_instancia = (infoInstancia_t*)malloc(sizeof(infoInstancia_t));

	nueva_instancia->nombre = nombre_instancia;
	nueva_instancia->fd = socket_cliente;
	nueva_instancia->espacio_disponible = config->CANT_ENTRADAS;
	nueva_instancia->desconectada = false;
	nueva_instancia->letra_inicio = '-';
	nueva_instancia->letra_fin = '-';
	nueva_instancia->claves = list_create();
	pthread_mutex_init(&nueva_instancia->semaforo, NULL);
	pthread_mutex_lock(&nueva_instancia->semaforo);

	return nueva_instancia;
}

infoParaHilo_t* crear_info_para_hilo_instancia(int socket_cliente, operacion_compartida_t* operacion, infoInstancia_t* nueva_instancia) {

	infoParaHilo_t* info = malloc(sizeof(infoParaHilo_t));

	info->fd = socket_cliente;
	return info;
}

operacion_compartida_t* inicializar_operacion_compartida(){

	operacion_compartida_t* operacionX = malloc(sizeof(operacion_compartida_t));

	operacionX->keyword = 0;
	operacionX->argumentos.GET.clave = string_new();
	operacionX->argumentos.SET.claveYValor = string_new();
	operacionX->argumentos.STORE.clave = string_new();

	return operacionX;
}

void enviar_ok_sentencia_a_ESI(){
	header_t header;
	header.comando = msj_sentencia_finalizada;
	header.tamanio = 0;

	enviar_mensaje(esiActual,&header,sizeof(header_t));
}

void enviar_configuracion_entradas_a_instancia(header_t* cabecera, config_t* config, int socket_cliente) {

	//Envio cantidad y tamaño de entradas a la instancia.
	cabecera->comando = msj_cantidad_entradas;
	cabecera->tamanio = sizeof(int);
	void* buffer = serializar(*cabecera, &config->CANT_ENTRADAS);
	enviar_mensaje(socket_cliente, buffer, sizeof(header_t) + cabecera->tamanio);
	free(buffer);

	cabecera->comando = msj_tamanio_entradas;
	cabecera->tamanio = sizeof(int);
	buffer = serializar(*cabecera, &config->ENTRADA_SIZE);
	enviar_mensaje(socket_cliente, buffer, sizeof(header_t) + cabecera->tamanio);
	free(buffer);
}

void identificar_proceso_y_crear_su_hilo(int socket_cliente) {

	//Recibo identidad y coloco en la bolsa correspondiente.
	header_t cabecera;
	int identificacion;
	infoInstancia_t* nueva_instancia;
	infoInstancia_t* instancia_existente;
	char* nombre_instancia = string_new();
	int resultado = recibir_mensaje(socket_cliente, &cabecera, sizeof(header_t));
	if(resultado == ERROR_RECV){
		printf("Error en el recv para socket %d!!!\n", socket_cliente);
	}

	switch (cabecera.comando) {
	case msj_handshake:
		resultado = recibir_mensaje(socket_cliente, &identificacion, cabecera.tamanio);
		if(resultado == ERROR_RECV){
			printf("Error en el recv para socket %d al hacer handshake!!!\n", socket_cliente);
		} else {
			switch (identificacion){
			case ESI:
				responder_ok_handshake(ESI, socket_cliente);

				//Creo el hilo para su ejecucion
				pthread_t hiloESI;
				pthread_create(&hiloESI,NULL,&atender_accion_esi, (void*)socket_cliente);
				pthread_detach(hiloESI);
			break;

			case Instancia:
				responder_ok_handshake(Instancia, socket_cliente);

				resultado = recibir_mensaje(socket_cliente, &cabecera, sizeof(header_t)); //Ahora recibo el nombre de la instancia
				if ((resultado == ERROR_RECV) || !(cabecera.comando == msj_nombre_instancia)) { //Si hay error en recv o cabecera no dice msj_nombre_instancia
					printf("Error al intentar recibir nombre de la instancia\n");
				} else {
					recibir_mensaje(socket_cliente, nombre_instancia, cabecera.tamanio);
				}

				//Agrego instancia a bolsa donde guardo todas las instancias.
				FD_SET(socket_cliente, &master);
				FD_SET(socket_cliente, &bolsa_instancias);

				//Envio cantidad y tamaño de entradas a la instancia.
				enviar_configuracion_entradas_a_instancia(&cabecera, &config, socket_cliente);

				//Pregunto si la instancia esta conectada.
				instancia_existente = instancia_conectada_anteriormente(nombre_instancia);

				if (instancia_existente == 0) { //No existia anteriormente en el sistema.

					nueva_instancia = crear_nueva_instancia(socket_cliente, &config,nombre_instancia); //No importa este warning porque la inicializo aca mismo.

					//Creo informacion de ejecucion para hilo instancia.
					pthread_t hiloInstancia;
					infoParaHilo_t* info;

					info = crear_info_para_hilo_instancia(socket_cliente, operacion, nueva_instancia);

					//Agrego nueva instancia a la lista de instancias conectadas al sistema.
					list_add(lista_instancias_claves,nueva_instancia);
					printf("Se ha conectado una nueva Instancia de ReDis al sistema en fd %d\n", socket_cliente);

					//Creo y mando a ejecutar el hilo.
					pthread_create(&hiloInstancia, NULL, &atender_accion_instancia,(void*) info);
					pthread_detach(hiloInstancia);

				}else { //Para cuando una instancia se quiere reincorporar.

					//Solo le modifico estos campos
					instancia_existente->desconectada = false;
					instancia_existente->fd = socket_cliente;

					//Creo informacion de ejecucion para hilo instancia.
					pthread_t hiloInstancia;
					infoParaHilo_t* info;

					info = crear_info_para_hilo_instancia(socket_cliente,operacion,instancia_existente);

					//Creo y mando a ejecutar el hilo.
					pthread_create(&hiloInstancia, NULL, &atender_accion_instancia,&info);
					pthread_detach(hiloInstancia);

					responder_ok_handshake(Instancia, socket_cliente);

					//Reincorporo la instancia al sistema. Ver tema de Dump en el enunciado: seccion "Almacenamiento". Nos quitan reincorporacion de instancias.
					//Verificar si van quedando hilos abiertos a medida que las instancias se van desconectando. Nos quitan reincorporacion de instancias.
				}
				break;

			case Planificador:
				if (planificador_conectado == 0){
					FD_SET(socket_cliente, &master);
//					FD_SET(socket_cliente, &bolsa_planificador);
					printf("Se ha conectado el planificador al sistema en fd %d\n", socket_cliente);
					planificador_conectado = 1; //Para que no se conecte mas de un planificador.
					responder_ok_handshake(Planificador, socket_cliente);
					socket_planificador = socket_cliente;
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

void escuchar_mensaje_de_instancia(int unFileDescriptor){

	header_t header;
	int resultado = 0;
	int cantidadInstanciasConectadas = 0;
	t_list* instanciasConectadas;

	resultado = recibir_mensaje(unFileDescriptor,&header,sizeof(header_t));

	if ((resultado == ERROR_RECV) || (resultado == ERROR_RECV_DISCONNECTED)){
		printf("Error al recibir header de INSTANCIA \n");
		conexion_de_cliente_finalizada();
		exit(EXIT_FAILURE);
	}

	switch (header.comando) {

		case msj_instancia_compactar:
//			//Notifico al planificador para que pare por pedido de compactacion.
//			enviar_mensaje_planificador(socket_planificador, &header, buffer,msj_instancia_compactar);

			//Guardo la ultima sentencia enviada por un esi para enviar de vuelta a la instancia que solicito compactar.
			ultimaOperacion = operacion->keyword;

			//Seteo operacion->keyword (region compartida) en COMPACTAR
			operacion->keyword = COMPACTAR;

			//Filtro instancias conectadas al sistema para enviarles mensaje de "compactense".
			instanciasConectadas = filtrar_instancias_conectadas();

			//Levanto el semaforo de cada hilo que atiende a su instancia.
			signal_a_todos_los_semaforos_hiloInstancia(instanciasConectadas);
		break;

		case msj_instancia_compactacion_finalizada:
			//Recibo msj de compactacion OK. Incremento la cantidad de compactaciones hechas hasta que sea igual al total de instancias conectadas.
			compactaciones++;
			instanciasConectadas = filtrar_instancias_conectadas();
			cantidadInstanciasConectadas = instanciasConectadas->elements_count;

			if (compactaciones == cantidadInstanciasConectadas) {
				//Reseteo compactaciones para futura solicitud de compactacion.
				compactaciones = 0;

				//Cambio la operacion->keyword a la ultimaOperacion.
				operacion->keyword = ultimaOperacion;

				//Levanto el semaforo de la instancia que solicito compactacion para que vuelva a ejecutar sobre la operacion compartida.
				instanciaQuePidioCompactacion = encontrar_instancia_por_fd(unFileDescriptor);
				pthread_mutex_unlock(&instanciaQuePidioCompactacion->semaforo);

//				//Notifico al PLANIFICADOR que ya se compactaron todas las instancias y hay que continuar la ejecucion.
//				enviar_mensaje_planificador(socket_planificador, &header, buffer,msj_compactacion_finalizada_continuar_planificacion);
			}
		break;

		case msj_instancia_sustituyo_clave:

			//TODO: Actualizar las claves que tiene esta instancia
		break;

		case msj_sentencia_finalizada:

			//Recibi ok de la instancia. Tengo que avisar al ESI que salio bien. Crear variable global que mantenga el fd del ESI que mando ultima operacion
			enviar_ok_sentencia_a_ESI();
		break;

		default:
			break;
	}
}

int main(void) {

	//Creo log de operaciones de los ESI
	log_operaciones_esis = log_create("coordinador.log", "Coordinador", true, LOG_LEVEL_TRACE);
	lista_instancias_claves = list_create();

	if (signal(SIGINT, sig_handler) == SIG_ERR){
	  printf("Error al interceptar SIGINT\n");
	  return EXIT_SUCCESS;
	}

	//Extraer informacion del archivo de configuracion.
	establecer_configuracion(config.PUERTO,config.PUERTO_PLANIFICADOR,config.ALGORITMO_DISTRIBUCION,config.CANT_ENTRADAS, config.ENTRADA_SIZE,config.RETARDO);

	//Establecer conexiones con el sistema.
	socket_coordinador = crear_socket_escucha(config.PUERTO);

	//Inicializo estructura de operacion compartida
	operacion = inicializar_operacion_compartida();

	//operacion = malloc(sizeof(operacion_compartida_t));

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
						printf("Error en el accept\n");
					} else {
						identificar_proceso_y_crear_su_hilo(socket_cliente);
					}

				}else if (FD_ISSET(fdCliente, &bolsa_instancias)) { // Una instancia envio un mensaje. 1)Necesita compactar. 2)Finalizo compactacion. 3) Libero una clave (entrada). 4)Me envio un OK.
						escuchar_mensaje_de_instancia(fdCliente);
				}
			}
		}
	} //Fin del bucle principal
}
