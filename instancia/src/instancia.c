/*
 ============================================================================
 Name        : instancia.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "commons/config.h"
#include "instancia.h"
#include "comunicacion/comunicacion.h"

char* creacion_y_mapeo_archivo(int entradaCantidad, int entradaTamanio,
		char* pathArchivo) {

	/*
	 /*Abrir o crear archivo de una entrada*/
	printf("Abriendo %s: \n", pathArchivo);
	int tamanioArchivo = entradaCantidad * entradaTamanio * sizeof(char);
	int fd = open(pathArchivo, O_RDWR, (mode_t) 0600);
	if (fd <= -1) {
		close(fd);
		fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
		if (fd == -1) {
			printf("Error al abrir para escritura\n");
			exit(EXIT_FAILURE);
		}
		/* confirmando tamaño nuevo		     */
		int result = lseek(fd, tamanioArchivo - 1, SEEK_SET);
		if (result == -1) {
			close(fd);
			printf("Error al saltar con lseeck\n");
			exit(EXIT_FAILURE);
		}
		/* Actualizar nuevo tamaño */
		result = write(fd, "", 1);
		if (result != 1) {
			close(fd);
			printf("Error al escribir ultimo byte del archivo\n");
			exit(EXIT_FAILURE);
		}

	}
	/* Mapiando archivo
	 */
	char* map = mmap(0, tamanioArchivo, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
	NULL);
	if (map == MAP_FAILED) {
		close(fd);
		printf("Error al mapiar achivo\n");
		exit(EXIT_FAILURE);
	}
	printf("Se mapio archivo \n");
	return map;
}


config_t cargarConfiguracion(char *path) {
	config_t config;

	t_config * config_Instancia = config_create(path);

	if (config_has_property(config_Instancia, "IP_COORDINADOR")) {
		config.ip_coordinador = config_get_string_value(config_Instancia,
				"IP_COORDINADOR");
		printf("IP_COORDINADOR: %s\n", config.ip_coordinador);
	}

	if (config_has_property(config_Instancia, "PUERTO_COORDIANDOR")) {
		config.puerto_coordinador = (int32_t) config_get_int_value(
				config_Instancia, "PUERTO_COORDIANDOR");
		printf("PUERTO_COORDIANDOR: %d\n", config.puerto_coordinador);
	}

	if (config_has_property(config_Instancia, "ALGORITMO_REMPLAZO")) {
		config.algoritmo_remplazo = config_get_string_value(config_Instancia,
				"ALGORITMO_REMPLAZO");
		printf("ALGORITMO_REMPLAZO: %s\n", config.algoritmo_remplazo);
	}

	if (config_has_property(config_Instancia, "PUNTO_MONTAJE")) {
		config.punto_montaje = config_get_string_value(config_Instancia,
				"PUNTO_MONTAJE");
		printf("PUNTO_MONTAJE: %s\n", config.punto_montaje);
	}

	if (config_has_property(config_Instancia, "NOMBRE_INSTANCIA")) {
		config.nombre_instancia = config_get_string_value(config_Instancia,
				"NOMBRE_INSTANCIA");
		printf("NOMBRE_INSTANCIA: %s\n", config.nombre_instancia);
	}

	if (config_has_property(config_Instancia, "INTERVALO_DUMP")) {
		config.intervalo_dump = (int) config_get_int_value(config_Instancia,
				"INTERVALO_DUMP");
		printf("INTERVALO_DUMP: %d\n", config.intervalo_dump);
	}
	return config;

}

char* escribirEntrada(char* map, int numeroEntrada, char* texto,
		int cantidadEntradas, int entradasTamanio) {
	int tamanioArchivo = cantidadEntradas * entradasTamanio;
	int posicion_texto = 0;
	int posicion_texto_fin = strlen(texto);
	int posicion_ini = numeroEntrada * entradasTamanio;
	int posicion_fin = ((numeroEntrada + 1) * entradasTamanio) - 1;
	int posicion;
	int posicion_libre = 0;
	posicion = posicion_ini;

	//posiciona en lugar libre de la entrada
	while ((posicion <= posicion_fin) && posicion_libre != 1) {

		if (map[posicion] != '\0') {
			posicion++;
		} else {
			posicion_libre = 1;
		};
	}
	while ((posicion <= posicion_fin) && posicion_texto <= posicion_texto_fin) {
		map[posicion] = texto[posicion_texto];
		posicion++;
		posicion_texto++;
	}
//	map[posicion] = '\0';

	return map;
}




