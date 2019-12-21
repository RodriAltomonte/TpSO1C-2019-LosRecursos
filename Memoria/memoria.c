#include "memoria.h"

int main() {

  configurarLogger();
  log_info(logger, "- LOG INICIADO");

  PROCESO_CONFIG = cargarConfiguracion(tconfig);
  log_info(logger, "- PROCESO CONFIGURADO CON EXITO!");
  printf("· Pen MAinnns = %s", PROCESO_CONFIG->puertoSeeds[1]);

  conectar();
  
  inicializarMemoria();

  if (pthread_create(&hiloConsola, NULL, (void*) consola, NULL)) {
         log_info(logger, "- Error! creando hilo consola\n");
         return 2;
  }
    
  if (pthread_create(&hiloGossiping, NULL, gossiping, NULL)) {
         log_info(logger, "- Error! creando hilo gossiping\n");
         return 3;
  }

  if (pthread_create(&hilojournal, NULL, ejecutarJournaling, NULL)) {
         log_info(logger, "- Error! creando hilo journalling \n");
         return 4;
  }

  infoServidor_t * unaInfoServidor = malloc(sizeof(infoServidor_t));
	unaInfoServidor->log = logger;
	unaInfoServidor->puerto = PROCESO_CONFIG->puertoEscucha;
  unaInfoServidor->ip = string_new();

  char* miIp = string_new();
  string_append(&miIp,PROCESO_CONFIG->ipSeeds[0]);

  miIp = string_substring_from(miIp, 1);
  miIp = string_reverse(miIp);
  miIp = string_substring_from(miIp, 1);
  miIp = string_reverse(miIp);

  log_info(logger,"IP DE LA MEMORIA: %s", miIp);

  string_append(&unaInfoServidor->ip,miIp);

  if (pthread_create(&hiloServidor,NULL,(void*)servidor_inicializar,(void*)unaInfoServidor)) {
         log_info(logger, "- Error! creando hilo servidor\n");
         return 5;
  }

  pthread_join(hiloConsola , NULL);
  
  

  //finalizar();
  
  return EXIT_SUCCESS;

}

///////////////////////////////////////////////////////////////////////////////////////////

void configurarLogger() {
  
  logger = log_create("logs/memoria.log", "Memoria", true, LOG_LEVEL_TRACE); 
  //cambiar 3er parámetro a "false" para que no muestre por pantalla el log.

}

memoriaConfig *cargarConfiguracion(t_config *tconfig) {

  pthread_mutex_lock(&mutex_Config);
	 
  memoriaConfig *config = malloc(sizeof(memoriaConfig));
  tconfig = config_create(MEM_CONFIG_PATH);

  if (tconfig == NULL) {
    log_error(logger,"- NO SE PUDO IMPORTAR LA CONFIGURACION");
    exit(1);
  }
  log_info(logger, "- CONFIGURACION IMPORTADA");

  config->puertoEscucha = config_get_int_value(tconfig, PUERTO_MEMORIA);
  config->ipFileSystem = config_get_string_value(tconfig, IP_FS);
  config->puertoFileSystem = config_get_int_value(tconfig, PUERTO_FS);
  config->ipSeeds = config_get_array_value(tconfig, IP_SEEDS);
  config->puertoSeeds = config_get_array_value(tconfig, PUERTO_SEEDS);
  config->retardoMemoria = config_get_int_value(tconfig, RETARDO_MEMORIA);
  config->retardoFileSystem = config_get_int_value(tconfig, RETARDO_FS);
  config->tamanioMemoria = config_get_int_value(tconfig, TAMANIO_MEMORIA);
  config->retardoJournal = config_get_int_value(tconfig, RETARDO_JOURNAL);
  config->retardoGossiping = config_get_int_value(tconfig, RETARDO_GOSSIPING);
  config->idMemoria = config_get_int_value(tconfig, NUMERO_MEMORIA);

  log_info(logger, "· Puerto escucha = %d", config->puertoEscucha);
  log_info(logger, "· IP FileSystem = %s", config->ipFileSystem);
  log_info(logger, "· Puerto FileSystem = %d", config->puertoFileSystem);
  log_info(logger, "· IP Seeds = %s", config->ipSeeds[1]);
  log_info(logger, "· Puerto Seeds = %s", config->puertoSeeds[1]);
  log_info(logger, "· Retardo Memoria = %d", config->retardoMemoria);
  log_info(logger, "· Retardo FileSystem = %d", config->retardoFileSystem);
  log_info(logger, "· Tamaño Memoria = %d", config->tamanioMemoria);
  log_info(logger, "· Retardo Journal = %d", config->retardoJournal);
  log_info(logger, "· Retardo Gossiping = %d", config->retardoGossiping);
  log_info(logger, "· id Memoria = %d", config->idMemoria);

  free(tconfig);

  pthread_mutex_unlock(&mutex_Config);

  return config;

}

