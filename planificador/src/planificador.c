#include <stdio.h>
#include <stdlib.h>
#include "planificador.h"
#include "commons/string.h"
#include "commons/config.h"
#include "conexiones.h"
#include <pthread.h>

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

int main(void) {
	configuracion_t config = cargarConfiguracion();
	//Conexion a Coordinador
	conectarConCoordinador(config.IP_COORDINADOR, config.PUERTO);
	//Abrir puerto para aceptar conexion de ESIs
	int socketEscucha = crear_socket_escucha(config.PUERTO);
	pthread_t hiloEscucha;
	pthread_create(&hiloEscucha,NULL, iniciarEscucha(socketEscucha), NULL);
	return EXIT_SUCCESS;
}
