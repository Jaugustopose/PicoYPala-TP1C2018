#include "planificador.h"
#include "includes.h"
#include "commons/config.h"
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
		printf("PUERTO: %d\n", config.PUERTO);
	}
	if (config_has_property(configPlanificador, "ESTIMACION_INICIAL")) {
		config.ESTIMACION_INICIAL = config_get_int_value(configPlanificador, "ESTIMACION_INICIAL");
		printf("ESTIMACION_INICIAL: %d\n", config.ESTIMACION_INICIAL);
	}
	if (config_has_property(configPlanificador, "ALGORITMO_PLANIFICACION")) {
		config.ALGORITMO_PLANIFICACION = config_get_string_value(configPlanificador, "ALGORITMO_PLANIFICACION");
		printf("ALGORITMO_PLANIFICACION: %s\n", config.ALGORITMO_PLANIFICACION);
	}
	if (config_has_property(configPlanificador, "ALFA_PLANIFICACION")) {
			config.ALFA_PLANIFICACION = config_get_int_value(configPlanificador, "ALFA_PLANIFICACION");
			printf("ALFA_PLANIFICACION: %d\n", config.ALFA_PLANIFICACION);
		}
	if (config_has_property(configPlanificador, "IP_COORDINADOR")) {
		config.IP_COORDINADOR = config_get_string_value(configPlanificador, "IP_COORDINADOR");
		printf("IP_COORDINADOR: %s\n", config.IP_COORDINADOR);
	}
	if (config_has_property(configPlanificador, "PUERTO_COORDINADOR")) {
		config.PUERTO_COORDINADOR = config_get_int_value(configPlanificador, "PUERTO_COORDINADOR");
		printf("PUERTO_COORDINADOR: %d\n", config.PUERTO_COORDINADOR);
	}
	if (config_has_property(configPlanificador, "CLAVES_BLOQUEADAS")) {
		config.CLAVES_BLOQUEADAS = config_get_array_value(configPlanificador, "CLAVES_BLOQUEADAS");
		printf("CLAVES_BLOQUEADAS: [");
		char** p = config.CLAVES_BLOQUEADAS;
		int flag = 1;
		while(p[0]){
			if(flag){
				flag=0;
			}else{
				printf(",");
			}
			printf("%s",p[0]);
			p++;
		}
		printf("]\n");
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
		printf("¡No se reconece el comando %s!\n", buffer);
	}

	free(subBufferSplitted);
}

void procesar_pausa_planificacion(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL) {
		printf("Pausar Planificación!\n");
		sem_wait(&planificacion_habilitada);
	} else {
		printf("Comando 'pausar' no requiere parámetros!\n");
	}
}

void procesar_continuar_planificacion(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL) {
		printf("Continuar Planificación!\n");
		sem_wait(&planificacion_habilitada);
	} else {
		printf("Comando 'continuar' no requiere parámetros!\n");
	}
}

void procesar_bloqueo_esi(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL || subBufferSplitted[2] == NULL) {
		printf("Comando 'bloquear' incompleto! Se requiere formato 'bloquear <clave> <ID>'\n");
	} else if (subBufferSplitted[3] == NULL) {
		printf("Bloqueo Esi %s de la cola del recurso %s!\n", subBufferSplitted[2], subBufferSplitted[1]);
		bloquearEsiPorConsola(strtol(subBufferSplitted[2], NULL, 10) , subBufferSplitted[1]);
	} else {
		printf("Comando 'bloquear' con demasiados parámetros! Se requiere formato 'bloquear <clave> <ID>'\n");
	}
}

void procesar_desbloqueo_esi(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL) {
		printf("Comando 'desbloquear' incompleto! Se requiere formato 'desbloquear <ID>'\n");
	} else if (subBufferSplitted[2] == NULL) {
		printf("Desbloqueo Esi %s!\n", subBufferSplitted[1]);
	} else {
		printf("Comando 'desbloquear' con demasiados parámetros! Se requiere formato 'desbloquear <ID>'\n");
	}
}

void procesar_listar_recurso(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL) {
		printf("Comando 'listar' incompleto! Se requiere formato 'listar <recurso>'\n");
	} else if (subBufferSplitted[2] == NULL) {
		printf("Listar procesos en cola de espera para recurso %s!\n", subBufferSplitted[1]);
	} else {
		printf("Comando 'listar' con demasiados parámetros! Se requiere formato 'listar <recurso>'\n");
	}
}

void procesar_kill_proceso(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL) {
		printf("Comando 'kill' incompleto! Se requiere formato 'kill <ID>'\n");
	} else if (subBufferSplitted[2] == NULL) {
		printf("Kill proceso %s!\n", subBufferSplitted[1]);
	} else {
		printf("Comando 'kill' con demasiados parámetros! Se requiere formato 'kill <ID>'\n");
	}
}

void procesar_status_instancias(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL) {
		printf("Comando 'status' incompleto! Se requiere formato 'status <clave>'\n");
	} else if (subBufferSplitted[2] == NULL) {
		printf("Status instancias para clave %s!\n", subBufferSplitted[1]);
	} else {
		printf("Comando 'status' con demasiados parámetros! Se requiere formato 'status <clave>'\n");
	}
}

void procesar_deadlock(char** subBufferSplitted) {
	if (subBufferSplitted[1] == NULL) {
		printf("Analizar deadlocks!\n");
	} else {
		printf("Comando 'deadlock' no requiere parámetros!\n");
	}
}


int main(void) {

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

	procesar_entradas_consola();

	sem_destroy(&planificacion_habilitada);
	return EXIT_SUCCESS;
}
