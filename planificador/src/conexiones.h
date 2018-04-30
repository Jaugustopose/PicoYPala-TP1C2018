#ifndef SRC_CONEXIONES_H_
#define SRC_CONEXIONES_H_

int conectarConCoordinador(char* ip, int puerto);
void iniciarEscucha(int socketEscucha);
void procesar_handshake(int socketCliente);

#endif /* SRC_CONEXIONES_H_ */