int main(int argc, char *argv[]) {

	//Creo el log
	log_errores = log_create("instancia.log", "Instance", true,
			LOG_LEVEL_ERROR);

	config_t configuracion;

	if (argc = 1) {
		/*llamo a la funcion de creacion de archivo de configuracion con la estructura t_config*/
		char* pathConfiguracion = argv[1];
		printf("Path de configuracion: %s\n", pathConfiguracion);
		configuracion = cargarConfiguracion(pathConfiguracion);
		printf("------------- FIN CONFIGURACION -------------\n");
	}

	//coneccion coordinador
	int socket_server = conectar_a_server(configuracion.ip_coordinador,
			configuracion.puerto_coordinador);
	if (socket_server < 0) {
		log_error(log_error, "Error conectar_a_server");
		exit(1);
	}

	//Envio de hadshake
	int id_mensaje = handshake;
	int tamanio = sizeof(id_proceso);
	int cuerpo = Instancia;

	int bufferTamanio = sizeof(id_mensaje) + sizeof(tamanio) + tamanio;
	void * buffer = malloc(bufferTamanio);
	memcpy(buffer, &id_mensaje, sizeof(id_mensaje));
	memcpy(buffer + sizeof(id_mensaje), &tamanio, sizeof(tamanio));
	memcpy(buffer + sizeof(id_mensaje) + sizeof(tamanio), &cuerpo, tamanio);

	enviar_mensaje(socket_server, buffer, bufferTamanio);

	free(buffer);

	//Envio de envio de nombre
	bufferTamanio=strlen(configuracion.nombre_instancia)+1;



	enviar_mensaje(socket_server, &bufferTamanio, sizeof(bufferTamanio));
	enviar_mensaje(socket_server, configuracion.nombre_instancia, bufferTamanio);

	//recibir entradas y tamanio

	int entradasCantidad = 0;
	int entradasTamanio = 0;

	recibir_mensaje(socket_server, &entradasCantidad, sizeof(entradasCantidad));
	recibir_mensaje(socket_server, &entradasTamanio, sizeof(entradasTamanio));

	printf("EntradasCantidad: %d entradasTamanio: %d \n ", entradasCantidad,
			entradasTamanio);

//
//	struct t_entrada entradas[entradasCantidad];
//	int i =0;
//	while(entradasCantidad>i){
//		struct t_entrada entradaNula;
//		entradaNula.clave=string_new();
//		entradaNula.numeroEntrada=i;
//		entradaNula.tamanioValor=0;
//		entradas[i]=entradaNula;
//		i=i+1;
//	}
//
	//	mapeo de archivo ArchivoMatrizDeEntradas
	char* pathArchivo = string_new();
	string_append(&pathArchivo, configuracion.punto_montaje);
	char* nombreTablaDeEntradas = "TablaDeEntradas";
	string_append(&pathArchivo, nombreTablaDeEntradas);
	string_append(&pathArchivo, ".txt");
	printf("Path de archivo creado y mapeado o solo mapeado \n%s\n:",
			pathArchivo);
	char* mapArchivoTablaDeEntradas = creacion_y_mapeo_archivo(entradasCantidad,
			entradasTamanio, pathArchivo);
	free(pathArchivo);


	int contadorDeEntradasInsertadas = 0;

//inicio de while
	int seguir;
	recibir_mensaje(socket_server, &seguir, sizeof(int));
printf("Seguir 1 si sigue o 0 si no = %d",seguir);
sleep(10);

	while (seguir==1) {
printf("Inicio de While linea 222\n");
sleep(10);




		//Recibir sentencia de Re Distinto
		int tamanioMensaje;

		recibir_mensaje(socket_server, &tamanioMensaje, sizeof(tamanioMensaje));
		if(tamanioMensaje>0){

		char * sentencia = malloc(tamanioMensaje);
		recibir_mensaje(socket_server, sentencia, tamanioMensaje);

		printf("Sentencia: %s\n", sentencia);

		//Procesado de sentencia de una entrada

		char** sentenciaSeparada = string_split(sentencia, " ");
		free(sentencia);
		char* instruccion = sentenciaSeparada[0];
		char* key = sentenciaSeparada[1];
		char* value = sentenciaSeparada[2];
		printf("Instruccion: %s\n", instruccion);

//obtiene clave asociada de salida de procesado de sentencia
		printf("Key: %s \n", key);
		printf("Value: %s \n", value);

//creacion de tabla de entradas
		struct t_dictionary *diccionarioEntradas = dictionary_create();
//se crea el dato del diccionario de entradas
		struct t_entrada {
			//char* clave;
			int numeroEntrada;
			int tamanioValor;
		};

		int contadorEntradasEscritasEnTabla = 0;
////entrada de prueba -- borrar
//		struct t_entrada unaEntrada;
//		printf("\n PRUEBA CARGO 1 entradacantidad en el diccionario %d\n",
//				dictionary_size(diccionarioEntradas));
//
//		unaEntrada.numeroEntrada = dictionary_size(diccionarioEntradas);
//
//		unaEntrada.tamanioValor = 4;
//		printf("eNTRADA NUMERO %d TAMANIOVALOR %d\n", unaEntrada.numeroEntrada,
//				unaEntrada.tamanioValor);
//		dictionary_put(diccionarioEntradas, "materias:K3001", &unaEntrada);
//		printf("PRUEBA fin cantidad en el diccionario %d\n\n",
//				dictionary_size(diccionarioEntradas));

		//analisis sentencia
		if (strcmp(sentenciaSeparada[0], "GET") == 0) {
			//INICIO procesado GET
			printf("INICIO procesado GET\n");
			//se accsede a la tabla de entradas si no la encuentra la crea


			if (dictionary_has_key(diccionarioEntradas, key)) {
				struct t_entrada *unaEntrada;
				unaEntrada = dictionary_get(diccionarioEntradas, key);
				printf("GET Entrada  numeroEntrada:%d tamanioValor:%d",
						unaEntrada->numeroEntrada, unaEntrada->tamanioValor);
			}else {

			}


			//FIN procesado GET
		} else {
			if (strcmp(sentenciaSeparada[0], "SET") == 0) {
				//INICIO procesado SET
				printf("INICIO procesado SET\n");
				//se accsede a la tabla de entradas

				struct t_entrada unaEntrada;
				unaEntrada.numeroEntrada = dictionary_size(diccionarioEntradas);
				unaEntrada.tamanioValor = strlen(value);
				dictionary_put(diccionarioEntradas, key, &unaEntrada);

				char* entradaEnArchivo = string_new();
				string_append(&entradaEnArchivo, key);
				string_append(&entradaEnArchivo, ";");
				string_append(&entradaEnArchivo,
						string_itoa(unaEntrada.numeroEntrada));
				string_append(&entradaEnArchivo, ";");
				string_append(&entradaEnArchivo,
						string_itoa(unaEntrada.tamanioValor));
				string_append(&entradaEnArchivo, ";");

				mapArchivoTablaDeEntradas = escribirEntrada(
						mapArchivoTablaDeEntradas,
						contadorEntradasEscritasEnTabla, entradaEnArchivo,
						entradasCantidad, 255);
				free(entradaEnArchivo);
				if (msync(mapArchivoTablaDeEntradas, (entradasCantidad * 255),
						MS_SYNC) == -1) {
					perror("No se puede sincronizar con el disco ");
				}
				printf("Se mapeo correctamente \n"
					);
				contadorDeEntradasInsertadas=contadorDeEntradasInsertadas+1;
				printf("FIN procesado SET\n");
				//FIN procesado SET
			} else {
				if (strcmp(sentenciaSeparada[0], "STORE") == 0) {
					//INICIO procesado STORE
					printf("INICIO procesado STORE\n");

					char* pathArchivo = string_new();
					string_append(&pathArchivo, configuracion.punto_montaje);
					string_append(&pathArchivo, key);
					string_append(&pathArchivo, ".txt");
					//se accsede a la tabla de entradas
					int tamanioArchivo = entradasTamanio * sizeof(char);
					int fd = open(pathArchivo, O_RDWR, (mode_t) 0600);
					if (fd <= -1) {
						close(fd);
						fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC,
								(mode_t) 0600);
						if (fd == -1) {
							printf("Error al abrir para escritura\n");
							exit(EXIT_FAILURE);
						}
						/* confirmando tamaño nuevo		     */
						int result = lseek(fd, tamanioArchivo - 1, SEEK_SET);
						if (result == -1) {
							close(fd);
							printf("Error al saltar con lseeck\n");
							exit(EXIT_FAILURE);
						}
						/* Actualizar nuevo tamaño */
						result = write(fd, "", 1);
						if (result != 1) {
							close(fd);
							printf(
									"Error al escribir ultimo byte del archivo\n");
							exit(EXIT_FAILURE);
						}

						//leer entrada en memoria
						if (dictionary_has_key(diccionarioEntradas, key)) {
							struct t_entrada *unaEntrada;
							unaEntrada = dictionary_get(diccionarioEntradas,
									key);
							printf("STORE Entrada  numeroEntrada:%d tamanioValor:%d",
															unaEntrada->numeroEntrada,
															unaEntrada->tamanioValor);
													/* Escribir entrada en archivo */
													result = write(fd, value, unaEntrada->tamanioValor);
													if (result != 1) {
														close(fd);
														printf(
																"Error al escribir ultimo byte del archivo\n");
														exit(EXIT_FAILURE);
													}
						}


						//FIN procesa do STORE
					} else {
						printf("Error de sentencia\n");
						return 0;
					}
				}
			}

			}
		}

		//se obtiene informacin para obtener el valor en el almancenamiento

		//se accsede al valor

		//respuesta resultado al coordinador

		//int t_nombre_instancia =strlen(configuracion.nombre_instancia);
		//printf("mensaje %d\n",t_nombre_instancia);
		//enviar_mensaje(socket_server,t_nombre_instancia,sizeof(int));
		//printf("se imprimio mensaje");
		recibir_mensaje(socket_server, &seguir, sizeof(int));
	}

	close(socket_server);
	return EXIT_SUCCESS;
}