///////////////////////////////////////////////////////////////////////////////////////////

void conectar() {

  SOCKET_LFS = cliente(PROCESO_CONFIG->ipFileSystem, PROCESO_CONFIG->puertoFileSystem, ID_CLIENTE, logger);
  pedirTamanioMaximo(SOCKET_LFS, 2);

}

///////////////////////////////////////////////////////////////////////////////////////////

void inicializarMemoria() {

  TAM_MAX_REGISTRO = sizeof(uint16_t) + sizeof(uint16_t) + TAM_MAX_VALUE + sizeof(uint32_t);
  int totalPaginas = PROCESO_CONFIG->tamanioMemoria / TAM_MAX_REGISTRO;

  if ((MEMORIA_PRINCIPAL = malloc(PROCESO_CONFIG->tamanioMemoria)) == NULL) {
   log_error(logger, "- Error! no se pudo alocar MEMORIA_PRINCIPAL.");
   exit(EXIT_FAILURE);
  }

  //TABLA_PAGINAS = list_create();
  TABLA_MARCOS_LIBRES = list_create();
  TABLA_SEGMENTOS = list_create();
  TABLA_GOSSIPING= list_create();

  for (int i = 0; i < totalPaginas; i++) {
    tPagina* nuevaPagina = malloc(sizeof(tPagina));
    nuevaPagina->nroPagina = i;
    nuevaPagina->flag = 0;
    nuevaPagina->reg = malloc(sizeof(registro_t));
    list_add(TABLA_MARCOS_LIBRES, nuevaPagina);
  }

   

  int m = 0;
  char * miIp;

  do{
      tSeed* unSeed = malloc(sizeof(tSeed));

      if(m == 0){
        unSeed->id = PROCESO_CONFIG->idMemoria;
        unSeed->flag = 1;
        unSeed->socket =-2;
      }else{
            unSeed->id = -1;
            unSeed->flag = 0;
            unSeed->socket = -1;
      }
      

      miIp = string_new();
      string_append(&miIp,PROCESO_CONFIG->ipSeeds[m]);

      //le saco las "" del valor
      miIp = string_substring_from(miIp, 1);
      miIp = string_reverse(miIp);
      miIp = string_substring_from(miIp, 1);
      miIp = string_reverse(miIp);

      unSeed->ip = string_new();
      string_append(&unSeed->ip,miIp);

      unSeed->puerto = atoi(PROCESO_CONFIG->puertoSeeds[m]);

      list_add_in_index(TABLA_GOSSIPING,m,unSeed);
      printf("\n TABLA GOSSIPING \n");
      printf("\n  unSeed->id = %d ", unSeed->id);
      printf("\n  unSeed->ip = %s ", unSeed->ip);
      printf("\n unSeed->puerto = %d ", unSeed->puerto);
      printf("\n  unSeed->socket = %d ", unSeed->socket);
      printf("\n *************************************");


      m++;
      //printf("\n usiguiente seed = %d ", atoi(PROCESO_CONFIG->puertoSeeds[m]));

  }while((PROCESO_CONFIG->puertoSeeds[m]) != NULL);
 

}

