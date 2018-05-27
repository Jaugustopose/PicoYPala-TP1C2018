#ifndef SRC_PLANIFICACION_H_
#define SRC_PLANIFICACION_H_

#include "commons/collections/queue.h"
#include "commons/collections/list.h"

void inicializarPlanificacion();
void procesoNuevo(int socketESI);
void sentenciaFinalizada();
void procesoTerminado();
void solicitarClave(char* clave);
void liberarClave(char* clave);


#endif /* SRC_PLANIFICACION_H_ */
