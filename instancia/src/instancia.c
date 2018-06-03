/*
 ============================================================================
 Name        : esi.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "esi.h"
#include "comunicacion/comunicacion.h"
#include "commons/config.h"
#include <commons/log.h>
#include <parsi/parser.h>

int socket_coordinador;
int socket_planificador;
t_log *log_esi;

configuracion_t cargarConfiguracion() {
	configuracion_t config;
	char* pat = string_new();
	char cwd[1024]; // Variable donde voy a guardar el path absoluto hasta el /Debug
	string_append(&pat, getcwd(cwd, sizeof(cwd)));
	if (string_contains(pat, "/Debug")) {
		string_append(&pat, "/esi.cfg");
	} else {
		string_append(&pat, "/Debug/esi.cfg");
	}
	t_config* configEsi = config_create(pat);
	free(pat);

	if (config_has_property(configEsi, "PUERTO_PLANIFICADOR")) {
		config.PUERTO_PLANIFICADOR = config_get_int_value(configEsi,
				"PUERTO_PLANIFICADOR");
		printf("PUERTO_PLANIFICADOR: %d\n", config.PUERTO_PLANIFICADOR);
	}
	if (config_has_property(configEsi, "IP_PLANIFICADOR")) {
		config.IP_PLANIFICADOR = config_get_string_value(configEsi,
				"IP_PLANIFICADOR");
		printf("IP_PLANIFICADOR: %s\n", config.IP_PLANIFICADOR);
	}
	if (config_has_property(configEsi, "PUERTO_COORDINADOR")) {
		config.PUERTO_COORDINADOR = config_get_int_value(configEsi,
				"PUERTO_COORDINADOR");
		printf("PUERTO_COORDINADOR: %d\n", config.PUERTO_COORDINADOR);
	}
	if (config_has_property(configEsi, "IP_COORDINADOR")) {
		config.IP_COORDINADOR = config_get_string_value(configEsi,
				"IP_COORDINADOR");
		printf("IP_COORDINADOR: %s\n", config.IP_COORDINADOR);
	}
	return config;
}

void leerScript(char **argv){
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen(argv[1], "r");
	log_esi = log_create("esi.log", "ESI", false, LOG_LEVEL_INFO);
	if (fp == NULL) {
		perror("Error al abrir el archivo: ");
		exit(EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		t_esi_operacion parsed = parse(line);

		if (parsed.valido) {
			switch (parsed.keyword) {
			case GET:
				printf("GET\tclave: <%s>\n", parsed.argumentos.GET.clave);
				break;
			case SET:
				printf("SET\tclave: <%s>\tvalor: <%s>\n",
						parsed.argumentos.SET.clave,
						parsed.argumentos.SET.valor);
				break;
			case STORE:
				printf("STORE\tclave: <%s>\n", parsed.argumentos.STORE.clave);
				break;
			default:
				fprintf(stderr, "No pude interpretar <%s>\n", line);
				exit(EXIT_FAILURE);
			}

			destruir_operacion(parsed);
		} else {
			fprintf(stderr, "La linea <%s> no es valida\n", line);
			exit(EXIT_FAILURE);
		}
	}

	fclose(fp);
	if (line)
		free(line);
}

int main(int argc, char **argv) {
	configuracion_t config = cargarConfiguracion();
	socket_coordinador = conectarConProceso(config.IP_COORDINADOR,
			config.PUERTO_COORDINADOR, ESI);
	socket_planificador = conectarConProceso(config.IP_PLANIFICADOR,
			config.PUERTO_PLANIFICADOR, ESI);
	leerScript(argv);
	return EXIT_SUCCESS;
}
