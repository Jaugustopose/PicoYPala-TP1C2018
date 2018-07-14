#include "planificador.h"
#include "includes.h"
#include "commons/config.h"
#include <commons/log.h>
#include "comunicacion/comunicacion.h"
#include "planificacion.h"
#include "conexiones.h"
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>

//Estructuras Consola
//Opciones de entrada por consola
char PAUSE_PLANIF[] = "pausar";
char CONTINUE_PLANIF[] = "continuar";
char BLOCK[] = "bloquear";
char UNBLOCK[] = "desbloquear";
char LIST[] = "listar";
char KILL[] = "kill";
char STATUS[] = "status";
char DEADLOCK[] = "deadlock";

//Prototipos Consola
void procesar_entradas_consola();
void procesar_entrada(char* buffer);
void procesar_pausa_planificacion(char** subBufferSplitted);
void procesar_continuar_planificacion(char** subBufferSplitted);
void procesar_bloqueo_esi(char** subBufferSplitted);
void procesar_desbloqueo_esi(char** subBufferSplitted);
void procesar_listar_recurso(char** subBufferSplitted);
void procesar_kill_proceso(char** subBufferSplitted);
void procesar_status_instancias(char** subBufferSplitted);
void procesar_deadlock(char** subBufferSplitted);

int socket_coordinador;
configuracion_t config;
extern t_log * logPlanificador;

configuracion_t cargarConfiguracion() {
	configuracion_t config;
	char* pat = string_new();
	char cwd[1024]; // Variable donde voy a guardar el path absoluto hasta el /Debug
	string_append(&pat, getcwd(cwd, sizeof(cwd)));
	if (string_contains(pat, "/Debug")){
		string_append(&pat,"/planificador.cfg");
	}else{
	string_append(&pat, "/Debug/planificador.cfg");
	}
	t_config* configPlanificador = config_create(pat);
	free(pat);

	if (config_has_property(configPlanificador, "PUERTO")) {
		config.PUERTO = config_get_int_value(configPlanificador, "PUERTO");
		log_debug(logPlanificador, "PUERTO: %d", config.PUERTO);
	}
	if (config_has_property(configPlanificador, "ESTIMACION_INICIAL")) {
		config.ESTIMACION_INICIAL = config_get_int_value(configPlanificador, "ESTIMACION_INICIAL");
		log_debug(logPlanificador, "ESTIMACION_INICIAL: %d", config.ESTIMACION_INICIAL);
	}
	if (config_has_property(configPlanificador, "ALGORITMO_PLANIFICACION")) {
		config.ALGORITMO_PLANIFICACION = config_get_string_value(configPlanificador, "ALGORITMO_PLANIFICACION");
		log_debug(logPlanificador, "ALGORITMO_PLANIFICACION: %s", config.ALGORITMO_PLANIFICACION);
	}
	if (config_has_property(configPlanificador, "ALFA_PLANIFICACION")) {
		config.ALFA_PLANIFICACION = config_get_int_value(configPlanificador, "ALFA_PLANIFICACION");
		log_debug(logPlanificador, "ALFA_PLANIFICACION: %d", config.ALFA_PLANIFICACION);
	}
	if (config_has_property(configPlanificador, "IP_COORDINADOR")) {
		config.IP_COORDINADOR = config_get_string_value(configPlanificador, "IP_COORDINADOR");
		log_debug(logPlanificador, "IP_COORDINADOR: %s", config.IP_COORDINADOR);
	}
	if (config_has_property(configPlanificador, "PUERTO_COORDINADOR")) {
		config.PUERTO_COORDINADOR = config_get_int_value(configPlanificador, "PUERTO_COORDINADOR");
		log_debug(logPlanificador, "PUERTO_COORDINADOR: %d", config.PUERTO_COORDINADOR);
	}
	if (config_has_property(configPlanificador, "CLAVES_BLOQUEADAS")) {
		config.CLAVES_BLOQUEADAS = config_get_array_value(configPlanificador, "CLAVES_BLOQUEADAS");
		char* claves = string_new();
		string_append(&claves, "CLAVES_BLOQUEADAS: [");
		char** p = config.CLAVES_BLOQUEADAS;
		int flag = 1;
		while(p[0]){
			if(flag){
				flag=0;
			}else{
				string_append(&claves, ",");//log_debug(logPlanificador, ",");
			}
			string_append(&claves, p[0]);
			p++;
		}
		string_append(&claves, "]");
		log_debug(logPlanificador, claves);
	}
	return config;
}

