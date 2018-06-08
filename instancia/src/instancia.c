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
#include "parsi/parser.h"
#include <math.h>

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


int redondiarArribaDivision(int divisor, int dividendo) {

	int resultado = ((divisor + dividendo - 1) / dividendo);

	return resultado;
}

char * escribirAPartir(char* matriz, int puntoInicio, char* textoAEscribir) {
	int puntoFin = puntoInicio + strlen(textoAEscribir) -1;
	int a = 0;
	int i = 0;
	for (a = puntoInicio; a <= puntoFin ; a++) {
		matriz[a] = textoAEscribir[i];
		i++;
	}

	//matriz[puntoFin] = '\0';
	return matriz;
}

char* leerDesdeAsta(char * matriz, int puntoInicio, int puntoFin) {
	texto = malloc(puntoFin - puntoInicio);
	printf("leerDesdeHasta INICIO Texto %s \n",texto);
	printf("leerDesdeHasta puntoInicio %d puntoFin %d \n",puntoInicio,puntoFin);

	int a = 0;
	int i = 0;
	for (a = puntoInicio; a < puntoFin; a++) {
		texto[i] = matriz[a];
		i++;
	}
	texto[puntoFin-puntoInicio] = '\0';


	printf("leerDesdeHasta fin Texto %s \n",texto);
	printf("leerDesdeHasta  logitud strlen %d \n",strlen(texto));
	printf("leerDesdeHasta string_length %d\n",string_length(texto));

	return texto;
}

char* escribirEntrada(char* matriz, int tamanioEntradas, int numEntrada,
		char* textoAEscribir) {

	char* texto = escribirAPartir(matriz, tamanioEntradas * numEntrada,
			textoAEscribir);
	return texto;
}

