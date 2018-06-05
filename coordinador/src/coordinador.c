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
#include <stdbool.h>

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
}infoInstancia_t;

typedef struct infoEsi_t{
	int fdESI;
	t_list* claves_tomadas; // Lista tiene las claves tomadas por el esi
}infoEsi_t;

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
int max_instancias = 0;
int puntero_algoritmo_equitative = 0;
t_list* lista_instancias_claves;
t_list* lista_esis_permisos_setear;





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

void identificar_proceso_e_ingresar_en_bolsa(int socket_cliente) {

	//Recibo identidad y coloco en la bolsa correspondiente.
	header_t cabecera;
	int identificacion;
	infoInstancia_t* nueva_instancia;
	infoInstancia_t* instancia_existente;
	char* nombre_instancia;
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
				FD_SET(socket_cliente, &master);
				FD_SET(socket_cliente, &bolsa_esis); //Agrego un nuevo esi a la bolsa de esis.
				printf("Se ha conectado un nuevo esi \n");
				responder_ok_handshake(ESI, socket_cliente);
				break;

			case Instancia:

				FD_SET(socket_cliente, &master);
				FD_SET(socket_cliente, &bolsa_instancias); //Agrego una nueva instancia a la bolsa de instancias.
				printf("Se ha conectado una nueva instancia de ReDis \n");

				resultado = recibir_mensaje(socket_cliente, &cabecera, sizeof(header_t)); //Ahora recibo el nombre de la instancia
				if ((resultado == ERROR_RECV) || !(cabecera.comando == msj_nombre_instancia)) { //Si hay error en recv o cabecera no dice msj_nombre_instancia
					printf("Error al intentar recibir nombre de la instancia\n");
				} else {
					recibir_mensaje(socket_cliente, &nombre_instancia, cabecera.tamanio);
				}

				instancia_existente = instancia_conectada_anteriormente(nombre_instancia);
				if (instancia_existente == 0) { //No existia anteriormente en el sistema.
					nueva_instancia->nombre = nombre_instancia;
					nueva_instancia->fd = socket_cliente;
					nueva_instancia->espacio_disponible = config.CANT_ENTRADAS;
					nueva_instancia->desconectada = false;
					nueva_instancia->letra_inicio = '-';
					nueva_instancia->letra_fin = '-';
					nueva_instancia->claves = list_create();

					list_add(lista_instancias_claves,nueva_instancia); //Agrego nueva instancia a la lista de instancias conectadas al sistema.

				}else { //Para cuando una instancia se quiere reincorporar.

					instancia_existente->desconectada = false;

					//TODO: Reincorporo la instancia al sistema. Ver tema de Dump en el enunciado: seccion "Almacenamiento". Probablemente tenga que enviar un mensaje a la instancia para que recupere su info.
				}

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

void* encontrar_esi_en_lista(int unESI){
	//Funcion de ayuda solo dentro de este scope
	int mismo_numero (infoEsi_t* p){
		return p->fdESI == unESI;
	}

	return list_find(lista_esis_permisos_setear, (void*) mismo_numero);
}

infoInstancia_t* elegir_instancia_por_algoritmo(char* algoritmo){ //El warning sale porque no estan imlementados LSU y KEY.

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

void* encontrar_clave (char* unaClave){

	//Funcion de ayuda solo dentro de este scope
	int misma_clave (char* p){
		return string_equals_ignore_case(p,unaClave);
	}

	int i;
	void* instanciaConlistaDeClaves;
	void* instanciaConClave;

	for (i = 0; i < lista_instancias_claves->elements_count; ++i) {

		instanciaConlistaDeClaves = list_get(lista_instancias_claves,i);
		instanciaConClave = list_find(instanciaConlistaDeClaves,(void*) misma_clave);

		if(instanciaConClave != NULL){
			return list_get(lista_instancias_claves,i);
		}
	}
		return 0; // Devuelvo 0 si no tenia la clave en ninguna instancia
}

void* esi_con_clave(int unESI, char* unaClave){
	void* laTieneTomada;
	//Funcion de ayuda solo dentro de este scope
		int mismo_numero (infoEsi_t* p){
			return p->fdESI == unESI;
		}

		int misma_clave (char* p){
				return (p == unaClave);
		}

	infoEsi_t* esiBuscado;
	esiBuscado = list_find(lista_esis_permisos_setear, (void*) mismo_numero);

	laTieneTomada = list_find(esiBuscado->claves_tomadas,(void*) misma_clave);

	if(laTieneTomada != NULL){
		return esiBuscado;
	}else {
		return NULL;
	}
}

void atender_accion_esi(int fdEsi) {
	int codAccion;
//	t_esi_operacion sentencia_esi;
	int operacion;
	int tamanioTotal;
//	int tamanioClave;
	char* claveValor;
	char* clave;
//	int tamanioValor;


	int resultado;
	printf("Atendiendo acción esi en socket %d!!!\n", fdEsi);

	recibir_mensaje(fdEsi,&codAccion,sizeof(int));

	if (codAccion != msj_instruccion_esi){
		printf("Error al recibir instruccion del ESI\n");
	} else {

		resultado = recibir_mensaje(fdEsi, &operacion, sizeof(operacion));

		if ((resultado == ERROR_RECV) || (resultado == ERROR_RECV_DISCONNECTED)){
			printf("Error al recibir instruccion del ESI\n");
		} else {

			//Recibimos el tamanio total que puede representar solo la clave o clave valor (dependiendo de si es GET/STORE o SET)
			recibir_mensaje(fdEsi, &tamanioTotal, sizeof(tamanioTotal));

			switch (operacion) {
				header_t header;
				infoEsi_t* esiAAgregarClave;
				infoInstancia_t* instanciaConClave;
				infoEsi_t* esiConClave;
				void* bufferAEnviar;

				case get:

					/* 1) Le pregunto al planificador si puede hacer el get de la clave indicada
					 * 		1.1) Si me responde con OK, continuo con 2)
					 * 		1.2) Si me responde con NO, corto.
					 */
					recibir_mensaje(fdEsi, clave, tamanioTotal);
					header.comando = msj_solicitud_get_clave;
					header.tamanio = tamanioTotal;
					bufferAEnviar = serializar(header, clave);

					enviar_mensaje(socket_planificador, bufferAEnviar, sizeof(header_t) + header.tamanio);
					free(bufferAEnviar);
					recibir_mensaje(socket_planificador, &header, sizeof(header_t));

					if (header.comando == msj_clave_permitida_para_get){ //Necesito que el planificador me diga si puede hacer el GET o no.

						instanciaConClave = encontrar_clave(clave);

						if (instanciaConClave != 0){ //Distinto de cero indica que se encontro la clave

							if(instanciaConClave->desconectada == false){ //Si la instancia que tiene esa clave esta desconectada
								esiAAgregarClave = encontrar_esi_en_lista(fdEsi);
								list_add(esiAAgregarClave->claves_tomadas, clave); //Agrego clave al ESI para controlar que primero hizo GET antes de SET/STORE.
								//TODO ¿Es necesario agregar a lista las claves tomadas por el ESI? ¿No dijimos que iba a preguntarle al planificador?
							}else {
								//Abortar el esi con error "instancia desconectada" y notificar al usuario. TODO: Enviar al PLANI el problema este.
							}
						}else { //Entro aqui si no encontre la clave en mi sistema

							instanciaConClave = elegir_instancia_por_algoritmo(config.ALGORITMO_DISTRIBUCION); //Selecciono instancia victima segun algoritmo de distribucion

							header.comando = msj_sentencia_get;
							header.tamanio = tamanioTotal;
							bufferAEnviar = serializar(header, clave);

							enviar_mensaje(instanciaConClave->fd, bufferAEnviar, sizeof(header_t) + (header.tamanio));
							free(bufferAEnviar);
							list_add(instanciaConClave->claves, clave);
						}
					} else {
						//Corto
					}

					 /* 2) Verificar si existe clave en mi lista interna (lista_instancias_claves) LISTO
					 * 		2.1) Si existe:
					 * 			2.1.1) Agrego clave tomada por el esi en mi lista interna (lista_esis_permisos_setear) LISTO
					 *
					 * 		2.2) Si existe pero la instancia que la posee esta desconectada:
					 * 			2.2.1) Notifico al usuario y aborto el ESI. TODO
					 *
					 *		2.3) Si no existe:
					 *			2.3.1) Le digo a la instancia correspondiente que cree la clave (segun algoritmo de distribucion que tenga) LISTO
					 *			2.3.2) Agrego la clave a la lista de claves que tiene la instancia en mi lista interna (lista_instancias_claves) LISTO
					 *
					 */
					break;
				case set:
					// TODO: Error de clave no identificada. Hacer un if que pregunte si la clave existe esta en el sistema.
					recibir_mensaje(fdEsi, claveValor, tamanioTotal);
					char** substrings = string_split(claveValor, "\0"); //TODO Ojo con esto. ¿Es lo mejor?
					clave = substrings[0]; //Solo uso la clave cuando separo porque es lo unico que requiere validacion
//					char* valor = substrings[1];
					esiConClave = esi_con_clave(fdEsi, clave);

					if (esiConClave != NULL) {

						instanciaConClave = encontrar_clave(clave);

						header.comando = msj_sentencia_set;
						header.tamanio = tamanioTotal;
						bufferAEnviar = serializar(header, claveValor);

						enviar_mensaje(instanciaConClave->fd, bufferAEnviar, sizeof(header_t) + (header.tamanio));
						free(bufferAEnviar);
					} else {
						header.comando = msj_inexistencia_clave;
						header.tamanio = 0;
						void* buffer = malloc(0);
						bufferAEnviar = serializar(header, buffer);

						enviar_mensaje(socket_planificador, bufferAEnviar, sizeof(header_t) + (header.tamanio)); //Envio mensaje al PLANIFICADOR para que aborte el esi por error clave inexistente.
						free(bufferAEnviar);
					}
					/* 1) Pregunto si esi tiene la clave geteada LISTO
					 * 		1.1) Si la tiene, paso instruccion a instancia segun algoritmo de distribucion LISTO
					 * 		1.2) Si no la tiene, informar que primero tiene que hacer el get. LISTO
					 */
					break;
				case store:

					recibir_mensaje(fdEsi, clave, tamanioTotal);
					esiConClave = esi_con_clave(fdEsi, clave);

					if (esiConClave != NULL) {

						instanciaConClave = encontrar_clave(clave);

						header.comando = msj_sentencia_store;
						header.tamanio = tamanioTotal;
						bufferAEnviar = serializar(header, clave);

						enviar_mensaje(instanciaConClave->fd, bufferAEnviar, sizeof(header_t) + (header.tamanio));
						free(bufferAEnviar);
						//Funcion de ayuda solo dentro de este scope
						int misma_clave_prefijada (char* p){
							return string_equals_ignore_case(p, clave);
						};
						list_remove_by_condition(esiConClave->claves_tomadas, (void*) misma_clave_prefijada); // Borro la clave de la lista de claves tomadas por el ESI.
					}else {
						header.comando = msj_inexistencia_clave;
						header.tamanio = 0;
						void* buffer = malloc(0);
						bufferAEnviar = serializar(header,buffer);

						enviar_mensaje(socket_planificador,bufferAEnviar,sizeof(header_t)+(header.tamanio)); //Envio mensaje al PLANIFICADOR para que aborte el esi por error clave inexistente.
					}
					break;
				default:
					break;
			}
		}
	}
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
						printf("Error en el accept\n");
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
						atender_accion_esi(fdCliente);
					} else if (FD_ISSET(fdCliente, &bolsa_instancias)) { // EN CASO DE QUE EL MENSAJE LO HAYA ENVIADO UNA INSTANCIA.
						atender_accion_instancia(fdCliente);
					} else {
						perror("El socket buscado no está en ningún set!!! Situación anómala. Finalizando...");
						exit(EXIT_FAILURE);
					}
				}
			}
		}
	}
}
