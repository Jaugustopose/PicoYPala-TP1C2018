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
#include "commons/bitarray.h"
#include "instancia.h"
#include "comunicacion/comunicacion.h"
#include "parsi/parser.h"
#include <math.h>
#include <string.h>
#include <errno.h>

int entradasCantidad ;
int entradasTamanio ;
int punteroSustitucion = 0;
int timer = 0;
int timerDump = 0;
config_t configuracion;
t_log * logInstancia;
int socketCoordinador;
char* matrizValores;
t_entrada * tablaEntradas;
t_dictionary *diccionarioEntradas;
int archivoNuevo;
int tamanioArchivoTablaEntradas;
t_bitarray* bitmap;

void crearBitmap(){
	char * bitarray = (char*) malloc(sizeof(char)* ((entradasCantidad+8-1)/8)); //Redondeado para arriba
	bitmap = bitarray_create_with_mode(bitarray,(entradasCantidad+8-1)/8, LSB_FIRST);
}

void destruirBitmap(t_bitarray *bitmap){
	free(bitmap->bitarray);
	bitarray_destroy(bitmap);
}

int buscarEntradasContiguas(int cantidad){
	int encontradas = 0;
	int i;
	for (i = 0; i < entradasCantidad; ++i) {
		if(bitarray_test_bit(bitmap, i)==0){
			encontradas++;
		}else{
			encontradas = 0;
		}
		if(encontradas==cantidad){
			return i+1-cantidad;
		}
	}
	return -1;
}

int cantEntradasLibres(){
	int libres = 0;
	int i;
	for (i = 0; i < entradasCantidad; ++i) {
		if(bitarray_test_bit(bitmap, i)==0){
			libres++;
		}
	}
	return libres;
}

void liberarEntrada(int index, int cantidad){
	int i;
	for(i = 0; i < cantidad; ++i)
	{
		bitarray_clean_bit(bitmap, index+i);
	}
}

void reservarEntrada(int index, int cantidad){
	int i;
	for(i = 0; i < cantidad; ++i)
	{
		bitarray_set_bit(bitmap, index+i);
	}
}

void agregarClaveEnDiccionario(char* clave, int index){
	int* indice = malloc(sizeof(int));
	*indice = index;
	dictionary_put(diccionarioEntradas, clave, indice);
}

void removerClaveEnDiccionario(char* clave){
	void* elemento = dictionary_remove(diccionarioEntradas, clave);
	free(elemento);
}

int obtenerIndiceClave(char* clave){
	return *(int*)dictionary_get(diccionarioEntradas, clave);
}

bool existeClave(char* clave){
	return dictionary_has_key(diccionarioEntradas, clave);
}

void exitFailure(){
	log_destroy(logInstancia);
	destruirBitmap(bitmap);
//	if (munmap(tablaEntradas, tamanioArchivoTablaEntradas) == -1) {
//		perror("Error unmapping file");
//	}
//	close(fdTablaEntradas);
	close(socketCoordinador);
	exit(EXIT_FAILURE);
}

