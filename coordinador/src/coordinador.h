/*
 * coordinador.h
 *
 *  Created on: 26 abr. 2018
 *      Author: utnso
 */

#ifndef COORDINADOR_H_
#define COORDINADOR_H_

void establecer_configuracion(int puertoEscucha, int puertoServidor, char* algoritmo, int entradas, int tamanioEntrada, int retard);
void responder_no_OK_handshake(int socketCliente);
void* instancia_conectada_anteriormente(char* unNombreInstancia);
void identificar_proceso_y_crear_su_hilo(int socketCliente);
void conexion_de_cliente_finalizada();
void* encontrar_esi_en_lista(int unESI);
void* encontrar_instancia_por_clave(char* unaClave);
void* esi_con_clave(int unESI, char* unaClave);
void atender_accion_esi(int fdEsi);
void atender_accion_instancia(int fdInstancia);

#endif /* COORDINADOR_H_ */
