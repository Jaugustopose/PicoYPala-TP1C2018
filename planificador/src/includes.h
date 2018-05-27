#ifndef SRC_INCLUDES_H_
#define SRC_INCLUDES_H_

#include <stdio.h>
#include <stdlib.h>

#include "commons/collections/list.h"
#include "commons/collections/dictionary.h"
#include "commons/string.h"

typedef struct{
	int PUERTO;
	char* ALGORITMO_PLANIFICACION;
	int ALFA_PLANIFICACION;
	int ESTIMACION_INICIAL;
	char* IP_COORDINADOR;
	int PUERTO_COORDINADOR;
	char** CLAVES_BLOQUEADAS;
}configuracion_t;

typedef struct{
	int idProceso;
	int socketESI;
	t_list* clavesBloqueadas;
	char* claveBloqueo;
	int rafagaEstimada;
	int rafagaActual;
	int rafagasEsperando;
}proceso_t;

typedef struct {
	int socket_escucha_esis;
	int socket_coordinador;
} sockets_escucha_t;

#endif /* SRC_INCLUDES_H_ */