///////////////////////////////////////////////////////////////////////////////////////////

void* consola(void* args) {

  builtins_t builtins[] = {
        {"SELECT", &memSelect},
        {"INSERT", &memInsert},
        {"CREATE", &memCreate},
        {"DESCRIBE", &memDescribe},
        {"DROP", &memDrop},
        {"JOURNAL", &memJournal},
  };
  log_info(logger, "- ESTOY EN consola JUJU");
 
  ConsolaInicializar("Memoria>", builtins, MAX_BUILTINS_MEMORIA);
  ConsolaMain();
  
}

///////////////////////////////////////////////////////////////////////////////////////////

void memSelect(char** args) {

  /*
  1: Verifica si existe el segmento de la tabla solicitada y busca en las páginas del mismo si contiene 
    key solicitada. Si la contiene, devuelve su valor y finaliza el proceso.
  2: Si no la contiene, envía la solicitud a FileSystem para obtener el valor solicitado y almacenarlo.
  3: Una vez obtenido se debe solicitar una nueva página libre para almacenar el mismo. En caso de no 
    disponer de una página libre, se debe ejecutar el algoritmo de reemplazo y, en caso de no poder 
    efectuarlo por estar la memoria full, ejecutar el Journal de la memoria.
  */

 	pthread_mutex_lock(&mutex_memSelect);

  log_info(logger,"COMANDO A EJECUTAR: SELECT");

  if (args[1] == NULL) {

    log_warning(logger,"NO SE INGRESÓ NINGUNA TABLA");

  }else if(args[2] == NULL){

    log_warning(logger,"NO SE INGRESÓ NINGUNA KEY.");

  }else{

    char* tabla = args[1];
    uint16_t clave = (uint16_t) atoi(args[2]);

    tSegmento* segmento = traerSegmento(tabla);

    char* valor = string_new();

    if(segmento) {

      tPagina* pagina = traerPagina(segmento, clave);

      if(pagina) { 

        string_append(&valor, pagina->reg->value);

      }else{

        registro_t* nuevoRegistro = pedirRegistroAlServidor(SOCKET_LFS, tabla, clave,2);

        if(nuevoRegistro != NULL){
          almacenarValor(clave, nuevoRegistro->value, nuevoRegistro->timestamp, segmento);
          string_append(&valor, nuevoRegistro->value);
          free(nuevoRegistro);
        }else{
          log_info(logger, "No existe el registro en memoria ni en el File System.");
        }
      }

      log_info(logger, "VALUE: %s", valor);
  
    }else{

      registro_t* nuevoRegistro = pedirRegistroAlServidor(SOCKET_LFS, tabla, clave, 2);

      if(nuevoRegistro != NULL){

        crearSegmento(tabla);
        segmento = traerSegmento(tabla); //CONSULTAR A RODRI SI ESTA BIEN!!

        almacenarValor(clave, nuevoRegistro->value, nuevoRegistro->timestamp, segmento);
        string_append(&valor, nuevoRegistro->value);
        log_info(logger, "No existe la tabla en memoria. Traemos el value del FS. VALUE: %s", nuevoRegistro->value);
        free(nuevoRegistro);

      }else{

        log_info(logger, "No existe el registro en memoria ni en el File System.");

      }
    }
  }

  pthread_mutex_unlock(&mutex_memSelect);

}

///////////////////////////////////////////////////////////////////////////////////////////

