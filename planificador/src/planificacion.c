#include "planificacion.h"
#include "includes.h"
#include "conexiones.h"
#include "comunicacion/comunicacion.h"
#include <stdbool.h>
#include <string.h>

t_list* colaListos;
proceso_t* procesoEjecucion;
t_queue* colaTerminados;
t_list* listaBloqueados;
t_dictionary* claves;

extern configuracion_t config;
int contadorProcesos = 1;

//Prototipos
bool esFIFO();
bool esSJFCD();
bool esSJFSD();
bool esHRRN();
bool planificadorConDesalojo();
bool comparadorSJF(void* arg1, void* arg2);
bool comparadorHRRN(void* arg1, void* arg2);
void ordenarColaListos();
void planificarConDesalojo();
void procesoBloquear(char* clave);
void procesoDesbloquear(char* clave);
void incrementarRafagasEsperando();
bool solicitarClave(char* clave);
void liberarClave(char* clave);
void colaListosPush(proceso_t* proceso);
proceso_t* colaListosPop();
proceso_t* colaListosPeek();
int procesoEjecutar(proceso_t* proceso);
void estimarRafaga(proceso_t* proceso);
void liberarRecursos(proceso_t* proceso);

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

int procesoNuevo(int socketESI) {
	proceso_t* proceso = malloc(sizeof(proceso_t));
	proceso->idProceso = contadorProcesos;
	contadorProcesos++;
	proceso->claveBloqueo = string_new();
	proceso->clavesBloqueadas = list_create();
	proceso->rafagaActual = 0;
	proceso->rafagaEstimada = config.ESTIMACION_INICIAL;
	proceso->rafagasEsperando = 0;
	proceso->socketESI = socketESI;
	int retorno;
	if(procesoEjecucion == 0){
		retorno = procesoEjecutar(proceso);
		if (retorno < 0) {
			//TODO Lo ponemos en la lista de Terminados???
			procesoTerminado(exit_abortado_inesperado);
		}
	} else {
		colaListosPush(proceso);
		if (planificadorConDesalojo()) {
			planificarConDesalojo();
		}
	}

	return EXIT_SUCCESS; //TODO: Revisar si habría que hacer algún return antes en los IF

}

int procesoEjecutar(proceso_t* proceso) {
	procesoEjecucion = proceso;
	return mandar_a_ejecutar_esi(proceso->socketESI);
}

void procesoTerminado(int exitStatus) {
	procesoEjecucion->exitStatus = exitStatus;
	queue_push(colaTerminados, (void*)procesoEjecucion);
	liberarRecursos(procesoEjecucion);
	procesoEjecutar(colaListosPop());
}

void procesoBloquear(char* clave){
	procesoEjecucion->claveBloqueo=clave;
	list_add(listaBloqueados, (void*)procesoEjecucion);
	procesoEjecutar(colaListosPop());
}

void procesoDesbloquear(char* clave) {
	int i;
	for (i=0;i<list_size(listaBloqueados);i++) {
		proceso_t* proceso;
		proceso = (proceso_t*)list_get(listaBloqueados, i);
		if (strcmp(proceso->claveBloqueo, clave)==0) {
			list_remove(listaBloqueados, i);
			estimarRafaga(proceso);
			colaListosPush(proceso);
			break;
		}
	}
}

void incrementarRafagasEsperando() {
	int i;
	for (i=0;i<list_size(colaListos);i++) {
		proceso_t* proceso;
		proceso = (proceso_t*)list_get(colaListos, i);
		proceso->rafagasEsperando++;
	}
}

void sentenciaFinalizada(int socket_esi) {

	procesoEjecucion->rafagaActual++;
	if (esHRRN()) {
		incrementarRafagasEsperando();
	}
	if (planificadorConDesalojo()) {
		planificarConDesalojo();
	} else {
		 mandar_a_ejecutar_esi(procesoEjecucion->socketESI);
		 //TODO: Manejar error Send.
	}
}

void estimarRafaga(proceso_t* proceso) {
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

bool solicitarClave(char* clave){
	bool pudo_solicitar = true;
	if(verificarClave(clave)){
		procesoBloquear(clave);
		pudo_solicitar = false;
	} else {
		bloquearClave(clave);
		list_add(procesoEjecucion->clavesBloqueadas, clave);
	}
	return pudo_solicitar;
}

//Siempre se llamará luego de asegurarse que el proceso tenga la clave bloqueada
void liberarClave(char* clave){
	//Closure para hacer el remove de la lista de claves bloqueadas
	bool _soy_clave_buscada(char* p) {
		return strcmp(p, clave) == 0;
	}
	void* elem = list_remove_by_condition(procesoEjecucion->clavesBloqueadas, (void*)_soy_clave_buscada);
	desbloquearClave(clave);
	free(elem);
}

void liberarRecursos(proceso_t* proceso) {
	list_iterate(proceso->clavesBloqueadas, (void*)desbloquearClave);
}

respuesta_operacion_t procesar_notificacion_coordinador(int comando, int tamanio, void* cuerpo) {

	respuesta_operacion_t retorno;

	//Closure para buscar si el proceso tiene la clave que intenta bloquear
	bool _soy_clave_buscada(char* p) {
		return strcmp(p, cuerpo) == 0;
	}

	//El cuerpo siempre tiene que ser una clave
	switch(comando) {
	case msj_solicitud_get_clave: //Procesar GET
		retorno.respuestaACoordinador = solicitarClave(cuerpo);
		retorno.fdESIAAbortar = -1;
		break;
	case msj_store_clave: //Procesar STORE
		liberarClave(cuerpo);
		procesoDesbloquear(cuerpo); //TODO Si hay desalojo acá se puede pisar procesoEnEjecucion
		retorno.respuestaACoordinador = 1;
		retorno.fdESIAAbortar = -1;
		break;
	case msj_inexistencia_clave: //Procesar inexistancia clave
		retorno.fdESIAAbortar = procesoEjecucion->socketESI;
		procesoTerminado(exit_abortado_por_clave_inexistente);
		retorno.respuestaACoordinador = 1;
		break;
	case msj_esi_tiene_tomada_clave:
		retorno.respuestaACoordinador = list_any_satisfy(procesoEjecucion->clavesBloqueadas, (void*)_soy_clave_buscada);
		retorno.fdESIAAbortar = (retorno.respuestaACoordinador == 1) ? -1 : procesoEjecucion->socketESI;
		break;
	}

	return retorno;

}
