#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "comunicacion/comunicacion.h"

typedef struct{
	header_t header;
	void* cuerpo;
}paquete_t;

paquete_t recibirPaquete(int socket){
	//TODO Manejar error que puede devolver recibir_mensaje
	paquete_t paquete;
	recibir_mensaje(socket,&paquete.header,sizeof(header_t));
	paquete.cuerpo = malloc(paquete.header.tamanio);
	recibir_mensaje(socket,paquete.cuerpo,paquete.header.tamanio);
	return paquete;
}

//TODO Más adelante devolver el fd del socket donde tenemos abierta la comunicación con Coordinador para futuros mensajes
void conectarConCoordinador(char* ip, int puerto){
	int socketCordinador = conectar_a_server(ip, puerto);
	//Preparo mensaje handshake
	header_t header;
	header.comando=handshake;
	header.tamanio=1;
	int cuerpo = Planificador;
	int tamanio = sizeof(header_t)+sizeof(int);
	void* buff = malloc(tamanio);
	memcpy(buff,&header,sizeof(header_t));
	memcpy(buff+sizeof(header_t),&cuerpo,sizeof(int));
	//Envio mensaje handshake
	enviar_mensaje(socketCordinador,buff,tamanio);
	//Libero Buffer
	free(buff);
	//Recibo Respuesta del Handshake
	paquete_t paquete = recibirPaquete(socketCordinador);
	if(paquete.header.comando==handshake && *(int*)paquete.cuerpo==Coordinador)
		printf("Conectado correctamente al coordinador");
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
			printf("Error en select");
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
