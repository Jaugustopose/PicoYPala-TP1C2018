#ifndef SRC_PLANIFICADOR_H_
#define SRC_PLANIFICADOR_H_

//Estructuras Consola
//Opciones de entrada por consola
char PAUSE_PLANIF[] = "pausar";
char CONTINUE_PLANIF[] = "continuar";
char BLOCK[] = "bloquear";
char UNBLOCK[] = "desbloquear";
char LIST[] = "listar";
char KILL[] = "kill";
char STATUS[] = "status";
char DEADLOCK[] = "deadlock";

//Prototipos Consola
void procesar_entradas_consola();
void procesar_entrada(char* buffer);
void procesar_pausa_planificacion(char** subBufferSplitted);
void procesar_continuar_planificacion(char** subBufferSplitted);
void procesar_bloqueo_esi(char** subBufferSplitted);
void procesar_desbloqueo_esi(char** subBufferSplitted);
void procesar_listar_recurso(char** subBufferSplitted);
void procesar_kill_proceso(char** subBufferSplitted);
void procesar_status_instancias(char** subBufferSplitted);
void procesar_deadlock(char** subBufferSplitted);

#endif /* SRC_PLANIFICADOR_H_ */