void procesar_entradas_consola() {
	char* linea;
	while (1) {
		linea = readline("consola_planificador> ");
		if (linea)
			add_history(linea);
		if (!strcmp(linea, "exit")) {
			free(linea);
			break;
		}
		procesar_entrada(linea);
		free(linea);
	}

	close(socket_coordinador);
	exit(EXIT_SUCCESS);
}

void procesar_entrada(char* buffer) {

	char** subBufferSplitted = string_split(buffer, " ");

	if (subBufferSplitted[0] == NULL) { //String vacío
		free(subBufferSplitted);
		return;
	} else if (!strcmp(subBufferSplitted[0], PAUSE_PLANIF)) {
		procesar_pausa_planificacion(subBufferSplitted);
	} else if (!strcmp(subBufferSplitted[0], CONTINUE_PLANIF)) {
		procesar_continuar_planificacion(subBufferSplitted);
	} else if (!strcmp(subBufferSplitted[0], BLOCK)){
		procesar_bloqueo_esi(subBufferSplitted);
	} else if (!strcmp(subBufferSplitted[0], UNBLOCK)){
		procesar_desbloqueo_esi(subBufferSplitted);
	} else if (!strcmp(subBufferSplitted[0], LIST)){
		procesar_listar_recurso(subBufferSplitted);
	} else if (!strcmp(subBufferSplitted[0], KILL)){
		procesar_kill_proceso(subBufferSplitted);
	} else if (!strcmp(subBufferSplitted[0], STATUS)){
		procesar_status_instancias(subBufferSplitted);
	} else if (!strcmp(subBufferSplitted[0], DEADLOCK)){
		procesar_deadlock(subBufferSplitted);
	} else {
		log_info(logPlanificador, "¡No se reconece el comando %s!", buffer);
	}

	free(subBufferSplitted);
}

void procesar_pausa_planificacion(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL) {
		log_info(logPlanificador, "Pausar Planificación!");
		sem_wait(&planificacion_habilitada);
	} else {
		log_info(logPlanificador, "Comando 'pausar' no requiere parámetros!");
	}
}

void procesar_continuar_planificacion(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL) {
		log_info(logPlanificador, "Continuar Planificación!");
		sem_post(&planificacion_habilitada);
		continuarPlanificacion();
	} else {
		log_info(logPlanificador, "Comando 'continuar' no requiere parámetros!");
	}
}

void procesar_bloqueo_esi(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL || subBufferSplitted[2] == NULL) {
		log_info(logPlanificador, "Comando 'bloquear' incompleto! Se requiere formato 'bloquear <clave> <ID>'");
	} else if (subBufferSplitted[3] == NULL) {
		log_info(logPlanificador, "Bloqueo Esi %s de la cola del recurso %s!", subBufferSplitted[2], subBufferSplitted[1]);
//		pthread_mutex_lock(&mutex_cola_listos);
//		pthread_mutex_lock(&mutex_proceso_ejecucion);
		bloquearEsiPorConsola(strtol(subBufferSplitted[2], NULL, 10) , subBufferSplitted[1]);
//		pthread_mutex_unlock(&mutex_proceso_ejecucion);
//		pthread_mutex_unlock(&mutex_cola_listos);
	} else {
		log_info(logPlanificador, "Comando 'bloquear' con demasiados parámetros! Se requiere formato 'bloquear <clave> <ID>'");
	}
}

void procesar_desbloqueo_esi(char** subBufferSplitted) {
	//sem_wait(&planificacion_habilitada);
	if (subBufferSplitted[1] == NULL) {
		log_info(logPlanificador, "Comando 'desbloquear' incompleto! Se requiere formato 'desbloquear <ID>'");
	} else if (subBufferSplitted[2] == NULL) {
		log_info(logPlanificador, "Desbloqueo Esi por clave %s!", subBufferSplitted[1]);
		procesoDesbloquear(subBufferSplitted[1]);
	} else {
		log_info(logPlanificador, "Comando 'desbloquear' con demasiados parámetros! Se requiere formato 'desbloquear <ID>'");
	}
	//sem_post(&planificacion_habilitada);
}