void memInsert(char** args) {
  /*
  1: Verifica si existe el segmento de la tabla en la memoria principal. De existir, busca en sus páginas 
    si contiene la key solicitada y de contenerla actualiza el valor insertando el Timestamp actual. 
    En caso que no contenga la Key, se solicita una nueva página para almacenar la misma. Se deberá tener 
    en cuenta que si no se disponen de páginas libres aplicar el algoritmo de reemplazo y en caso de que la 
    memoria se encuentre full iniciar el proceso Journal.
  2: En caso que no se encuentre el segmento en memoria principal, se creará y se agregará la nueva Key con 
    el Timestamp actual, junto con el nombre de la tabla en el segmento. Para esto se debe generar el nuevo 
    segmento y solicitar una nueva página (aplicando para este último la misma lógica que el punto anterior).
  */

 	pthread_mutex_lock(&mutex_memInsert);

  log_info(logger,"COMANDO A EJECUTAR: INSERT");

  if (args[1] == NULL) {
      log_warning(logger,"NO SE INGRESÓ NINGUNA TABLA");
  }else if(args[2] == NULL){
           log_warning(logger,"NO SE INGRESÓ NINGUNA KEY.");
  }else if(args[3] == NULL){
           log_warning(logger,"NO SE INGRESÓ NINGUN VALOR.");
  }else{

    	  char* tablaActual = string_new();
        string_append(&tablaActual,args[1]); 
        string_trim(&tablaActual);
        uint32_t timeStampActual;

        uint16_t claveActual = (uint16_t) atoi(args[2]);
        char* valor =string_new();

        valorRegistro_t* valorRegistro;

        valorRegistro = chequearValorEntreComillas(args, TAM_MAX_VALUE);

        string_append(&valor,valorRegistro->valor);

        int proximoParametro = valorRegistro->proximoParametro;

        free(valorRegistro);

        if(!(proximoParametro > 0)){

           log_warning(logger,"EL VALOR DEBE INGRESARSE ENTRE COMILLAS Y NO DEBE SUPERAR EL MAXIMO DE CARACTERES PERMITIDO.");
        
         }else{

                if(args[proximoParametro] == NULL){
                  timeStampActual = (uint32_t)time(NULL);
        
                }else{
                      timeStampActual = (uint32_t) atoi(args[proximoParametro]);
                }      

                log_info(logger,"TIMESTAMP DEL USUARIO: %d", timeStampActual);

                tSegmento *segmentoActual = traerSegmento(tablaActual);
                tPagina *paginaActual = NULL; 
  
                if (segmentoActual){

                    paginaActual = traerPagina(segmentoActual, claveActual);
    
                    if(paginaActual){

                      paginaActual->reg->value  = string_new();
                      string_append(&paginaActual->reg->value,valor);
                      paginaActual->reg->timestamp = timeStampActual;
                      paginaActual->flag = 1;
                      actualizarFrame(paginaActual);
      
                    }else{
                          almacenarValor(claveActual, valor, timeStampActual, segmentoActual);
                    }

                }else{
                      tSegmento *nuevoSegmento = crearSegmento(tablaActual);
                      almacenarValor(claveActual, valor, timeStampActual, nuevoSegmento);
                }

                log_info(logger, "-TABLA %s, KEY %d, actualizada correctamente!", tablaActual, claveActual);



          }    
  }

  pthread_mutex_unlock(&mutex_memInsert);

}

///////////////////////////////////////////////////////////////////////////////////////////

void memCreate(char** args) {
  /*
  1: Se envía al FileSystem la operación para crear la tabla.
  2:Tanto si el FileSystem indica que la operación se realizó de forma exitosa o en caso de falla por tabla 
    ya existente, continúa su ejecución normalmente.
  */

 	pthread_mutex_lock(&mutex_memCreate);

  log_info(logger,"COMANDO A EJECUTAR: CREATE");


    if (args[1] == NULL) {
        log_warning(logger,"NO SE INGRESÓ NINGUNA TABLA");
    }else if(args[2] == NULL){
             log_warning(logger,"NO SE INGRESÓ UN TIPO DE CONSISTENCIA");
    }else if(args[3] == NULL){
             log_warning(logger,"NO SE INGRESÓ UNA CANTIDAD DE PARTICIONES");
    }else if(args[4] == NULL){
             log_warning(logger,"NO SE INGRESÓ UN TIEMPO DE COMPACTACIÓN");
    }else{
          int tamanioCreateCreado = 0;
	        t_create* unCreateCreado = malloc(sizeof(t_create));

          unCreateCreado->comando = string_new();
          string_append(&unCreateCreado->comando ,args[0]);

          unCreateCreado->tabla = string_new();
          string_append(&unCreateCreado->tabla ,args[1]);

          unCreateCreado->tipoConsistencia = string_new();
          string_append(&unCreateCreado->tipoConsistencia ,args[2]);

          unCreateCreado->cantParticiones = (uint16_t) atoi(args[3]);
  
          unCreateCreado->tiempo_entre_compactaciones = (uint16_t) atoi(args[4]);

          enviarInt(SOCKET_LFS, 2);
          enviarPaquete(SOCKET_LFS, tCreate, unCreateCreado, tamanioCreateCreado, logger);

          int resultado;
          int tipoResultado = 0;

          if((resultado = recibirInt(SOCKET_LFS,&tipoResultado)) > 0){

            if(tipoResultado == 13){

              log_info(logger,"Tabla creada con éxito en FS: %s",unCreateCreado->tabla);

            }else if(tipoResultado == 14){

              log_info(logger,"No se pudo crear la tabla en FS: %s",unCreateCreado->tabla);

            }
          }
          free(unCreateCreado);
    }

  pthread_mutex_unlock(&mutex_memCreate);

}

