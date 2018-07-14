#ifndef SRC_CONEXIONES_H_
#define SRC_CONEXIONES_H_

void* iniciarEscucha(void* sockets);
void procesar_handshake(int socketCliente);
int mandar_a_ejecutar_esi(int socket_esi);
void procesarStatusClave(int socket_coordinador, char* clave);

#endif /* SRC_CONEXIONES_H_ */
