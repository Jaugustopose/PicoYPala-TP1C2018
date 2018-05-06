/*
 * esi.h
 *
 *  Created on: 3/5/2018
 *      Author: utnso
 */

#ifndef SRC_ESI_H_
#define SRC_ESI_H_

typedef struct{
	char *IP_PLANIFICADOR;
	int PUERTO_PLANIFICADOR;
	char *IP_COORDINADOR;
	int PUERTO_COORDINADOR;
}configuracion_t;

configuracion_t cargarConfiguracion();

#endif /* SRC_ESI_H_ */
