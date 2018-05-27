#include "planificacion.h"
#include "includes.h"
#include <stdbool.h>
#include <string.h>

t_list* colaListos;
proceso_t* procesoEjecucion;
t_queue* colaTerminados;
t_list* listaBloqueados;
t_dictionary claves;

extern configuracion_t config;
int contadorProcesos = 1;

//Prototipos
void procesoEjecutar(proceso_t* proceso);
void colaListosPush(proceso_t* proceso);
proceso_t* colaListosPop();
proceso_t* colaListosPeek();

//Funciones
bool esFIFO(){
	char *FIFO = "FIFO";
	return strcmp(config.ALGORITMO_PLANIFICACION, FIFO)==0;
}

bool esSJFCD(){
	char *SJFCD= "SJF-CD";
	return strcmp(config.ALGORITMO_PLANIFICACION, SJFCD)==0;
}

bool esSJFSD(){
	char *SJFSD= "SJF-SD";
	return strcmp(config.ALGORITMO_PLANIFICACION, SJFSD)==0;
}

bool esHRRN(){
	char *HRRN= "HRRN";
	return strcmp(config.ALGORITMO_PLANIFICACION, HRRN)==0;
}

void inicializarPlanificacion(){
	colaListos = list_create();
	procesoEjecucion = 0;
	colaTerminados = queue_create();
	listaBloqueados = list_create();
}

bool planificadorConDesalojo(){
	return esSJFCD() || esHRRN();
}

bool comparadorSJF(void* arg1, void* arg2){
	proceso_t* proceso1 = (proceso_t*)arg1;
	proceso_t* proceso2 = (proceso_t*)arg2;
	return proceso1->rafagaEstimada <= proceso2->rafagaEstimada;
}

bool comparadorHRRN(void* arg1, void* arg2){
	proceso_t* proceso1 = (proceso_t*)arg1;
	proceso_t* proceso2 = (proceso_t*)arg2;
	return (1 + proceso1->rafagasEsperando/proceso1->rafagaEstimada) >= (1 + proceso2->rafagasEsperando/proceso2->rafagaEstimada);
}

void ordenarColaListos(){
	if(esHRRN()){
		list_sort(colaListos, &comparadorHRRN);
	}else if(esFIFO()){
		//No ordena
	}else{
		list_sort(colaListos, &comparadorSJF);
	}

}

void planificarConDesalojo(){
	if(esHRRN()){
		proceso_t* proceso1 = colaListosPeek();
		proceso_t* proceso2 = procesoEjecucion;
		bool cambiarProceso = (1 + proceso1->rafagasEsperando/proceso1->rafagaEstimada) > (1 + proceso2->rafagasEsperando/proceso2->rafagaEstimada);
		if(cambiarProceso){
			colaListosPush(procesoEjecucion);
			procesoEjecutar(colaListosPop());
		}
	}else{
		proceso_t* proceso1 = colaListosPeek();
		proceso_t* proceso2 = procesoEjecucion;
		bool cambiarProceso = proceso1->rafagaEstimada < proceso2->rafagaEstimada;
		if(cambiarProceso){
			colaListosPush(procesoEjecucion);
			procesoEjecutar(colaListosPop());
		}
	}
}

void colaListosPush(proceso_t* proceso){
	list_add(colaListos, (void*)proceso);
	ordenarColaListos();
}

proceso_t* colaListosPop(){
	return list_remove(colaListos, 0);
}

proceso_t* colaListosPeek(){
	return list_get(colaListos, 0);
}

void procesoNuevo(int socketESI){
	proceso_t* proceso = malloc(sizeof(proceso_t));
	proceso->idProceso = contadorProcesos;
	contadorProcesos++;
	proceso->claveBloqueo = string_new();
	proceso->clavesBloqueadas = list_create();
	proceso->rafagaActual = 0;
	proceso->rafagaEstimada = config.ESTIMACION_INICIAL;
	proceso->rafagasEsperando = 0;
	proceso->socketESI = socketESI;
	if(procesoEjecucion == 0){
		procesoEjecutar(proceso);
	}else{
		colaListosPush(proceso);
		if(planificadorConDesalojo()){
			planificarConDesalojo();
		}
	}
}

void procesoEjecutar(proceso_t* proceso){
	procesoEjecucion = proceso;
	//TODO: MANDAR AL ESI CORRESPONDIENTE EL COMANDO PARA QUE EJECUTE LA PRIMER SENTENCIA;
}

void procesoTerminado(){
	queue_push(colaTerminados, (void*)procesoEjecucion);
	procesoEjecutar(colaListosPop());
}

void procesoBloquear(char* clave){
	procesoEjecucion->claveBloqueo=clave;
	list_add(listaBloqueados, (void*)procesoEjecucion);
	procesoEjecutar(colaListosPop());
}

void procesoDesbloquear(char* clave){
	int i;
	for(i=0;i<list_size(listaBloqueados);i++){
		proceso_t* proceso;
		proceso = (proceso_t*)list_get(listaBloqueados, i);
		if(strcmp(proceso->claveBloqueo, clave)==0){
			list_remove(listaBloqueados, i);
			colaListosPush(proceso);
			if(planificadorConDesalojo()){
				planificarConDesalojo();
			}
			break;
		}
	}
}

void incrementarRafagasEsperando(){
	int i;
	for(i=0;i<list_size(colaListos);i++){
		proceso_t* proceso;
		proceso = (proceso_t*)list_get(colaListos, i);
		proceso->rafagasEsperando++;
	}
}

void sentenciaFinalizada(){
	procesoEjecucion->rafagaActual++;
	if(esHRRN()){
		incrementarRafagasEsperando();
	}
	if(procesoEjecucion->rafagaActual == procesoEjecucion->rafagaEstimada){
		colaListosPush(procesoEjecucion);
		procesoEjecutar(colaListosPop());
	}else{
		//TODO: Mandar a ejecutar la siguiente rafaga
	}
}

void estimarRafaga(proceso_t* proceso){
	int alfa = config.ALFA_PLANIFICACION;
	int estimacion = proceso->rafagaActual*alfa/100 + (100-alfa)*proceso->rafagaEstimada/100;
	proceso->rafagaEstimada = estimacion;
}

bool verificarClave(char* clave){
	return dictionary_has_key(claves, clave);
}

void bloquearClave(char* clave){
	dictionary_put(claves, clave, 0);
}

void desbloquearClave(char* clave){
	dictionary_remove(claves, clave);
}

void solicitarClave(char* clave){
	if(verificarClave(clave)){
		procesoBloquear(clave);
	}else{
		bloquearClave(clave);
		list_add(procesoEjecucion->clavesBloqueadas, clave);
	}
}

void liberarClave(char* clave){
	void* elem = list_remove(procesoEjecucion->clavesBloqueadas, clave);
	desbloquearClave(clave);
	free(elem);
}
