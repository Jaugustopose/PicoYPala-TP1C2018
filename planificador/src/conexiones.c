#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "comunicacion/comunicacion.h"

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

void* iniciarEscucha(void* socket) {
	int socketEscucha = *(int*)socket;
	fd_set master; // fdset de los procesos ESI conectados
	fd_set read_fds; // fdset temporal para pasar al select
	//Añadir listener al conjunto maestro
	FD_ZERO(&master);
	FD_SET(socketEscucha, &master);
	//Mantener actualizado cual es el maxSock
	int maxFd = socketEscucha;
	int fdCliente;
	int socketCliente;

	//Bucle principal
	for (;;) {
		read_fds = master;
		if (select(maxFd + 1, &read_fds, NULL, NULL, NULL) == -1) { //Compruebo si algun cliente quiere interactuar.
			printf("Error en select\n");
			exit(1);
		}
		for (fdCliente = 0; fdCliente <= maxFd; fdCliente++) {
			if (FD_ISSET(fdCliente, &read_fds)) { // Me fijo si tengo datos listos para leer
				if (fdCliente == socketEscucha) {
					//Acepto conexion nueva
					//TODO Manejar error que puede devolver aceptar_conexion
					socketCliente = aceptar_conexion(socketEscucha);
					if (socketCliente == ERROR_ACCEPT) {
						printf("Error en el accept\n"); //TODO Deberiamos tomar el error y armar un exit_gracefully como en el tp0.
					} else {
						//Recibo identidad y coloco en la bolsa correspondiente.
						procesar_handshake(socketCliente);
					}
					FD_SET(socketCliente, &master); // Añadir al fdset
				} else {
					// Recibir mensaje de un ESI
					paquete_t paquete = recibirPaquete(socketCliente);
				}
			}
		}
	}
	return 0;
}
