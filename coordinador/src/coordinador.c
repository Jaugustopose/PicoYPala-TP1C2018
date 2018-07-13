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
#include <semaphore.h>

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
	sem_t semaforo;
}infoInstancia_t;

typedef struct {
		enum {
			GET,
			SET,
			STORE,
			COMPACTAR,
			STATUS
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
fd_set bolsa_planificador; //Bolsa donde guardo el planificador.
int fdCliente = 0; //Para recorrer conjunto de fd.
int maxfd = 0; //Numero del ultimo fd creado.
t_config* config_coordinador; //Para cuando tenga que traer cosas del .cfg
config_t config; //Para cuando tenga que traer cosas del .cfg
int planificador_conectado = 0;
int puntero_algoritmo_equitative = 0;
int contadorDeInstancias = 0;
int okPlanificador = 0;
t_list* lista_instancias_claves;
t_list* lista_esis_permisos_setear;
operacion_compartida_t* operacion; //Estructura que se compartira con todas las instancias. Se sincroniza para su ejecucion particular en una instancia.
int compactaciones = 0;
bool compactacionFinalizada = false;
int ultimaOperacion = 0;
int fdQuePidioCompactacion;
infoInstancia_t* instanciaQuePidioCompactacion;
t_log* log_coordinador;
int esiActual = 0;
//sem_t atenderEsi;
//sem_t atenderInstancia;
//sem_t escucharInstancia;
pthread_mutex_t mutexMaster;
sem_t semRespuestaPlanificador;
char* statusClave;
char* statusValorClave;
char* statusNombreInstanciaConClave;
char* statusNombreInstanciaCandidata;

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

void conexion_de_cliente_finalizada(int unFD) {
	pthread_mutex_lock(&mutexMaster);
	// conexión cerrada.
	printf("Server: socket %d termino la conexion\n", unFD);
	if (FD_ISSET(unFD, &bolsa_instancias)){
		contadorDeInstancias--;
		FD_CLR(unFD, &bolsa_instancias);
		printf("Se desconecto instancia del socket %d\n", unFD); //Reorganizar la distribucion para las claves no se hace. Nos quitan Key Explicit
	}else if (!FD_ISSET(unFD,&bolsa_planificador)){

		int respuesta = 0;
		respuesta = msj_ok_solicitud_operacion;
		enviar_mensaje(unFD, &respuesta, sizeof(respuesta));

	}
	// Eliminar del conjunto maestro y su respectiva bolsa.
	FD_CLR(unFD, &master);

	pthread_mutex_unlock(&mutexMaster);
	close(unFD); // Si se perdio la conexion, la cierro.
}

infoInstancia_t* elegir_instancia_por_algoritmo(char* algoritmo){ //El warning sale porque no estan implementados LSU y KEY.

	//Funcion solo para dentro de este scope.
	bool _instancia_menor_espacio(infoInstancia_t* unaInstancia, infoInstancia_t* instanciaConMenosEspacio) {
		return unaInstancia->espacio_disponible < instanciaConMenosEspacio->espacio_disponible;
	}

	if (string_equals_ignore_case(algoritmo,"EL")){
		int punteroActual = 0;
		log_debug(log_coordinador, "Estoy distribuyendo por EQUITATIVE LOAD");
		punteroActual = puntero_algoritmo_equitative;

			if ((puntero_algoritmo_equitative + 1) == contadorDeInstancias){

				puntero_algoritmo_equitative = 0;

			}else{
				puntero_algoritmo_equitative++;
			}
			return list_get(lista_instancias_claves,punteroActual);

	}else if (string_equals_ignore_case(algoritmo, "LSU")){

		log_debug(log_coordinador, "Estoy distribuyendo por LSU");
		// Ordeno la lista_instancias_claves por espacio_disponible
		list_sort(lista_instancias_claves,(void*) _instancia_menor_espacio);

		return list_get(lista_instancias_claves,0);

	}else if (string_equals_ignore_case(algoritmo, "KEY")){
		log_debug(log_coordinador, "El algoritmo KEY EXPLICIT es restriccion de TP por cantidad de integrantes");
		exit(EXIT_FAILURE);

	}else {
		log_debug(log_coordinador,"Algoritmo de distribucion desconocido");
		exit(EXIT_FAILURE);
	}
}

infoInstancia_t* simular_eleccion_instancia_por_algoritmo(char* algoritmo){ //El warning sale porque no estan implementados LSU y KEY.

	//Funcion solo para dentro de este scope.
	bool _instancia_menor_espacio(infoInstancia_t* unaInstancia, infoInstancia_t* instanciaConMenosEspacio) {
		return unaInstancia->espacio_disponible < instanciaConMenosEspacio->espacio_disponible;
	}

	if (string_equals_ignore_case(algoritmo,"EL")){
		return list_get(lista_instancias_claves,puntero_algoritmo_equitative);

	}else if (string_equals_ignore_case(algoritmo, "LSU")){
		// TODO: Ordeno la lista_instancias_claves por espacio_disponible
		list_sort(lista_instancias_claves,(void*) _instancia_menor_espacio);

		return list_get(lista_instancias_claves,0);


	}else if (string_equals_ignore_case(algoritmo, "KEY")){
		log_debug(log_coordinador, "El algoritmo KEY EXPLICIT es restriccion de TP por cantidad de integrantes");
		exit(EXIT_FAILURE);

	}else {
		log_debug(log_coordinador,"Algoritmo de distribucion desconocido");
		exit(EXIT_FAILURE);
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
		sem_post(&unaInstancia->semaforo);
	}
}

void enviar_mensaje_planificador(int socket_planificador, header_t* header, void* buffer, int id_mensaje) {
	void* bufferAEnviar;
//	int retornoPlanificador = 0;

	switch(id_mensaje){

		case msj_solicitud_get_clave:

			header->comando = msj_solicitud_get_clave;
			bufferAEnviar = serializar(*header, buffer);
			log_debug(log_coordinador, "Header.tamanio = %d", header->tamanio);
			enviar_mensaje(socket_planificador, bufferAEnviar, sizeof(header_t) + header->tamanio);
			free(bufferAEnviar);
			log_debug(log_coordinador, "Se envió mensaje SOLICITUD CLAVE al planificador");
//			recibir_mensaje(socket_planificador, &retornoPlanificador, sizeof(int)); //Posible que rompa aca al no esperarse un header y solo un numero (se rompe protocolo).
//			log_debug(log_coordinador, "Se recibió respuesta del planificador"); //No necesito respuesta del planificador pero lo hago por forma generica de envio de OK en PLANI.

		break;

		case msj_error_clave_inaccesible:

			header->comando = msj_error_clave_inaccesible;
			header->tamanio = 0;
			bufferAEnviar = serializar(*header,buffer);
			enviar_mensaje(socket_planificador,bufferAEnviar,sizeof(header_t));
			free(bufferAEnviar);
			log_debug(log_coordinador, "Se envió mensaje ERROR CLAVE INACCESIBLE al planificador");
//			recibir_mensaje(socket_planificador, &retornoPlanificador, sizeof(int));
//			log_debug(log_coordinador, "Se recibió respuesta del planificador"); //No necesito respuesta del planificador pero lo hago por forma generica de envio de OK en PLANI.
		break;

		case msj_esi_tiene_tomada_clave:

			header->comando = msj_esi_tiene_tomada_clave;
			bufferAEnviar = serializar(*header, buffer);
			enviar_mensaje(socket_planificador, bufferAEnviar, sizeof(header_t) + header->tamanio); //Pregunto al PLANI si el ESI tiene la clave tomada como para operar.
			free(bufferAEnviar);
			log_debug(log_coordinador, "Se envió pregunta ESI_TIENE_TOMADA_CLAVE al planificador");
//			recibir_mensaje(socket_planificador, &retornoPlanificador, sizeof(int));
//			log_debug(log_coordinador, "Se recibió respuesta del planificador"); //No necesito respuesta del planificador pero lo hago por forma generica de envio de OK en PLANI.
		break;

		case msj_error_clave_no_identificada:

			header->comando = msj_error_clave_no_identificada;
			header->tamanio = 0;
			bufferAEnviar = serializar(*header, buffer);
			enviar_mensaje(socket_planificador, bufferAEnviar, sizeof(header_t) + header->tamanio);
			free(bufferAEnviar);
			log_debug(log_coordinador, "Se envió mensaje ERROR CLAVE NO IDENTIFICADA al planificador");
//			recibir_mensaje(socket_planificador, &retornoPlanificador, sizeof(int));
//			log_debug(log_coordinador, "Se recibió respuesta del planificador"); //No necesito respuesta del planificador pero lo hago por forma generica de envio de OK en PLANI.
		break;

//		case msj_instancia_compactar:
//			header->comando = msj_instancia_compactar;
//			header->tamanio = 0;
//			bufferAEnviar = serializar(*header,buffer);
//			enviar_mensaje(socket_planificador,bufferAEnviar,sizeof(header_t) + header->tamanio);
//			free(bufferAEnviar);
//			log_debug(log_coordinador, "Se envió necesidad de COMPACTAR al planificador");
//			recibir_mensaje(socket_planificador, &retornoPlanificador, sizeof(int));
//			log_debug(log_coordinador, "Se recibió respuesta del planificador"); //No necesito respuesta del planificador pero lo hago por forma generica de envio de OK en PLANI.
//		break;

		case msj_store_clave:
			header->comando = msj_store_clave;
			bufferAEnviar = serializar(*header,buffer);
			enviar_mensaje(socket_planificador,bufferAEnviar,sizeof(header_t) + header->tamanio);
			free(bufferAEnviar);
			log_debug(log_coordinador, "Se envió mensaje LIBERAR CLAVES POR STORE al planificador");
//			recibir_mensaje(socket_planificador, &retornoPlanificador, sizeof(int));
//			log_debug(log_coordinador, "Se recibió respuesta del planificador"); //No necesito respuesta del planificador pero lo hago por forma generica de envio de OK en PLANI.
		break;

		case msj_status_clave:
			log_debug(log_coordinador, "Se envia respuesta de status al planificador");
			bufferAEnviar = serializar(*header,buffer);
			enviar_mensaje(socket_planificador,bufferAEnviar,sizeof(header_t) + header->tamanio);

		break;

		default:
			printf("Encabezado erroneo. No envie nada al planificador\n");
			break;
	}

//	return retornoPlanificador;

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

void enviar_ok_sentencia_a_ESI(){
	header_t header;
	header.comando = msj_sentencia_finalizada;
	header.tamanio = 0;
	log_debug(log_coordinador,"Se envia sentencia finalizada a esi: %d",esiActual);
	enviar_mensaje(esiActual,&header,sizeof(header_t));
}

void* atender_accion_esi(void* fd) { //Hecho con void* para evitar casteo en creacion del hilo.

	int fdEsi = (int) fd;
	header_t header;
	int resultado;

	while(1){ //Comienzo a atender el esi en el hilo hasta que el header sea msj_esi_finalizado

		//sem_wait(&atenderEsi); //Semaforo bloqueante para orden

		printf("Atendiendo acción esi en socket %d!!!\n", fdEsi);

		resultado = recibir_mensaje(fdEsi,&header,sizeof(header_t));
		if ((resultado == ERROR_RECV) || (resultado == ERROR_RECV_DISCONNECTED)){
			printf("Error al recibir header del ESI \n");
			conexion_de_cliente_finalizada(fdEsi);
			int ret = EXIT_FAILURE;
			pthread_exit(&ret);
		}

		if(header.comando == msj_esi_finalizado){ //Si el ESI me indica que termino, cierro el hilo
			conexion_de_cliente_finalizada(fdEsi);
			printf("ESI ha finalizado, se interrumpe la conexion en fd %d\n", fdEsi); //TODO: Probablemente haya que hacer algo mas, hay que ver el enunciado.
			int ret = EXIT_FAILURE;
			pthread_exit(&ret);

		}
		esiActual = fdEsi;

		void* buffer = malloc(header.tamanio);
		resultado = recibir_mensaje(fdEsi,buffer,header.tamanio);

		if ((resultado == ERROR_RECV) || (resultado == ERROR_RECV_DISCONNECTED)){
			printf("Error al recibir payload del ESI \n");
			conexion_de_cliente_finalizada(fdEsi);
			int ret = EXIT_FAILURE;
			pthread_exit(&ret);
		}

		usleep(config.RETARDO); //Para retardo ficticio. OJO que va a los pedos pero esta bien

		switch (header.comando) {
			infoInstancia_t* instanciaConClave;
			char* clave;

			case msj_sentencia_get:

				log_trace(log_coordinador, "ESI %d realizo un GET clave %s",fdEsi,(char*) buffer);

				/* 1) Le pregunto al planificador si puede hacer el get de la clave indicada
				 *		1.1) Si puede, el PLANIFICADOR le avisa al ESI que ejecute la siguiente instruccion.
				 *		1.2) Si no puede, el planificador lo para y le dice a otro ESI que me mande una instruccion.
				 */
				clave = (char*)buffer;

				printf("Envio solicitud GET a planificador! %d\n", msj_solicitud_get_clave);
				enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_solicitud_get_clave);
				sem_wait(&semRespuestaPlanificador); //Hasta que el planificador no me responda, no continuo.
				log_debug(log_coordinador,"Despues semaforo: %d", okPlanificador);
				if(okPlanificador == msj_ok_solicitud_operacion){
					instanciaConClave = encontrar_instancia_por_clave(clave);

					if (instanciaConClave != 0){ //Distinto de cero indica que se encontro la clave

						if(instanciaConClave->desconectada == false){ //Si la instancia que tiene esa clave no esta desconectada
							printf("GET - Instancia con clave %s\n", instanciaConClave->nombre);
							operacion->keyword = GET;
							log_debug(log_coordinador,"Se realizo GET sobre clave ya existente pero no bloqueada");
							enviar_ok_sentencia_a_ESI();
						}else {
							printf("GET - No hay instancia con esa clave!\n");
							//Envio mensaje a planificador diciendo que el ESI debe abortar por tratar de ingresar a una clave en instancia desconectada.
							enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_error_clave_inaccesible);
						}
					}else { //Entro aqui si no encontre la clave en mi sistema
						printf("GET - No se encontro clave en el sistema!!\n");
						//Selecciono instancia victima segun algoritmo de distribucion
						instanciaConClave = elegir_instancia_por_algoritmo(config.ALGORITMO_DISTRIBUCION);

						//Agrego clave a la lista de claves que tiene la respectiva instancia.
						list_add(instanciaConClave->claves, clave);

						//Modifico estructuras en variable global operacion compartida para que tome la respectiva instancia.
						operacion = preparar_operacion_compartida_GET(clave);

						//Levanto el semaforo de la instancia seleccionada para que trabaje con variable global operacion compartida.
						sem_post(&instanciaConClave->semaforo);
					}
				}
			break;

			case msj_sentencia_set:

				log_trace(log_coordinador, "ESI %d realizo un SET clave %s",fdEsi,(char*) buffer);
				clave = (char*)buffer; //Anda de una porque como separamos con /0
				printf("clave = %s\n", clave);
				int fin = 0;
				int i = 0;
				int cantidadTerminator = 0;
				while (!fin) {
					if(clave[i] == '\0'){
						cantidadTerminator++;
					}
					printf("%c", clave[i]);
					fin = (cantidadTerminator == 2);
					i++;
				}
				printf("\n");
				instanciaConClave = encontrar_instancia_por_clave(clave);

				if (instanciaConClave != 0){ //Distinto de cero indica que se encontro la clave

					if(instanciaConClave->desconectada == false){ //Si la instancia que tiene esa clave no esta desconectada
						printf("SET - Instancia con clave %s\n", instanciaConClave->nombre);
						//Envio mensaje a planificador con pregunta si ESI tiene tomada la clave y verifico su respuesta en variable global okPlanificador.
						enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_esi_tiene_tomada_clave);
						sem_wait(&semRespuestaPlanificador);
						log_debug(log_coordinador,"Despues semaforo: %d", okPlanificador);

						if (okPlanificador == msj_ok_solicitud_operacion){

							//Modifico estructuras en variable global operacion compartida para que tome la respectiva instancia.
							operacion = preparar_operacion_compartida_SET(clave);

							//Levanto el semaforo de la instancia seleccionada para que trabaje con variable global operacion compartida.
							sem_post(&instanciaConClave->semaforo);
						}else{
							//El ESI no tiene geteada la clave para operar con SET o STORE. PLANI hace lo que tenga que hacer, COORDINADOR termina el hilo si el planificador contesta que no tenia bloqueada la clave..
							conexion_de_cliente_finalizada(fdEsi);
							printf("ESI ha finalizado por intentar acceder a una clave no bloqueada %d\n", fdEsi); //TODO: Probablemente haya que hacer algo mas, hay que ver el enunciado.
							int ret = EXIT_FAILURE;
							pthread_exit(&ret);
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

				log_trace(log_coordinador, "ESI %d realizo un STORE clave %s",fdEsi,(char*) buffer);
				clave = (char*)buffer;

				instanciaConClave = encontrar_instancia_por_clave(clave);

				if (instanciaConClave != 0){ //Distinto de cero indica que se encontro la clave

					if(instanciaConClave->desconectada == false){ //Si la instancia que tiene esa clave no esta desconectada

						//Envio mensaje a planificador con pregunta si ESI tiene tomada la clave y verifico su respuesta en variable global okPlanificador.
						enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_esi_tiene_tomada_clave);
						sem_wait(&semRespuestaPlanificador);
						log_debug(log_coordinador,"Despues semaforo: %d", okPlanificador);

						if (okPlanificador == msj_ok_solicitud_operacion){

							//Modifico estructuras en variable global operacion compartida para que tome la respectiva instancia.
							operacion = preparar_operacion_compartida_STORE(clave);

							//Levanto el semaforo de la instancia seleccionada para que trabaje con variable global operacion compartida.
							sem_post(&instanciaConClave->semaforo);

							//TODO: Avisarle al planificador que se libero la clave con STORE.

						}else{
							//El ESI no tiene geteada la clave para operar con SET o STORE. PLANI hace lo que tenga que hacer, COORDINADOR no toca nada, solo avisa al PLANI.
							conexion_de_cliente_finalizada(fdEsi);
							printf("ESI ha finalizado por intentar acceder a una clave no bloqueada %d\n", fdEsi); //TODO: Probablemente haya que hacer algo mas, hay que ver el enunciado.
							int ret = EXIT_FAILURE;
							pthread_exit(&ret);
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
				log_error(log_coordinador,"Se recibio comando desconocido desde ESI");
			break;

		} //Fin del switch

		//sem_post(&atenderInstancia); //Signal a semaforo para que sea el turno del hilo atender instancia.
	}
} //Fin del while

void* atender_accion_instancia(void* info) { // Puesto void* para evitar casteo en creacion de hilo.

	header_t* header = malloc(sizeof(header_t));
	void* buffer;
	void* bufferAEnviar;
	infoInstancia_t* miInstancia = malloc(sizeof(infoInstancia_t));
	infoParaHilo_t* informacionQueSeComparte = malloc(sizeof(infoParaHilo_t));
	informacionQueSeComparte = (infoParaHilo_t*) info;

	miInstancia = encontrar_instancia_por_fd(informacionQueSeComparte->fd);

	while(1){

		sem_wait(&miInstancia->semaforo); //Hago el wait al semaforo binario.

		//sem_wait(&atenderInstancia); //Semaforo bloqueante para orden

		if(miInstancia->desconectada == true){ //Si la instancia que atendia este hilo se desconecto, el hilo muere.
			sem_destroy(&miInstancia->semaforo);
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
					int index = strlen(operacion->argumentos.SET.claveYValor);
					char* valor = operacion->argumentos.SET.claveYValor + index + 1;
					header->tamanio = strlen(operacion->argumentos.SET.claveYValor) + strlen(valor) + 2; //los dos caracteres nulos

					bufferAEnviar = serializar(*header, operacion->argumentos.SET.claveYValor);
					enviar_mensaje(informacionQueSeComparte->fd, bufferAEnviar, sizeof(header_t) + header->tamanio);
					break;

				case STORE:
					header->comando = msj_sentencia_store;
					header->tamanio = strlen(operacion->argumentos.STORE.clave) + 1;

					bufferAEnviar = serializar(*header, operacion->argumentos.STORE.clave);
					enviar_mensaje(informacionQueSeComparte->fd,bufferAEnviar,sizeof(header_t) + header->tamanio);
				break;

				case COMPACTAR:
					header->comando = msj_instancia_compactar;
					header->tamanio = 0;
					log_debug(log_coordinador, "Envio orden de compactación");

					buffer = malloc(header->tamanio);

					bufferAEnviar = serializar(*header,buffer);
					enviar_mensaje(informacionQueSeComparte->fd,bufferAEnviar,sizeof(header_t));
				break;

				case STATUS:
					operacion->keyword = ultimaOperacion;

					header->comando = msj_status_clave;
					header->tamanio = strlen(statusClave) + 1;

					buffer = malloc(header->tamanio);

					bufferAEnviar = serializar(*header,buffer);
					enviar_mensaje(informacionQueSeComparte->fd,bufferAEnviar,sizeof(header_t) + header->tamanio);
					log_debug(log_coordinador, "Se envio mensaje de STATUS a INSTANCIA");
				break;

				default:
					printf("Le metieron cualquier cosa al keyword de la operacion compartida\n");
				break;
			}//Fin del switch

			//sem_post(&escucharInstancia); //Signal a semaforo para que sea el turno del hilo main.
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
	sem_init(&nueva_instancia->semaforo,0,0);

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

int posicion_de_clave_en_lista(t_list* unaListaDeClaves, char* claveBuscada){

	int posicion,i = 0;
	char* valor = string_new();

	for (i = 0; i < unaListaDeClaves->elements_count; ++i) {

		valor = list_get(unaListaDeClaves,i);

		if(string_equals_ignore_case(valor,claveBuscada)){
			posicion = i;
			return posicion;
		}
	}
	printf("La clave no esta en la lista dada\n");
	return -1; //Retorna -1 en caso que la clave no este en la lista
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
				printf("Se ha conectado un nuevo ESI al sistema en fd %d\n", socket_cliente);

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

				//Aumento cantidad de instancias conectadas al sistema.
				contadorDeInstancias++;

				//Agrego instancia a bolsa donde guardo todas las instancias.
				FD_SET(socket_cliente, &master);
				FD_SET(socket_cliente, &bolsa_instancias);

				//Envio cantidad y tamaño de entradas a la instancia.
				enviar_configuracion_entradas_a_instancia(&cabecera, &config, socket_cliente);

				//Pregunto si la instancia estaba conectada.
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
					FD_SET(socket_cliente, &bolsa_planificador);
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

	//sem_wait(&escucharInstancia); //Semaforo bloqueante para orden

	header_t header;
	int resultado = 0;
	int cantidadInstanciasConectadas = 0;
	int indiceClaveVieja = 0;
	t_list* instanciasConectadas;
	infoInstancia_t* instanciaQueSustituyo;
	infoInstancia_t* instanciaConClave;
	void* buffer;
	void* claveVieja;
	int tamanioValorClave = 0;
	int tamanioNombreInstanciaConClave = 0;
	int tamanioNombreInstanciaCandidata = 0;
	int tamanioParaBufferStatus = 0;



	resultado = recibir_mensaje(unFileDescriptor,&header,sizeof(header_t));
	if ((resultado == ERROR_RECV) || (resultado == ERROR_RECV_DISCONNECTED)){
		printf("Error al recibir header de INSTANCIA \n");
		conexion_de_cliente_finalizada(unFileDescriptor);
		exit(EXIT_FAILURE);
	}

	switch (header.comando) {

		case msj_instancia_compactar:
			log_trace(log_coordinador,"Una instancia ha solicitado compactacion");


			//Guardo la ultima sentencia enviada por un esi para enviar de vuelta a la instancia que solicito compactar.
			ultimaOperacion = operacion->keyword;
			fdQuePidioCompactacion = unFileDescriptor;

			//Seteo operacion->keyword (region compartida) en COMPACTAR
			operacion->keyword = COMPACTAR;

			//Filtro instancias conectadas al sistema para enviarles mensaje de "compactense".
			instanciasConectadas = filtrar_instancias_conectadas();

			//Levanto el semaforo de cada hilo que atiende a su instancia.
			signal_a_todos_los_semaforos_hiloInstancia(instanciasConectadas);
		break;

		case msj_instancia_compactacion_finalizada:
			log_debug(log_coordinador,"Una instancia finalizo su compactacion");
			//Recibo msj de compactacion OK. Incremento la cantidad de compactaciones hechas hasta que sea igual al total de instancias conectadas.
			compactaciones++;
			instanciasConectadas = filtrar_instancias_conectadas();
			cantidadInstanciasConectadas = instanciasConectadas->elements_count;

			if (compactaciones == cantidadInstanciasConectadas) {
				log_debug(log_coordinador,"Todas las instancias se han compactado correctamente");
				//Reseteo compactaciones para futura solicitud de compactacion.
				compactaciones = 0;

				//Cambio la operacion->keyword a la ultimaOperacion.
				operacion->keyword = ultimaOperacion;

				//Levanto el semaforo de la instancia que solicito compactacion para que vuelva a ejecutar sobre la operacion compartida.
				instanciaQuePidioCompactacion = encontrar_instancia_por_fd(fdQuePidioCompactacion);
				sem_post(&instanciaQuePidioCompactacion->semaforo);

//				//Notifico al PLANIFICADOR que ya se compactaron todas las instancias y hay que continuar la ejecucion.
//				enviar_mensaje_planificador(socket_planificador, &header, buffer,msj_compactacion_finalizada_continuar_planificacion);
			}
		break;

		case msj_instancia_sustituyo_clave:

			//Quitar clave sustituida de la lista de claves en la instancia
			claveVieja = malloc(header.tamanio);

			log_debug(log_coordinador, "Se ha sustituido una clave en la instancia conectada en fd %d",unFileDescriptor);
			recibir_mensaje(unFileDescriptor,claveVieja,header.tamanio);
			instanciaQueSustituyo = encontrar_instancia_por_fd(unFileDescriptor); //Encuentro la misma instancia que me mando el mensaje.

			indiceClaveVieja = posicion_de_clave_en_lista(instanciaQueSustituyo->claves,claveVieja);

			list_remove(instanciaQueSustituyo->claves,indiceClaveVieja);

			log_trace(log_coordinador,"Se ha reemplazado la clave %s del sistema", claveVieja);
		break;

		case msj_sentencia_finalizada:

			//Recibi ok de la instancia. Tengo que avisar al ESI que salio bien. Crear variable global que mantenga el fd del ESI que mando ultima operacion
			log_debug(log_coordinador,"Instancia ejecuto operacion correctamente");
			if (operacion->keyword == STORE){
				log_debug(log_coordinador,"Enviando LIBERAR CLAVES a Planificador. Clave %s", operacion->argumentos.STORE.clave);
				header.tamanio = strlen(operacion->argumentos.STORE.clave) + 1;
				buffer = malloc(header.tamanio);
				memcpy(buffer,operacion->argumentos.STORE.clave, header.tamanio);
				enviar_mensaje_planificador(socket_planificador, &header, buffer, msj_store_clave);
				//sem_wait(&semRespuestaPlanificador);
				//log_debug(log_coordinador,"Despues semaforo: %d", okPlanificador);
				free(buffer);
			}
			enviar_ok_sentencia_a_ESI();
		break;

		case msj_status_clave:

			//La data del valor de la clave
			recibir_mensaje(unFileDescriptor,statusValorClave,header.tamanio);
			tamanioValorClave = header.tamanio;

			//La data del nombre de la instancia
			instanciaConClave = encontrar_instancia_por_clave(statusValorClave);
			statusNombreInstanciaConClave = instanciaConClave->nombre;
			tamanioNombreInstanciaConClave = strlen(statusNombreInstanciaConClave) + 1;

			//La data de la clave candidata que se llena con vacio
			statusNombreInstanciaCandidata = "";
			tamanioNombreInstanciaCandidata = sizeof(char);

			//Armado de buffer para enviar a Planificador
			tamanioParaBufferStatus = sizeof(int) + tamanioValorClave + sizeof(int) + tamanioNombreInstanciaConClave + sizeof(int) + tamanioNombreInstanciaCandidata;

			void* buffer = malloc(tamanioParaBufferStatus);

			memcpy(buffer, &tamanioValorClave, sizeof(int));
			memcpy(buffer + sizeof(int), statusValorClave, sizeof(char));
			memcpy(buffer + sizeof(int) + sizeof(char), &tamanioNombreInstanciaConClave, sizeof(int));
			memcpy(buffer + sizeof(int) + sizeof(char) + sizeof(int),statusNombreInstanciaConClave, sizeof(char));
			memcpy(buffer + sizeof(int) + sizeof(char) + sizeof(int) + sizeof(char),&tamanioNombreInstanciaCandidata, sizeof(int));
			memcpy(buffer + sizeof(int) + sizeof(char) + sizeof(int) + sizeof(char) + sizeof(int), statusNombreInstanciaCandidata, tamanioNombreInstanciaCandidata);

			header.comando = msj_status_clave;
			header.tamanio = tamanioParaBufferStatus;
			enviar_mensaje_planificador(socket_planificador,&header,buffer,msj_status_clave);
		break;

		default:
			break;
	} //Fin del switch

	//sem_post(&atenderEsi); //Signal a semaforo para que sea el turno del hilo atender esi.
}

//void* preparar_buffer_status_clave(int tamanioValorClave,int tamanioNombreInstanciaConClave, int tamanioNombreInstanciaCandidata, int tamanioBuffer, char* statusValorClave, char* statusNombreInstanciaConClave, char* statusNombreInstanciaCandidata){
//
//	void* buffer = malloc(tamanioBuffer);
//
//	memcpy(buffer, &tamanioValorClave, sizeof(int));
//	memcpy(buffer + sizeof(int), statusValorClave, sizeof(char));
//	memcpy(buffer + sizeof(int) + sizeof(char), &tamanioNombreInstanciaConClave, sizeof(int));
//	memcpy(buffer + sizeof(int) + sizeof(char) + sizeof(int),statusNombreInstanciaConClave, sizeof(char));
//	memcpy(buffer + sizeof(int) + sizeof(char) + sizeof(int) + sizeof(char),&tamanioNombreInstanciaCandidata, sizeof(int));
//	memcpy(buffer + sizeof(int) + sizeof(char) + sizeof(int) + sizeof(char) + sizeof(int), statusNombreInstanciaCandidata, tamanioNombreInstanciaCandidata);
//
//	return buffer;
//}

void atender_mensaje_planificador(){

	header_t header;
	infoInstancia_t* instanciaConClave = malloc(sizeof(infoInstancia_t));
	infoInstancia_t* candidata = malloc(sizeof(infoInstancia_t));

	recibir_mensaje(socket_planificador,&header,sizeof(header_t));

	switch (header.comando) {


		case msj_status_clave:

			log_trace(log_coordinador,"Planificador ha solicitado el status de una clave");

			recibir_mensaje(socket_planificador,statusClave,header.tamanio);

			instanciaConClave = encontrar_instancia_por_clave(statusClave);

			if(instanciaConClave != 0){ //Si hay una instancia que tenga la clave, levanto el semaforo de su hilo para que envie el respectivo mensaje a la instancia.

				ultimaOperacion = operacion->keyword;

				operacion->keyword = STATUS;
				sem_post(&instanciaConClave->semaforo);

			}else{ //Ninguna instancia tiene esta clave. Ver instancia candidata y devolverle informacion al planificador.
				statusValorClave = "";
				int tamanioValorClave = sizeof(char);
				statusNombreInstanciaConClave = "";
				int tamanioNombreInstanciaConClave = sizeof(char);

				candidata = simular_eleccion_instancia_por_algoritmo(config.ALGORITMO_DISTRIBUCION);

				statusNombreInstanciaCandidata = candidata->nombre;
				int tamanioNombreInstanciaCandidata = strlen(statusNombreInstanciaCandidata) + 1;
				int tamanioTotalParaBuffer = sizeof(int) + sizeof(char) + sizeof(int) + sizeof(char) + sizeof(int) + tamanioNombreInstanciaCandidata;
				//El malloc es: tamanioValor + sizeof(char) para guardar \0 por valor vacio + tamanioNombreInstanciaConClave + sizeof(char) para guardar \0 por instancia vacia + tamanioNombreCandidata + longitudNombreCandidata

				void* buffer = malloc(tamanioTotalParaBuffer);

				memcpy(buffer, &tamanioValorClave, sizeof(int));
				memcpy(buffer + sizeof(int), statusValorClave, sizeof(char));
				memcpy(buffer + sizeof(int) + sizeof(char), &tamanioNombreInstanciaConClave, sizeof(int));
				memcpy(buffer + sizeof(int) + sizeof(char) + sizeof(int),statusNombreInstanciaConClave, sizeof(char));
				memcpy(buffer + sizeof(int) + sizeof(char) + sizeof(int) + sizeof(char),&tamanioNombreInstanciaCandidata, sizeof(int));
				memcpy(buffer + sizeof(int) + sizeof(char) + sizeof(int) + sizeof(char) + sizeof(int), statusNombreInstanciaCandidata, tamanioNombreInstanciaCandidata);

				header.tamanio = tamanioTotalParaBuffer;
				enviar_mensaje_planificador(socket_planificador,&header,buffer,msj_status_clave);
			}
		break;

		case msj_ok_solicitud_operacion:

			okPlanificador = msj_ok_solicitud_operacion;
			log_debug(log_coordinador, "Se recibió respuesta del planificador: %d", okPlanificador); //No necesito respuesta del planificador pero lo hago por forma generica de envio de OK en PLANI.

			sem_post(&semRespuestaPlanificador); //Levanto el semaforo con el que me esta esperando el hilo atender_accion_ESI.

		break;

		case msj_fail_solicitud_operacion:

			okPlanificador = msj_fail_solicitud_operacion;
			log_debug(log_coordinador, "Se recibió respuesta del planificador: %d", okPlanificador);

			sem_post(&semRespuestaPlanificador);//Levanto el semaforo con el que me esta esperando el hilo atender_accion_ESI.
		break;

		default:
			log_error(log_coordinador,"ERROR en el header enviado por el PLANIFICADOR. Probable bloqueo de todo el sistema");
		break;

	}

}

void inicializar_status(){

	statusClave = string_new();
	statusNombreInstanciaCandidata = string_new();
	statusNombreInstanciaConClave = string_new();
	statusValorClave = string_new();

}

int main(void) {

	//Inicializar semaforos para sincronizacion de hilos
	pthread_mutex_init(&mutexMaster,NULL);
	sem_init(&semRespuestaPlanificador,0,0);
	//sem_init(&atenderEsi,0,1);
	//sem_init(&atenderInstancia,0,0);
	//sem_init(&escucharInstancia,0,0);

	//Creo log de operaciones de los ESI
	log_coordinador = log_create("coordinador.log", "Coordinador", true, LOG_LEVEL_TRACE);

	lista_instancias_claves = list_create();

	inicializar_status();

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
		pthread_mutex_lock(&mutexMaster);
		read_fds = master;
		pthread_mutex_unlock(&mutexMaster);

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
				else if (FD_ISSET(fdCliente,&bolsa_planificador)){
						atender_mensaje_planificador();
				}
			}
		}
	} //Fin del bucle principal
}