///////////////////////////////////////////////////////////////////////////////////////////

void memDescribe(char** args) {
  /*
  Esta operación consulta al FileSystem por la metadata de las tablas. Sirve principalmente para poder 
    responder las solicitudes del Kernel.
  */

 	pthread_mutex_lock(&mutex_memDescribe);

  log_info(logger,"COMANDO A EJECUTAR: DESCRIBE");

	t_describe* unDescribeCreado = malloc(sizeof(t_describe));

  unDescribeCreado->comando = string_new();
  string_append(&unDescribeCreado->comando,args[0]);

  unDescribeCreado->tabla = string_new();

  if (args[1] == NULL){
      string_append(&unDescribeCreado->tabla,"");
  }else{      
      string_append(&unDescribeCreado->tabla,args[1]);
  }

  t_list* listaMetadatas = pedirMetadatasAlServidor(SOCKET_LFS, unDescribeCreado, 2);

  log_info(logger,"Describe mandado con éxito al FS");

  if(listaMetadatas != NULL){
    list_destroy_and_destroy_elements(listaMetadatas,eliminarNodoMetadata);
  }  

	pthread_mutex_unlock(&mutex_memDescribe);

}

///////////////////////////////////////////////////////////////////////////////////////////

void memDrop(char** args) {
  /*
  1: Verifica si existe un segmento de dicha tabla en la memoria principal y de haberlo libera dicho 
    espacio.
  2: Informa al FileSystem dicha operación para que este último realice la operación adecuada.
  */

 	pthread_mutex_lock(&mutex_memDrop);
  
  log_info(logger,"COMANDO A EJECUTAR: DROP");

	if (args[1] == NULL) {
        log_warning(logger,"NO SE INGRESÓ NINGUNA TABLA");
  }else{
    char* tablaActual = string_new();
    string_append(&tablaActual,args[1]); 
    tSegmento *segmentoActual = traerSegmento(tablaActual);
  
    if(segmentoActual){
      eliminarSegmentoDeMemoria(segmentoActual);
    }

    int tamanioDropCreado = 0;
	  t_drop* unDropCreado = malloc(sizeof(t_drop));

    unDropCreado->comando = string_new();
    string_append(&unDropCreado->comando ,args[0]);

    unDropCreado->tabla = string_new();
    string_append(&unDropCreado->tabla ,args[1]);

    enviarInt(SOCKET_LFS, 2);
    enviarPaquete(SOCKET_LFS, tDrop, unDropCreado, tamanioDropCreado, logger);

    int resultado;
    int tipoResultado = 0;

    if((resultado = recibirInt(SOCKET_LFS,&tipoResultado)) > 0){

      if(tipoResultado == 11){

        log_info(logger,"Tabla borrada con éxito en FS");

      }else if(tipoResultado == 12){

        log_info(logger,"No se pudo borrar la tabla en FS");

      }
    }
    free(unDropCreado);
  }  
  pthread_mutex_unlock(&mutex_memDrop);
}