char* leerEntrada(char * matriz, int tamanioEntradas, int numEntrada,int longitud) {
	int puntoInicio = tamanioEntradas * numEntrada;
	int puntoFin = tamanioEntradas * numEntrada + longitud;
	return leerDesdeAsta(matriz, puntoInicio, puntoFin);
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


void imprimirPorPantallaEstucturas(t_entrada* matriz,t_dictionary* diccionario,char* matrizDeChar,int cantidadEntradas,int tamanioEntradas){
	if(seguir==2){



		int i=0;
	for(i=0;i<cantidadEntradas ;i++){
		if(dictionary_has_key(diccionario,matriz[i].clave)){
		printf("-------Diccionario:  %s %d ",matriz[i].clave,dictionary_get(diccionario,matriz[i].clave));
		printf(" MatrizMap:%d %d %d %d ",i,matriz[i].numeroEntrada,matriz[i].tamanioValor,matriz[i].tiempo);
		char* texto =leerEntrada(matrizDeChar,tamanioEntradas,matriz[i].numeroEntrada,matriz[i].tamanioValor);
		printf(" MatrizChar Valor: %s\n",texto);
		free(texto);
		}
		}
	}
}

void sustituirMatrizEntradas(char * algoritmo,int  punteroIUltimoInsertadoMatriz,t_entrada * matriz,t_dictionary * diccionario,char * clave){
	printf("Sustituye Algoritmo declarado %s\n",algoritmo);
	if(string_equals_ignore_case(algoritmo,"Ciclico")){
		//busco siguiente atomico
		int i=0;
		if(punteroIUltimoInsertadoMatriz+1<entradasCantidad){
			i=punteroIUltimoInsertadoMatriz+1;
		}else {
			i=0;
		}
		int fin=0;
		while(fin==0){
			if(redondiarArribaDivision(matriz[i].tamanioValor,entradasTamanio)==1){
				//inserto directo clave

printf("Se remueve de diccionari y matriz clave %s \n",matriz[i].clave);
				dictionary_remove(diccionario,matriz[i].clave);
				dictionary_put(diccionario,clave,i);
				strcpy(matriz[i].clave,clave);
				//inserto tamanio valor -1
				matriz[i].tamanioValor=-1;

				fin=1;
			}else{
				if(redondiarArribaDivision(matriz[i].tamanioValor,entradasTamanio)==0){
					i++;
					if(i==entradasCantidad){
						i=0;
					}
				}else{
				if(i+redondiarArribaDivision(matriz[i].tamanioValor,entradasTamanio)<entradasCantidad){
					//salto al proximo
				i=i+redondiarArribaDivision(matriz[i].tamanioValor,entradasTamanio);
					}else{
						i=0;
					}
				}
				}

			}









		//pongo la clave en matriz y diccionario

		//cambio tamaniovalor de matriz y timer

	}else{
		if(string_equals_ignore_case(algoritmo,"LRU")){



		}else{
			if(string_equals_ignore_case(algoritmo,"BSU")){




			}


		}
	}
}

int main(int argc, char *argv[]) {

	//variables nescesarias
	int maximoNumeroEntradaInicio = 0;
	int maximoNumeroEntradaFin = 0;

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
	bufferTamanio = strlen(configuracion.nombre_instancia) + 1;

	enviar_mensaje(socket_server, &bufferTamanio, sizeof(bufferTamanio));
	enviar_mensaje(socket_server, configuracion.nombre_instancia,
			bufferTamanio);

	//recibir entradas y tamanio

	entradasCantidad = 0;
	entradasTamanio = 0;

	recibir_mensaje(socket_server, &entradasCantidad, sizeof(entradasCantidad));
	recibir_mensaje(socket_server, &entradasTamanio, sizeof(entradasTamanio));

	printf("EntradasCantidad: %d entradasTamanio: %d \n ", entradasCantidad,
			entradasTamanio);

	//	mapeo de archivo ArchivoMatrizDeEntradas
	char* pathArchivo = string_new();
	string_append(&pathArchivo, configuracion.punto_montaje);
	char* nombreTablaDeEntradas = "TablaDeEntradas";
	string_append(&pathArchivo, nombreTablaDeEntradas);
	string_append(&pathArchivo, ".bin");
	printf("Path de archivo creado y mapeado o solo mapeado: %s\n:",
			pathArchivo);

	/*Abrir o crear archivo de una entrada*/
	printf("Abriendo %s: \n", pathArchivo);
	//int tamanioArchivo = entradasCantidad * entradasTamanio ;
	int tamanioArchivo = entradasTamanio * entradasCantidad * sizeof(char);
	int tamanioArchivoTablaEntradas = sizeof(t_entrada) * entradasCantidad ;
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

	for (i = 0; i < entradasCantidad && archivoNoExistiaPrimeraCarga == 1;
			i++) {
		mapArchivoTablaDeEntradas[i].numeroEntrada = -1;
		mapArchivoTablaDeEntradas[i].tamanioValor = -1;
		mapArchivoTablaDeEntradas[i].tiempo = -1;
		//mapArchivoTablaDeEntradas[i].clave[0] = '\0';
		printf("%d %d %d \n",mapArchivoTablaDeEntradas[i].numeroEntrada,mapArchivoTablaDeEntradas[i].tamanioValor,mapArchivoTablaDeEntradas[i].tiempo);
	}


	free(pathArchivo);
	struct t_dictionary *diccionarioEntradas = dictionary_create();



	//se carga diccionarioEntradas, matrizValoresEntradas con datos de mapArchivoTablaDeEntradas

	int b = 0;
	for (i = 0; i < entradasCantidad && archivoNoExistiaPrimeraCarga == 0;
			i++) {

		if (mapArchivoTablaDeEntradas[i].clave[0] != '\0') {
			dictionary_put(diccionarioEntradas,
					mapArchivoTablaDeEntradas[i].clave, i);
			char* pathArchivo = string_new();
			string_append(&pathArchivo, configuracion.punto_montaje);
			string_append(&pathArchivo, "DUMP/");
			string_append(&pathArchivo, mapArchivoTablaDeEntradas[i].clave);
			string_append(&pathArchivo, ".txt");
			int fd = fopen(pathArchivo, "r");

			//cargo matrizValoresEntradas

			char *valor = malloc(mapArchivoTablaDeEntradas[i].tamanioValor+1);
			fgets(valor, mapArchivoTablaDeEntradas[i].tamanioValor+1, fd);
			matrizValoresEntradas = escribirEntrada(matrizValoresEntradas,
					entradasTamanio, mapArchivoTablaDeEntradas[i].numeroEntrada,
					valor);
			free(valor);

			close(fd);
			free(pathArchivo);
		}

//------cargar diccionario con lo rescatado en el archivo
	}



//inicio de while
	timer=0;
	timerDump=0;
	 seguir = 0;
	recibir_mensaje(socket_server, &seguir, sizeof(seguir));
	printf("--------------------Seguir 1 si sigue o 0 si no = %d\n", seguir);

	while (seguir >0) {
printf("\n");
		//Recibir sentencia de Re Distinto
		int tamanioMensaje;


		recibir_mensaje(socket_server, &tamanioMensaje, sizeof(tamanioMensaje));
		if (tamanioMensaje > 0) {

			char * sentencia = malloc(tamanioMensaje);
			recibir_mensaje(socket_server, sentencia, tamanioMensaje);

			printf("Sentencia: %s\n", sentencia);
			t_esi_operacion esi_operacion = parse(sentencia);
			//Procesado de sentencia de una entrada

			free(sentencia);

			int instruccion = esi_operacion.keyword;

			printf("Instruccion: %d\n", instruccion);



			//analisis sentencia
			if (instruccion == 0) {
				//INICIO procesado GET
				printf("INICIO procesado GET\n");
				imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas,diccionarioEntradas,matrizValoresEntradas,entradasCantidad,entradasTamanio);
				//se accsede a la tabla de entradas si no la encuentra la crea


				if (dictionary_has_key(diccionarioEntradas,
						esi_operacion.argumentos.GET.clave)) {
					printf("si exite la clave\n");
					//si exite la clave

					int posicionTablaDeEntradas;
					posicionTablaDeEntradas = dictionary_get(
							diccionarioEntradas,
							esi_operacion.argumentos.GET.clave);
					t_entrada unaEntrada =
							mapArchivoTablaDeEntradas[posicionTablaDeEntradas];


					printf("GET Entrada  numeroEntrada:%d tamanioValor:%d \n",
							unaEntrada.numeroEntrada, unaEntrada.tamanioValor);

				} else {
					printf("si no exite la clave\n");
					//si no exite la clave
					//crea

					int fin=0;
					int i=0;
					while(mapArchivoTablaDeEntradas[i].numeroEntrada!=-1 && fin==0){

						if(i==entradasCantidad){
							//sustituir

													sustituirMatrizEntradas(
																	configuracion.algoritmo_remplazo,
																	punteroIUltimoInsertadoMatriz,
																	mapArchivoTablaDeEntradas,
																	diccionarioEntradas,
																	esi_operacion.argumentos.GET.clave);
							fin=1;
						}else{
															printf(
																	"mapArchivoTablaDeEntradas con clave ocupada indice %d, numeroEntrada: %d tamanioValor %d\n",
															i,mapArchivoTablaDeEntradas[i].numeroEntrada,mapArchivoTablaDeEntradas[i].tamanioValor);
															i=i+redondiarArribaDivision(mapArchivoTablaDeEntradas[i].tamanioValor,entradasTamanio)-1;
						}

						i++;
					}

					if(mapArchivoTablaDeEntradas[i].numeroEntrada==-1 && fin==0){
													printf(
															"mapArchivoTablaDeEntradas Se encontro clave libre indice %d, numeroEntrada: %d\n",
															i,mapArchivoTablaDeEntradas[i].numeroEntrada);
													dictionary_put(diccionarioEntradas,
															esi_operacion.argumentos.GET.clave, i);
													strcpy(mapArchivoTablaDeEntradas[i].clave,
															esi_operacion.argumentos.GET.clave);
													punteroIUltimoInsertadoMatriz = i;
					}

//					//busqueda de mapArchivoTablaDeEntradas en null
//					int a = 0;
//					int fin = 0;
//					for (a = 0; a <= entradasCantidad && fin != 1; a++) {
//						if (mapArchivoTablaDeEntradas[a].numeroEntrada != -1) {
//
//							if (a == entradasCantidad) {
//								printf(
//										"mapArchivoTablaDeEntradas no se encontro clave libre sustituir%d\n",
//										a);
//
//								//algoritmos de sustitucion
//								sustituirMatrizEntradas(
//										configuracion.algoritmo_remplazo,
//										punteroIUltimoInsertadoMatriz,
//										mapArchivoTablaDeEntradas,
//										diccionarioEntradas,
//										esi_operacion.argumentos.GET.clave);
//								fin = 1;
//							} else {
//
//								printf(
//										"mapArchivoTablaDeEntradas con clave ocupada %d\n",
//										a);
//							}
//						} else {
//							printf(
//									"mapArchivoTablaDeEntradas Se encontro clave libre %d\n",
//									a);
//							dictionary_put(diccionarioEntradas,
//									esi_operacion.argumentos.GET.clave, a);
//							strcpy(mapArchivoTablaDeEntradas[a].clave,
//									esi_operacion.argumentos.GET.clave);
//							punteroIUltimoInsertadoMatriz = a;
//
//							fin = 1;
//
//						}
//
//					}





				}
				imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas,diccionarioEntradas,matrizValoresEntradas,entradasCantidad,entradasTamanio);
				//FIN procesado GET
			} else {
				if (instruccion == 1) {
					//INICIO procesado SET
					printf("INICIO procesado SET\n");
					imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas,diccionarioEntradas,matrizValoresEntradas,entradasCantidad,entradasTamanio);

					//se accsede a la tabla de entradas
					if (dictionary_has_key(diccionarioEntradas,
							esi_operacion.argumentos.SET.clave)) {

						int i = dictionary_get(diccionarioEntradas,
								esi_operacion.argumentos.SET.clave);

						int cantidadDeEntradasAUsar = redondiarArribaDivision(
								strlen(esi_operacion.argumentos.SET.valor),
								entradasTamanio);

						printf(
								"texto: %s  valor:%d cantidadDeEntradasAUsar: %d\n",
								esi_operacion.argumentos.SET.valor,
								strlen(esi_operacion.argumentos.SET.valor),
								cantidadDeEntradasAUsar);
						//primera vez que se inserta
						printf(
								"antes de set mapArchivoTablaDeEntradas indice %d numeroEntrada %d\n",
								i, mapArchivoTablaDeEntradas[i].numeroEntrada);
						if (mapArchivoTablaDeEntradas[i].numeroEntrada == -1) {
							//busqueda de entradas contiguas
							int a = 0;
							int contadorContiguas = 0;
							int numeroEntradaInicialAInsertar = -1;
							for (a = 0;
									a < entradasCantidad
											&& cantidadDeEntradasAUsar
													!= contadorContiguas; a++) {

								if (mapArchivoTablaDeEntradas[a].numeroEntrada
										== -1) {
									contadorContiguas++;
								} else {
									//existe entrada y se calcula cuantas entradas ocupa
									contadorContiguas = 0;
									a =a
													+ redondiarArribaDivision(
															mapArchivoTablaDeEntradas[a].tamanioValor,
															entradasTamanio)
													-1;
								};
								if (contadorContiguas
										== cantidadDeEntradasAUsar) {
									numeroEntradaInicialAInsertar = a-contadorContiguas+1;
								}
							}

							//se actualiza tamanio valor de matriz de entrada
							mapArchivoTablaDeEntradas[i].tamanioValor=strlen(esi_operacion.argumentos.SET.valor);
							//se inserta datos en matriz de valores

							matrizValoresEntradas = escribirEntrada(
									matrizValoresEntradas, entradasTamanio,
									numeroEntradaInicialAInsertar,
									esi_operacion.argumentos.SET.valor);
							printf(
									"despues de set mapArchivoTablaDeEntradas indice %d numeroEntrada %d\n",
									i, numeroEntradaInicialAInsertar);
							char* texto=leerEntrada(matrizValoresEntradas,
																		entradasTamanio,
																		numeroEntradaInicialAInsertar,
																		mapArchivoTablaDeEntradas[i].tamanioValor);
							printf(
									"Se inserto en matriz de valores el valor %s\n",texto									);
							free(texto);

							//se inserta datos en matriz de entradas
							mapArchivoTablaDeEntradas[i].numeroEntrada =
									numeroEntradaInicialAInsertar;
							mapArchivoTablaDeEntradas[i].tamanioValor = strlen(
									esi_operacion.argumentos.SET.valor);
							strcpy(mapArchivoTablaDeEntradas[i].clave,
									esi_operacion.argumentos.SET.clave);

							printf(
									"en matriz %d tamanioValor %d escribioentrada %s\n",
									i,
									mapArchivoTablaDeEntradas[i].tamanioValor,
									esi_operacion.argumentos.SET.valor);

						} else {
							//si ya habia asignado un indice de la matriz de entrada, se sustituye por una entrada una sola segun lo que me dijo lucas
							mapArchivoTablaDeEntradas[i].tamanioValor =
									string_length(
											esi_operacion.argumentos.SET.valor);
							matrizValoresEntradas = escribirEntrada(
									matrizValoresEntradas, entradasTamanio,
									mapArchivoTablaDeEntradas[i].numeroEntrada,
									esi_operacion.argumentos.SET.valor);

							printf(
									"en matriz %d tamanioValor %d escribioentrada %s\n",
									i,
									mapArchivoTablaDeEntradas[i].tamanioValor,
									esi_operacion.argumentos.SET.valor);

						}

					} else {
						printf("Error de Clave no Identificada\n");
						//Error de Clave no Identificada
						//						Cuando un Usuario ejecute una operación de
						//						STORE o SET y la clave no exista, se deberá generar un error informando de dicha inexistencia al Usuario. Abortando el ESI culpable.

					}
					imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas,diccionarioEntradas,matrizValoresEntradas,entradasCantidad,entradasTamanio);
					printf("FIN procesado SET\n");
					//FIN procesado SET
				} else {
					if (instruccion == 2) {
						//INICIO procesado STORE
						printf("INICIO procesado STORE\n");
						imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas,diccionarioEntradas,matrizValoresEntradas,entradasCantidad,entradasTamanio);

						if (dictionary_has_key(diccionarioEntradas,
								esi_operacion.argumentos.STORE.clave)) {
							//leer entrada en memoria

							int i = dictionary_get(diccionarioEntradas,
									esi_operacion.argumentos.STORE.clave);

							char* pathArchivo = string_new();
							string_append(&pathArchivo,
									configuracion.punto_montaje);
							string_append(&pathArchivo,
									esi_operacion.argumentos.STORE.clave);
							string_append(&pathArchivo, ".txt");
							//se accsede a la tabla de entradas
							char* textoValor = leerEntrada(
									matrizValoresEntradas, entradasTamanio,
									mapArchivoTablaDeEntradas[i].numeroEntrada,
									mapArchivoTablaDeEntradas[i].tamanioValor);

							printf("texto: %s\n", textoValor);
							int tamanioArchivo = strlen(textoValor)
									* sizeof(char);
							if (remove(pathArchivo)) {
								printf(
										"Se elimino archivo satifactoriamente\n");
							} else {
								printf("no se elimino archivo\n");
							}

							fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC,
									(mode_t) 0600);
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
									printf("texto de de tabla num i clave:%s\n",
																		mapArchivoTablaDeEntradas[i].clave);
//									//se elimina de memoria la entrada y del diccionario
//									dictionary_remove(diccionarioEntradas,mapArchivoTablaDeEntradas[i].clave);
//									mapArchivoTablaDeEntradas[i].clave[0]='\0';
//									mapArchivoTablaDeEntradas[i].numeroEntrada=-1;
//									mapArchivoTablaDeEntradas[i].tamanioValor=-1;
//									mapArchivoTablaDeEntradas[i].tiempo=-1;
								}
							}


							free(textoValor);

							imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas,diccionarioEntradas,matrizValoresEntradas,entradasCantidad,entradasTamanio);
							printf("FIN procesa do STORE\n");
