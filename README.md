# tp-2018-1c-Pico-y-Pala

# Mensajes dentro del sistema:

1) msj_handshake = 1 //Todos
2) msj_imposibilidad_conexion = 2 //COORDINADOR envia a 2do PLANIFICADOR que se quiere conectar.
3) msj_nombre_instancia = 3 // INSTANCIA envia su nombre al COORDINADOR.
4) msj_instruccion_esi = 4 //ESI envia sentencia parseada al COORDINADOR.
5) msj_solicitud_get_clave = 5 //COORDINADOR avisa a PLANIFICADOR que un ESI esta queriendo tomar cierta clave.
6) msj_clave_permitida_para_get = 6 PLANIFICADOR avisa a COORDINADOR que cierta clave puede ser geteada.
7) msj_crear_clave_get = 7 // COORDINADOR avisa a INSTANCIA que tiene que crear la clave por GET.
8) msj_requerimiento_ejecucion = 8 //
9) msj_sentencia_finalizada = 9 //
10) msj_esi_finalizado = 10 //
