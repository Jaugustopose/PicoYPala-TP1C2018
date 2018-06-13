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
#include <errno.h>

int entradasCantidad ;
int entradasTamanio ;
int punteroIUltimoInsertadoMatriz;
int timer = 0;
int timerDump = 0;
config_t configuracion;
t_log * logInstancia;
int socketCoordinador;
char* matrizValoresEntradas;
t_entrada * mapArchivoTablaDeEntradas;
t_dictionary *diccionarioEntradas;
int archivoNuevo;
int tamanioArchivoTablaEntradas;

void exitFailure(){
	log_destroy(logInstancia);
	close(socketCoordinador);
	exit(EXIT_FAILURE);
}

void exitSuccess(){
	log_destroy(logInstancia);
	close(socketCoordinador);
	exit(EXIT_SUCCESS);
}

config_t cargarConfiguracion() {
	config_t config;
	char* pat = string_new();
	char cwd[1024]; // Variable donde voy a guardar el path absoluto hasta el /Debug
	string_append(&pat, getcwd(cwd, sizeof(cwd)));
	if (string_contains(pat, "/Debug")) {
		string_append(&pat, "/instancia.cfg");
	} else {
		string_append(&pat, "/Debug/instancia.cfg");
	}
	log_debug(logInstancia, "Path archivo configuración: %s", pat);
	t_config* configInstancia = config_create(pat);
	if(!configInstancia){
		log_error(logInstancia, "No se encontró archivo de configuración.");
		exitFailure();
	}

	if (config_has_property(configInstancia, "IP_COORDINADOR")) {
		config.ip_coordinador = config_get_string_value(configInstancia,
				"IP_COORDINADOR");
		log_debug(logInstancia, "IP_COORDINADOR: %s", config.ip_coordinador);
	}else{
		log_error(logInstancia, "No se encontró IP_COORDINADOR en el archivo de configuraciones.");
		exitFailure();
	}

	if (config_has_property(configInstancia, "PUERTO_COORDINADOR")) {
		config.puerto_coordinador = (int32_t) config_get_int_value(
				configInstancia, "PUERTO_COORDINADOR");
		log_debug(logInstancia, "PUERTO_COORDINADOR: %d", config.puerto_coordinador);
	}else{
		log_error(logInstancia, "No se encontró PUERTO_COORDINADOR en el archivo de configuraciones.");
		exitFailure();
	}

	if (config_has_property(configInstancia, "ALGORITMO_REMPLAZO")) {
		config.algoritmo_remplazo = config_get_string_value(configInstancia,
				"ALGORITMO_REMPLAZO");
		log_debug(logInstancia, "ALGORITMO_REMPLAZO: %s", config.algoritmo_remplazo);
	}else{
		log_error(logInstancia, "No se encontró ALGORITMO_REMPLAZO en el archivo de configuraciones.");
		exitFailure();
	}

	if (config_has_property(configInstancia, "PUNTO_MONTAJE")) {
		config.punto_montaje = config_get_string_value(configInstancia,
				"PUNTO_MONTAJE");
		log_debug(logInstancia, "PUNTO_MONTAJE: %s", config.punto_montaje);
	}else{
		log_error(logInstancia, "No se encontró PUNTO_MONTAJE en el archivo de configuraciones.");
		exitFailure();
	}

	if (config_has_property(configInstancia, "NOMBRE_INSTANCIA")) {
		config.nombre_instancia = config_get_string_value(configInstancia,
				"NOMBRE_INSTANCIA");
		log_debug(logInstancia, "NOMBRE_INSTANCIA: %s", config.nombre_instancia);
	}else{
		log_error(logInstancia, "No se encontró NOMBRE_INSTANCIA en el archivo de configuraciones.");
		exitFailure();
	}

	if (config_has_property(configInstancia, "INTERVALO_DUMP")) {
		config.intervalo_dump = (int) config_get_int_value(configInstancia,
				"INTERVALO_DUMP");
		log_debug(logInstancia, "INTERVALO_DUMP: %d", config.intervalo_dump);
	}else{
		log_error(logInstancia, "No se encontró INTERVALO_DUMP en el archivo de configuraciones.");
		exitFailure();
	}
	free(configInstancia);
	return config;
}

