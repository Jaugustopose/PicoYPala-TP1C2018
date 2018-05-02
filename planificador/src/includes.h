#ifndef SRC_INCLUDES_H_
#define SRC_INCLUDES_H_

#include "commons/collections/list.h"

typedef struct{
	int PUERTO;
	char* ALGORITMO_PLANIFICACION;
	int ESTIMACION_INICIAL;
	char* IP_COORDINADOR;
	int PUERTO_COORDINADOR;
	char** CLAVES_BLOQUEADAS;
}configuracion_t;

typedef struct{
	int socketESI;
	t_list* clavesBloqueadas;
	char* claveBloqueo;
}proceso_t;

#endif /* SRC_INCLUDES_H_ */
