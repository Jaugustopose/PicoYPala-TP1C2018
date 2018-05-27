#ifndef SRC_PLANIFICACION_H_
#define SRC_PLANIFICACION_H_

#include "commons/collections/queue.h"
#include "commons/collections/list.h"

//Prototipos
bool esFIFO();
bool esSJFCD();
bool esSJFSD();
bool esHRRN();
void inicializarPlanificacion();
bool planificadorConDesalojo();
bool comparadorSJF(void* arg1, void* arg2);
bool comparadorHRRN(void* arg1, void* arg2);
void ordenarColaListos();
void planificarConDesalojo();
int procesoNuevo(int socketESI);
void procesoTerminado();
void procesoBloquear(char* clave);
void procesoDesbloquear(char* clave);
void incrementarRafagasEsperando();
void sentenciaFinalizada();
void solicitarClave(char* clave);
void liberarClave(char* clave);

#endif /* SRC_PLANIFICACION_H_ */
