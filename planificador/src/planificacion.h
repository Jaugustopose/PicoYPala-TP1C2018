#ifndef SRC_PLANIFICACION_H_
#define SRC_PLANIFICACION_H_

#include "commons/collections/queue.h"
#include "commons/collections/list.h"
#include "includes.h"

//Prototipos
void inicializarPlanificacion();
int procesoNuevo(int socketESI, char* nombre);
void procesoTerminado(int exitStatus);
void sentenciaFinalizada();
respuesta_operacion_t procesar_notificacion_coordinador(int comando, int tamanio, void* cuerpo, int socketCoordinador);
void bloquearEsiPorConsola(int idEsi, char* clave);
void listarRecursosBloqueadosPorClave(char* clave);
t_list* killProcesoPorID(int idProceso);
void analizarDeadlocks();
int fdProcesoEnEjecucion();
void bloquearClave(char* clave);
void procesoDesbloquear(char* clave);

sem_t planificacion_habilitada;
sem_t semaforo_coordinador;
pthread_mutex_t mutex_cola_listos;
pthread_mutex_t mutex_proceso_ejecucion;

#endif /* SRC_PLANIFICACION_H_ */
