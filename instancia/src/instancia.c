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
#include "commons/collections/list.h"
#include "instancia.h"
#include "comunicacion/comunicacion.h"



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
//
//char* escribirEntrada(char* map, int numeroEntrada, char* texto,
//		int cantidadEntradas, int entradasTamanio) {
//	int tamanioArchivo = cantidadEntradas * entradasTamanio;
//	int posicion_texto = 0;
//	int posicion_texto_fin = strlen(texto);
//	int posicion_ini = numeroEntrada * entradasTamanio;
//	int posicion_fin = ((numeroEntrada + 1) * entradasTamanio) - 1;
//	int posicion;
//	int posicion_libre = 0;
//	posicion = posicion_ini;
//
//	//posiciona en lugar libre de la entrada
//	while ((posicion <= posicion_fin) && posicion_libre != 1) {
//
//		if (map[posicion] != '\0') {
//			posicion++;
//		} else {
//			posicion_libre = 1;
//		};
//	}
//	while ((posicion <= posicion_fin) && posicion_texto <= posicion_texto_fin) {
//		map[posicion] = texto[posicion_texto];
//		posicion++;
//		posicion_texto++;
//	}
////	map[posicion] = '\0';
//
//	return map;
//}
//
//


persistirDiccionarioDeTablaDeEntradas(t_dictionary * mapArchivoTablaDeEntradas ,  t_dictionary * diccionarioEntradas ){

	memcpy(&mapArchivoTablaDeEntradas,&diccionarioEntradas,sizeof( t_dictionary ));


	if (msync(mapArchivoTablaDeEntradas, entradasCantidad*255,	MS_SYNC) == -1) {
							perror("No se puede sincronizar con el disco ");
						}
						printf("Se mapeo correctamente \n"
							);

						struct t_entrada *unaEntrada;
						unaEntrada = dictionary_get(diccionarioEntradas,
								"messi");
						printf("se mapeo bien Entrada  numeroEntrada:%d tamanioValor:%d",
														unaEntrada->numeroEntrada,
														unaEntrada->tamanioValor);

}




