/*
 * instancia.h
 *
 *  Created on: 22 abr. 2018
 *      Author: utnso
 */

#ifndef SRC_INSTANCIA_H_
#define SRC_INSTANCIA_H_
#include <commons/log.h>



#endif /* SRC_INSTANCIA_H_ */

//struct t_dictionary* mapArchivoTablaDeEntrada;

int entradasCantidad ;
int entradasTamanio ;

t_log * log_errores;

/*defino estructura de datanode*/
typedef struct{
	char* ip_coordinador;
	int puerto_coordinador;
	char* algoritmo_remplazo;
	char* punto_montaje;
	char* nombre_instancia;
	int intervalo_dump;

}config_t;

typedef struct  {
	char clave[41];
	int numeroEntrada;
	int tamanioValor;
	int tiempo;

}t_entrada;



config_t cargarConfiguracion(char *path);
//char* creacion_y_mapeo_archivo(int entradaCantidad, int entradaTamanio,		char* pathArchivo);
char* escribirEntrada(char* matriz, int tamanioEntradas,int numEntrada,char* textoAEscribir) ;
char* leerEntrada(char * matriz, int tamanioEntradas, int numEntrada,int longitud);