///////////////////////////////////////////////////////////////////////////////////////////

void memJournal() {
  /*
  1: La operación Journal permite el envío manual de todos los datos de la memoria al FileSystem.
  */

 	pthread_mutex_lock(&mutex_memJournal);

 	pthread_mutex_lock(&mutex_memSelect);

 	pthread_mutex_lock(&mutex_memInsert);

 	pthread_mutex_lock(&mutex_memCreate);

 	pthread_mutex_lock(&mutex_memDescribe);

 	pthread_mutex_lock(&mutex_memDrop);

/*codigo*/
  tSegmento* unSegmento;
  
  while(!list_is_empty(TABLA_SEGMENTOS)){

        tPagina* unPagina;
        unSegmento = list_remove(TABLA_SEGMENTOS,0);

        while(!list_is_empty(unSegmento->paginas)){
  
              unPagina = list_remove(unSegmento->paginas,0);

              printf("\nDentro de journal....\n");

              if(unPagina->flag == 1){
                //creo un t_insert y lo envio al lfs


                printf("\nDentro del if del journal....\n");
                unPagina =llenarRegistro(unPagina);
                unPagina->flag = 0;
                int tamanioInsertCreado = 0;
    	          t_insert* unInsertCreado = malloc(sizeof(t_insert));

                unInsertCreado->comando = string_new();
                string_append(&unInsertCreado->comando ,"INSERT");

                unInsertCreado->tabla = string_new();
                string_append(&unInsertCreado->tabla ,unSegmento->path);

                unInsertCreado->key = unPagina->reg->key;

	              unInsertCreado->valor = string_new();
                string_append(&unInsertCreado->valor ,unPagina->reg->value);

	              unInsertCreado->timestamp = unPagina->reg->timestamp;

                enviarInt(SOCKET_LFS, 2);
                enviarPaquete(SOCKET_LFS, tInsert, unInsertCreado, tamanioInsertCreado, logger);

                int resultado;
                int tipoResultado = 0;

                if((resultado = recibirInt(SOCKET_LFS,&tipoResultado)) > 0){

                  if(tipoResultado == 9){

                    log_info(logger,"Se insertó el registro con éxito en FS: %s",unInsertCreado->tabla);

                  }else if(tipoResultado == 10){

                    log_info(logger,"No se pudo insertar el registro en FS (tabla inexistente o bloqueada): %s",unInsertCreado->tabla);

                  }
                }
                free(unInsertCreado);
              }
    
              list_add_in_index(TABLA_MARCOS_LIBRES,unPagina->nroPagina,unPagina);

        }

        list_destroy(unSegmento->paginas);
        free(unSegmento);

  }

  pthread_mutex_unlock(&mutex_memDrop);

	pthread_mutex_unlock(&mutex_memDescribe);

	pthread_mutex_unlock(&mutex_memCreate);

	pthread_mutex_unlock(&mutex_memInsert);

	pthread_mutex_unlock(&mutex_memSelect);

	pthread_mutex_unlock(&mutex_memJournal);

}

///////////////////////////////////////////////////////////////////////////////////////////

void finalizar() {

  tPagina* unaPag;

  while(!list_is_empty(TABLA_MARCOS_LIBRES)){
        
    unaPag = list_remove(TABLA_MARCOS_LIBRES,0);
    free(unaPag->reg);   

  }

  list_destroy(TABLA_MARCOS_LIBRES);
  list_destroy(TABLA_SEGMENTOS);
  free(MEMORIA_PRINCIPAL);
  free(PROCESO_CONFIG);
  free(logger);

}

///////////////////////////////////////////////////////////////////////////////////////////