//						//FIN procesa do STORE

						} else {
							imprimirPorPantallaEstucturas(mapArchivoTablaDeEntradas,diccionarioEntradas,matrizValoresEntradas,entradasCantidad,entradasTamanio);
							printf("Error de Clave no Identificada\n");
							printf("FIN procesa do STORE\n");
						}
					}




				}
			}

}





		//proceso dump en cantidad de ciclos
		if (configuracion.intervalo_dump == timerDump+1) {

			//recorro matrizde entradas de 0 a cantidad de entradas
			i = 0;
			while (i < entradasCantidad) {

				if(mapArchivoTablaDeEntradas[i].numeroEntrada>-1 && mapArchivoTablaDeEntradas[i].numeroEntrada<entradasCantidad &&
						mapArchivoTablaDeEntradas[i].tamanioValor>-1){

				char* pathArchivo = string_new();
				string_append(&pathArchivo, configuracion.punto_montaje);
				string_append(&pathArchivo, "DUMP/");
				string_append(&pathArchivo, mapArchivoTablaDeEntradas[i].clave);
				string_append(&pathArchivo, ".txt");
				//se accsede a la tabla de entradas
				char* textoValor = leerEntrada(matrizValoresEntradas,
						entradasTamanio,
						mapArchivoTablaDeEntradas[i].numeroEntrada,
						mapArchivoTablaDeEntradas[i].tamanioValor);
				int tamanioArchivo=0;
				tamanioArchivo= string_length(textoValor) ;
				printf("IMPORTANTE linea 746 texto: %s %d\n", textoValor, tamanioArchivo);

				if (remove(pathArchivo)) {
					printf("Se elimino archivo satifactoriamente\n");
				} else {
					printf("no se elimino archivo\n");
				}

				fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC,
						(mode_t) 0600);
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

				if(redondiarArribaDivision(
								mapArchivoTablaDeEntradas[i].tamanioValor,
								entradasTamanio)==0){
					i++;
				}else{
				i = i
						+ redondiarArribaDivision(
								mapArchivoTablaDeEntradas[i].tamanioValor,
								entradasTamanio);
				}



			}

			timerDump = 0;
		} else {
			timerDump++;
		}


		//timer suma 1
		timer++;

		//recibe mensaje 0 para parar 1 para seguir y 2 para seguir mostrando por pantalla estructuras
			recibir_mensaje(socket_server, &seguir, sizeof(int));
		}


		close(socket_server);
		return EXIT_SUCCESS;

}