void procesar_listar_recurso(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL) {
		log_info(logPlanificador, "Comando 'listar' incompleto! Se requiere formato 'listar <recurso>'");
	} else if (subBufferSplitted[2] == NULL) {
		log_info(logPlanificador, "Listar procesos en cola de espera para recurso %s!", subBufferSplitted[1]);
		listarRecursosBloqueadosPorClave(subBufferSplitted[1]);
	} else {
		log_info(logPlanificador, "Comando 'listar' con demasiados parámetros! Se requiere formato 'listar <recurso>'");
	}
}

void procesar_kill_proceso(char** subBufferSplitted) {
	//Closure para recorrer la lista de proceso bloqueados por la clave y guardar el string a imprimir
	char* stringLog = string_new();

	void _listarClaves(char* clave) {
		string_append(&stringLog, clave);
		string_append(&stringLog, "-");
	}

	if (subBufferSplitted[1] == NULL) {
		log_info(logPlanificador, "Comando 'kill' incompleto! Se requiere formato 'kill <ID>'");
	} else if (subBufferSplitted[2] == NULL) {
		log_info(logPlanificador, "Kill proceso %s!", subBufferSplitted[1]);
		t_list* claves = killProcesoPorID(strtol(subBufferSplitted[1], NULL, 10));
		if (claves != NULL && !list_is_empty(claves)) {
			list_iterate(claves, (void*)_listarClaves);
			log_info(logPlanificador, "Claves liberadas al hacer 'kill' sobre proceso %s: %s", subBufferSplitted[1], string_substring_until(stringLog, string_length(stringLog) - 1));
		} else {
			log_info(logPlanificador, "No se produjo liberación de claves al hacer 'kill' sobre proceso %s:", subBufferSplitted[1]);
		}
	} else {
		log_info(logPlanificador, "Comando 'kill' con demasiados parámetros! Se requiere formato 'kill <ID>'");
	}
	free(stringLog);
}

void procesar_status_instancias(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL) {
		log_info(logPlanificador, "Comando 'status' incompleto! Se requiere formato 'status <clave>'");
	} else if (subBufferSplitted[2] == NULL) {
		log_info(logPlanificador, "Status instancias para clave %s!", subBufferSplitted[1]);
		status_clave_t* status = procesarStatusClave(socket_coordinador, subBufferSplitted[1]);
		log_info(logPlanificador, "Valor Clave: %s\nNombre Instancia con Clave: %s\nNombre Instancia Candidata para Clave: %s\n"
								, status->valor, status->nombreInstanciaClave, status->nombreInstanciaCandidata);
		listarRecursosBloqueadosPorClave(subBufferSplitted[1]);
	} else {
		log_info(logPlanificador, "Comando 'status' con demasiados parámetros! Se requiere formato 'status <clave>'");
	}
}

void procesar_deadlock(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL) {
		log_info(logPlanificador, "Analizar deadlocks!");
		analizarDeadlocks();
	} else {
		log_info(logPlanificador, "Comando 'deadlock' no requiere parámetros!");
	}
}


int main(void) {

	logPlanificador = log_create("planificador.log", "Planificador", true, LOG_LEVEL_TRACE);
	config = cargarConfiguracion();
	inicializarPlanificacion();

	sockets_escucha_t sockets;

	//Conexion a Coordinador
	socket_coordinador = conectarConProceso(config.IP_COORDINADOR, config.PUERTO_COORDINADOR, Planificador);
	sockets.socket_coordinador = socket_coordinador;

	//Abrir puerto para aceptar conexion de ESIs en un hilo nuevo
	int socketEscucha = crear_socket_escucha(config.PUERTO);
	sockets.socket_escucha_esis = socketEscucha;
	pthread_t hiloEscucha;
	pthread_create(&hiloEscucha, NULL, &iniciarEscucha, &sockets);
	pthread_detach(hiloEscucha);

	procesar_entradas_consola(socket_coordinador);

	sem_destroy(&planificacion_habilitada);
	return EXIT_SUCCESS;
}
