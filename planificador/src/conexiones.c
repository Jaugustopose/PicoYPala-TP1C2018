#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "comunicacion/comunicacion.h"

//TODO Más adelante devolver el fd del socket donde tenemos abierta la comunicación con Coordinador para futuros mensajes
int conectarConCoordinador(char* ip, int puerto){
	int socketCoordinador = conectar_a_server(ip, puerto);
	//Preparo mensaje handshake
	header_t header;
	header.comando = handshake;
	header.tamanio = sizeof(int);
	int cuerpo = Planificador;
	int tamanio = sizeof(header_t) + sizeof(header.tamanio);
	void* buff = malloc(tamanio);
	memcpy(buff, &header, sizeof(header_t));
	memcpy(buff + sizeof(header_t), &cuerpo, sizeof(int));

	//Envio mensaje handshake
	enviar_mensaje(socketCoordinador,buff,tamanio);
	//Libero Buffer
	free(buff);
	//Recibo Respuesta del Handshake
	paquete_t paquete = recibirPaquete(socketCoordinador);
	if(paquete.header.comando == handshake && *(int*)paquete.cuerpo == Coordinador)
		printf("Conectado correctamente al coordinador\n");

	return socketCoordinador;

}

void iniciarEscucha(int socketEscucha){
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
					FD_SET(socketCliente, &master); // Añadir al fdset
				} else {
					// Recibir mensaje de un ESI
					paquete_t paquete = recibirPaquete(socketCliente);
				}
			}
		}
	}
}
