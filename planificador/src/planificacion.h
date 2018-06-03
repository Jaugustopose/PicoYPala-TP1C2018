#ifndef SRC_PLANIFICACION_H_
#define SRC_PLANIFICACION_H_

#include "commons/collections/queue.h"
#include "commons/collections/list.h"

//Prototipos
void inicializarPlanificacion();
int procesoNuevo(int socketESI);
void procesoTerminado(int exitStatus);
void sentenciaFinalizada();
int procesar_notificacion_coordinador(int comando, int tamanio, void* cuerpo);

#endif /* SRC_PLANIFICACION_H_ */
