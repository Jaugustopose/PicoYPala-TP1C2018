#ifndef SRC_CONEXIONES_H_
#define SRC_CONEXIONES_H_

void* iniciarEscucha(void* sockets);
void procesar_handshake(int socketCliente);
int mandar_a_ejecutar_esi(int socket_esi);

#endif /* SRC_CONEXIONES_H_ */