void inicializarComunicacionCordinadoor(){
	//Envio nombre instancia
	header_t header;
	header.comando = msj_nombre_instancia;
	header.tamanio = strlen(configuracion.nombre_instancia) + 1;
	void* buffer = serializar(header, configuracion.nombre_instancia);
	int retorno = enviar_mensaje(socketCoordinador, buffer, sizeof(header_t) + header.tamanio);
	free(buffer);
	if (retorno < 0) {
		log_error(logInstancia, "Se perdió la conexión con el Coordinador");
		exitFailure();
	}

	//Recibo Cantidad de Entradas
	int resultado = recibir_mensaje(socketCoordinador, &header, sizeof(header_t));
	if ((resultado == ERROR_RECV) || !(header.comando == msj_cantidad_entradas)) {
		log_error(logInstancia, "Error al intentar recibir cantidad de entradas");
		exitFailure();
	}
	recibir_mensaje(socketCoordinador, &entradasCantidad, header.tamanio);

	//Recibo Tamaño de Entradas
	resultado = recibir_mensaje(socketCoordinador, &header, sizeof(header_t));
	if ((resultado == ERROR_RECV) || !(header.comando == msj_tamanio_entradas)) {
		log_error(logInstancia, "Error al intentar recibir tamaño de entradas");
		exitFailure();
	}
	recibir_mensaje(socketCoordinador, &entradasTamanio, header.tamanio);

	log_debug(logInstancia, "Cantidad Entradas: %d Tamaño Entradas: %d", entradasCantidad, entradasTamanio);
}

int abrirArchivoTablaEntradas(){
	//Path archivo Tabla de Entradas
	char* pathArchivo = string_new();
	string_append(&pathArchivo, configuracion.punto_montaje);
	string_append(&pathArchivo, "TablaDeEntradas.bin");
	log_debug(logInstancia, "Path para mmap de Tabla de Entradas: %s", pathArchivo);

	//Calculo tamaño del archivo para hacer mmap
	tamanioArchivoTablaEntradas = sizeof(t_entrada) * entradasCantidad;
	//Intento abrir archivo previamente creado
	int fd = open(pathArchivo, O_RDWR, (mode_t) 0600);
	archivoNuevo = 0;
	if (fd <= -1) {
		//Si no pudo abrir un archivo viejo se crea uno nuevo
		fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
		archivoNuevo = 1;
		if (fd == -1) {
			log_error(logInstancia, "Error al crear el archivo de Tabla de Entradas");
			exitFailure();
		}
		/*Al crear una archivo nuevo se le escribe un string vacio al final para
		 *aumentar su tamaño al necesario para guardar la Tabla de Entradas completa*/
		int result = lseek(fd, tamanioArchivoTablaEntradas - 1, SEEK_SET);
		if (result == -1) {
			close(fd);
			log_error(logInstancia, "Error al saltar con lseek");
			exitFailure();
		}
		result = write(fd, "", 1);
		if (result != 1) {
			close(fd);
			log_error(logInstancia, "Error al escribir ultimo byte del archivo");
			exitFailure();
		}

	}
	free(pathArchivo);
	return fd;
}