int main(int argc, char *argv[]) {





	//variables nescesarias
	int maximoNumeroEntradaInicio=0;
	int maximoNumeroEntradaFin=0;

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

	 entradasCantidad = 0;
	entradasTamanio = 0;

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
	string_append(&pathArchivo, ".bin");
	printf("Path de archivo creado y mapeado o solo mapeado \n%s\n:",
			pathArchivo);




	 /*Abrir o crear archivo de una entrada*/
	printf("Abriendo %s: \n", pathArchivo);
	//int tamanioArchivo = entradasCantidad * entradasTamanio ;
	int tamanioArchivo =sizeof(t_dictionary);
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
	struct t_dictionary* mapArchivoTablaDeEntradas = (struct t_dictionary*) mmap(0, tamanioArchivo, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
	NULL);
	if (mapArchivoTablaDeEntradas == MAP_FAILED) {
		close(fd);
		printf("Error al mapiar achivo\n");
		exit(EXIT_FAILURE);
	}
	printf("Se mapio archivo \n");

	if(sizeof(mapArchivoTablaDeEntradas)>0){
		printf("255 se mapeo bien Entrada y se encontro key messi%d\n",sizeof(mapArchivoTablaDeEntradas));
	}else{printf("255 no pude leer entrada %d\n",sizeof(mapArchivoTablaDeEntradas));};


	free(pathArchivo);
	struct t_dictionary *diccionarioValoresEntradas=dictionary_create();
	struct t_dictionary *diccionarioEntradas=dictionary_create();
if(dictionary_size(mapArchivoTablaDeEntradas)>0){
	printf("tiene datos el archivo de entrada %d \n",dictionary_size(mapArchivoTablaDeEntradas));
	memcpy(&diccionarioEntradas,&mapArchivoTablaDeEntradas,sizeof(t_dictionary));
}

	int contadorDeEntradasInsertadas = 0;

//inicio de while
	int seguir=0;
	recibir_mensaje(socket_server, &seguir, sizeof(seguir));
printf("Seguir 1 si sigue o 0 si no = %d\n",seguir);


	while (seguir==1) {





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

//creacion de tabla de entradas y de lista de keys



		int contadorEntradasEscritasEnTabla = 0;

		//analisis sentencia
		if (strcmp(sentenciaSeparada[0], "GET") == 0) {
			//INICIO procesado GET
			printf("INICIO procesado GET\n");
			//se accsede a la tabla de entradas si no la encuentra la crea

		if(strlen(key)>40){
//			Error de Tamaño de Clave
//			De exceder el tamaño máximo de 40 caracteres para la Clave, la operación fallará y se informará al Usuario. Abortando el ESI culpable.

			}else {

			if (dictionary_has_key(diccionarioEntradas, key)) {
				printf("si exite la clave\n");
				//si exite la clave
				struct t_entrada *unaEntrada;
				unaEntrada = dictionary_get(diccionarioEntradas, key);
				//intento de bloquear
				if(unaEntrada->bloquiado==0){

				printf("GET Entrada  numeroEntrada:%d tamanioValor:%d bloquiado:%d\n",
						unaEntrada->numeroEntrada, unaEntrada->tamanioValor, unaEntrada->bloquiado);
				unaEntrada->bloquiado=1;
				}else{
					printf("GET ERROR clave  ya bloquiada  numeroEntrada:%d tamanioValor:%d bloquiado:%d\n",
						unaEntrada->numeroEntrada, unaEntrada->tamanioValor,unaEntrada->bloquiado);
				}
			}else {
				printf("si no exite la clave\n");
				//si no exite la clave
				//crea
				struct t_entrada unaEntrada;
				unaEntrada.numeroEntrada=contadorDeEntradasInsertadas;
				contadorDeEntradasInsertadas=contadorDeEntradasInsertadas+1;
				unaEntrada.tamanioValor = 0;
				unaEntrada.bloquiado=1;

				dictionary_put(diccionarioEntradas,key,&unaEntrada);

				//persistirDiccionarioDeTablaDeEntradas(diccionarioEntradas);

				memcpy(&mapArchivoTablaDeEntradas,&diccionarioEntradas,sizeof(&diccionarioEntradas));


//				int sincronizo=msync(mapArchivoTablaDeEntradas, tamanioArchivo,	MS_SYNC);
				printf("mapArchivoTablaDeEntradas:%d\n t_dictionary:%d\n",sizeof(&mapArchivoTablaDeEntradas),sizeof(t_dictionary));
				int sincronizo=msync(mapArchivoTablaDeEntradas, sizeof(t_dictionary) ,	MS_SYNC);
				if (sincronizo == -1) {
										perror("No se puede sincronizar con el disco ");
									}else{
									printf("Se mapeo correctamente \n");}

if( dictionary_has_key(mapArchivoTablaDeEntradas,"messi")){
	printf("se mapeo bien Entrada y se encontro key messi\n");
}else{printf("no pude leer entrada\n");

};



			}

		}

			//FIN procesado GET
		} else {
			if (strcmp(sentenciaSeparada[0], "SET") == 0) {
				//INICIO procesado SET
				printf("INICIO procesado SET\n");


				//se accsede a la tabla de entradas
				if (dictionary_has_key(diccionarioEntradas, key)){
					struct t_entrada *unaEntrada;
					unaEntrada = dictionary_get(diccionarioEntradas, key);
					if(unaEntrada->bloquiado==1){
					//escribe entrada
						printf("Clave bloquiada y guardando valor \n");


						unaEntrada->tamanioValor = strlen(value);



						dictionary_put(diccionarioValoresEntradas,key,value);
						printf("Se guardo valor en diccionario par %s\n",dictionary_get(diccionarioValoresEntradas,key));
//		//RECONFIGURANDO LAS ENTRADAS MAXIMAS DEL SISTEMA
//						maximoNumeroEntradaInicio=unaEntrada->numeroEntrada;
//
//						  if ((unaEntrada->tamanioValor%entradasTamanio)==0){
//							  maximoNumeroEntradaFin=maximoNumeroEntradaInicio + (unaEntrada->tamanioValor/entradasTamanio);
//						  }else{
//							  maximoNumeroEntradaFin=maximoNumeroEntradaInicio + (unaEntrada->tamanioValor/entradasTamanio) + 1;
//						  }
//						  printf("Reconfigurado de maximoNumeroEntradaInicio:%d maximoNumeroEntradaFin:%d", maximoNumeroEntradaInicio,maximoNumeroEntradaFin);
//
//		//FIN DE RECONFIGURADO


					}else{
						//Error de Clave no Bloqueada
						printf("Error de Clave no Bloqueada \n");
						//Si la clave que intenta acceder no se encuentra tomada por el ESI en cuestión, se debera informar al Usuario y se debera abortar el ESI.
}
				}else{
					printf("Error de Clave no Identificada\n");
					//Error de Clave no Identificada
					//						Cuando un Usuario ejecute una operación de
					//						STORE o SET y la clave no exista, se deberá generar un error informando de dicha inexistencia al Usuario. Abortando el ESI culpable.

				}


//				char* entradaEnArchivo = string_new();
//				string_append(&entradaEnArchivo, key);
//				string_append(&entradaEnArchivo, ";");
//				string_append(&entradaEnArchivo,
//						string_itoa(unaEntrada.numeroEntrada));
//				string_append(&entradaEnArchivo, ";");
//				string_append(&entradaEnArchivo,
//						string_itoa(unaEntrada.tamanioValor));
//				string_append(&entradaEnArchivo, ";");
//
//				mapArchivoTablaDeEntradas = escribirEntrada(
//						mapArchivoTablaDeEntradas,
//						contadorEntradasEscritasEnTabla, entradaEnArchivo,
//						entradasCantidad, 255);
//				free(entradaEnArchivo);
//				if (msync(mapArchivoTablaDeEntradas, (entradasCantidad * 255),
//						MS_SYNC) == -1) {
//					perror("No se puede sincronizar con el disco ");
//				}
//				printf("Se mapeo correctamente \n"
//					);
//				contadorDeEntradasInsertadas=contadorDeEntradasInsertadas+1;
				printf("FIN procesado SET\n");
				//FIN procesado SET
			} else {
				if (strcmp(sentenciaSeparada[0], "STORE") == 0) {
					//INICIO procesado STORE
					printf("INICIO procesado STORE\n");

					if (dictionary_has_key(diccionarioEntradas, key) ) {
						//leer entrada en memoria

							struct t_entrada *unaEntrada;
							unaEntrada = dictionary_get(diccionarioEntradas,
									key);
						if(unaEntrada->bloquiado==1){
					char* pathArchivo = string_new();
					string_append(&pathArchivo, configuracion.punto_montaje);
					string_append(&pathArchivo, key);
					string_append(&pathArchivo, ".txt");
					//se accsede a la tabla de entradas
					int tamanioArchivo = unaEntrada->tamanioValor * sizeof(char) +1;
					if(remove(pathArchivo)){
						printf("Se elimino archivo satifactoriamente\n");}else{
							printf("no se elimino archivo\n")
						}

					int fd = open(pathArchivo, O_RDWR|O_TRUNC, (mode_t) 0600);
					if (fd <= -1) {
						close(fd);
						fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC,
								(mode_t) 0600);
						if (fd == -1) {
							printf("Error al abrir para escritura\n");
							exit(EXIT_FAILURE);
						}else{




							printf("STORE Entrada numeroEntrada:%d tamanioValor:%d\n",unaEntrada->numeroEntrada,unaEntrada->tamanioValor);
													/* Escribir entrada en archivo */



							struct t_entrada copiaEntrada;
							copiaEntrada.numeroEntrada=unaEntrada->numeroEntrada;
							copiaEntrada.tamanioValor=unaEntrada->tamanioValor;
							copiaEntrada.bloquiado=0;

						dictionary_remove(diccionarioEntradas, key);
						if(dictionary_has_key(diccionarioEntradas,key)!=1){
							printf("Elimino entrada\n");
						}
							dictionary_put(diccionarioEntradas, key,&copiaEntrada);
							if(dictionary_has_key(diccionarioEntradas,key)){printf("Inserto la nueva\n");}
						//	memcpy(&mapArchivoTablaDeEntradas,&diccionarioEntradas,sizeof(&diccionarioEntradas));
							char * value1=dictionary_get(diccionarioValoresEntradas, key);
													int result = write(fd, value1, tamanioArchivo);
printf("result=%d\n",result);
													if (result <0) {
														close(fd);
														printf(
																"Error al escribir  \n");
														exit(EXIT_FAILURE);
													}else{
														printf("se guardo archivo \n");
														close(fd);
													}
	    							}

							}
		    			 }else{
		    				 printf("La clave se encuentra pero no esta bloquiada\n");

		    			 }

						//FIN procesa do STORE
					} else {
						printf("Error de Clave no Identificada\n");

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
