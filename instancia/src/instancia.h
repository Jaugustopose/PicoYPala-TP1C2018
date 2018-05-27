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

config_t cargarConfiguracion(char *path);
char* creacion_y_mapeo_archivo(int entradaCantidad, int entradaTamanio,
		char* pathArchivo);
char* escribirEntrada(char* map, int numeroEntrada, char* texto, int cantidadEntradas,int entradasTamanio) ;