t_entrada* mapearTablaEntradas(int fd){
	t_entrada* retorno = malloc(sizeof(t_entrada) * entradasCantidad);
	retorno = mmap(0, tamanioArchivoTablaEntradas, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	//Checkeo error de mmap
	if (mapArchivoTablaDeEntradas == MAP_FAILED) {
		close(fd);
		log_error(logInstancia, "Error al mapear Tabla de Entradas");
		exitFailure();
	}
	log_debug(logInstancia, "Se mapeo Tabla de Entradas correctamente");
	return retorno;
}

void inicializarTablaEntradas(){
	int i = 0;
	if(archivoNuevo){
		//Si el archivo es nuevo se lo inicializa en -1
		for (i = 0; i < entradasCantidad; i++) {
			mapArchivoTablaDeEntradas[i].numeroEntrada = -1;
			mapArchivoTablaDeEntradas[i].tamanioValor = -1;
			mapArchivoTablaDeEntradas[i].tiempo = -1;
		}
	}else{
		//Si el archivo ya existia se carga diccionarioEntradas y matrizValoresEntradas con sus datos
		for (i = 0; i < entradasCantidad; i++) {
			if (mapArchivoTablaDeEntradas[i].clave[0] != '\0') {
				/* Cambie el &i por un compound literal porque guardar la direccion de i hace que todos
				 * las entradas tengan el mismo i, y para terminar de cagarla i se borra cuando se sale
				 * de esta funcion (Un compound literal es como hacer malloc de un int y ponerle el valor de i
				 * */
				dictionary_put(diccionarioEntradas, mapArchivoTablaDeEntradas[i].clave, &(int){ i });
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
				free(pathArchivo);
				fclose(file);
			}

			//cargar diccionario con lo rescatado en el archivo
		}
	}


}

void inicializarEstructurasAdministrativas(){
	//Creo e inicializo matriz de Entradas
	matrizValoresEntradas = crearMatrizEntradas(entradasCantidad, entradasTamanio);
	//Creo diccionario de entradas
	diccionarioEntradas = dictionary_create();
	//Abro archivo para mmap
	int fd = abrirArchivoTablaEntradas();
	//Mapeo archivo a memoria
	t_entrada* mapArchivoTablaDeEntradas = malloc(sizeof(t_entrada) * entradasCantidad);
	mapArchivoTablaDeEntradas = mapearTablaEntradas(fd);
	//Inicializar Tabla de Entradas
	inicializarTablaEntradas();
}

int redondearArribaDivision(int divisor, int dividendo) {
	int resultado = ((divisor + dividendo - 1) / dividendo);
	return resultado;
}

void escribirAPartir(char* matriz, int puntoInicio, char* textoAEscribir) {
	int puntoFin = puntoInicio + strlen(textoAEscribir);
	int a = 0;
	int i = 0;
	for (a = puntoInicio; a < puntoFin; a++) {
		matriz[a] = textoAEscribir[i];
		i++;
	}
}

char* leerDesdeHasta(char * matriz, int puntoInicio, int puntoFin) {
	char* texto = malloc(puntoFin - puntoInicio + 1);
	printf("leerDesdeHasta INICIO\n");
	printf("leerDesdeHasta puntoInicio %d puntoFin %d \n", puntoInicio, puntoFin);

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
}

char* leerEntrada(char * matriz, int tamanioEntradas, int numEntrada, int longitud) {
	int puntoInicio = tamanioEntradas * numEntrada;
	int puntoFin = tamanioEntradas * numEntrada + longitud;
	return leerDesdeHasta(matriz, puntoInicio, puntoFin);
}

char * crearMatrizEntradas(int numeroEntradas, int tamanioEntradas) {
	char * matriz = malloc(entradasCantidad * entradasTamanio);
	int i = 0;
	for (i = 0; i < numeroEntradas * tamanioEntradas; i++) {
		matriz[i] = '\0';
	}
	return matriz;
}

void imprimirPorPantallaEstucturas(t_entrada* matriz, t_dictionary* diccionario, char* matrizDeChar, int cantidadEntradas, int tamanioEntradas) {
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

void ejecutarGet(void* buffer){
	char* clave = (char*) buffer;

	log_debug(logInstancia, "INICIO procesado GET");
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
}

void ejecutarSet(void* buffer){
	//INICIO procesado SET
	printf("INICIO procesado SET\n");
	// Vos fuma que esta lógica de punteros no falla
	char* clave = (char*) buffer;
	int index = strlen(clave);
	char* valor = (char*) buffer + index + 1;

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
			escribirEntrada(matrizValoresEntradas, entradasTamanio, numeroEntradaInicialAInsertar, valor);
			mapArchivoTablaDeEntradas[i].numeroEntrada = numeroEntradaInicialAInsertar;
			printf("despues de set mapArchivoTablaDeEntradas indice %d numeroEntrada %d\n", i, numeroEntradaInicialAInsertar);
			char* texto2 = leerEntrada(matrizValoresEntradas, entradasTamanio, numeroEntradaInicialAInsertar, mapArchivoTablaDeEntradas[i].tamanioValor);
			printf("Se inserto en matriz de valores el valor %s\n", texto2);
//				free(texto2);

			//si ya habia asignado un indice de la matriz de entrada, se sustituye por una entrada una sola segun lo que me dijo lucas
			mapArchivoTablaDeEntradas[i].tamanioValor = string_length(valor);
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
}

void ejecutarStore(void* buffer){
	//INICIO procesado STORE
	printf("INICIO procesado STORE\n");
	char* clave = (char*) buffer;
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

		int fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
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
		printf("FIN procesado STORE\n");
		//FIN procesado STORE
	} else {
		imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas, diccionarioEntradas, matrizValoresEntradas, entradasCantidad, entradasTamanio);
		printf("Error de Clave no Identificada\n");
		printf("FIN procesado STORE\n");
	}
}

