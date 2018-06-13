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
//char* texto;

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

int redondearArribaDivision(int divisor, int dividendo);
void crearMatrizEntradas();
void escribirMatrizValores(int indexEntrada, char* valor);
char* leerMatrizValores(int indexEntrada, int tamanio);
void imprimirPorPantallaEstucturas();
