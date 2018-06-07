# tp-2018-1c-Pico-y-Pala

# Mensajes dentro del sistema:

1) msj_handshake = 1 //Todos
2) msj_imposibilidad_conexion = 2 //COORDINADOR envia a 2do PLANIFICADOR que se quiere conectar.
3) msj_nombre_instancia = 3 // INSTANCIA envia su nombre al COORDINADOR.
4) msj_instruccion_esi = 4 //ESI envia sentencia parseada al COORDINADOR.
5) msj_solicitud_get_clave = 5 //COORDINADOR avisa a PLANIFICADOR que un ESI esta queriendo tomar cierta clave.
6) msj_clave_permitida_para_operar = 6 PLANIFICADOR avisa a COORDINADOR que cierto ESI puede operar (tiene tomada la clave).
7) msj_crear_clave_get = 7 // COORDINADOR avisa a INSTANCIA que tiene que crear la clave por GET.
8) msj_store_clave = 8 //
9) msj_requerimiento_ejecucion = 8 //
10) msj_sentencia_finalizada = 9 //
11) msj_esi_finalizado = 10 //
12) msj_esi_tiene_tomada_clave = 12 // COORDINADOR pregunta al PLANIFICADOR si el ESI tiene tomada la clave.
13) msj_ok_solicitud_operacion = 13 //
14) msj_fail_solicitud_operacion = 14 //
15) msj_abortar_esi = 15 //
16) msj_sentencia_get = 16 // MENSAJE GENERAL indicando que la instruccion es un GET.
17) msj_sentencia_set = 17 // MENSAJE GENERAL indicando que la instruccion es un SET.
18) msj_sentencia_store = 18 // MENSAJE GENERAL indicando que la instruccion es un STORE.
19) msj_error_tamanio_clave = 19 // Mensaje para manejo del error: Tamaño de clave.
20) msj_error_clave_no_identificada = 20 // Mensaje para manejo del error: Clave no identificada.
21) msj_error_comunicacion = 21 // Mensaje para manejo del error: Error de comunicación.
22) msj_error_clave_inaccesible = 22 // Mensaje para manejo del error: Clave inaccesible (existe pero en instancia desconectada)
23) msj_error_clave_no_bloqueada = 23 // Mensaje para manejo del error: Clave no bloqueada RELACIONADO CON 6).
