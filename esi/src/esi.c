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
#include <string.h>
#include <unistd.h>
#include "esi.h"
#include "comunicacion/comunicacion.h"
#include "commons/config.h"
#include <commons/log.h>
#include <parsi/parser.h>
#include <signal.h>

int socket_coordinador;
int socket_planificador;
t_log *logESI;
FILE * fp;

typedef struct {
	t_esi_operacion parsed;
	int finArchivo;
} t_retornoParsearLinea;

void exitFailure() {
	log_destroy(logESI);
	close(socket_coordinador);
	close(socket_planificador);
	exit(EXIT_FAILURE);
}

void exitSuccess() {
	log_destroy(logESI);
	close(socket_coordinador);
	close(socket_planificador);
	exit(EXIT_SUCCESS);
}

void inicializarLogger() {
	logESI = log_create("esi.log", "ESI", true, LOG_LEVEL_DEBUG);
}

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
	log_debug(logESI, "Path archivo configuración: %s", pat);
	t_config* configEsi = config_create(pat);
	free(pat);
	if (!configEsi) {
		log_error(logESI, "No se encontró archivo de configuración.");
		exitFailure();
	}
	if (config_has_property(configEsi, "PUERTO_PLANIFICADOR")) {
		config.PUERTO_PLANIFICADOR = config_get_int_value(configEsi,
				"PUERTO_PLANIFICADOR");
		log_debug(logESI, "PUERTO_PLANIFICADOR: %d\n",
				config.PUERTO_PLANIFICADOR);
	} else {
		log_error(logESI,
				"No se encontró PUERTO_PLANIFICADOR en el archivo de configuraciones.");
		exitFailure();
	}
	if (config_has_property(configEsi, "IP_PLANIFICADOR")) {
		config.IP_PLANIFICADOR = config_get_string_value(configEsi,
				"IP_PLANIFICADOR");
		log_debug(logESI, "IP_PLANIFICADOR: %s\n", config.IP_PLANIFICADOR);
	} else {
		log_error(logESI,
				"No se encontró IP_PLANIFICADOR en el archivo de configuraciones.");
		exitFailure();
	}
	if (config_has_property(configEsi, "PUERTO_COORDINADOR")) {
		config.PUERTO_COORDINADOR = config_get_int_value(configEsi,
				"PUERTO_COORDINADOR");
		log_debug(logESI, "PUERTO_COORDINADOR: %d\n",
				config.PUERTO_COORDINADOR);
	} else {
		log_error(logESI,
				"No se encontró PUERTO_COORDINADOR en el archivo de configuraciones.");
		exitFailure();
	}
	if (config_has_property(configEsi, "IP_COORDINADOR")) {
		config.IP_COORDINADOR = config_get_string_value(configEsi,
				"IP_COORDINADOR");
		log_debug(logESI, "IP_COORDINADOR: %s\n", config.IP_COORDINADOR);
	} else {
		log_error(logESI,
				"No se encontró IP_COORDINADOR en el archivo de configuraciones.");
		exitFailure();
	}
	return config;
}

void abrirScript(char* path) {
	log_debug(logESI, "Ruta Script: %s", path);
	fp = fopen(path, "r");
	if (fp == NULL) {
		log_error(logESI, "Error al abrir el archivo");
		exitFailure();
	}
}

t_retornoParsearLinea parsearLinea() {
	t_retornoParsearLinea retorno;
	retorno.finArchivo = 0;
	char * line = NULL;
	size_t len = 0;
	if (getline(&line, &len, fp) != -1) {
		log_info(logESI, "Parseando linea: %s", line);
		retorno.parsed = parse(line);
		if (!retorno.parsed.valido) {
			log_error(logESI, "Error al parsear linea");
			exitFailure();
		}
	} else {
		retorno.finArchivo = 1;
	}
	return retorno;
}

void leerScript(char **argv) {
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	log_debug(logESI, "Ruta Archivo: %s\n", argv[1]);
	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		perror("Error al abrir el archivo: ");
		exitFailure();
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		log_info(logESI, "Parseando linea: %s\n", line);
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
				printf("No pude interpretar <%s>\n", line);
				exitFailure();
			}

			destruir_operacion(parsed);
		} else {
			fprintf(stderr, "La linea <%s> no es valida\n", line);
			exitFailure();
		}
	}
	printf("Fin de archivo\n");

	fclose(fp);
	if (line)
		free(line);
}

void msgEjecucion(t_esi_operacion operacion) {
	header_t header;
	int tamanio;
	void* buffer;
	if (operacion.keyword == GET) {
		header.comando = msj_sentencia_get;
		header.tamanio = strlen(operacion.argumentos.GET.clave) + 1; //+1 para que copie tambien el /0
		tamanio = sizeof(header_t) + header.tamanio;
		buffer = malloc(tamanio);
		memcpy(buffer, &header, sizeof(header_t));
		memcpy(buffer + sizeof(header_t), operacion.argumentos.GET.clave,
				header.tamanio);
	} else if (operacion.keyword == SET) {
		header.comando = msj_sentencia_set;
		int tamanioClave = strlen(operacion.argumentos.SET.clave) + 1; //+1 para que copie tambien el /0
		int tamanioValor = strlen(operacion.argumentos.SET.valor) + 1; //+1 para que copie tambien el /0
		header.tamanio = tamanioClave + tamanioValor;
		tamanio = sizeof(header_t) + header.tamanio;
		buffer = malloc(tamanio);
		memcpy(buffer, &header, sizeof(header_t));
		memcpy(buffer + sizeof(header_t), operacion.argumentos.SET.clave,
				tamanioClave);
		memcpy(buffer + sizeof(header_t) + tamanioClave,
				operacion.argumentos.SET.valor, tamanioValor);
	} else {
		header.comando = msj_sentencia_store;
		header.tamanio = strlen(operacion.argumentos.STORE.clave) + 1; //+1 para que copie tambien el /0
		tamanio = sizeof(header_t) + header.tamanio;
		buffer = malloc(tamanio);
		memcpy(buffer, &header, sizeof(header_t));
		memcpy(buffer + sizeof(header_t), operacion.argumentos.STORE.clave,
				header.tamanio);
	}
	int retorno = enviar_mensaje(socket_coordinador, buffer, tamanio);
	free(buffer);
	if (retorno < 0) {
		log_error(logESI,
				"Problema con el ESI en el socket %d. Se cierra conexión con él.\n",
				socket_coordinador);
		exitFailure();
	}
}

