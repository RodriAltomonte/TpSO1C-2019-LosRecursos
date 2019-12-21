#include "memoria.h"

///////////////////////////////////////////////////////////////////////////////////////////

tSegmento* traerSegmento(char* nombreTabla) {

  pthread_mutex_lock(&mutex_pathABuscar);
	
  pathABuscar = string_new();
  string_append(&pathABuscar, nombreTabla);
  tSegmento *segmento = (tSegmento*) list_find(TABLA_SEGMENTOS, &existePath);

  pthread_mutex_unlock(&mutex_pathABuscar);

  return segmento;

}

///////////////////////////////////////////////////////////////////////////////////////////

bool existePath(void* segmento) {

  tSegmento* s = (tSegmento*) segmento;
  bool existe = false;
  
  if (strcmp(s->path, pathABuscar) == 0) {
    existe = true;
  }

  return existe;

}

///////////////////////////////////////////////////////////////////////////////////////////

tPagina* traerPagina(tSegmento* segmento, uint16_t key) {
 
  pthread_mutex_lock(&mutex_keyABuscar);
 
  keyABuscar = key;
  tPagina* pagina = (tPagina*) list_find(segmento->paginas, &existeKey);
  
  pthread_mutex_unlock(&mutex_keyABuscar);

  pagina = llenarRegistro(pagina);
 
  return pagina;

}

///////////////////////////////////////////////////////////////////////////////////////////

bool existeKey(void *pagina) {

  tPagina *p = (tPagina*) pagina;
  bool existe = false;

  int desplazamiento = p->nroPagina * TAM_MAX_REGISTRO;
  uint16_t keyDePagina;

  memcpy(&keyDePagina, MEMORIA_PRINCIPAL+desplazamiento, sizeof(uint16_t));//primera posicion es la key
  

  if (keyDePagina == keyABuscar) {
    existe = true;
  }


  return existe;

} 


///////////////////////////////////////////////////////////////////////////////////////////

void almacenarValor(uint16_t clave, char *valor,uint32_t unTimeStamp, tSegmento* unSegmento) {

  bool almacenado = false;

  while (!almacenado) {
    

    if(!list_is_empty(TABLA_MARCOS_LIBRES)) {

      tPagina* pagina = list_remove(TABLA_MARCOS_LIBRES,0);

      pagina->reg->key = clave;
      pagina->reg->value = string_new();
      string_append(&pagina->reg->value,valor);
      pagina->reg->timestamp = unTimeStamp; 
      if(unTimeStamp == 0){
      pagina->reg->timestamp = time(NULL);
      }
      pagina->flag = 1;
      actualizarFrame(pagina);

      list_add(unSegmento->paginas,pagina);

      almacenado = true;


    } else {
      ejecutarAlgoritmoReemplazo();
    }

  }

}

///////////////////////////////////////////////////////////////////////////////////////////

tPagina* pedirPagina(t_list* tablaPaginasLibres) {

  //pedir pagina libre

}

///////////////////////////////////////////////////////////////////////////////////////////

void ejecutarAlgoritmoReemplazo() {//revisar lru si funca bien
//LRU
  int cantSegmentos;
  int cantPaginas;
  uint32_t timeElegido = (uint32_t) time(NULL);
  tSegmento* segmentoElegido = NULL;
  tPagina* paginaElegida = NULL;
  int indexPagina;

 	pthread_mutex_lock(&mutex_LRU);
	

  if(!list_is_empty(TABLA_SEGMENTOS)) {
    tSegmento* unSegmento;
    cantSegmentos = list_size(TABLA_SEGMENTOS);
    
    for(int i= 0; i< cantSegmentos;i++){

      unSegmento = list_get(TABLA_SEGMENTOS,i);

       if(!list_is_empty(unSegmento->paginas)) {

          tPagina* unPag;
          cantPaginas = list_size(unSegmento->paginas);

          for(int j= 0; i< cantPaginas;j++){

            unPag = list_get(unSegmento->paginas,j);

            if(unPag->flag == 0){

              unPag =llenarRegistro(unPag);

              if(unPag->reg->timestamp <= timeElegido){

                timeElegido = unPag->reg->timestamp;
                segmentoElegido = unSegmento;
                indexPagina = j;

              }

            }
          }
       }

    }

    if(segmentoElegido != NULL){

      paginaElegida = list_remove(segmentoElegido->paginas,indexPagina);
      list_add_in_index(TABLA_MARCOS_LIBRES,paginaElegida->nroPagina,paginaElegida);

    }else{
      memJournal();
    }



  }

  pthread_mutex_unlock(&mutex_LRU);

}