void test(){//pruebo traer pagina y existekey

  printf("\n\n\n");
  log_info(logger, "estoy adentro del test jojoj");
  log_info(logger, "-Valor uint16_t = %d", sizeof(uint16_t));//2
  log_info(logger, "-Valor int = %d", sizeof(int));//4
  log_info(logger, "-Valor time_t = %d", sizeof(time_t));//4

  memset(MEMORIA_PRINCIPAL,0,sizeof(MEMORIA_PRINCIPAL));

  TAM_MAX_VALUE=40;//para prueba
  TAM_MAX_REGISTRO = 48;//2+2+4+TAM_MAX_VALUE
  
 /* tSegmento* nuevoSegmento = malloc(sizeof(tSegmento));
  nuevoSegmento->path = string_new();
  string_append(& nuevoSegmento->path,"bebidas");
  nuevoSegmento->paginas= list_create();

  tPagina* nuevoPagina1 = malloc(sizeof(tPagina));
  nuevoPagina1->nroPagina = 4;
  nuevoPagina1->flag=1;

  registro_t* nuevoreg = malloc(sizeof(registro_t));
  nuevoreg->key = 111;
  //nuevoreg->tamanioValue = 4;
  nuevoreg->timeStamp = time(NULL) * 0.001;;
  nuevoreg->value = string_new();
  string_append(&nuevoreg->value,"coca");

  nuevoPagina1->reg = nuevoreg;

  tPagina* nuevoPagina2 = malloc(sizeof(tPagina));
  nuevoPagina2->nroPagina = 8;
  nuevoPagina2->flag=0;

  registro_t* nuevoreg2 = malloc(sizeof(registro_t));
  nuevoreg2->key = 123;
  nuevoreg2->timeStamp = time(NULL) * 0.001;;
  nuevoreg2->value = string_new();
  string_append(&nuevoreg2->value,"pepsi");

  nuevoPagina2->reg = nuevoreg2;

  printf("\nAntes el 111 :\n");
  printf("flag: %d\n",nuevoPagina1->flag);
  printf("nropagina: %d\n",nuevoPagina1->nroPagina);
  printf("registro key : %d\n",nuevoPagina1->reg->key);
  printf("registro value : %s\n",nuevoPagina1->reg->value);
  printf("registro timestamp : %d\n", (int) nuevoPagina1->reg->timeStamp);
  printf("*****************************\n");

  printf("\nAntes el 123 :\n");
  printf("flag: %d\n",nuevoPagina2->flag);
  printf("nropagina: %d\n",nuevoPagina2->nroPagina);
  printf("registro key : %d\n",nuevoPagina2->reg->key);
  printf("registro value : %s\n",nuevoPagina2->reg->value);
  printf("registro timestamp : %d\n", (int) nuevoPagina2->reg->timeStamp);
  printf("*****************************\n");

  list_add(nuevoSegmento->paginas,nuevoPagina1);
  list_add(nuevoSegmento->paginas,nuevoPagina2);

  actualizarFrame(nuevoPagina1);
  actualizarFrame(nuevoPagina2);

  list_add(TABLA_SEGMENTOS,nuevoSegmento);
  tPagina* paginaTest = traerPagina(nuevoSegmento,111);
   
  printf("la pagina 111 tiene los valores :\n");
  printf("flag: %d\n",paginaTest->flag);
  printf("nropagina: %d\n",paginaTest->nroPagina);
  printf("registro key : %d\n",paginaTest->reg->key);
  printf("registro value : %s\n",paginaTest->reg->value);
  printf("registro timestamp : %d\n", (int) paginaTest->reg->timeStamp);
  printf("*****************************\n");
  paginaTest = traerPagina(nuevoSegmento,123);
  
  printf("la pagina 123 tiene los valores :\n");
  printf("flag: %d\n",paginaTest->flag);
  printf("nropagina: %d\n",paginaTest->nroPagina);
  printf("registro key : %d\n",paginaTest->reg->key);
  printf("registro value : %s\n",paginaTest->reg->value);
  printf("registro timestamp : %d\n", (int) paginaTest->reg->timeStamp);
  printf("*****************************\n");
  printf("*****************************\n");
  printf("*****************************\n");
  printf("*****************************\n");
  printf("*****************************\n");



*/

  
}

///////////////////////////////////////////////////////////////////////////////////////////