void procesarDump(){
	//proceso dump en cantidad de ciclos
	if (configuracion.intervalo_dump == timerDump + 1) {

		//recorro matriz de entradas de 0 a cantidad de entradas
		int i = 0;
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

				int fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
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

header_t recibirHeader(){
	header_t header;
	int resultado = recibir_mensaje(socketCoordinador, &header, sizeof(header_t));
	if(resultado < 0){
		log_error(logInstancia, "Error al recibir");
		exitFailure();
	}
	return header;
}

void* recibirBuffer(int tamanio){
	void* buffer = malloc(tamanio);
	int resultado = recibir_mensaje(socketCoordinador, buffer, sizeof(header_t));
	if(resultado < 0){
		log_error(logInstancia, "Error al recibir");
		exitFailure();
	}
	return buffer;
}

int main(int argc, char *argv[]) {

	//Creo el log
	logInstancia = log_create("instancia.log", "Instance", true, LOG_LEVEL_DEBUG);

	//Lee archivo de configuraciones
	configuracion = cargarConfiguracion();

	//Conexion Coordinador
	socketCoordinador = conectarConProceso(configuracion.ip_coordinador, configuracion.puerto_coordinador, Instancia);
	log_debug(logInstancia, "Socket coordinador: %d", socketCoordinador);
	if (socketCoordinador < 0) {
		log_error(logInstancia, "Error conectar_a_server");
		exitFailure();
	}

	//Se envia el nombre de la instancia y se reciben el tamaño y cantidad de entradas
	inicializarComunicacionCordinadoor();

	//Se inicializan las estructuras administrativas
	inicializarEstructurasAdministrativas();
	//Bucle principal
	for (;;) {
		//Escucha de mensajes de Coordinador
		header_t header = recibirHeader();
		void* buffer = recibirBuffer(header.tamanio);
		if(header.comando < 0){
			log_error(logInstancia, "Error al recibir mensaje. Detalle: %s", strerror(errno));
			exitFailure();
		}
		switch (header.comando) {
		case msj_sentencia_get:
			ejecutarGet(buffer);
			break;
		case msj_sentencia_set:
			ejecutarSet(buffer);
			break;
		case msj_sentencia_store:
			ejecutarStore(buffer);
			break;
		}
		//Verifico y realizo el Dump en caso de que sea necesario
		procesarDump();
	}
	close(socketCoordinador);
	return EXIT_SUCCESS;
}
