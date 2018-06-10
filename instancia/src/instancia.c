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
#include <string.h>
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
#include "parsi/parser.h"
#include <math.h>
#include <string.h>

config_t cargarConfiguracion(char *path) {
	config_t config;

	t_config * config_Instancia = config_create(path);

	if (config_has_property(config_Instancia, "IP_COORDINADOR")) {
		config.ip_coordinador = config_get_string_value(config_Instancia,
				"IP_COORDINADOR");
		printf("IP_COORDINADOR: %s\n", config.ip_coordinador);
	}

	if (config_has_property(config_Instancia, "PUERTO_COORDINADOR")) {
		config.puerto_coordinador = (int32_t) config_get_int_value(
				config_Instancia, "PUERTO_COORDINADOR");
		printf("PUERTO_COORDINADOR: %d\n", config.puerto_coordinador);
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

int redondearArribaDivision(int divisor, int dividendo) {
	int resultado = ((divisor + dividendo - 1) / dividendo);
	return resultado;
}

void escribirAPartir(char* matriz, int puntoInicio, char* textoAEscribir) {
	int puntoFin = puntoInicio + strlen(textoAEscribir) - 1;
	int a = 0;
	int i = 0;
	for (a = puntoInicio; a <= puntoFin; a++) {
		matriz[a] = textoAEscribir[i];
		i++;
	}

//	return matriz;
}

char* leerDesdeHasta(char * matriz, int puntoInicio, int puntoFin) {
	char* texto = malloc(puntoFin - puntoInicio);
	printf("leerDesdeHasta INICIO Texto %s \n", texto);
	printf("leerDesdeHasta puntoInicio %d puntoFin %d \n", puntoInicio,
			puntoFin);

	int a = 0;
	int i = 0;
	for (a = puntoInicio; a < puntoFin; a++) {
		texto[i] = matriz[a];
		i++;
	}
	texto[puntoFin - puntoInicio] = '\0';

	printf("leerDesdeHasta fin Texto %s \n", texto);
	printf("leerDesdeHasta  logitud strlen %d \n", strlen(texto));
	printf("leerDesdeHasta string_length %d\n", string_length(texto));

	return texto;
}

void escribirEntrada(char* matriz, int tamanioEntradas, int numEntrada, char* textoAEscribir) {
	escribirAPartir(matriz, tamanioEntradas * numEntrada, textoAEscribir);
//	return texto;
}

char* leerEntrada(char * matriz, int tamanioEntradas, int numEntrada, int longitud) {
	int puntoInicio = tamanioEntradas * numEntrada;
	int puntoFin = tamanioEntradas * numEntrada + longitud;
	return leerDesdeHasta(matriz, puntoInicio, puntoFin);
}

char * crearMatriz(int numeroEntradas, int tamanioEntradas) {
	char * matriz = malloc(entradasCantidad * entradasTamanio);
	strcpy(matriz, "");
	int i = 0;
	for (i = 0; i <= numeroEntradas * tamanioEntradas; i++) {
		matriz[i] = '\0';
	}
	return matriz;
}

void imprimirPorPantallaEstucturas(t_entrada* matriz, t_dictionary* diccionario, char* matrizDeChar, int cantidadEntradas, int tamanioEntradas) {
	if (seguir == 2) {
		int i = 0;
		for (i = 0; i < cantidadEntradas; i++) {
			if (dictionary_has_key(diccionario, matriz[i].clave)) {
				printf("-------Diccionario:  %s %d ", matriz[i].clave, (*(int*)dictionary_get(diccionario, matriz[i].clave)));
				printf(" MatrizMap:%d %d %d %d ", i, matriz[i].numeroEntrada, matriz[i].tamanioValor, matriz[i].tiempo);
				char* texto = leerEntrada(matrizDeChar, tamanioEntradas, matriz[i].numeroEntrada, matriz[i].tamanioValor);
				printf(" MatrizChar Valor: %s\n", texto);
				free(texto);
			}
		}
	}
}


void sustituirMatrizEntradas(char * algoritmo,int  punteroIUltimoInsertadoMatriz,t_entrada * matriz,t_dictionary * diccionario,char * clave){

	printf("Sustituye Algoritmo declarado %s\n",algoritmo);
	if(string_equals_ignore_case(algoritmo,"Ciclico")){
		//busco siguiente atomico
		int i = 0;
		if (punteroIUltimoInsertadoMatriz + 1 < entradasCantidad) {
			i = punteroIUltimoInsertadoMatriz + 1;
		} else {
			i = 0;
		}
		int fin = 0;
		while (fin == 0) {
			if (redondearArribaDivision(matriz[i].tamanioValor, entradasTamanio) == 1) {
				//inserto directo clave
				printf("Se remueve de diccionari y matriz clave %s \n", matriz[i].clave);
				dictionary_remove(diccionario, matriz[i].clave);
				dictionary_put(diccionario, clave, &i);
				strcpy(matriz[i].clave, clave);

				//inserto tamanio valor -1
				matriz[i].tamanioValor=-1;
				matriz[i].tiempo=-1;

				fin = 1;
			} else {
				if (redondearArribaDivision(matriz[i].tamanioValor, entradasTamanio) == 0) {
					i++;
					if (i == entradasCantidad) {
						i = 0;
					}
				} else {
					if (i + redondearArribaDivision(matriz[i].tamanioValor, entradasTamanio) < entradasCantidad) {
						//salto al proximo
						i = i + redondearArribaDivision(matriz[i].tamanioValor, entradasTamanio);
					} else {
						i = 0;
					}
				}
			}
		}
		//pongo la clave en matriz y diccionario
		//cambio tamaniovalor de matriz y timer
	} else {
		if (string_equals_ignore_case(algoritmo,"LRU")) {
			int i = 0;
			int aSustituir = -1;
			int aSustituirTiempo = 0;
			while (i < entradasCantidad) {
				if (redondearArribaDivision(matriz[i].tamanioValor, entradasTamanio) == 1) {
					if (timer - matriz[i].tiempo > aSustituirTiempo) {
						aSustituirTiempo = matriz[i].tiempo;
						aSustituir = i;
					}
				}
				printf("IdEntrada: %d timerEntrada: %d timerSistema: %d \n",i ,matriz[i].tiempo, timer);
				if (i + redondearArribaDivision(matriz[i].tamanioValor, entradasTamanio) <= entradasCantidad) {
					//salto al proximo
					i = i + redondearArribaDivision(matriz[i].tamanioValor, entradasTamanio);
				}
			}
			if (aSustituir > -1) {
				//remuevo y sustituyo
				printf("\n a sustituir %s\n", (char*)dictionary_get(diccionario, matriz[i].clave));
				dictionary_remove(diccionario, matriz[i].clave);
				dictionary_put(diccionario, clave, &aSustituir);
				strcpy(matriz[aSustituir].clave, clave);
				//inserto tamanio valor -1
				matriz[aSustituir].tamanioValor=-1;
				matriz[aSustituir].tiempo=-1;
				printf("removido por %s\n", (char*)dictionary_get(diccionario,matriz[i].clave));
			}
		} else if (string_equals_ignore_case(algoritmo, "BSU")) {
				int i = 0;
				int aSustituir = -1;
				int aSustituirTamanio = 0;
				while (i < entradasCantidad) {
					if (redondearArribaDivision(matriz[i].tamanioValor, entradasTamanio) == 1) {
						if (matriz[i].tamanioValor > aSustituirTamanio) {
							aSustituirTamanio=matriz[i].tamanioValor;
							aSustituir=i;
						}
					}
					if (i + redondearArribaDivision(matriz[i].tamanioValor, entradasTamanio) <= entradasCantidad) {
						//salto al proximo
						i = i + redondearArribaDivision(matriz[i].tamanioValor, entradasTamanio);
					}
				}
				if (aSustituir > -1) {
					//remuevo y sustituyo
					int valorRemover = -1;
					dictionary_put(diccionario, matriz[aSustituir].clave, &valorRemover);

					if ((*(int*)dictionary_get(diccionario, matriz[aSustituir].clave)) == -1) {
						printf("Se realizo el put de -1 en la clave\n");
					}
					dictionary_remove_and_destroy(diccionario, matriz[aSustituir].clave, free);
					dictionary_put(diccionario, clave, &aSustituir);
					strcpy(matriz[aSustituir].clave,clave);

					//inserto tamanio valor -1
					matriz[aSustituir].tamanioValor=-1;
					matriz[aSustituir].tiempo=-1;
					printf("removido por %s\n", (char*)dictionary_get(diccionario, matriz[i].clave));
				}

		} else {
			if (string_equals_ignore_case(algoritmo, "BSU")) {

			}

		}
}
}

int main(int argc, char *argv[]) {

	//Creo el log
	log_errores = log_create("instancia.log", "Instance", true, LOG_LEVEL_ERROR);

	config_t configuracion;

	if (argc > 1) {
		/*llamo a la funcion de creacion de archivo de configuracion con la estructura t_config*/
		char* pathConfiguracion = argv[1];
		printf("Path de configuracion: %s\n", pathConfiguracion);
		configuracion = cargarConfiguracion(pathConfiguracion);
		printf("------------- FIN CONFIGURACION -------------\n");
	}

	//coneccion coordinador
	int socketCoordinador = conectarConProceso(configuracion.ip_coordinador, configuracion.puerto_coordinador, Instancia);
	if (socketCoordinador < 0) {
		log_error(log_errores, "Error conectar_a_server");
		exit(1);
	}

	//Envio de envio de nombre
	header_t header;
	header.comando = msj_nombre_instancia;
	header.tamanio = strlen(configuracion.nombre_instancia) + 1;
	void* buffer = serializar(header, configuracion.nombre_instancia);
	enviar_mensaje(socketCoordinador, buffer, sizeof(header_t) + header.tamanio);

	//recibir entradas y tamanio
	entradasCantidad = 0;
	entradasTamanio = 0;

	int resultado = recibir_mensaje(socketCoordinador, &header, sizeof(header_t));
	if ((resultado == ERROR_RECV) || !(header.comando == msj_cantidad_entradas)) {
		printf("Error al intentar recibir cantidad de entradas\n");
	}
	recibir_mensaje(socketCoordinador, &entradasCantidad, header.tamanio);

	resultado = recibir_mensaje(socketCoordinador, &header, sizeof(header_t));
	if ((resultado == ERROR_RECV) || !(header.comando == msj_tamanio_entradas)) {
		printf("Error al intentar recibir tamaño de entradas\n");
	}
	recibir_mensaje(socketCoordinador, &entradasTamanio, header.tamanio);

	printf("EntradasCantidad: %d entradasTamanio: %d \n ", entradasCantidad, entradasTamanio);

	//	mapeo de archivo ArchivoMatrizDeEntradas
	char* pathArchivo = string_new();
	string_append(&pathArchivo, configuracion.punto_montaje);
	char* nombreTablaDeEntradas = "TablaDeEntradas";
	string_append(&pathArchivo, nombreTablaDeEntradas);
	string_append(&pathArchivo, ".bin");
	printf("Path de archivo creado y mapeado o solo mapeado: %s\n:", pathArchivo);

	/*Abrir o crear archivo de una entrada*/
	printf("Abriendo %s: \n", pathArchivo);
	int tamanioArchivoTablaEntradas = sizeof(t_entrada) * entradasCantidad;
	int fd = open(pathArchivo, O_RDWR, (mode_t) 0600);
	int archivoNoExistiaPrimeraCarga = 0;
	if (fd <= -1) {
		close(fd);
		fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
		archivoNoExistiaPrimeraCarga = 1;
		if (fd == -1) {
			printf("Error al abrir para escritura\n");
			exit(EXIT_FAILURE);
		}
		/* confirmando tamaño nuevo		     */
		int result = lseek(fd, tamanioArchivoTablaEntradas - 1, SEEK_SET);
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
	char * matrizValoresEntradas = crearMatriz(entradasCantidad,
			entradasTamanio);

	/* Mapiando archivo */

	t_entrada * mapArchivoTablaDeEntradas = mmap(0, tamanioArchivoTablaEntradas,
	PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mapArchivoTablaDeEntradas == MAP_FAILED) {
		close(fd);
		printf("Error al mapiar achivo\n");
		exit(EXIT_FAILURE);
	}
	printf("Se mapio archivo \n");
	int i = 0;

	for (i = 0; i < entradasCantidad && archivoNoExistiaPrimeraCarga == 1; i++) {
		mapArchivoTablaDeEntradas[i].numeroEntrada = -1;
		mapArchivoTablaDeEntradas[i].tamanioValor = -1;
		mapArchivoTablaDeEntradas[i].tiempo = -1;
		//mapArchivoTablaDeEntradas[i].clave[0] = '\0';
		printf("%d %d %d \n", mapArchivoTablaDeEntradas[i].numeroEntrada,
							  mapArchivoTablaDeEntradas[i].tamanioValor,
							  mapArchivoTablaDeEntradas[i].tiempo);
	}

	free(pathArchivo);
	t_dictionary *diccionarioEntradas = dictionary_create();

	//se carga diccionarioEntradas, matrizValoresEntradas con datos de mapArchivoTablaDeEntradas
	for (i = 0; i < entradasCantidad && archivoNoExistiaPrimeraCarga == 0; i++) {

		if (mapArchivoTablaDeEntradas[i].clave[0] != '\0') {
			dictionary_put(diccionarioEntradas, mapArchivoTablaDeEntradas[i].clave, &i);
			char* pathArchivo = string_new();
			string_append(&pathArchivo, configuracion.punto_montaje);
			string_append(&pathArchivo, "DUMP/");
			string_append(&pathArchivo, mapArchivoTablaDeEntradas[i].clave);
			string_append(&pathArchivo, ".txt");
			FILE* file = fopen(pathArchivo, "r");

			char *valor = malloc(mapArchivoTablaDeEntradas[i].tamanioValor + 1);
			fgets(valor, mapArchivoTablaDeEntradas[i].tamanioValor + 1, file);
			escribirEntrada(matrizValoresEntradas, entradasTamanio, mapArchivoTablaDeEntradas[i].numeroEntrada, valor);
			free(valor);

			fclose(file);
			free(pathArchivo);
		}

		//cargar diccionario con lo rescatado en el archivo
	}

	//inicio de while
	timer = 0;
	timerDump = 0;

	for (;;) {
		printf("\n");
		//Recibir sentencia de Re Distinto
		header_t header2;
		header2.comando=0;
		header2.tamanio=0;

		int resultado = recibir_mensaje(socketCoordinador, &header2,
				sizeof(header_t));
		if (resultado == ERROR_RECV_DISCONNECTED
				|| resultado == ERROR_RECV_DISCONNECTED) {
			printf("Se perdió la conexión con el Coordinador\n");
		}

		void* buffer = malloc(header2.tamanio);
		resultado = recibir_mensaje(socketCoordinador, buffer, header2.tamanio);
		if (resultado == ERROR_RECV_DISCONNECTED
				|| resultado == ERROR_RECV_DISCONNECTED) {
			printf("Se perdió la conexión con el Coordinador\n");
		}

		switch (header2.comando) {
		char* clave;
		char* valor;
		case msj_sentencia_get:
			clave = (char*) buffer;

			//INICIO procesado GET

			printf("INICIO procesado GET\n");
			imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas,
					diccionarioEntradas, matrizValoresEntradas, entradasCantidad,
					entradasTamanio);
			//se accede a la tabla de entradas si no la encuentra la crea

			if (dictionary_has_key(diccionarioEntradas, clave)) {
				printf("Exite la clave\n");

				int* posicionTablaDeEntradas = dictionary_get(diccionarioEntradas, clave);
				t_entrada unaEntrada = mapArchivoTablaDeEntradas[(*posicionTablaDeEntradas)];

				printf("GET Entrada  numeroEntrada:%d tamanioValor:%d \n", unaEntrada.numeroEntrada, unaEntrada.tamanioValor);

			} else {
				printf("No exite la clave. La crea.\n");
				int fin = 0;
				int i = 0;
				while (mapArchivoTablaDeEntradas[i].numeroEntrada != -1 && fin == 0) {
					if (i == entradasCantidad) {
						//sustituir
						sustituirMatrizEntradas(configuracion.algoritmo_remplazo,
								punteroIUltimoInsertadoMatriz,
								mapArchivoTablaDeEntradas, diccionarioEntradas,
								clave);
						fin = 1;
					} else {
						printf("mapArchivoTablaDeEntradas con clave ocupada indice %d, numeroEntrada: %d tamanioValor %d\n",
								i, mapArchivoTablaDeEntradas[i].numeroEntrada,
								mapArchivoTablaDeEntradas[i].tamanioValor);
						i = i + redondearArribaDivision(mapArchivoTablaDeEntradas[i].tamanioValor, entradasTamanio) - 1;
					}
					i++;
				}

				if (mapArchivoTablaDeEntradas[i].numeroEntrada == -1 && fin == 0) {
					printf("mapArchivoTablaDeEntradas Se encontro clave libre indice %d, numeroEntrada: %d\n",
							i, mapArchivoTablaDeEntradas[i].numeroEntrada);
					dictionary_put(diccionarioEntradas, clave, &i);
					strcpy(mapArchivoTablaDeEntradas[i].clave, clave);
					punteroIUltimoInsertadoMatriz = i;
				}
			}
			imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas, diccionarioEntradas, matrizValoresEntradas, entradasCantidad, entradasTamanio);
			//FIN procesado GET
			break;

		case msj_sentencia_set:
			//INICIO procesado SET
			printf("INICIO procesado SET\n");
			// Vos fuma que esta lógica de punteros no falla
			clave = (char*) buffer;
			int index = strlen(clave);
			valor = (char*) buffer + index + 1;

			imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas, diccionarioEntradas, matrizValoresEntradas, entradasCantidad, entradasTamanio);

			//se accede a la tabla de entradas
			if (dictionary_has_key(diccionarioEntradas, clave)) {

				int i = (*(int*)dictionary_get(diccionarioEntradas, clave));

				int cantidadDeEntradasAUsar = redondearArribaDivision(strlen(valor), entradasTamanio);

				printf("texto: %s  valor:%d cantidadDeEntradasAUsar: %d\n", valor, strlen(valor), cantidadDeEntradasAUsar);
				//primera vez que se inserta
				printf("Antes de set mapArchivoTablaDeEntradas indice %d numeroEntrada %d\n", i, mapArchivoTablaDeEntradas[i].numeroEntrada);
				if (mapArchivoTablaDeEntradas[i].numeroEntrada == -1) {
					//busqueda de entradas contiguas
					int a = 0;
					int contadorContiguas = 0;
					int numeroEntradaInicialAInsertar = -1;
					for (a = 0; a < entradasCantidad && cantidadDeEntradasAUsar != contadorContiguas; a++) {

						if (mapArchivoTablaDeEntradas[a].numeroEntrada == -1) {
							contadorContiguas++;
						} else {
							//existe entrada y se calcula cuantas entradas ocupa
							contadorContiguas = 0;
							a = a + redondearArribaDivision(mapArchivoTablaDeEntradas[a].tamanioValor, entradasTamanio) - 1;
						}
						if (contadorContiguas == cantidadDeEntradasAUsar) {
							numeroEntradaInicialAInsertar = a - contadorContiguas + 1;
						}
					}

					//se actualiza tamanio valor de matriz de entrada
					mapArchivoTablaDeEntradas[i].tamanioValor = strlen(valor);
					//se inserta datos en matriz de valores

	//				matrizValoresEntradas = escribirEntrada(matrizValoresEntradas,
	//						entradasTamanio, numeroEntradaInicialAInsertar, valor);
					escribirEntrada(matrizValoresEntradas,
											entradasTamanio, numeroEntradaInicialAInsertar, valor);
					printf("despues de set mapArchivoTablaDeEntradas indice %d numeroEntrada %d\n", i, numeroEntradaInicialAInsertar);
					char* texto2 = leerEntrada(matrizValoresEntradas, entradasTamanio, numeroEntradaInicialAInsertar, mapArchivoTablaDeEntradas[i].tamanioValor);
					printf("Se inserto en matriz de valores el valor %s\n", texto2);
	//				free(texto2);

					//si ya habia asignado un indice de la matriz de entrada, se sustituye por una entrada una sola segun lo que me dijo lucas
					mapArchivoTablaDeEntradas[i].tamanioValor = string_length(
							valor);
	//				matrizValoresEntradas = escribirEntrada(matrizValoresEntradas,
	//						entradasTamanio,
	//						mapArchivoTablaDeEntradas[i].numeroEntrada, valor);
					escribirEntrada(matrizValoresEntradas, entradasTamanio, mapArchivoTablaDeEntradas[i].numeroEntrada, valor);

					printf("en matriz %d tamanioValor %d escribioentrada %s\n", i, mapArchivoTablaDeEntradas[i].tamanioValor, valor);

				}

			} else {
				printf("Error de Clave no Identificada\n");
				//Error de Clave no Identificada
				//Cuando un Usuario ejecute una operación de
				//STORE o SET y la clave no exista, se deberá generar un error informando de dicha inexistencia al Usuario. Abortando el ESI culpable.
			}
			imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas, diccionarioEntradas, matrizValoresEntradas, entradasCantidad, entradasTamanio);
			printf("FIN procesado SET\n");
			//FIN procesado SET
			break;

		case msj_sentencia_store:
			//INICIO procesado STORE
			printf("INICIO procesado STORE\n");
			clave = (char*) buffer;
			imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas, diccionarioEntradas, matrizValoresEntradas, entradasCantidad, entradasTamanio);

			if (dictionary_has_key(diccionarioEntradas, clave)) {
				//leer entrada en memoria
				int i = (*(int*)dictionary_get(diccionarioEntradas, clave));
				char* pathArchivo = string_new();
				string_append(&pathArchivo, configuracion.punto_montaje);
				string_append(&pathArchivo, clave);
				string_append(&pathArchivo, ".txt");

				//se accede a la tabla de entradas
				char* textoValor = leerEntrada( matrizValoresEntradas, entradasTamanio, mapArchivoTablaDeEntradas[i].numeroEntrada, mapArchivoTablaDeEntradas[i].tamanioValor);
				printf("texto: %s\n", textoValor);
				int tamanioArchivo = strlen(textoValor) * sizeof(char);
				if (remove(pathArchivo)) {
					printf("Se eliminó archivo satifactoriamente\n");
				} else {
					printf("No se eliminó archivo\n");
				}

				fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
				if (fd == -1) {
					printf("Error al abrir para escritura\n");
					exit(EXIT_FAILURE);
				} else {

					int result = write(fd, textoValor,
							tamanioArchivo);

					printf("result=%d\n", result);
					if (result < 0) {
						close(fd);
						printf("Error al escribir  \n");
						exit(EXIT_FAILURE);
					} else {
						printf("se guardo archivo \n");
						close(fd);
						printf("texto de de tabla num i clave:%s\n", mapArchivoTablaDeEntradas[i].clave);
						mapArchivoTablaDeEntradas[i].tiempo=timer;
	//									//se elimina de memoria la entrada y del diccionario
	//									dictionary_remove(diccionarioEntradas,mapArchivoTablaDeEntradas[i].clave);
	//									mapArchivoTablaDeEntradas[i].clave[0]='\0';
	//									mapArchivoTablaDeEntradas[i].numeroEntrada=-1;
	//									mapArchivoTablaDeEntradas[i].tamanioValor=-1;
	//									mapArchivoTablaDeEntradas[i].tiempo=-1;
					}
				}
				free(textoValor);
				imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas, diccionarioEntradas, matrizValoresEntradas, entradasCantidad, entradasTamanio);
				printf("FIN procesa do STORE\n");
				//FIN procesado STORE
			} else {
				imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas, diccionarioEntradas, matrizValoresEntradas, entradasCantidad, entradasTamanio);
				printf("Error de Clave no Identificada\n");
				printf("FIN procesa do STORE\n");
			}
			break;
		}

		//proceso dump en cantidad de ciclos
		if (configuracion.intervalo_dump == timerDump + 1) {

			//recorro matriz de entradas de 0 a cantidad de entradas
			i = 0;
			while (i < entradasCantidad) {
				if (mapArchivoTablaDeEntradas[i].numeroEntrada > -1
						&& mapArchivoTablaDeEntradas[i].numeroEntrada < entradasCantidad
						&& mapArchivoTablaDeEntradas[i].tamanioValor > -1) {

					char* pathArchivo = string_new();
					string_append(&pathArchivo, configuracion.punto_montaje);
					string_append(&pathArchivo, "DUMP/");
					string_append(&pathArchivo,
							mapArchivoTablaDeEntradas[i].clave);
					string_append(&pathArchivo, ".txt");
					//se accede a la tabla de entradas
					char* textoValor = leerEntrada(matrizValoresEntradas, entradasTamanio, mapArchivoTablaDeEntradas[i].numeroEntrada, mapArchivoTablaDeEntradas[i].tamanioValor);
					int tamanioArchivo = 0;
					tamanioArchivo = string_length(textoValor);
					printf("IMPORTANTE linea 746 texto: %s %d\n", textoValor, tamanioArchivo);

					if (remove(pathArchivo)) {
						printf("Se eliminó archivo satifactoriamente\n");
					} else {
						printf("No se eliminó archivo\n");
					}

					fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
					if (fd == -1) {
						printf("Error al abrir para escritura\n");
						exit(EXIT_FAILURE);
					} else {
						int result = write(fd, textoValor, tamanioArchivo);
						printf("result=%d\n", result);
						if (result < 0) {
							close(fd);
							printf("Error al escribir  \n");
							exit(EXIT_FAILURE);
						} else {
							printf("se guardo archivo \n");
							close(fd);

						}
					}
					free(textoValor);
				}//TERMINO EL IF

				if (redondearArribaDivision( mapArchivoTablaDeEntradas[i].tamanioValor, entradasTamanio) == 0) {
					i++;
				} else {
					i = i + redondearArribaDivision(mapArchivoTablaDeEntradas[i].tamanioValor, entradasTamanio);
				}
			}
			timerDump = 0;
		} else {
			timerDump++;
		}
		//timer suma 1
		timer++;
	}

	close(socketCoordinador);
	return EXIT_SUCCESS;

}
