#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "comunicacion/comunicacion.h"
#include "planificacion.h"
#include "includes.h"

void procesar_handshake(int socketCliente) {
	header_t cabecera;
	int identificacion;
	//Recibimos Header con info de operación
	int resultado = recibir_mensaje(socketCliente, &cabecera, sizeof(header_t));
	if (resultado == ERROR_RECV) {
		printf("Error en el recv para socket %d!!!\n", socketCliente); //TODO Manejar el error de cierta forma si queremos.
	} else {
		//Recibimos identificación del handshake
		resultado = recibir_mensaje(socketCliente, &identificacion, cabecera.tamanio);
		if(resultado == ERROR_RECV) {
			printf("Error en el recv para socket %d al hacer handshake!!!\n", socketCliente); //TODO Manejar el error de cierta forma si queremos.
		} else {
			//Respondemos el ok del handshake
			responder_ok_handshake(Planificador, socketCliente);
		}
	}
}

/*TODO
 * Es importante destacar que la operación GET generará una clave sin valor y ademas modifica el estado de bloqueos y desbloqueos en el planificador.
 * Es el SET el encargado de alterar el valor.
 */

void procesar_notificacion_coordinador(int socket_coordinador) {
//	paquete_t paquete = recibirPaquete(socket_coordinador);

	//TODO Recibir notificación de bloqueo de ESI

	//TODO Recibir notificación de Bloqueo de Clave de recurso por Parte de ESI (GET). Procesar, Si está tomada informar bloqueo de ESI, si no, sólo almacenar resultado

	//TODO Recibir notificación de inexistencia de Instancia. Requiere abortar al ESI en cuestión.
}

void* iniciarEscucha(void* sockets) {
	sockets_escucha_t sockets_predefinidos = *(sockets_escucha_t*) sockets;

	fd_set master; // fdset de los procesos ESI conectados
	fd_set read_fds; // fdset temporal para pasar al select
	//Añadir listener al conjunto maestro
	FD_ZERO(&master);
	FD_SET(sockets_predefinidos.socket_escucha_esis, &master);
	//Mantener actualizado cual es el maxSock
	int maxFd = sockets_predefinidos.socket_escucha_esis;
	int fdCliente;
	int socketCliente;
	int bytesRecibidos;

	//Bucle principal
	for (;;) {
		read_fds = master;
		if (select(maxFd + 1, &read_fds, NULL, NULL, NULL) == -1) { //Compruebo si algun cliente quiere interactuar.
			printf("Error en select\n");
			exit(1);
		}
		for (fdCliente = 0; fdCliente <= maxFd; fdCliente++) {
			if (FD_ISSET(fdCliente, &read_fds)) { // Me fijo si tengo datos listos para leer
				if (fdCliente == sockets_predefinidos.socket_escucha_esis) {
					socketCliente = aceptar_conexion(sockets_predefinidos.socket_escucha_esis);
					if (socketCliente == ERROR_ACCEPT) {
						printf("Error en el accept\n"); //TODO Deberiamos tomar el error y armar un exit_gracefully como en el tp0.
						exit(ERROR_ACCEPT);
					} else {
						// Procesamos el handshake e ingresamos el nuevo ESI en el sistema. Siempre será un ESI, el handshake con coordinador se
						// hace al revés: Planificador se conectará a Coordinador previamente.
						procesar_handshake(socketCliente);
						FD_SET(socketCliente, &master);
						//TODO Analizar si es necesario sincronizar. En principio el select es secuencial así que no. Pero ver si algún comando
						//     de consola podría llamar a este método procesoNuevo.
						int retorno = procesoNuevo(socketCliente);
						if (retorno < 0) {
							//TODO: Revisar manejo de error. Sacar de los SET y listo.
						}
					}

				} else if (fdCliente == sockets_predefinidos.socket_coordinador) {
					// El coordinador está notificando de inexistencia de clave, bloqueo de clave o de ESI
					// Handshake hecho previo a ingresar al Select.
					procesar_notificacion_coordinador(sockets_predefinidos.socket_coordinador);

				} else {
					header_t header;
					bytesRecibidos = recibir_mensaje(socketCliente, &header, sizeof(header_t));
					if (bytesRecibidos == ERROR_RECV_DISCONNECTED || bytesRecibidos == ERROR_RECV_DISCONNECTED) {
						//TODO: ESI se desconectó. Sacar de los FD, el close del socket ya lo hizo el recibir_mensaje.
						FD_CLR(fdCliente, &master);
					} else {
						switch(header.comando) {
						case sentencia_finalizada:
							if (planificadorConDesalojo()) {
								planificarConDesalojo();
							} else {
								//Mandamos a ejecutar siguiente ráfaga
								mandar_a_ejecutar_esi(socketCliente);
							}
							break;
						case esi_finalizado:
							procesoTerminado();
							break;
						}
					}
				}

			}
		}
	}

	return EXIT_SUCCESS;
}

int mandar_a_ejecutar_esi(int socket_esi) {
	header_t header;
	header.comando = requerimiento_ejecucion;
	header.tamanio = 0;
	int retorno = enviar_mensaje(socket_esi, &header, 0);
	if (retorno < 0) {
		printf("Problema con el ESI en el socket %d. Se cierra conexión con él.\n", socket_esi);
	}

	//TODO: Qué hacemos acá? Se podría mandar el ESI a la cola de finalizados, pero no sería un proceso terminado normalmente.
	// Si se van a sacar estadísticas con esa cola no convendría (no sé si se menciona eso). Pero por otro lado en este issue:
	// https://github.com/sisoputnfrba/foro/issues/1037 se menciona "de cualquier estado se puede pasar a "finalizados" si se mata un proceso."
	// Que lo maneje el select una vez que escalen hacia arriba los retornos de errores y lleguen
	return retorno;
}

//void conexion_de_cliente_finalizada() {
//	// conexión cerrada.
//	printf("Server: socket %d termino la conexion\n", fdCliente);
//	// Eliminar del conjunto maestro y su respectiva bolsa.
//	FD_CLR(fdCliente, &master);
//	if (FD_ISSET(fdCliente, &bolsa_esis)) {
//		FD_CLR(fdCliente, &bolsa_esis);
//		printf("Se desconecto esi del socket %d\n", fdCliente); //TODO: Probablemente haya que hacer algo mas, hay que ver el enunciado.
//
//	} else if (FD_ISSET(fdCliente, &bolsa_instancias)){
//		FD_CLR(fdCliente, &bolsa_instancias);
//		printf("Se desconecto instancia del socket %d\n", fdCliente); //TODO: Reorganizar la distribucion para las claves.
//	} else {
//		FD_CLR(fdCliente, &bolsa_planificador);
//		printf("Se desconecto el planificador\n");
//	}
//
//	close(fdCliente); // Si se perdio la conexion, la cierro.
//}