///////////////////////////////////////////////////////////////////////////////////////////

void actualizarFrame(tPagina* unaPagina){ 
  
  //en la memoria se guarda key, tamaño value,timest,value sin \0

  int desplazamiento = unaPagina->nroPagina * TAM_MAX_REGISTRO;
  uint16_t tamValue = 0;
  registro_t* unReg = unaPagina->reg;
  if(!string_is_empty(unReg->value))
		tamValue = (uint16_t)string_length(unReg->value);
  
  if(tamValue > TAM_MAX_VALUE){
    log_error(logger, "- Error! el tamanio del value es muy grande.");
   exit(EXIT_FAILURE);
  }


  memcpy(MEMORIA_PRINCIPAL + desplazamiento, &unReg->key, sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);
  memcpy(MEMORIA_PRINCIPAL + desplazamiento, &tamValue, sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);
  memcpy(MEMORIA_PRINCIPAL + desplazamiento, &unReg->timestamp, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);  
  memcpy(MEMORIA_PRINCIPAL + desplazamiento, unReg->value, sizeof(char)* tamValue);
	desplazamiento += sizeof(char)* tamValue;  
    
}

///////////////////////////////////////////////////////////////////////////////////////////

//LLENA EL REGISTRO CON LOS VALORES DEL FRAME
tPagina* llenarRegistro(tPagina* pagina) {
  
  uint16_t tam;
  int desplazamiento;
  
   
  if(pagina != NULL){

    //pagina->reg = malloc(sizeof(registro_t));  NO VA YA CREA LAS PAGINAS CON MALLOC DE REGISTRO EN inicializarMemoria();

    desplazamiento = pagina->nroPagina * TAM_MAX_REGISTRO;
      
    memcpy(&pagina->reg->key, MEMORIA_PRINCIPAL+desplazamiento, sizeof(uint16_t));
    desplazamiento += sizeof(uint16_t);
    memcpy(&tam, MEMORIA_PRINCIPAL+desplazamiento, sizeof(uint16_t));
    desplazamiento += sizeof(uint16_t);
    memcpy(&pagina->reg->timestamp, MEMORIA_PRINCIPAL+desplazamiento, sizeof(uint32_t));
	  desplazamiento += sizeof(uint32_t);

    

    if(tam != 0){
      
      char * buff = malloc(sizeof(char)*(tam+1));
      memcpy(buff, MEMORIA_PRINCIPAL+desplazamiento, sizeof(char)* tam);
			buff[tam] = '\0';
			pagina->reg->value = string_new();
			string_append(&(pagina->reg->value), buff);
			free(buff);

    }

    

  }

  return pagina;

}

///////////////////////////////////////////////////////////////////////////////////////////

tSegmento* crearSegmento(char *nombreTabla){

  tSegmento* nuevoSeg = malloc(sizeof(tSegmento));
  nuevoSeg->path = string_new();
  string_append(&nuevoSeg->path,nombreTabla);
  nuevoSeg->paginas= list_create();
  list_add(TABLA_SEGMENTOS, nuevoSeg);

  return nuevoSeg;
}

///////////////////////////////////////////////////////////////////////////////////////////

void eliminarSegmentoDeMemoria(tSegmento* segmento){

  liberarPaginas(segmento->paginas);

  pthread_mutex_lock(&mutex_pathABuscar);
	pathABuscar = string_new();
  string_append(&pathABuscar,segmento->path);
	list_remove_by_condition(TABLA_SEGMENTOS,&existePath);
	pthread_mutex_unlock(&mutex_pathABuscar);

  
}

///////////////////////////////////////////////////////////////////////////////////////////

void liberarPaginas(t_list* paginas){

  if(paginas != NULL){

    while(!list_is_empty(paginas)){

     tPagina* unaPag =  list_get(paginas, 0);
     unaPag->flag=0;
     list_add(TABLA_MARCOS_LIBRES, unaPag);
     list_remove(paginas,0);

     }

   list_destroy(paginas);
  }

}

