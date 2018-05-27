#ifndef SRC_CONEXIONES_H_
#define SRC_CONEXIONES_H_

void* iniciarEscucha(void* sockets);
void procesar_handshake(int socketCliente);
void procesar_notificacion_coordinador(int socket_coordinador);
int mandar_a_ejecutar_esi(int socket_esi);

#endif /* SRC_CONEXIONES_H_ */
