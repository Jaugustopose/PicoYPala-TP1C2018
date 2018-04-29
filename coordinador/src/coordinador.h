/*
 * coordinador.h
 *
 *  Created on: 26 abr. 2018
 *      Author: utnso
 */

#ifndef COORDINADOR_H_
#define COORDINADOR_H_

void establecer_configuracion(int puerto_escucha, int puerto_servidor, char* algoritmo, int entradas, int tamanio_entrada, int retard);
void identificar_proceso_e_ingresar_en_bolsa(int socket_cliente);
void conexion_de_cliente_finalizada();
void atender_accion_esi(int fdEsi);
void atender_accion_instancia(int fdInstancia);

#endif /* COORDINADOR_H_ */