///////////////////////////////////////////////////////////////////////////////////////////

/* PENDIENTE!*/
void * gossiping(void *arg) {

    while(true) {
        //codigo..
    pthread_mutex_lock(&mutex_Gossip);
    
        log_info(logger, "- EJECUTANDO GOSSIPING....");
        
/*
        tengo que recorrer lista gossiping menos a mi 
        me trato de conectar si falla pongo flag en o si no flag en uno y pido su tabla gossiping con flag en 1 
        me manda cuantos tiene
        si no los tengo en tabla gossiping los agrego
        le mando mi tabla gossip sin los que me mando y con flag en 1

*/

        int cantSeed = list_size(TABLA_GOSSIPING);
        int socketSeed;

        int* tamanioMensaje = malloc(sizeof(int));
        int* tipoMensaje = malloc(sizeof(int));
        int resultado;
        int tipoResultado = 0;

        t_list* unaNuevaTablaGossip;

        for(int i = 1; i< cantSeed;i++){

            tSeed* unSeed = list_get(TABLA_GOSSIPING,i);

            if(unSeed->socket == -1){
              unSeed->socket = cliente(unSeed->ip, unSeed->puerto, 2, logger);//2 es memoria
              if(unSeed->socket == 0){
                unSeed->socket = -1;
                unSeed->flag = 0;
                
              }
            }
           printf("**SE CONECTA A MEMORIA CON SOCKET %d", unSeed->socket);
            socketSeed = unSeed->socket;
            


            if(socketSeed == -1){
              unSeed->flag = 0;
            }else{

              unSeed->flag = 1;
              

              t_list* unaTablaGossipAEnviar = seedsRunning();
              enviarInt(socketSeed,2);
              enviarPaquete(socketSeed, tPoolMemorias, unaTablaGossipAEnviar, tamanioMensaje, logger);

              list_destroy_and_destroy_elements(unaTablaGossipAEnviar,eliminarSeedMemoria);

                if((resultado = recibirInt(socketSeed,&tipoResultado)) > 0){

                 if(tipoResultado == 44){

                    log_info(logger,"Tabla recibida con éxito en función gossipping");
                    printf("\nTABLA RECIBIDA CON EXITO EN GOSSIPPING\n");

                    void * buffer = recibirPaquete(socketSeed, tipoMensaje, tamanioMensaje, logger);

                    t_list* unaNuevaTablaGossip = (t_list*) buffer;

                    ActualizarTablaGossiping(unaNuevaTablaGossip);

                     unSeed->socket = -1;
                    log_info(logger,"Actualizamos nuestro pool de memorias");

                  }else if(tipoResultado == 45){

                    unSeed->socket = -1;
                    log_info(logger,"No se pudo realizar envio de gossiping");

                  }
                }                                   

              // close(socketSeed);
              
            }


        }

      
        PROCESO_CONFIG = cargarConfiguracion(tconfig);
        usleep(PROCESO_CONFIG->retardoGossiping * 1000);
        pthread_mutex_unlock(&mutex_Gossip);
    }

}

///////////////////////////////////////////////////////////////////////////////////////////

void * ejecutarJournaling(void *arg) {

    while(true) {
        
        log_info(logger, "- EJECUTANDO JOURNAL....");
        memJournal();
        PROCESO_CONFIG = cargarConfiguracion(tconfig);
        usleep(PROCESO_CONFIG->retardoJournal * 1000);
        
    }

}

///////////////////////////////////////////////////////////////////////////////////////////