void msgFinProceso(int unSocket) {
	header_t header;
	header.comando = msj_esi_finalizado;
	header.tamanio = 0;
	int retorno = enviar_mensaje(unSocket, &header, sizeof(header_t));
	if (retorno < 0) {
		log_error(logESI, "Problema con el ESI en el socket %d. Se cierra conexión con él.\n", unSocket);
		exitFailure();
	}
	int respuestaFinalizacion;
	recibir_mensaje(unSocket, &respuestaFinalizacion, sizeof(respuestaFinalizacion));

}

void msgSentenciaFinalizada() {
	header_t header;
	header.comando = msj_sentencia_finalizada;
	header.tamanio = 0;
	int retorno = enviar_mensaje(socket_planificador, &header, sizeof(header_t));
	if (retorno < 0) {
		log_error(logESI, "Problema con el ESI en el socket %d. Se cierra conexión con él.\n", socket_planificador);
		exitFailure();
	}
	printf("MSJ Sentencia finalizada enviada al plani\n");
}

void sig_handler(int signo) {
  if (signo == SIGINT) {
  printf("SIGINT interceptado. Finalizando... \n");
  msgFinProceso(socket_coordinador);
  msgFinProceso(socket_planificador);
  close(socket_coordinador);
  exit(EXIT_SUCCESS);
  }
}

void atenderMsgPlanificador() {
	//Recibo mensaje de Planificador
	header_t header;
	int bytesRecibidos = recibir_mensaje(socket_planificador, &header,
			sizeof(header_t));
	if (bytesRecibidos == ERROR_RECV_DISCONNECTED
			|| bytesRecibidos == ERROR_RECV_DISCONNECTED) {
		log_error(logESI, "Se perdió la conexión con el Planificador");
		exitFailure();
	} else {
		switch (header.comando) {
		case msj_requerimiento_ejecucion:
			log_info(logESI, "Msg requerimiento de ejecución recibido del Planificador");
			t_retornoParsearLinea retorno = parsearLinea();
			if (retorno.finArchivo) {
				log_info(logESI, "Fin Script");
				msgFinProceso(socket_planificador);
				msgFinProceso(socket_coordinador);
				exitSuccess();
			} else {
				msgEjecucion(retorno.parsed);
			}
			break;
		case msj_abortar_esi:
			log_info(logESI, "Msg para abortar recibido del Planificador");
//			exitFailure(); //TODO descomentar. El Esi se tiene que abortar por el Planificador.
			msgFinProceso(socket_coordinador);
			break;
		default:
			log_error(logESI, "Se recibió comando desconocido (Planificador): %d", header.comando);
			break;
		}
	}
}

void atenderMsgCoordinador() {
	//Recibo mensaje de Coordinador
	header_t header;
	int bytesRecibidos = recibir_mensaje(socket_coordinador, &header,
			sizeof(header_t));
	if (bytesRecibidos == ERROR_RECV_DISCONNECTED
			|| bytesRecibidos == ERROR_RECV_DISCONNECTED) {
		log_error(logESI, "Se perdió la conexión con el Coordinador");
		exitFailure();
	} else {
		switch (header.comando) {
		case msj_sentencia_finalizada:
			log_info(logESI,
					"Msg Sentencia finalizada recibido del Coordinador");
			msgSentenciaFinalizada();
			break;
		default:
			log_error(logESI,
					"Se recibió comando desconocido (Coordinador): %d",
					header.comando);
			break;
		}
	}
}

int main(int argc, char **argv) {
	inicializarLogger();
	if (argc < 2) {
		printf(
				"Falta argumento para ejecución. Uso correcto: ./esi \"Path_script\"\n");
		return EXIT_FAILURE;
	}

	if (signal(SIGINT, sig_handler) == SIG_ERR){ //Manejar el ctrl+c del ESI.
		printf("Error al interceptar SIGINT\n");
		return EXIT_SUCCESS;
	}

	abrirScript(argv[1]);
	configuracion_t config = cargarConfiguracion();
	socket_coordinador = conectarConProceso(config.IP_COORDINADOR,
			config.PUERTO_COORDINADOR, ESI);
	socket_planificador = conectarConProceso(config.IP_PLANIFICADOR,
			config.PUERTO_PLANIFICADOR, ESI);

	fd_set master;
	fd_set readfs;
	FD_ZERO(&master);
	FD_SET(socket_coordinador, &master);
	FD_SET(socket_planificador, &master);
	int fdmax = socket_planificador;
	//Loop para atender mensajes
	for (;;) {
		readfs = master;
		if (select(fdmax + 1, &readfs, NULL, NULL, NULL) == -1) {
			log_error(logESI, "Error en select");
			exitFailure();
		}
		if (FD_ISSET(socket_coordinador, &readfs)) {
			//Atiendo coordinador
			atenderMsgCoordinador();
		} else if (FD_ISSET(socket_planificador, &readfs)) {
			//Atiendo planificador
			atenderMsgPlanificador();
		} else {
			//No dedeberia pasar jamas
		}
	}
	return EXIT_SUCCESS;
}