void exitSuccess(){
	log_destroy(logInstancia);
	destruirBitmap(bitmap);
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
	string_append(&pathArchivo, configuracion.nombre_instancia);
	string_append(&pathArchivo, "/");
	string_append(&pathArchivo, "TablaDeEntradas.bin");
	log_debug(logInstancia, "Path para mmap de Tabla de Entradas: %s", pathArchivo);

	//Calculo tamaño del archivo para hacer mmap
	tamanioArchivoTablaEntradas = sizeof(t_entrada) * entradasCantidad;
	//Intento abrir archivo previamente creado
	int fd = open(pathArchivo, O_RDWR, (mode_t) 0600);
	archivoNuevo = 0;
	if (fd < 0) {
		//Si no pudo abrir un archivo viejo se crea uno nuevo
		fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
		archivoNuevo = 1;
		if (fd < 0) {
			log_error(logInstancia, "Error al crear el archivo de Tabla de Entradas");
			exitFailure();
		}
		/*Al crear una archivo nuevo se le escribe un string vacio al final para
		 *aumentar su tamaño al necesario para guardar la Tabla de Entradas completa*/
		int result = lseek(fd, tamanioArchivoTablaEntradas - 1, SEEK_SET);
		if (result < 0) {
			close(fd);
			log_error(logInstancia, "Error al saltar con lseek");
			exitFailure();
		}
		result = write(fd, "", 1);
		if (result < 0) {
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
	if (tablaEntradas == MAP_FAILED) {
		close(fd);
		log_error(logInstancia, "Error al mapear Tabla de Entradas");
		exitFailure();
	}
	log_debug(logInstancia, "Se mapeo Tabla de Entradas correctamente");
	return retorno;
}

void inicializarTablaEntradas(){
	if(!archivoNuevo){
		//Si el archivo ya existia se carga diccionarioEntradas y matrizValoresEntradas con sus datos
		int i = 0;
		for (i = 0; i < entradasCantidad; i++) {
			if (tablaEntradas[i].clave[0]) {
				agregarClaveEnDiccionario(tablaEntradas[i].clave, i);
				char* pathArchivo = string_new();
				string_append(&pathArchivo, configuracion.punto_montaje);
				string_append(&pathArchivo, configuracion.nombre_instancia);
				string_append(&pathArchivo, "/");
				string_append(&pathArchivo, "DUMP/");
				string_append(&pathArchivo, tablaEntradas[i].clave);
				string_append(&pathArchivo, ".txt");
				FILE* file = fopen(pathArchivo, "r");
				free(pathArchivo);
				if(file){
					char *valor = malloc(tablaEntradas[i].tamanioValor + 1);
					fgets(valor, tablaEntradas[i].tamanioValor + 1, file);
					escribirMatrizValores(tablaEntradas[i].numeroEntrada, valor);
					free(valor);
					fclose(file);
				}else{
					//TODO: Que hacemos si no esta el archivo?? Por ahora borro la entrada
					log_debug(logInstancia, "Se borra entrada porque no se encontró archivo de dump. Clave:%s", tablaEntradas[i].clave);
					removerClaveEnDiccionario(tablaEntradas[i].clave);
					tablaEntradas[i].clave[0]=0;
				}
			}
		}
		imprimirPorPantallaEstucturas();
	}
}

int buscarPosicionLibreTablaEntradas(){
	int i;
	for (i = 0; i < entradasCantidad; i++) {
		if(tablaEntradas[i].clave[0] == 0){
			return i;
		}
	}
	return -1;
}

void inicializarEstructurasAdministrativas(){
	//Creo e inicializo matriz de Entradas
	crearMatrizValores(entradasCantidad, entradasTamanio);
	//Creo diccionario de entradas
	diccionarioEntradas = dictionary_create();
	//Creo bitmap
	crearBitmap();
	//Abro archivo para mmap
	int fd = abrirArchivoTablaEntradas();
	//Mapeo archivo a memoria
	tablaEntradas = mapearTablaEntradas(fd);
	//Inicializar Tabla de Entradas
	inicializarTablaEntradas();
}

int redondearArribaDivision(int divisor, int dividendo) {
	int resultado = ((divisor + dividendo - 1) / dividendo);
	return resultado;
}

void escribirMatrizValores(int index, char* valor){
	int tamanio = strlen(valor);
	memcpy(matrizValores + index * entradasTamanio, valor, tamanio);
}

char* leerMatrizValores(int index, int tamanio){
	char* texto = malloc(tamanio + 1);
	memcpy(texto, matrizValores + index * entradasTamanio, tamanio);
	texto[tamanio] = '\0';
	return texto;
}

void crearMatrizValores(){
	matrizValores = malloc(entradasCantidad * entradasTamanio);
	memset(matrizValores, 0, entradasCantidad * entradasTamanio);
}

void imprimirPorPantallaEstucturas() {
	int i = 0;
	log_debug(logInstancia, "Tabla Entradas");
	log_debug(logInstancia, "I\tClave\t\tEntrada\tTamaño\tTiempo");
	for (i = 0; i < entradasCantidad; i++) {
		if(tablaEntradas[i].clave[0]){
			log_debug(logInstancia, "%d\t%s\t%d\t%d\t%d",i, tablaEntradas[i].clave, tablaEntradas[i].numeroEntrada,
					tablaEntradas[i].tamanioValor, tablaEntradas[i].tiempo);
		}
	}
}

void agregarNuevaClave(char* clave, int indice){
	agregarClaveEnDiccionario(clave, indice);
	strcpy(tablaEntradas[indice].clave, clave);
	//punteroSustitucion = indice;
}

void setValorEntrada(t_entrada* entrada, char*valor, int tamanio, int indice){
	entrada->tamanioValor = tamanio;
	entrada->numeroEntrada = indice;
	escribirMatrizValores(indice, valor);
	log_debug(logInstancia, "Se inserta Valor:%s en Indice:%d", valor, indice);
}

void enviar_ok_sentencia_a_Coordinador(){
	header_t header;
	header.comando = msj_sentencia_finalizada;
	header.tamanio = 0;

	enviar_mensaje(socketCoordinador,&header,sizeof(header_t));
}

void enviar_msj_instancia_sustituyo_clave(){
	header_t header;
	header.comando = msj_instancia_sustituyo_clave;
	header.tamanio = 0;

	enviar_mensaje(socketCoordinador,&header,sizeof(header_t));
}

void enviar_msj_instancia_compactar(){
	header_t header;
	header.comando = msj_instancia_compactar;
	header.tamanio = 0;

	enviar_mensaje(socketCoordinador,&header,sizeof(header_t));
}

void enviar_msj_instancia_compactacion_finalizada(){
	header_t header;
	header.comando = msj_instancia_compactacion_finalizada;
	header.tamanio = 0;

	enviar_mensaje(socketCoordinador,&header,sizeof(header_t));
}

void borrarClave(int index){
	t_entrada* entrada = tablaEntradas + index;
	int cantidad = redondearArribaDivision(entrada->tamanioValor, entradasTamanio);
	liberarEntrada(entrada->numeroEntrada, cantidad);
	removerClaveEnDiccionario(entrada->clave);
	//TODO: Avisar al coordinador que la clave se borró
	entrada->clave[0] = 0;
}

void sustituirCiclico(t_entrada* entrada, char* valor, int tamanio, int entradasASustituir){

	int i = punteroSustitucion;
	int aSustituir[entradasASustituir];
	int encontradas = 0;
	for(;;) {
		if(redondearArribaDivision(tablaEntradas[i].tamanioValor, entradasTamanio) < 2) {
			//Era una entrada atómica, se guarda indice para borrarla luego
			log_debug(logInstancia, "Entrada atómica a borrar: %s", tablaEntradas[i].clave);
			aSustituir[encontradas]= i;
			encontradas++;
			if(encontradas == entradasASustituir){
				//Conseguí todas las entradas necesarias asi que salgo del for
				break;
			}else{
				//Faltan mas entradas, incremento indice y continuo
				i = (i + 1) % entradasCantidad;
			}
		} else {
			//No era atomica la ignoro
			i = (i + 1) % entradasCantidad;
		}
		if (i == punteroSustitucion){
			//Di toda la vuelta a la tabla y llegue al mismo puntero inicial
			//Lo que significa que no hay suficientes entradas atómicas para reemplazar
			break;
		}
	}
	if(encontradas == entradasASustituir){
		//Se encontraron la cantidad de entradas necesarias así que se deben borrar todas
		for(i=0; i < entradasASustituir; i++){
			borrarClave(aSustituir[i]);
		}
		//Actualizo puntero de sustitución
		punteroSustitucion = (aSustituir[i-1] + 1) % entradasCantidad;
		//Me fijo si de casualidad quedaron todos los espacios contiguos
		int entradasNecesarias =  redondearArribaDivision(tamanio, entradasTamanio);
		int indiceEntrada = buscarEntradasContiguas(entradasNecesarias);
		//Verifico si se encontraron entradas libres
		if (indiceEntrada < 0) {
			//No quedaron contiguas asi que se compacta
			enviar_msj_instancia_compactar();
		} else {
			//Quedaron contiguas asi que se inserta directamente
			setValorEntrada(entrada, valor, tamanio, indiceEntrada);
			reservarEntrada(indiceEntrada, entradasASustituir);
		}
	}else{
		//No se encontraron entradas suficientes,
		//TODO: Se debe avisar al coordinador que no se puede sustituir
	}
}

void sustituirLRU(t_entrada* entrada, char* valor, int tamanio, int entradasNecesarias){
	/*int i = 0;
	int aSustituir = -1;
	int aSustituirTiempo = 0;
	while (i < entradasCantidad) {
		if (redondearArribaDivision(tablaEntradas[i].tamanioValor, entradasTamanio) == 1) {
			if (timer - tablaEntradas[i].tiempo > aSustituirTiempo) {
				aSustituirTiempo = tablaEntradas[i].tiempo;
				aSustituir = i;
			}
		}
		log_debug(logInstancia, "IdEntrada: %d timerEntrada: %d timerSistema: %d",i ,tablaEntradas[i].tiempo, timer);
		if (i + redondearArribaDivision(tablaEntradas[i].tamanioValor, entradasTamanio) <= entradasCantidad) {
			//salto al proximo
			i = i + redondearArribaDivision(tablaEntradas[i].tamanioValor, entradasTamanio);
		}
	}
	if (aSustituir > -1) {
		//remuevo y sustituyo
		removerClaveEnDiccionario(tablaEntradas[i].clave);
		agregarClaveEnDiccionario(clave, aSustituir);
		strcpy(tablaEntradas[aSustituir].clave, clave);
		//inserto tamanio valor -1
		tablaEntradas[aSustituir].tamanioValor=0;
		tablaEntradas[aSustituir].tiempo=0;
	}*/
}

void sustituirBSU(t_entrada* entrada, char* valor, int tamanio, int entradasNecesarias){
	/*int i = 0;
	int aSustituir = -1;
	int aSustituirTamanio = 0;
	while (i < entradasCantidad) {
		if (redondearArribaDivision(tablaEntradas[i].tamanioValor, entradasTamanio) == 1) {
			if (tablaEntradas[i].tamanioValor > aSustituirTamanio) {
				aSustituirTamanio=tablaEntradas[i].tamanioValor;
				aSustituir=i;
			}
		}
		if (i + redondearArribaDivision(tablaEntradas[i].tamanioValor, entradasTamanio) <= entradasCantidad) {
			//salto al proximo
			i = i + redondearArribaDivision(tablaEntradas[i].tamanioValor, entradasTamanio);
		}
	}
	if (aSustituir > -1) {
		//remuevo y sustituyo
		int valorRemover = -1;
		agregarClaveEnDiccionario(tablaEntradas[aSustituir].clave, valorRemover);

		if (obtenerIndiceClave(tablaEntradas[aSustituir].clave) == -1) {
			log_debug(logInstancia, "Se realizo el put de -1 en la clave");
		}
		removerClaveEnDiccionario(tablaEntradas[aSustituir].clave);
		agregarClaveEnDiccionario(clave, aSustituir);
		strcpy(tablaEntradas[aSustituir].clave,clave);

		//inserto tamanio valor -1
		tablaEntradas[aSustituir].tamanioValor=0;
		tablaEntradas[aSustituir].tiempo=0;
	}*/
}

void sustituirMatrizEntradas(t_entrada* entrada, char* valor, int tamanio, int entradasASustituir){
	char* algoritmo = configuracion.algoritmo_remplazo;

	log_debug(logInstancia, "Sustituye por algoritmo %s",algoritmo);
	if(string_equals_ignore_case(algoritmo,"Ciclico")){
		sustituirCiclico(entrada, valor, tamanio, entradasASustituir);
	} else if (string_equals_ignore_case(algoritmo,"LRU")){
		sustituirLRU(entrada, valor, tamanio, entradasASustituir);
	} else if (string_equals_ignore_case(algoritmo, "BSU")){
		sustituirBSU(entrada, valor, tamanio, entradasASustituir);
	}
}

void ejecutarGet(void* buffer){
	char* clave = (char*)buffer;
	log_info(logInstancia, "Procesando GET. Clave: %s", clave);

	//Se verifica en el diccionario si la clave existe
	if (existeClave(clave)) {
		//Existe clave, se trae la entrada correspondiente de la Tabla de Entradas y se la imprime para debug
		int posicionTablaDeEntradas = obtenerIndiceClave(clave);
		t_entrada unaEntrada = tablaEntradas[posicionTablaDeEntradas];
		log_debug(logInstancia, "Clave existente. Numero Entrada:%d Tamanio Entrada:%d", unaEntrada.numeroEntrada, unaEntrada.tamanioValor);
	} else {
		//No existe clave, se debe crear en tabla de entradas
		//Primero se busca un lugar libre en la tabla de entradas
		int indice = buscarPosicionLibreTablaEntradas();
		if(indice < 0){
			//No hay lugar libre, se ejecuta algoritmo de sustitución
			//TODO: Revisar LC
			sustituirMatrizEntradas(0, "", 0, 0);
		}else{
			//Se inserta nueva entrada en la tabla
			agregarNuevaClave(clave, indice);
			log_debug(logInstancia, "Clave inexistente. Se crea registro en Tabla de Entradas indice: %d", indice);
		}
	}
	enviar_ok_sentencia_a_Coordinador();
	imprimirPorPantallaEstucturas();
}

void moverValorEnMatriz(t_entrada* entrada, int indice, int cantidadEntradas){
	//Primero obtengo el valor a mover
	char* valor = leerMatrizValores(entrada->numeroEntrada, entrada->tamanioValor);
	//Libero entradas usadas previamente
	liberarEntrada(entrada->numeroEntrada, cantidadEntradas);
	//Escribo valor en posicion nueva y reservo las entradas usadas
	setValorEntrada(entrada, valor, entrada->tamanioValor, indice);
	reservarEntrada(indice, cantidadEntradas);
}

void compactarMatrizValores(){
	//Armo Lista de entradas y la ordeno por numero de entrada
	t_list* listaEntradas = list_create();
	int i;
	for (i = 0; i < entradasCantidad; i++) {
		if(tablaEntradas[i].clave[0]){
			list_add(listaEntradas, tablaEntradas + i);
		}
	}
	//Creo funcion local para ordenar lista
	bool ordenarPorIndice(void* arg1, void* arg2){
		t_entrada* entrada1 = (t_entrada*)arg1;
		t_entrada* entrada2 = (t_entrada*)arg2;
		return entrada1->numeroEntrada > entrada2->numeroEntrada;
	}
	//Ordeno lista por numero de entrada
	list_sort(listaEntradas, &ordenarPorIndice);
	//Ahora recorro la lista y voy moviendo los valores de cada entrada hacia la izquierda
	int indice = 0;
	for(i = 0; i < list_size(listaEntradas); ++i){
		t_entrada* entrada = list_get(listaEntradas, i);
		int entradasUsadas = redondearArribaDivision(entrada->tamanioValor, entradasTamanio);
		moverValorEnMatriz(entrada, indice, entradasUsadas);
		indice += entradasUsadas;
	}
	//Destruyo lista y NO borro los elementos porque son los que estan adentro de la TablaEntradas
	list_destroy(listaEntradas);

	//LAG
	enviar_msj_instancia_compactacion_finalizada();
	//fin LAG
}

void sustitucionEntrada(t_entrada* entrada, char* valor, int tamanio, int entradasNecesarias){
	//Primero verifico si hay entradas libres para insertar haciendo compactacion en vez de sustituir
	int entradasLibres = cantEntradasLibres();
	if(entradasLibres >= entradasNecesarias){
		//Se avisa a coordinador de que se necesita compactar
		enviar_msj_instancia_compactar();
	}else{
		int entradasASustituir = entradasNecesarias - entradasLibres;
		sustituirMatrizEntradas(entrada, valor, tamanio, entradasASustituir);
	}
}

void setSinValorPrevio(t_entrada* entrada, char* valor, int tamanio) {
	//Se inserta por primera vez, busco entradas libres
	int entradasNecesarias = redondearArribaDivision(tamanio, entradasTamanio);
	int indiceEntrada = buscarEntradasContiguas(entradasNecesarias);
	//Verifico si se encontraron entradas libres y contiguas
	if (indiceEntrada < 0) {
		sustitucionEntrada(entrada, valor, tamanio , entradasNecesarias);
	} else {
		//Habia entradas libres,se inserta valor y se marca en el bitmap las entradas usadas
		setValorEntrada(entrada, valor, tamanio, indiceEntrada);
		reservarEntrada(indiceEntrada, entradasNecesarias);
	}
}

void setConValorPrevio(t_entrada* entrada, char* valor, int tamanio) {
	//Se reemplaza valor previo, se debe verificar si el valor nuevo entra en las entradas ya asignadas a la clave
	int entradasNecesarias = redondearArribaDivision(tamanio, entradasTamanio);
	int entradasAsignadas = redondearArribaDivision(entrada->tamanioValor, entradasTamanio);
	if (entradasAsignadas == entradasNecesarias) {
		//Coincide justo, simplemente se escribe el nuevo valor
		setValorEntrada(entrada, valor, tamanio, entrada->numeroEntrada);
	} else if (entradasAsignadas > entradasNecesarias) {
		//Sobran entradas, se deben liberar las que sobran y escribir el valor
		setValorEntrada(entrada, valor, tamanio, entrada->numeroEntrada);
		int cantLiberar = entradasAsignadas - entradasNecesarias;
		liberarEntrada(entrada->numeroEntrada + cantLiberar - 1, cantLiberar);
	} else {
		//Faltan entradas, se liberan las entradas usadas y se busca donde guardar el nuevo valor
		liberarEntrada(entrada->numeroEntrada, entradasAsignadas);
		int indiceEntrada = buscarEntradasContiguas(entradasNecesarias);
		//Verifico si se encontraron entradas libres
		if (indiceEntrada < 0) {
			//No hay lugar, se liberan las entradas usadas y se inicia algoritmo de sustitución
			liberarEntrada(entrada->numeroEntrada, entradasAsignadas);
			sustitucionEntrada(entrada, valor, tamanio , entradasNecesarias);
		} else {
			//Habia entradas libres,se inserta valor y se marca en el bitmap las entradas usadas y las entradas liberadas
			setValorEntrada(entrada, valor, tamanio, indiceEntrada);
			reservarEntrada(indiceEntrada, entradasNecesarias);
		}
	}



}

void ejecutarSet(void* buffer){
	// Vos fuma que esta lógica de punteros no falla
	char* clave = (char*) buffer;
	int index = strlen(clave);
	char* valor = (char*) buffer + index + 1;
	log_info(logInstancia, "Procesando SET. Clave: %s - Valor:%s", clave, valor);
	//Se verifica que la clave exista
	if(existeClave(clave)) {
		//Existe la clave
		//Obtengo indice, entrada, tamanio
		int indice = obtenerIndiceClave(clave);
		t_entrada* entrada = tablaEntradas + indice;
		int tamanio = strlen(valor);
		//Verifico si se inserta por primera vez o si se reemplaza valor
		if (entrada->tamanioValor == 0) {
			//Se inserta por primera vez
			setSinValorPrevio(entrada, valor, tamanio);
		}else{
			//Se reemplaza valor previo
			setConValorPrevio(entrada, valor, tamanio);
		}
		enviar_ok_sentencia_a_Coordinador();
	}else{
		//No existe la clave TODO que mas hacemos? Se avisa al coordinador?
		log_error(logInstancia, "Error, se intento hacer SET de una clave inexistente");
	}
	imprimirPorPantallaEstucturas();
}

void ejecutarStore(void* buffer){
	char* clave = (char*) buffer;
	log_info(logInstancia, "Procesando STORE. Clave: %s", clave);

	if (existeClave(clave)) {
		//Se obtiene la entrada correspondiente
		int indice = obtenerIndiceClave(clave);
		t_entrada* entrada = tablaEntradas + indice;
		//Se lee el valor de la matriz de valores
		char* valor = leerMatrizValores(entrada->numeroEntrada, entrada->tamanioValor);
		log_debug(logInstancia, "Valor a realizar store:%s", valor);
		int tamanioArchivo = strlen(valor) * sizeof(char);
		//Creo path al archivo de store
		char* pathArchivo = string_new();
		string_append(&pathArchivo, configuracion.punto_montaje);
		string_append(&pathArchivo, configuracion.nombre_instancia);
		string_append(&pathArchivo, "/");
		string_append(&pathArchivo, clave);
		string_append(&pathArchivo, ".txt");
		//Intento borrar archivo existente
		if (remove(pathArchivo)) {
			log_debug(logInstancia, "Se eliminó archivo antes de realizar STORE");
		} else {
			log_debug(logInstancia, "No habia un archivo previo al STORE");
		}
		//Creo archivo para escritura
		int fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
		if (fd == -1) {
			log_error(logInstancia, "Error al crear el archivo para el STORE");
			exitFailure();
		} else {
			//Escribo archivo
			int result = write(fd, valor, tamanioArchivo);
			if (result < 0) {
				close(fd);
				log_error(logInstancia, "Error al escribir el archivo de STORE");
				exitFailure();
			} else {
				log_debug(logInstancia, "Se escribió correctamente el valor en el archivo de STORE");
				close(fd);
				entrada->tiempo=timer;
			}
		}
		free(valor);
		enviar_ok_sentencia_a_Coordinador();
		imprimirPorPantallaEstucturas(tablaEntradas, diccionarioEntradas, matrizValores, entradasCantidad, entradasTamanio);
	} else {
		log_debug(logInstancia, "Clave no existente para realizar STORE");
	}
}

void procesarDump(){
	//proceso dump en cantidad de ciclos
	if (configuracion.intervalo_dump == timerDump + 1) {

		//recorro matriz de entradas de 0 a cantidad de entradas
		int i = 0;
		while (i < entradasCantidad) {
			if (tablaEntradas[i].clave[0]	&& tablaEntradas[i].tamanioValor) {
				char* pathArchivo = string_new();
				string_append(&pathArchivo, configuracion.punto_montaje);
				string_append(&pathArchivo, configuracion.nombre_instancia);
				string_append(&pathArchivo, "/");
				string_append(&pathArchivo, "DUMP/");
				string_append(&pathArchivo,	tablaEntradas[i].clave);
				string_append(&pathArchivo, ".txt");
				//se accede a la tabla de entradas
				char* textoValor = leerMatrizValores(tablaEntradas[i].numeroEntrada, tablaEntradas[i].tamanioValor);
				int tamanioArchivo = 0;
				tamanioArchivo = string_length(textoValor);
				log_debug(logInstancia, "IMPORTANTE linea 746 texto: %s %d", textoValor, tamanioArchivo);

				if (remove(pathArchivo)) {
					log_debug(logInstancia, "Se eliminó archivo satifactoriamente");
				} else {
					log_debug(logInstancia, "No se eliminó archivo");
				}

				int fd = open(pathArchivo, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0600);
				if (fd == -1) {
					log_error(logInstancia, "Error al abrir para escritura");
					exitFailure();
				} else {
					int result = write(fd, textoValor, tamanioArchivo);

					if (result < 0) {
						close(fd);
						log_error(logInstancia, "Error al escribir");
						exit(EXIT_FAILURE);
					} else {
						log_debug(logInstancia, "Se guardo archivo");
						close(fd);

					}
				}
				free(textoValor);
			}//TERMINO EL IF

			if (redondearArribaDivision( tablaEntradas[i].tamanioValor, entradasTamanio) == 0) {
				i++;
			} else {
				i = i + redondearArribaDivision(tablaEntradas[i].tamanioValor, entradasTamanio);
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
	int resultado = recibir_mensaje(socketCoordinador, buffer, tamanio);
	if(resultado < 0){
		log_error(logInstancia, "Error al recibir");
		exitFailure();
	}
	return buffer;
}

//LAG


int enviarBuffer(void*buffer,int tamanio){

	int resultado = enviar_mensaje(socketCoordinador,&buffer,tamanio);
	if(resultado < 0){
		log_error(logInstancia, "Error al enviar");
		exitFailure();
	}
	return resultado;
}
//fin LAG

int main(int argc, char *argv[]) {

	//Creo el log
	logInstancia = log_create("instancia.log", "Instance", true, LOG_LEVEL_DEBUG);

	//Lee archivo de configuraciones
	configuracion = cargarConfiguracion();


	//creo carpeta si no existe
char * path= string_new();
string_append(&path,"mkdir ");
string_append(&path,configuracion.punto_montaje);
string_substring(&path,0,strlen(&path)-1);
system(path);
string_append(&path,"/");
string_append(&path,configuracion.nombre_instancia);
printf("DIRECTORIO DATOS: %s\n",path);
system(path);
string_append(&path,"/DUMP");
system(path);

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
		case msj_instancia_compactar:
			compactarMatrizValores();
			break;
		}
		//Verifico y realizo el Dump en caso de que sea necesario
		//TODO: Revisar y descomentar procesarDump();
		procesarDump();
	}
	close(socketCoordinador);
	return EXIT_SUCCESS;
}