void AdministradorDeConexiones(void* infoAdmin){

 infoAdminConexiones_t* unaInfoAdmin = (infoAdminConexiones_t*) infoAdmin;

    int idCliente = 0;
    int resultado;

    while((resultado = recibirInt(unaInfoAdmin->socketCliente,&idCliente)) > 0){

        printf("\nMensaje recibido! Se me conectó un %s. Procedo a responderle...", devuelveNombreProceso(idCliente));

        if(idCliente != -1){

          if(idCliente == 2){
          
          manejarRespuestaAMemoria(unaInfoAdmin->socketCliente,idCliente,unaInfoAdmin->log);
        
          }else if(idCliente == 3){

          manejarRespuestaAKernel(unaInfoAdmin->socketCliente,idCliente,unaInfoAdmin->log);

        }   
      }
    }

    if(resultado == 0){
        printf("\nCliente desconectado");
        fflush(stdout);
        //close(unaInfoAdmin->socketCliente);
       
    }else if(resultado < 0){
       
        printf("\nError al recibir");
        //close(unaInfoAdmin->socketCliente);
       
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void manejarRespuestaAKernel(int socket, int idSocket, t_log* log){

	int* tipoMensaje = malloc(sizeof(int));
	int* tamanioMensaje = malloc(sizeof(int));

	void* buffer = recibirPaquete(socket, tipoMensaje, tamanioMensaje, log);

	switch(*tipoMensaje){

		case tSelect:{

      pthread_mutex_lock(&mutex_memSelect);

      t_select* unSelect = (t_select*) buffer;

      log_info(logger,"EJECUTO: SELECT de Kernel");

      char* tabla = string_new();
      string_append(&tabla, unSelect->tabla);
      uint16_t clave = unSelect->key;

      tSegmento* segmento = traerSegmento(tabla);

      char* valor = string_new();

      if(segmento){

        tPagina* pagina = traerPagina(segmento, clave);

        if(pagina){    

          string_append(&valor, pagina->reg->value);

          log_info(logger, "Valor de registro pedido por Kernel encontrado: %s", valor);

          registro_t* nuevoRegistro = malloc(sizeof(registro_t));

          nuevoRegistro->value = string_new();
          string_append(&nuevoRegistro->value, valor);

          nuevoRegistro->key = unSelect->key;

          nuevoRegistro->timestamp = pagina->reg->timestamp;

          int tamanioRegistro = 0;

          enviarInt(socket, 7);
          enviarPaquete(socket, tRegistro, nuevoRegistro, tamanioRegistro, log);

          log_info(logger, "Encontramos el registro en memoria. Registro enviado a Kernel.");

        }else{

          registro_t* nuevoRegistro = pedirRegistroAlServidor(SOCKET_LFS, tabla, clave, 2);

          if(nuevoRegistro != NULL){

            almacenarValor(clave, nuevoRegistro->value, nuevoRegistro->timestamp, segmento);
            string_append(&valor, nuevoRegistro->value);

            log_info(logger, "No existe la tabla en memoria. Traemos el value del FS. VALUE: %s", nuevoRegistro->value);

            int tamanioRegistro = 0;

            enviarInt(socket, 7);
            enviarPaquete(socket, tRegistro, nuevoRegistro, tamanioRegistro, log);
            log_info(logger, "Registro enviado a Kernel");

            free(nuevoRegistro);

          }else{

            enviarInt(socket, 8); //SIGNIFICA SELECT INVALIDO
            log_info(logger, "No existe el registro en memoria ni en el File System.");

          }        
        }

      }else{

        registro_t* nuevoRegistro = pedirRegistroAlServidor(SOCKET_LFS, tabla, clave, 2);

        if(nuevoRegistro != NULL){

          crearSegmento(tabla);
          segmento = traerSegmento(tabla); //CONSULTAR A RODRI SI ESTA BIEN!!

          almacenarValor(clave, nuevoRegistro->value, nuevoRegistro->timestamp, segmento);
          string_append(&valor, nuevoRegistro->value);
          log_info(logger, "No existe la tabla en memoria. Traemos el value del FS. VALUE: %s", nuevoRegistro->value);

          int tamanioRegistro = 0;

          enviarInt(socket, 7);
          enviarPaquete(socket, tRegistro, nuevoRegistro, tamanioRegistro, log);
          log_info(logger, "Registro enviado a Kernel");

          free(nuevoRegistro);

        }else{

          enviarInt(socket, 8); //SIGNIFICA SELECT INVALIDO
          log_info(logger, "No existe el registro en memoria ni en el File System.");

        }
      }

      pthread_mutex_unlock(&mutex_memSelect);
			break;

    }

			case tInsert: {
        pthread_mutex_lock(&mutex_memInsert);

        log_info(logger,"COMANDO A EJECUTAR: INSERT");

        t_insert* unInsert = (t_insert*) buffer;

        char* tablaActual = string_new();

        string_append(&tablaActual,unInsert->tabla); 

        uint32_t timeStampActual = unInsert->timestamp;

        uint16_t claveActual = unInsert->key;

        char* valor =string_new();
        string_append(&valor,unInsert->valor);
    
        tSegmento *segmentoActual = traerSegmento(tablaActual);
        tPagina *paginaActual = NULL; 
  
        if(segmentoActual){

          paginaActual = traerPagina(segmentoActual, claveActual);
    
          if(paginaActual){

            paginaActual->reg->value  = string_new();
            string_append(&paginaActual->reg->value,valor);
            paginaActual->reg->timestamp = timeStampActual;
            paginaActual->flag =1;
            actualizarFrame(paginaActual);
      
          }else{
            almacenarValor(claveActual, valor, timeStampActual, segmentoActual);
          }

        }else{
          tSegmento *nuevoSegmento = crearSegmento(tablaActual);
          almacenarValor(claveActual, valor, timeStampActual, nuevoSegmento);
        }

        log_info(logger, "-TABLA %s, KEY %d, actualizada correctamente!", tablaActual, claveActual);
        enviarInt(socket, 22); //SIGNIFICA TABLA ACTUALIZADA CORRECTAMENTE
        pthread_mutex_unlock(&mutex_memInsert);
        break;
      }

			case tCreate:{

        pthread_mutex_lock(&mutex_memCreate);

        log_info(logger,"COMANDO A EJECUTAR: CREATE");

        t_create* unCreate = (t_create*) buffer;

        int tamanioCreateCreado = 0;

        enviarInt(SOCKET_LFS, 2);
        enviarPaquete(SOCKET_LFS, tCreate, unCreate, tamanioCreateCreado, logger);

        int resultado;
        int tipoResultado = 0;

        printf("\nTABLA A CREAR ENVIADA POR KERNEL: %s", unCreate->tabla);
        if((resultado = recibirInt(SOCKET_LFS,&tipoResultado)) > 0){
          printf("\n\n\nResultado del recive %d\n\n\n", tipoResultado);
          if(tipoResultado == 13){
            
            enviarInt(socket,13);
            printf("\nLA TABLA SE CREÓ CORRECTAMENTE Y LE MANDÉ '13'\n");
            log_info(logger,"Tabla creada con éxito en FS: %s",unCreate->tabla);

          }else if(tipoResultado == 14){

            enviarInt(socket,14);
            printf("\nLA TABLA NO PUDO CREARSE Y LE MANDÉ '14'\n");
            log_info(logger,"No se pudo crear la tabla en FS: %d",unCreate->tabla);

          }
        }

        pthread_mutex_unlock(&mutex_memCreate);
        break;
      }

			case tDescribe: {

        pthread_mutex_lock(&mutex_memDescribe);

        t_describe* unDesc = (t_describe*) buffer;

        log_info(logger,"COMANDO A EJECUTAR: DESCRIBE");

        int tamanioDescribeCreado = 0;

        t_list* listaMetadatas = pedirMetadatasAlServidor(SOCKET_LFS, unDesc, 2);

        log_info(logger,"Describe mandado con éxito al FS");

        if(listaMetadatas != NULL){
          int tamanioDescribe = 0;
          enviarInt(socket, 15);
          enviarPaquete(socket, tMetadata, listaMetadatas, tamanioDescribe, log);
          log_info(logger,"Metadatas mandadas con éxito al Kernel");
        }else{
          enviarInt(socket, 16);
          log_info(logger,"La/s metadata/s pedida/s no existen. Le avisamos el Kernel.");
        }

	      pthread_mutex_unlock(&mutex_memDescribe);
        break;
      }

			case tDrop: {

        pthread_mutex_lock(&mutex_memDrop);
  
        log_info(logger,"COMANDO A EJECUTAR: DROP");

        t_drop* unDrop = (t_drop*) buffer;

	      char* tablaActual = string_new();
        string_append(&tablaActual,unDrop->tabla); 
        tSegmento *segmentoActual = traerSegmento(tablaActual);
  
        if(segmentoActual){
          eliminarSegmentoDeMemoria(segmentoActual);
        }

        int tamanioDropCreado = 0;

        enviarInt(SOCKET_LFS,2);
        enviarPaquete(SOCKET_LFS, tDrop, unDrop, tamanioDropCreado, logger);

        int resultado;
        int tipoResultado = 0;

        if((resultado = recibirInt(SOCKET_LFS,&tipoResultado)) > 0){

          if(tipoResultado == 11){

            enviarInt(socket, 11);
            log_info(logger,"Tabla borrada con éxito en FS. Le avisamos al Kernel.");

          }else if(tipoResultado == 12){

            enviarInt(socket, 12);
            log_info(logger,"No se pudo borrar la tabla en FS. Le avisamos al Kernel.");

          }
        }

        pthread_mutex_unlock(&mutex_memDrop);
			  break;

      }

      case tJournal: {

        printf("\nME ENTRÓ UN JOURNAL DE KERNELELELEL");
        memJournal();
        enviarInt(socket, 27); //SIGNIFICA JOURNAL REALIZADO CON EXITO
        log_info(logger,"Journal realizado con éxito. Le avisamos al Kernel.");
        break;
        
      }

      case tAdministrativo: {

        t_administrativo* unAdministrativo = (t_administrativo*) buffer;

        int tamanioAdministrativo = 0;

        unAdministrativo->valor = TAM_MAX_VALUE;

        log_info(log,"Devolvemos a kernel el tamaño max value: %d",TAM_MAX_VALUE);
        enviarInt(socket, 17);
        enviarPaquete(socket, tAdministrativo, unAdministrativo, tamanioAdministrativo, logger);

   			break;

        }

      case tPedidoMemorias: {

        t_administrativo* unPedidoMemorias = (t_administrativo*) buffer;

        int tamanioPoolMemorias = 0;

        enviarInt(socket, 30);

        t_list* listaMemoriasActivas = seedsRunning();

        enviarPaquete(socket, tPoolMemorias, listaMemoriasActivas, tamanioPoolMemorias, logger);
        log_info(logger,"Se envió el pool de memorias a Kernel");

        list_destroy_and_destroy_elements(listaMemoriasActivas,eliminarSeedMemoria);

        break;
      }

			default:

				log_error(logger,"Recibimos algo de Kernel que no sabemos manejar.");
				abort();
				break;
			
    }

  free(tipoMensaje);
  free(tamanioMensaje);
	free(buffer);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void manejarRespuestaAMemoria(int socket, int idSocket, t_log* log){

	int* tipoMensaje = malloc(sizeof(int));
	int* tamanioMensaje = malloc(sizeof(int));
  int tamanioGoss= 0;

	void* buffer = recibirPaquete(socket, tipoMensaje, tamanioMensaje, log);

		switch(*tipoMensaje){

			case tPoolMemorias: {

        t_list* listaMemoriasRecibida = (t_list*) buffer;
        log_info(logger,"Solicitud de Gossipping recibida del socket %d", socket);
  

        t_list* listaAEnviar = list_create();
        listaAEnviar = seedsRunning();
        enviarInt(socket, 44);
        enviarPaquete(socket, tPoolMemorias, listaAEnviar, tamanioGoss, logger);
        ActualizarTablaGossiping(listaMemoriasRecibida);

        list_destroy_and_destroy_elements(listaAEnviar,eliminarSeedMemoria);
        
        close(socket);
        pthread_cancel(pthread_self());
       
				break;
			
      } 

			default:
				log_error(log,"Recibimos algo de Memoria que no sabemos manejar.");
				abort();
				break;
			
	}

  free(tipoMensaje);
  free(tamanioMensaje);
	free(buffer);
}

///////////////////////////////////////////////////////////////////////////////////////////


t_list* seedsRunning(){

  t_list*  listGossip = list_create();
  int count = list_size(TABLA_GOSSIPING);

  for(int i =0; i< count;i++){
    tSeed* unSeed = list_get(TABLA_GOSSIPING,i);
    //print
      printf("\nunSeed->id = %d",unSeed->id);
      printf("\nunSeed->ip = %s", unSeed->ip);
      printf("\nunSeed->puerto = %d",unSeed->puerto);
      printf("\nunSeed->Socket = %d",unSeed->socket);
      printf("\nunSeed->Flag = %d",unSeed->flag);
      printf("***************\n");

    if(unSeed->flag == 1 && unSeed->id != -1 ){
      t_gossip* unGossip = malloc(sizeof(t_gossip));
      unGossip->id = unSeed->id;
      unGossip->ip = string_new();
      string_append(&unGossip->ip,unSeed->ip);
      unGossip->puerto = unSeed->puerto;

      list_add(listGossip,unGossip);


    }
  }

  return listGossip;
}

///////////////////////////////////////////////////////////////////////////////////////////

void ActualizarTablaGossiping(t_list* unaTabla){

 while(!list_is_empty(unaTabla)){

    t_gossip* unGossip = (t_gossip*) list_remove(unaTabla,0);

    pthread_mutex_lock(&mutex_ipMemoriaABuscar);	
    pthread_mutex_lock(&mutex_puertoMemoriaABuscar);
	
    ipMemoriaABuscar = string_new();
    string_append(&ipMemoriaABuscar,unGossip->ip);
    puertoMemoriaABuscar = unGossip->puerto;

    tSeed *unSeed = (tSeed*) list_find(TABLA_GOSSIPING, &existeIdMemoria);

    pthread_mutex_unlock(&mutex_ipMemoriaABuscar);
    pthread_mutex_unlock(&mutex_puertoMemoriaABuscar);

    if(unSeed != NULL){
      unSeed->flag = 1;
      unSeed->id = unGossip->id;
      
    }else{
      unSeed = malloc(sizeof(tSeed));
      unSeed->flag =1;
      unSeed->id = unGossip->id;
      unSeed->puerto = unGossip->puerto;
      unSeed->ip = string_new();
      string_append(&unSeed->ip,unGossip->ip);
      unSeed->socket = -1;
      list_add(TABLA_GOSSIPING,unSeed);
    }

    free(unGossip);

  }

}


///////////////////////////////////////////////////////////////////////////////////////////

bool existeIdMemoria(void *unSeed) {

  tSeed *p = (tSeed*) unSeed;

  bool existe = false;

  if ((p->puerto == puertoMemoriaABuscar) && (!strcmp(p->ip,ipMemoriaABuscar))){
      existe = true;
  }


  return existe;

} 

///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void eliminarSeedMemoria(t_gossip* unGoss){
    free(unGoss->ip);
    free(unGoss);
}

///////////////////////////////////////////////////////////////////////////////////////////


/*

bool estaLibre(void *unaPagina) {

  pagina *p = (pagina*) unaPagina;
  bool libre = false;

  if (p->reg->value == NULL) {
    libre = true;
  }

  return libre;

}

void ejecutarAlgoritmoReemplazo(segmento *unSegmento) {
  //toma la página (con flag en 0) cuyo registro tenga el timeStamp menos reciente y pone su valor en NULL 
  time_t minTimeStamp = time(NULL) * 0.001;
  pagina *victima = NULL;

  if (!estaFull(unSegmento)) {

  //recorrer lPaginas, chequear el timeStamp y elegir la víctima

    victima->reg->value = NULL;
  
  } else {
    ejecutarJournaling();
    ejecutarAlgoritmoReemplazo(unSegmento);
  }

}

bool estaFull(segmento *unSegmento) {
  
  bool full = false;
  pagina *paginaActual = (pagina*) list_find(unSegmento->lPaginas, &esConsistenteConFS);
  
  if(paginaActual == NULL) {
    full = true;
  }

  return full;

}

bool esConsistenteConFS(void *unaPagina) {

  pagina *p = (pagina*) unaPagina;
  bool consistente = false;

  if (p->flag == 0) {
    consistente = true;
  }

  return consistente;

}

segmento *crearSegmento(char *nombreTabla) {
  //crea un segmento nuevo dentro de la memoria, para luego almacenar un valor.
}

void ejecutarJournaling() {
  /*
  Este proceso consiste en encontrar todas aquellas páginas cuyas Key deben ser actualizadas dentro del FS 
    y enviar por cada una de estas una petición de insert al FileSystem indicando los datos adecuados.
  Una vez efectuados estos envíos se procederá a eliminar los segmentos actuales.
  */


