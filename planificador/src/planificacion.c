#include "planificacion.h"
#include "includes.h"
#include <stdbool.h>
#include <string.h>

t_queue* colaListos;
t_queue* colaEjecucion;
t_queue* colaTerminados;
t_list* listaBloqueados;

extern configuracion_t config;
const char *FIFO = "FIFO";
const char *SJFCD= "SJF-CD";
const char *SJFSD= "SJF-SD";
const char *HRRN= "HRRN";

void inicializarPlanificacion(){
	colaListos = queue_create();
	colaEjecucion = queue_create();
	colaTerminados = queue_create();
	listaBloqueados = list_create();
}

void planificar(){
	//Aca se van a ejecutar los diferentes algoritmos de planificación
	if(strcmp(config.ALGORITMO_PLANIFICACION, FIFO)==0){
		//TODO IMPLEMENTAR FIFO
	}else if(strcmp(config.ALGORITMO_PLANIFICACION, SJFCD)==0){
		//TODO IMPLEMENTAR SJF CON DESALOJO
	}else if(strcmp(config.ALGORITMO_PLANIFICACION, SJFSD)==0){
		//TODO IMPLEMENTAR SJF SIN DESALOJO
	}else if(strcmp(config.ALGORITMO_PLANIFICACION, HRRN)==0){
		//TODO IMPLEMENTAR HRRN
	}
}

void procesoListo(proceso_t* proceso){
	queue_push(colaListos, (void*)proceso);
	planificar();
}

void procesoEjecucion(proceso_t* proceso){
	queue_push(colaEjecucion, (void*)proceso);
}

void procesoTerminado(proceso_t* proceso){
	queue_push(colaTerminados, (void*)proceso);
	planificar();
}

void procesoBloquear(proceso_t* proceso, char* clave){
	proceso->claveBloqueo=clave;
	list_add(listaBloqueados, (void*)proceso);
	planificar();
}

void procesoDesbloquear(char* clave){
	int i;
	for(i=0;i<list_size(listaBloqueados);i++){
		proceso_t* proceso;
		proceso = (proceso_t*)list_get(listaBloqueados, i);
		if(strcmp(proceso->claveBloqueo, clave)==0){
			list_remove(listaBloqueados, i);
			procesoListo(proceso);
			break;
		}
	}
}

