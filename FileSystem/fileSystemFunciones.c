#include "fileSystem.h"

void configurarLoggerFS() {

	logger = log_create("logs/fileSystem.log", "FileSystem", false, LOG_LEVEL_TRACE);

}

///////////////////////////////////////////////////////////////////////////////////////////

fileSystemConfig *cargarConfiguracionFS(t_config* configArchivoConfiguracion, t_config* configMetadata) {

	fileSystemConfig* config = malloc(sizeof(fileSystemConfig));
	configArchivoConfiguracion = config_create(LFS_CONFIG_PATH);
	    
	if (configArchivoConfiguracion == NULL) {
        log_info(logger, "=============================================================================================");
	    log_error(logger,"- NO SE PUDO IMPORTAR LA CONFIGURACION");
	    exit(1);
	}
    log_info(logger, "=======================================================================================");
	log_info(logger, "- CONFIGURACION Y METADATA IMPORTADA");

	config->puertoEscucha = config_get_int_value(configArchivoConfiguracion, PUERTO_LFS);
	config->puntoMontaje = config_get_string_value(configArchivoConfiguracion, PUNTO_MONTAJE);
	config->retardo = config_get_int_value(configArchivoConfiguracion, RETARDO);
	config->tamanioValue = config_get_int_value(configArchivoConfiguracion, TAMANIO_VALUE);
	config->tiempoDump = config_get_int_value(configArchivoConfiguracion, TIEMPO_DUMP);

    char* pathMetadata = string_new();
    string_append_with_format(&pathMetadata,"%sMetadata/Metadata.bin", config->puntoMontaje);

    configMetadata = config_create(pathMetadata);

    if(configMetadata == NULL) {
	    log_error(logger,"- NO SE PUDO IMPORTAR LA METADATA");
	    exit(1);
    }

    config->magic_number = config_get_string_value(configMetadata, MAGIC_NUMBER);
    config->cantidadDeBloques = config_get_int_value(configMetadata, BLOCKS);
    config->tamanioBloques = config_get_int_value(configMetadata, BLOCK_SIZE);

	log_info(logger, "· Puerto escucha = %d", config->puertoEscucha);
	log_info(logger, "· Punto de Montaje = %s", config->puntoMontaje);
	log_info(logger, "· Retardo = %d", config->retardo);
	log_info(logger, "· Tamaño Máximo de un Value = %d", config->tamanioValue);
	log_info(logger, "· Tiempo Dump = %d", config->tiempoDump);

    log_info(logger, "· Magic number = %s", config->magic_number);
	log_info(logger, "· Cantidad de bloques = %d", config->cantidadDeBloques);
	log_info(logger, "· Tamaño de bloques = %d", config->tamanioBloques);

    free(configArchivoConfiguracion);
	free(configMetadata);

    char* pathTablas = string_new();
    string_append_with_format(&pathTablas,"%sTables/", config->puntoMontaje);

    mkdir(pathTablas,0777);

    free(pathMetadata);
    free(pathTablas);

    return config;

}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////LISSANDRA///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

void fsSelect(char **query){

    fsMostrarEstadoBitmap();

    log_info(logger,"COMANDO A EJECUTAR: SELECT");

    sleep(procesoConfig->retardo/1000);

    if (query[1] == NULL) {
        log_warning(logger,"NO SE INGRESÓ NINGUNA TABLA");
    }else if(fsTablaExiste(query[1])){
        log_info(logger,"TABLA A BUSCAR ENCONTRADA: %s", query[1]);
        if(query[2] == NULL){ 
            log_warning(logger,"NO SE INGRESÓ NINGUNA KEY.");
        }else{
            uint16_t key = atoi(query[2]);

            char* tabla = string_new();
            string_append(&tabla,query[1]);

            log_info(logger,"KEY INGRESADA: %s", query[2]);

            if(!fsTablaBloqueada(tabla)){
                registro_t* registro;

                registro = fsEncontrarRegistro(tabla,key);

                if(registro != NULL){
                    log_info(logger,"REGISTRO ENCONTRADO:");
                    log_info(logger,"VALOR: %s",registro->value);
                    log_info(logger,"TIMESTAMP: %d", registro->timestamp);
                    printf("\nREGISTRO ENCONTRADO:");
                    printf("\nVALOR: %s",registro->value);
                    printf("\nTIMESTAMP: %d", registro->timestamp);
                }else{
                    printf("\nREGISTRO NO ENCONTRADO.");    
                    log_warning(logger,"REGISTRO NO ENCONTRADO.");         
                }
            } else {
                printf("\nLA TABLA %s SE ENCUENTRA BLOQUEADA Y NO ES POSIBLE REALIZAR ESTA OPERACION", tabla);    
                log_warning(logger,"TABLA %s BLOQUEDA.", tabla); 
            }

        }
    }
    else{
        log_warning(logger,"TABLA A BUSCAR NO ENCONTRADA: %s", query[1]);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

void fsDrop(char **query){

    log_info(logger,"COMANDO A EJECUTAR: DROP");

    sleep(procesoConfig->retardo/1000);

	if (query[1] == NULL) {
        log_warning(logger,"NO SE INGRESÓ NINGUNA TABLA");
    }else if(fsTablaExiste(query[1])){
        
        log_info(logger,"TABLA A BORRAR: %s", query[1]);
        //verificar si alguien tiene abierta la tabla u otras cosas que haya que tener en cuenta
        //SEMAFORO
        if(!fsTablaBloqueada(query[1])){
            char* pathTabla = string_new();     
            string_append_with_format(&pathTabla,"%sTables/%s",procesoConfig->puntoMontaje, query[1]);

            fsLiberarBloquesDeTabla(query[1]); //mutex dentro de la funcion

            if(fsBorrarTabla(pathTabla) != 0){
                
                pthread_mutex_lock(&mutexMemtable);
                fsBorrarTablaMemtable(query[1]);
                pthread_mutex_unlock(&mutexMemtable);

                pthread_mutex_lock(&mutexListaHilos);
                fsBorrarTablaHilosCompactador(query[1]);
                pthread_mutex_unlock(&mutexListaHilos);
                
                //printf("\nCANTIDAD ELEMENTOS LISTA HILOS COMPACTADOR: %d", list_size(hilosCompactador));
                log_info(logger,"TABLA BORRADA CON EXITO");
                printf("\nTABLA BORRADA CON EXITO");
            }else{
                log_warning(logger,"ERROR AL BORRAR LA TABLA");
            }
        } else {
            printf("\nLA TABLA %s SE ENCUENTRA BLOQUEADA Y NO ES POSIBLE REALIZAR ESTA OPERACION", query[1]);    
            log_warning(logger,"TABLA %s BLOQUEDA.", query[1]); 
        }
    }
    else{
        log_warning(logger,"LA TABLA A BORRAR NO EXISTE EN EL SISTEMA");
    }

}

///////////////////////////////////////////////////////////////////////////////////////////

void fsInsert(char **query){

    log_info(logger,"COMANDO A EJECUTAR: INSERT");

    sleep(procesoConfig->retardo/1000);

            if (query[1] == NULL) {
                log_warning(logger,"NO SE INGRESÓ NINGUNA TABLA");
            }else if(fsTablaExiste(query[1])){
                log_info(logger,"TABLA A INSERTAR ENCONTRADA: %s", query[1]);
                if(query[2] == NULL){
                    log_warning(logger,"NO SE INGRESÓ NINGUNA KEY.");
                }else{
                    uint16_t key = atoi(query[2]);

                    char* tabla = string_new();
                    string_append(&tabla,query[1]);

                        if(query[3] == NULL){

                            log_warning(logger,"NO SE INGRESÓ NINGUN VALOR.");

                        }else{

                            if(!fsTablaBloqueada(tabla)){
                            
                                valorRegistro_t* valorRegistro;

                                valorRegistro = chequearValorEntreComillas(query,procesoConfig->tamanioValue);

                                char* valor = string_new();
                                string_append(&valor,valorRegistro->valor);

                                int proximoParametro = valorRegistro->proximoParametro;

                                free(valorRegistro);

                                if(!(proximoParametro > 0)){

                                    log_warning(logger,"EL VALOR DEBE INGRESARSE ENTRE COMILLAS Y NO DEBE SUPERAR EL MAXIMO DE CARACTERES PERMITIDO.");
                                    printf("\n--->EL VALOR DEBE INGRESARSE ENTRE COMILLAS Y NO DEBE SUPERAR EL MAXIMO DE CARACTERES PERMITIDO.\n");

                                }else if(query[proximoParametro] == NULL){

                                    uint32_t timeStampActual = (uint32_t)time(NULL);

                                    log_info(logger,"CREANDO REGISTRO CON TIMESTAMP DEL SISTEMA: %d", timeStampActual);

                                    if(fsCrearRegistroEnMemtable(tabla,key,valor,timeStampActual)){
                                        log_info(logger,"REGISTRO CREADO CON EXITO, KEY: %s, VALOR: %s", query[2], valor);
                                        printf("\n--->Registro creado con éxito!\n");
                                    }else{
                                        log_error(logger,"ERROR AL INSERTAR EL REGISTRO.");
                                    }

                                }else{

                                    uint32_t timeStamp = (uint32_t) atoi(query[proximoParametro]);

                                    log_info(logger,"CREANDO REGISTRO CON TIMESTAMP DEL INGRESADO: %d", timeStamp);

                                    if(fsCrearRegistroEnMemtable(tabla,key,valor,timeStamp)){
                                        log_info(logger,"REGISTRO CREADO CON EXITO, KEY: %s, VALOR: %s", query[2], valor);
                                    }else{
                                        log_error(logger,"ERROR AL INSERTAR EL REGISTRO.");
                                    }
                                }
                            } else { 
                                printf("\nLA TABLA %s SE ENCUENTRA BLOQUEADA Y NO ES POSIBLE REALIZAR ESTA OPERACION", tabla);    
                                log_warning(logger,"TABLA %s BLOQUEDA.", tabla); 
                            }
                        }
                    }    
            }else{
                log_warning(logger,"TABLA ESPECIFICADA NO ENCONTRADA: %s", query[1]);
            }
}

///////////////////////////////////////////////////////////////////////////////////////////

void fsDescribe(char **query){

    log_info(logger,"COMANDO A EJECUTAR: DESCRIBE");

    sleep(procesoConfig->retardo/1000);

    metadata_t* m = malloc(sizeof(metadata_t));

    if (query[1] == NULL) {
            log_info(logger,"TABLA NO ESPECIFICADA, SE LISTARÁN LA METADATA DE TODAS LAS TABLAS");
            char* pathMetadatas = string_new();
            string_append_with_format(&pathMetadatas,"%sTables",procesoConfig->puntoMontaje);
            fsListarTodasLasMetadatas(pathMetadatas);
            free(pathMetadatas);
    }else if(fsTablaExiste(query[1])){
            log_info(logger,"TABLA ESPECIFICADA: %s", query[1]);
            m = fsObtenerMetadata(query[1]);
            fsListarMetadata(m);
            free(m);
    }else{
            log_warning(logger,"TABLA ESPECIFICADA NO ENCONTRADA: %s", query[1]);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

void fsCreate(char **query){

    log_info(logger,"COMANDO A EJECUTAR: CREATE");

    sleep(procesoConfig->retardo/1000);

    if (query[1] == NULL) {
            log_warning(logger,"NO SE INGRESÓ NINGUNA TABLA");
            }else if(!fsTablaExiste(query[1])){
                log_info(logger,"TABLA INGRESADA NO PREEXISTENTE: %s", query[1]);
                if(query[2] == NULL){
                    log_warning(logger,"NO SE INGRESÓ UN TIPO DE CONSISTENCIA");
                }
                else if (!fsCheckearConsistencia(query[2])){
                    printf("\n-->Error en el tipo de consistencia (debe ser SC, SHC o EC)\n"); //LOG
                    log_warning(logger,"CONSISTENCIA INGRESADA ERRONEA");
                }
                else if(query[3] == NULL){
                    log_warning(logger,"NO SE INGRESÓ UNA CANTIDAD DE PARTICIONES");
                } 
                else if(query[4] == NULL){
                    log_warning(logger,"NO SE INGRESÓ UN TIEMPO DE COMPACTACIÓN");
                }
                else{
                    char* tabla = string_new();
                    string_append(&tabla,query[1]);

                    char* consistencia = string_new();
                    string_append(&consistencia, query[2]);

                    char* particiones = string_new();
                    string_append(&particiones, query[3]);

                    char* tiempoCompactacion = string_new();
                    string_append(&tiempoCompactacion, query[4]);

                    fsCrearTabla(tabla,consistencia,particiones,tiempoCompactacion);
                    printf("\nCANTIDAD ELEMENTOS LISTA HILOS COMPACTADOR: %d", list_size(hilosCompactador));
                    log_info(logger,"TABLA CREADA CORRECTAMENTE: %s",tabla);
                }
            }
            else{
                log_warning(logger,"TABLA A CREAR YA EXISTENTE: %s", query[1]);
            }
}

///////////////////////////////////////////////////////////////////////////////////////////

registro_t* fsEncontrarRegistro(char* tabla,uint16_t keyRecibida){

    pthread_mutex_lock(&mutexMemtable);
    registro_t* registroMemtable = fsBuscarMayorTimestampMemtable(tabla, keyRecibida);
    pthread_mutex_unlock(&mutexMemtable);
    

    registro_t* registroBloques = fsBuscarRegistroEnBloques(tabla, keyRecibida);

    if(registroBloques == NULL && registroMemtable == NULL){
        return NULL;
    } else if(registroBloques == NULL && registroMemtable !=NULL){
        return registroMemtable;
    } else if(registroMemtable == NULL && registroBloques!=NULL){
        return registroBloques;
    } else {
        if(registroMemtable->timestamp > registroBloques->timestamp){
            free(registroBloques);
            return registroMemtable;
        }else{
            free(registroMemtable);
            return registroBloques;
        }
    }    
}

// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// valorRegistro_t* chequearValorEntreComillas(char** query){

//     int cantidadDeElementos = 0;
//     int contador = 3;
//     int posicionProximoParametro = 0;
//     int hayComillaAbierta = 0;
//     int hayComillaCerrada = 0;
//     valorRegistro_t* valorRegistro = malloc(sizeof(valorRegistro_t));

//     valorRegistro->proximoParametro = 0;
//     valorRegistro->valor = string_new();
//     string_append(&valorRegistro->valor,"");

//     while(query[cantidadDeElementos] != NULL){
//         cantidadDeElementos+=1;
//     }

//     for(contador; contador<cantidadDeElementos; contador++){

//         if((string_starts_with(query[contador], "\"")) && (!string_ends_with(query[contador], "\""))){
//             hayComillaAbierta = 1;
//             string_append(&valorRegistro->valor,query[contador]);
//         }else if((hayComillaAbierta == 1) && (hayComillaCerrada == 0) && (!string_ends_with(query[contador], "\""))){
//             string_append_with_format(&valorRegistro->valor," %s",query[contador]);
//         }else if((!string_starts_with(query[contador], "\"")) && (string_ends_with(query[contador], "\""))){
//             hayComillaCerrada = 1;
//             string_append_with_format(&valorRegistro->valor," %s",query[contador]);
//             valorRegistro->proximoParametro = contador + 1;
//         }else if((string_starts_with(query[contador], "\"")) && (string_ends_with(query[contador], "\""))){
//             hayComillaAbierta = 1;
//             hayComillaCerrada = 1;
//             string_append(&valorRegistro->valor,query[contador]);
//             valorRegistro->proximoParametro = contador + 1;
//         }
//     }

//     if(string_length(valorRegistro->valor) > 0){
//         char* aux = string_new();
//         string_append(&aux,string_substring(valorRegistro->valor,1,string_length(valorRegistro->valor)-2));
//         if(string_length(aux) <= procesoConfig->tamanioValue){
//             valorRegistro->valor = string_new();
//             string_append(&valorRegistro->valor, aux);
//             free(aux);
//         }else{
//             free(aux);
//             valorRegistro->valor = string_new();
//             string_append(&valorRegistro->valor,"");
//             valorRegistro->proximoParametro = 0;
//         }
//     }
//     return valorRegistro;
// }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

registro_t* fsBuscarRegistroEnString(char* todosLosRegistros, uint16_t keyRecibida){

    char** registrosEnVector = string_split(todosLosRegistros,"\n");
    char* keyEnString = string_new();
    string_append(&keyEnString,string_itoa(keyRecibida));
    uint32_t unTimestamp = 0;
    registro_t* registro = malloc(sizeof(registro_t));
    registro->timestamp = unTimestamp;

    int size;

    for (size = 0; registrosEnVector[size] != NULL; size++){
        char** unRegistro = string_split(registrosEnVector[size],";");
        if(strcmp(unRegistro[1],keyEnString) == 0 && (uint32_t)atoi(unRegistro[0]) > unTimestamp){
            registro->key = keyRecibida;
            registro->timestamp = (uint32_t)atoi(unRegistro[0]);
            registro->value = string_new();
            string_append(&registro->value, unRegistro[2]);
            unTimestamp = (uint32_t)atoi(unRegistro[0]);
        }
        free(unRegistro);
    }

    free(registrosEnVector);


    if(registro->timestamp == 0){
        free(registro);
        return NULL;
    }else{
        return registro;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////FILE SYSTEM/////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

int fsTablaExiste(char *tabla){

	struct stat infoArchivo;
	char* path = string_new();
    string_append_with_format(&path,"%sTables/%s",procesoConfig->puntoMontaje, tabla);
    
	if(stat(path,&infoArchivo) == 0 && S_ISDIR(infoArchivo.st_mode))
	{
		return 1; 
	}
	return 0; 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsCrearTabla(char* tabla, char* consistencia, char* particiones, char* tiempoCompactacion){

    FILE* f;
    char* formatoConsistencia = string_new();
    char* formatoParticiones = string_new();
    char* formatoTiempoCompactacion = string_new();
    char* pathNuevaTabla = string_new();
    char* pathParticiones = string_new();

    string_append_with_format(&pathNuevaTabla, "%sTables/%s",procesoConfig->puntoMontaje, tabla);

    mkdir(pathNuevaTabla,0777);

    string_append(&pathParticiones,pathNuevaTabla);
    string_append(&pathParticiones,"/");
    string_append(&pathNuevaTabla,"/MetaData");

    f = fopen(pathNuevaTabla,"w");

    if(f == NULL){
        log_error(logger,"NO SE PUDO CREAR EL ARCHIVO METADATA PARA LA TABLA %s", tabla);
        log_error(logger,"PATH: %s",pathNuevaTabla); 
	}else{ 

        string_append(&formatoConsistencia,"CONSISTENCY=");
        string_append(&formatoConsistencia,consistencia);
        string_append(&formatoParticiones,"\nPARTITIONS=");
        string_append(&formatoParticiones,particiones);
        string_append(&formatoTiempoCompactacion,"\nCOMPACTION_TIME=");
        string_append(&formatoTiempoCompactacion,tiempoCompactacion);

        fputs(formatoConsistencia,f);
        fputs(formatoParticiones,f);
        fputs(formatoTiempoCompactacion,f);
        fseek(f, 0, SEEK_SET);
        fclose(f);

        log_info(logger,"ARCHIVO METADATA DE TABLA %s CREADO CORRECTAMENTE", tabla); 

        int particionesEnInt = atoi(particiones);

        for(int i=0;i<particionesEnInt;i++){
            log_info(logger,"CREANDO PARTICION: %d",i);
            char* particionACrear = string_new();
            char* bloques = string_new();

            string_append_with_format(&particionACrear,"%s%d.bin",pathParticiones,i);
            FILE* f = fopen(particionACrear,"wb+");
            fputs("SIZE=0",f);
            pthread_mutex_lock(&mutexBitmap);
            int bloqueDeParticion = fsBuscarBloqueLibreYOcupar();
            pthread_mutex_unlock(&mutexBitmap);
            fsVaciarBloque(bloqueDeParticion);
            string_append_with_format(&bloques,"\nBLOCKS=[%d]",bloqueDeParticion);
            fputs(bloques,f);
            fclose(f);
        }

        infoCompactador_t* unaInfo = malloc(sizeof(infoCompactador_t));

        hiloCompactador_t* unNodoCompactador;

        unaInfo->tabla = string_new();
        string_append(&unaInfo->tabla, tabla);

        unaInfo->metadata.tiempo_entre_compactaciones = (uint32_t)atoi(tiempoCompactacion);
        unaInfo->metadata.num_particiones = particionesEnInt;
        unaInfo->metadata.consistencia = fsTransformarTipoConsistencia(consistencia);

        pthread_t hiloCompactador;

        if(pthread_create(&hiloCompactador, NULL, (void*)fsCompactador, (void*)unaInfo)) {
            log_error(logger,"ERROR CREANDO EL THREAD DE COMPACTADOR");
        }

        unNodoCompactador = crearNodoHiloCompactador(tabla, hiloCompactador);

        pthread_mutex_lock(&mutexListaHilos);
        list_add(hilosCompactador, unNodoCompactador);
        pthread_mutex_unlock(&mutexListaHilos);

    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int fsBorrarTabla(char* pathTabla){

    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX];

    if (path == NULL) {
        log_error(logger,"ERROR DEL PATH");
        return 0;
    }
    dir = opendir(pathTabla);
    if (dir == NULL) {
        log_error(logger,"ERROR EN OPENDIR"); 
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            snprintf(path, (size_t) PATH_MAX, "%s/%s", pathTabla, entry->d_name);
            if (entry->d_type == DT_DIR) {
                fsBorrarTabla(path);
            }
            log_info(logger,"BORRANDO: %s",path);
            remove(path);
        }
    }
    closedir(dir);
    log_info(logger,"BORRANDO: %s",pathTabla);
    rmdir(pathTabla);

    return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int fsCantBloquesParaSize(int size){
    int cantBloques = size / procesoConfig->tamanioBloques;
    int resto = size % procesoConfig->tamanioBloques;

    if(resto > 0){
        cantBloques = cantBloques + 1 ;
    }

    return cantBloques;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int fsEscribirEnBloques(char* registros, int arregloBloques[], int cantBloques){
    
    int totalEscrito = 0;

    for (int i = 0; i<cantBloques-1;i++){
        char* pathBloque = string_new();
        string_append_with_format(&pathBloque,"%sBloques/%d.bin",procesoConfig->puntoMontaje,arregloBloques[i]);
        FILE* f;
        fopen(pathBloque,f);
        char* escrituraPorBloque = string_substring(registros,totalEscrito,procesoConfig->tamanioBloques);
        f = fopen(pathBloque,"wb+");
        fputs(escrituraPorBloque,f);
        fclose(f);
        totalEscrito += string_length(escrituraPorBloque);
        free(pathBloque);
    }
    
    int restante = string_length(registros) - totalEscrito;
    
    char* ultimaPorcionAEscribir = string_substring(registros,totalEscrito,restante);
    char* pathUltimoBloque = string_new();

    string_append_with_format(&pathUltimoBloque,"%sBloques/%d.bin",procesoConfig->puntoMontaje,arregloBloques[cantBloques-1]);
    FILE* g;
    g = fopen(pathUltimoBloque,"wb+");
    fputs(ultimaPorcionAEscribir,g);
    fclose(g);

    free(pathUltimoBloque);
    free(ultimaPorcionAEscribir);
    free(registros);
    
    return 1;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

registro_t* fsBuscarRegistroEnBloques (char* tabla, uint16_t keyRecibida){

    metadata_t *meta;
    meta = fsObtenerMetadata(tabla);
    int particion = fsCalcularParticion(meta, keyRecibida);
    free(meta);
    
    char* todosLosRegistros = fsCrearCharDeRegistrosDeTabla(tabla, particion);
    registro_t* registroMayorTimestamp = fsBuscarRegistroEnString(todosLosRegistros,keyRecibida);
    return registroMayorTimestamp;

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char* fsCrearCharDeRegistrosDeTabla(char* tabla, int particion){

    DIR* dir;
    struct dirent* entry;
    char path[PATH_MAX];

    char* pathTabla = string_new();
    string_append_with_format(&pathTabla,"%sTables/%s",procesoConfig->puntoMontaje,tabla);
    
    char* particionDeKey = string_new();

    if(particion > 0){
        string_append_with_format(&particionDeKey,"%d.bin",particion);
    } else{
        string_append(&particionDeKey,".bin");
    }

    infoBloques_t* infoBloques;
    char* todosLosRegistros = string_new();
    t_config* bloquesConfig;
    int cantBloques = 0;

    dir = opendir(pathTabla);

    if (dir == NULL) {
        log_error(logger,"ERROR EN OPENDIR");
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {

            snprintf(path, (size_t) PATH_MAX, "%s/%s", pathTabla, entry->d_name);
            
            char* archivo = string_new();
            string_append(&archivo, entry->d_name);

            if(string_ends_with(archivo,"tmp") || string_ends_with(archivo,particionDeKey) || string_ends_with(archivo,"tmpc")){

                log_info(logger,"INGRESANDO AL ARCHIVO CON PATH: %s", path);
                
                bloquesConfig = config_create(path);

	            if (bloquesConfig == NULL) {
	                log_error(logger,"ERROR: NO SE PUDO LEER LA INFORMACION DEL ARCHIVO");
	                exit(1);
	            }else{

                    infoBloques = fsCrearEstructuraInfoBloques(bloquesConfig);
                    cantBloques = fsCantBloquesParaSize(infoBloques->size);

                    for(int i = 0; i<cantBloques;i++){
                        char* pathBloque = string_new();
                        char* lineaBloque = string_new();
                        size_t tamLinea = 0;

                        string_append_with_format(&pathBloque,"%sBloques/%s.bin", procesoConfig->puntoMontaje,infoBloques->blocks[i]);
                        FILE* f;
                        f = fopen(pathBloque,"r");
                        
                        while(getline(&lineaBloque,&tamLinea,f)>0){
                            string_append(&todosLosRegistros,lineaBloque);
                        }
                        free(lineaBloque);
                        free(pathBloque);
                    }

                    free(infoBloques);            

                }
            }
            free(archivo);     
        }
    }

    closedir(dir);
    free(particionDeKey);
    free(bloquesConfig);
    return todosLosRegistros;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void crearBloquesFileSystem(){

    char* pathBloques = string_new();
    string_append_with_format(&pathBloques, "%sBloques/",procesoConfig->puntoMontaje);

    if (!mkdir(pathBloques,0777)){
       int i = 0;

        while(i<procesoConfig->cantidadDeBloques){
            FILE* bloque;
            char* pathBloquei = string_new();
            string_append_with_format(&pathBloquei, "%s%d.bin",pathBloques,i);
            bloque = fopen(pathBloquei, "wb+");
            fclose(bloque);
            free(pathBloquei);
            i++;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsVaciarBloque(int bloque){
    char* pathBloque = string_new();
    string_append_with_format(&pathBloque, "%sBloques/%d.bin",procesoConfig->puntoMontaje, bloque);
    FILE* f;
    f = fopen(pathBloque,"wb+");
    if(f == NULL){
        log_error(logger,"NO PUDO ABRIRSE EL BLOQUE %d", bloque);
        log_error(logger,"PATH: %s",pathBloque);
    }else{
        fclose(f);
        log_info(logger,"BLOQUE %d VACIADO", bloque);
    }
    free(pathBloque);
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////METADATA////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////


metadata_t* fsObtenerMetadata(char *tabla){

	metadata_t *m = malloc(sizeof(metadata_t)*1024);
    t_config *metaConfig;
    char* pathMetadata = string_new();

    string_append_with_format(&pathMetadata,"%sTables/%s/MetaData",procesoConfig->puntoMontaje, tabla);

    metaConfig = config_create(pathMetadata);
	if (metaConfig == NULL) {
	    log_error(logger,"ERROR: NO SE PUDO IMPORTAR LA METADATA DE LA TABLA: %s", tabla);
	    exit(1);
	}

    fsLlenarEstructuraMeta(m, metaConfig);

	free(metaConfig);
	return m;
}

///////////////////////////////////////////////////////////////////////////////////////////

int fsCalcularParticion(metadata_t* meta,uint16_t key){

	return key % meta->num_particiones;
}

///////////////////////////////////////////////////////////////////////////////////////////

void fsListarTodasLasMetadatas(char* pathMetadatas){

    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX];

    metadata_t *m = malloc(sizeof(metadata_t));
    t_config *metaConfig = NULL;

    if (path == NULL) {
        log_error(logger,"ERROR DEL PATH");
    }
    dir = opendir(pathMetadatas);
    if (dir == NULL) {
        log_error(logger,"ERROR EN OPENDIR"); 
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {

            snprintf(path, (size_t) PATH_MAX, "%s/%s", pathMetadatas, entry->d_name);

            if (entry->d_type == DT_DIR) {
                log_info(logger,"METADATA DE TABLA %s:", entry->d_name);
                printf("\nNOMBRE TABLA: %s", entry->d_name);                
                fsListarTodasLasMetadatas(path);

            }else if(strcmp(entry->d_name,"MetaData") == 0){

                log_info(logger,"INGRESANDO A METADATA, PATH: %s", path);
                
                metaConfig = config_create(path);

	            if (metaConfig == NULL) {
	                log_error(logger,"ERROR: NO SE PUDO IMPORTAR LA METADATA");
	                exit(1);
	            }

                fsLlenarEstructuraMeta(m, metaConfig);
                fsListarMetadata(m);
                
            }
        }
    }   

    if(metaConfig != NULL){
        config_destroy(metaConfig);
    }

    closedir(dir);
    free(m);
}

///////////////////////////////////////////////////////////////////////////////////////////

void fsLlenarEstructuraMeta(metadata_t* m, t_config* metaConfig){

    char* tipoConsistencia = string_new();
    string_append(&tipoConsistencia, config_get_string_value(metaConfig, "CONSISTENCY"));

 	m->consistencia = fsTransformarTipoConsistencia(tipoConsistencia);
	m->num_particiones = config_get_int_value(metaConfig, "PARTITIONS");
	m->tiempo_entre_compactaciones = config_get_int_value(metaConfig, "COMPACTION_TIME");
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int fsCheckearConsistencia(char* consistenciaQuery){

    if(strcmp(consistenciaQuery,"SC") == 0 || strcmp(consistenciaQuery,"SHC") == 0 || strcmp(consistenciaQuery,"EC") == 0){
        return 1;
    }
    else{
        return 0; 
    } 
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsDameMetadatas(char* pathMetadatas, t_list* unaListaMetadatas, char* tabla){

    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX];

    metadata_t *m = malloc(sizeof(metadata_t));
    t_config *metaConfig = NULL;

    if (path == NULL) {
        log_error(logger,"ERROR DEL PATH");
    }
    dir = opendir(pathMetadatas);
    if (dir == NULL) {
        log_error(logger,"ERROR EN OPENDIR"); 
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {

            snprintf(path, (size_t) PATH_MAX, "%s/%s", pathMetadatas, entry->d_name);

            if (entry->d_type == DT_DIR) {
                tabla = string_new();
                string_append(&tabla, entry->d_name);
                log_info(logger,"METADATA DE TABLA %s:", entry->d_name);
                fsDameMetadatas(path,unaListaMetadatas,tabla);

            }else if(strcmp(entry->d_name,"MetaData") == 0){

                log_info(logger,"INGRESANDO A METADATA, PATH: %s", path);

                metaConfig = config_create(path);

                if (metaConfig == NULL) {
                    log_error(logger,"ERROR: NO SE PUDO IMPORTAR LA METADATA");
                    exit(1);
                }

                fsLlenarEstructuraMeta(m, metaConfig);
                nodoMetadata_t* nodoMeta = fsCrearNodoMetadata(tabla, m);
                list_add(unaListaMetadatas, nodoMeta);
                free(tabla);
            }
        }
    }

    if(metaConfig != NULL){
        config_destroy(metaConfig);
    }

    closedir(dir);
    free(m);
    
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

nodoMetadata_t* fsCrearNodoMetadata (char* tabla, metadata_t* meta){
    nodoMetadata_t* nodo = malloc(sizeof(nodoMetadata_t));
    metadata_t* metita = malloc(sizeof(metadata_t));
    metita->consistencia = meta->consistencia;
    metita->num_particiones = meta->num_particiones;
    metita->tiempo_entre_compactaciones = meta->tiempo_entre_compactaciones;
    nodo->metadata = metita;
    nodo->tabla = string_new();
    string_append(&nodo->tabla, tabla);
    return nodo;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////MEMTABLE////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsBorrarTablaMemtable(char* tabla){
    pthread_mutex_lock(&mutexTablaABuscar);
    tablaABuscar = string_new();
    string_append(&tablaABuscar, tabla);
    list_remove_and_destroy_by_condition(MEMTABLE, (void*)mismoNombre, eliminarTablasMemtable);
    pthread_mutex_unlock(&mutexTablaABuscar);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

tablaMemtable_t* buscarTablaEnMemtable(char* tablaAEvaluar){
    
    if(list_is_empty(MEMTABLE)){
        return NULL;
    }else{
    
	int tamLista = list_size(MEMTABLE);
	int indexTabla = 0;

    tablaMemtable_t* unaTabla;

    for(indexTabla; indexTabla < tamLista; indexTabla++){
		unaTabla = list_get(MEMTABLE, indexTabla);
		if(0 == strcmp(unaTabla->nombreTabla, tablaAEvaluar)){
			return unaTabla;
		}
	}
    return NULL;
    }
}
		
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

registro_t* buscarRegistroEnTabla(t_list* registrosTablaBuscada, uint16_t keyRecibida){

	int tamListaRegistros = list_size(registrosTablaBuscada);
	int indexListaRegistros = 0;

    if(tamListaRegistros > 0){

        registro_t* unRegistro = malloc(sizeof(registro_t));

	    while(indexListaRegistros < tamListaRegistros){

		    unRegistro = list_get(registrosTablaBuscada, indexListaRegistros);

		    if(unRegistro->key == keyRecibida){
			    return unRegistro;
		    }else{
			    indexListaRegistros += 1;
		    }
	    }

        unRegistro = NULL;
        free(unRegistro);
	    return NULL;

    }else{
        return NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

tablaMemtable_t* crearNodoTablaMemtable(char* tablaRecibida){

	tablaMemtable_t* nodoTablaMemtable = malloc(sizeof(tablaMemtable_t));
	nodoTablaMemtable->nombreTabla = string_new();
    string_append(&nodoTablaMemtable->nombreTabla, tablaRecibida);
    nodoTablaMemtable->listaRegistros = list_create();

	return nodoTablaMemtable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

registro_t* crearNodoRegistro(char* valor, uint16_t keyRecibido, uint32_t timestampRecibido){

	registro_t* nodoRegistroMemtable = malloc(sizeof(registro_t));
    nodoRegistroMemtable->value = string_new();
	string_append(&nodoRegistroMemtable->value,valor);
	nodoRegistroMemtable->key = keyRecibido;
	nodoRegistroMemtable->timestamp = timestampRecibido;

	return nodoRegistroMemtable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int fsCrearRegistroEnMemtable(char* tablaRecibida, uint16_t keyRecibido, char* valorRecibido, uint32_t timeStampRecibido){

    pthread_mutex_lock(&mutexMemtable);
    tablaMemtable_t* tablaBuscada = buscarTablaEnMemtable(tablaRecibida);
    pthread_mutex_unlock(&mutexMemtable);
    
    registro_t* nodoInsertado = crearNodoRegistro(valorRecibido,keyRecibido,timeStampRecibido);

    pthread_mutex_lock(&mutexMemtable);
    if(tablaBuscada == NULL){
        tablaMemtable_t* nuevaTabla = crearNodoTablaMemtable(tablaRecibida);
        list_add(MEMTABLE, nuevaTabla);
        list_add(nuevaTabla->listaRegistros,nodoInsertado);
    } else {
        list_add(tablaBuscada->listaRegistros, nodoInsertado);
    }
    pthread_mutex_unlock(&mutexMemtable);

    return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

registro_t* fsBuscarMayorTimestampMemtable(char* tabla, uint16_t key){

    if(!list_is_empty(MEMTABLE)){

        tablaMemtable_t* tablaEncontrada = buscarTablaEnMemtable(tabla);

        if(tablaEncontrada != NULL){
            if(!list_is_empty(tablaEncontrada->listaRegistros)){
                registro_t* registroMemtable = malloc(sizeof(registro_t));
                list_sort(tablaEncontrada->listaRegistros, (void*) mayorTimestamp);
                registroMemtable = buscarRegistroEnTabla(tablaEncontrada->listaRegistros, key);         
                return registroMemtable;
            }else{
                return NULL;
            }
        }else{
            return NULL;
        }   
    }else {
        return NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////MANEJO DE LISTAS//////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool mayorTimestamp(registro_t* menorRegistro, registro_t * mayorRegistro){
    return menorRegistro->timestamp > mayorRegistro->timestamp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool mismoNombre(tablaMemtable_t* unaTabla){
    if(strcmp(unaTabla->nombreTabla, tablaABuscar)!=0){
        return false;
    }else{
        return true;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool mismoNombreCompactador(hiloCompactador_t* unHiloCompactador){
    if(strcmp(unHiloCompactador->tabla, tablaABuscar)!=0){
        return false;
    }else{
        return true;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void eliminarTablasMemtable(tablaMemtable_t* unaTabla) {
    list_destroy_and_destroy_elements(unaTabla->listaRegistros, eliminarRegistrosMemtable);
    free(unaTabla->nombreTabla);
    free(unaTabla);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void eliminarTablasHilosCompactador(hiloCompactador_t* unHiloCompactador){
    free(unHiloCompactador->tabla);
    pthread_cancel(unHiloCompactador->hilo);
    free(unHiloCompactador);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void eliminarListaParticiones(nodoParticion_t* unaParticion) {
    list_destroy_and_destroy_elements(unaParticion->listaRegistros, eliminarRegistrosMemtable);
    free(unaParticion);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void eliminarRegistrosMemtable(registro_t* unRegistro){
    free(unRegistro->value);
    free(unRegistro);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////DUMP///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsDump(){

    int cantidadDeDumps = 0;

    while(1){

        sleep(procesoConfig->tiempoDump / 1000);
        log_info(logger,"INICIO DE PROCESO DUMP");
        pthread_mutex_lock(&mutexMemtable);
        int cantidadDeTablas = list_size(MEMTABLE);
        pthread_mutex_unlock(&mutexMemtable);

        if(cantidadDeTablas > 0){

            for (int contador = 0; contador < cantidadDeTablas; contador++){
                tablaMemtable_t* unaTabla;

                pthread_mutex_lock(&mutexMemtable);
                unaTabla = list_get(MEMTABLE, contador);
                if(!list_is_empty(unaTabla->listaRegistros)){
                    pthread_mutex_unlock(&mutexMemtable);
                    crearArchivoTemporal(unaTabla, cantidadDeDumps);
                }else{
                    pthread_mutex_unlock(&mutexMemtable);
                }
            }
            
            cantidadDeDumps+=1;
        }        
        pthread_mutex_lock(&mutexMemtable);
        list_clean_and_destroy_elements(MEMTABLE, eliminarTablasMemtable);
        pthread_mutex_unlock(&mutexMemtable);

        log_info(logger,"MEMTABLE RESETEADA");
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void crearArchivoTemporal(tablaMemtable_t* unaTabla, int cantidadDeDumps){

    log_info(logger,"CREANDO ARCHIVO TEMPORAL %d, PARA LA TABLA %s",cantidadDeDumps,unaTabla->nombreTabla);
    
    pthread_mutex_lock(&mutexMemtable);
    int tamanioTotalRegistros = list_size(unaTabla->listaRegistros);//para contar 1 \n por registro
    pthread_mutex_unlock(&mutexMemtable);

    int cantRegistros = tamanioTotalRegistros;
    
    char* todosLosRegistros = string_new();
        
    for(int i=0;i<cantRegistros;i++){

        pthread_mutex_lock(&mutexMemtable);
        registro_t* registroLista = list_get(unaTabla->listaRegistros,i);
        pthread_mutex_unlock(&mutexMemtable);

        char* registro = string_new();
        string_append_with_format(&registro,"%d;%d;%s\n",registroLista->timestamp,registroLista->key,registroLista->value);
        string_append_with_format(&todosLosRegistros, registro);
        tamanioTotalRegistros += string_length(registro);
        free (registro);
    }


    char* tamanioDump = string_new();
    string_append_with_format(&tamanioDump,"SIZE=%d\n",tamanioTotalRegistros);

    int cantBloquesAOcupar = fsCantBloquesParaSize(tamanioTotalRegistros);

    char* bloquesDump = string_new();
    string_append(&bloquesDump,"BLOCKS=[");
    
    int bloquesAEscribir[cantBloquesAOcupar];

    if(cantBloquesAOcupar<fsCantidadBloquesLibres()){
        
        for(int j=0; j<cantBloquesAOcupar; j++ ){

           pthread_mutex_lock(&mutexBitmap);
           bloquesAEscribir[j] = fsBuscarBloqueLibreYOcupar();
           pthread_mutex_unlock(&mutexBitmap);

           if (j==(cantBloquesAOcupar-1)){
               string_append_with_format(&bloquesDump,"%d]",bloquesAEscribir[j]);
            }else{
               string_append_with_format(&bloquesDump,"%d,",bloquesAEscribir[j]);
            }
        }
    }else{
        log_error(logger,"NO HAY SUFICIENTES BLOQUES LIBRES PARA REALIZAR EL DUMP");
        return NULL;
    }
    
    FILE* f;
    char* pathTemporal = string_new();
    string_append_with_format(&pathTemporal,"%sTables/%s/%d.tmp",procesoConfig->puntoMontaje,unaTabla->nombreTabla,cantidadDeDumps);

    while(validarArchivo(pathTemporal)){
        cantidadDeDumps++;
        pathTemporal = string_new();
        string_append_with_format(&pathTemporal,"%sTables/%s/%d.tmp",procesoConfig->puntoMontaje,unaTabla->nombreTabla,cantidadDeDumps);
    }
    
    f = fopen(pathTemporal,"wb+");
	if(f == NULL)
	{
        log_error(logger,"NO PUDO ABRIRSE EL ARCHIVO TEMPORAL %d", cantidadDeDumps);
        log_error(logger,"PATH", cantidadDeDumps);
		return NULL;
	}

    if(fputs(tamanioDump,f) != NULL && fputs(bloquesDump,f) != NULL){
        fclose(f);
        log_info(logger,"ARCHIVO TEMPORAL %d DE LA TABLA %s ACTUALIZADO", cantidadDeDumps, unaTabla->nombreTabla);
        fsEscribirEnBloques(todosLosRegistros, bloquesAEscribir, cantBloquesAOcupar);
        return 1;
        
    }else{
        fclose(f);
        log_error(logger,"NO SE PUDO ESCRIBIR EN EL ARCHIVO TEMPORAL %d DE LA TABLA %s", cantidadDeDumps,unaTabla->nombreTabla);
        return NULL;
    }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int hayEspacioParaDump(){
    pthread_mutex_lock(&mutexMemtable);
    int cantidadTablas = list_size(MEMTABLE);
    pthread_mutex_unlock(&mutexMemtable);
    if(cantidadTablas <= fsCantidadBloquesLibres()){
        return 1;
    }else{
        return 0;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int fsObtenerCantidadDeTemporalesACompactar(char* tabla){

    char* pathTabla = string_new();
    string_append_with_format(&pathTabla,"%sTables/%s",procesoConfig->puntoMontaje, tabla);

    metadata_t *m = malloc(sizeof(metadata_t));
    struct dirent * entry;
    DIR * dir;

    int cantTemporales = 0;

    dir = opendir(pathTabla);

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            if(string_ends_with(entry->d_name,"tmpc")){
                cantTemporales++;
            }
        }
    }

    closedir(dir);
    return cantTemporales;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////BITMAP///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void inicializarBitMap(){


    char* pathBitmap = string_new();
    string_append_with_format(&pathBitmap,"%sMetadata/Bitmap.bin",procesoConfig->puntoMontaje);
         
    if (validarArchivo(pathBitmap)){
        log_info(logger,"BITMAP EXISTENTE ENCONTRADO, SE CARGA EN MEMORIA");
        printf("\nEXISTE UN BITMAP PREVIO!");
        verificarBitmapPrevio(pathBitmap);
    } else {
        log_info(logger,"NO EXISTE UN BITMAP PREVIO, CREO UNO NUEVO");
        printf("\nNO EXISTE UN BITMAP PREVIO, CREO UNO NUEVO");
        crearArchivoBitmap(pathBitmap);
        verificarBitmapPrevio(pathBitmap);
        
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void verificarBitmapPrevio(char* pathBitmap){
    
    int tamBitmap = procesoConfig->cantidadDeBloques/8;

	if(procesoConfig->cantidadDeBloques % 8 != 0){
		tamBitmap++;
	}

	int fd = open(pathBitmap, O_RDWR);
	ftruncate(fd, tamBitmap);
    
    struct stat mystat;

    if (fstat(fd, &mystat) < 0) {
		log_error(logger, "ERROR AL ESTABLECER FSTAT PARA BITMAP. SE CREARÁ UNO NUEVO.");
		close(fd);
        exit(-1);
	} else {
        char *bitmap = (char *) mmap(NULL, mystat.st_size, PROT_READ | PROT_WRITE,	MAP_SHARED, fd, 0);
        if (bitmap == MAP_FAILED) {
            log_error(logger, "ERROR AL MAPEAR BITMAP A MEMORIA. SE CREARÁ UNO NUEVO.");
            close(fd);
            //crearArchivoBitmap(pathBitmap);
        } else{
            
            bitarray = bitarray_create_with_mode(bitmap, tamBitmap, LSB_FIRST);
            close(fd);
        }
    }

    //free(pathBitmap);

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void crearArchivoBitmap(char* pathBitmap){
    
    int tamBitarray = procesoConfig->cantidadDeBloques/8;
    int bit = 0;

    if(procesoConfig->cantidadDeBloques % 8 != 0){
        tamBitarray++;
    }

    char* data = malloc(tamBitarray);
    t_bitarray* bitarrayInicial = bitarray_create_with_mode(data,tamBitarray,MSB_FIRST);

    while(bit < procesoConfig->cantidadDeBloques){
        bitarray_clean_bit(bitarrayInicial, bit);
        bit ++;
    }

	FILE *bitmap;
	bitmap = fopen(pathBitmap, "wb+");

	//fseek(bitmap, 0, SEEK_SET);
	fwrite(bitarrayInicial->bitarray, 1, bitarrayInicial->size, bitmap);

    log_info(logger,"ARCHIVO BITMAP CREADO EN: %s", pathBitmap);

    bitarray_destroy(bitarrayInicial);
	fclose(bitmap);
	//free(pathBitmap);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int validarArchivo(char *path) {
	struct stat buffer;
	int status;
	status = stat(path, &buffer);
	if (status < 0) {
		return 0;
	} else {
		return 1;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int fsBuscarBloqueLibreYOcupar(){
    for(int i = 0; i< procesoConfig->cantidadDeBloques; i++){
        if(bitarray_test_bit(bitarray, i)==0){
            bitarray_set_bit(bitarray,i);
            return i;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int fsCantidadBloquesLibres(){

	int posicion = 0;
	int libres = 0;

    pthread_mutex_lock(&mutexMemtable);
	while(posicion < procesoConfig->cantidadDeBloques){
		if(bitarray_test_bit(bitarray, posicion) == 0){
			libres ++;
		}
		posicion ++;
	}
    pthread_mutex_unlock(&mutexMemtable);

	return libres;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsMostrarEstadoBitmap(){
	int posicion = 0;
	bool a;

    pthread_mutex_lock(&mutexMemtable);
    while(posicion < procesoConfig->cantidadDeBloques){
        if((posicion%10) == 0) printf ("\n");
		a = bitarray_test_bit(bitarray,posicion);
		printf("%i", a);
        posicion ++;
	}
    pthread_mutex_unlock(&mutexMemtable);

	printf("\n");
	printf("Cantidad de espacio disponible: %d.",fsCantidadBloquesLibres());

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////COMPACTADOR///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsCompactador(void* infoCompactador){

    infoCompactador_t* infoTabla = (infoCompactador_t*)infoCompactador;
    clock_t inicio, fin;
    double tiempoDeBloqueo;
 

    while(1){

        sleep(infoTabla->metadata.tiempo_entre_compactaciones/1000);

        fsRenombrarTemporales(infoTabla->tabla);
        int cantidadDeTemporales = fsObtenerCantidadDeTemporalesACompactar(infoTabla->tabla);

        if(cantidadDeTemporales > 0){

            char* registrosDeLaTabla = fsCrearCharDeRegistrosDeTabla(infoTabla->tabla,-1);
            char** registrosSeparados = string_split(registrosDeLaTabla, "\n");

            t_list* listaParticiones = fsCrearListaParticiones(infoTabla->metadata.num_particiones);
            int size = 0;

            for (size; registrosSeparados[size] != NULL; size++);

                uint16_t keysEvaluadas[size];
                int tamanioReal = 0;
            
            for (int i = 0; i<size; i++){

                char** registroVector = string_split(registrosSeparados[i],";");
                uint16_t key = (uint16_t) atoi(registroVector[1]);

                if(!keyEstaEnArreglo(key,keysEvaluadas,tamanioReal)){

                    registro_t* registro = fsBuscarRegistroEnString(registrosDeLaTabla, key);
                    int particion = fsCalcularParticion(&infoTabla->metadata, key);

                    nodoParticion_t* nodoParticion = list_get(listaParticiones,particion);
                    list_add(nodoParticion->listaRegistros,registro);
                    keysEvaluadas[tamanioReal] = key;

                    tamanioReal++;
                }
                
                free(registroVector);

            }

            free(registrosSeparados);
            free(registrosDeLaTabla);

            //BLOQUEAR TABLA E INICIAR CONTADOR DE TIEMPO DE BLOQUEO
            pthread_mutex_lock(&mutexListaHilos);
            fsBloquearTabla(infoTabla->tabla);
            pthread_mutex_unlock(&mutexListaHilos);

            inicio = clock();

            fsLiberarBloquesDeTabla(infoTabla->tabla);
            fsEscribirNuevasParticiones(listaParticiones, infoTabla->tabla);
            fsBorrarTemporales(infoTabla->tabla);
            
            //DESBLOQUEAR TABLA Y LOGGEAR EL TIEMPO DE BLOQUEO
            pthread_mutex_lock(&mutexListaHilos);
            fsDesbloquearTabla(infoTabla->tabla);
            pthread_mutex_unlock(&mutexListaHilos);

            fin = clock();
            tiempoDeBloqueo =  (double)(fin - inicio)/(double)CLOCKS_PER_SEC;
            log_info(logger, "LA TABLA %s ESTUVO BLOQUEADA %d SEGUNDOS",infoTabla->tabla,tiempoDeBloqueo);
            log_info(logger, "FIN: %d", (double)fin);
            log_info(logger, "INICIO: %d", (double)inicio);
            
            list_destroy_and_destroy_elements(listaParticiones, eliminarListaParticiones);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

t_list* fsCrearListaParticiones(int particiones){
    t_list* listaParticiones = list_create();

    for(int i=0; i<particiones; i++){
        nodoParticion_t* nodo = malloc(sizeof(nodoParticion_t));
        nodo->particion = i;
        nodo->listaRegistros = list_create();
        list_add(listaParticiones,nodo);
    }
    return listaParticiones;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int keyEstaEnArreglo(uint16_t key, uint16_t arregloKeys[], int tamArreglo){
    
    for(int i = 0; i<tamArreglo; i++){
        if (key == arregloKeys[i]){
            return 1;
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int fsEscribirNuevasParticiones(t_list* listaParticiones, char* tabla){
    
    int size = list_size(listaParticiones);

    for (int i = 0; i<size ; i++){

        char* particionEnString = string_new();
        nodoParticion_t* particion = list_get(listaParticiones,i);

        int cantRegistros = list_size(particion->listaRegistros);

        if(cantRegistros > 0){
            
            for(int j=0; j<cantRegistros; j++){
                registro_t* registro = list_get(particion->listaRegistros, j);
                string_append_with_format(&particionEnString, "%d;%d;%s\n", registro->timestamp, registro->key, registro->value);
            }

            int tamanioString = string_length(particionEnString);

            int cantBloques = fsCantBloquesParaSize(tamanioString);

            char* sizeParticion = string_new();
            string_append_with_format(&sizeParticion,"SIZE=%d\n",tamanioString);

            char* blocksParticion = string_new();
            string_append(&blocksParticion,"BLOCKS=[");
    
            int bloquesAEscribir[cantBloques];

            if(cantBloques<fsCantidadBloquesLibres()){
        
                for(int j=0; j<cantBloques; j++ ){

                    pthread_mutex_lock(&mutexBitmap);
                    bloquesAEscribir[j] = fsBuscarBloqueLibreYOcupar();
                    pthread_mutex_unlock(&mutexBitmap);

           
                    if (j==(cantBloques-1)){
                        string_append_with_format(&blocksParticion,"%d]",bloquesAEscribir[j]);
                    }else{
                        string_append_with_format(&blocksParticion,"%d,",bloquesAEscribir[j]);
                    }
                }
            }else{
                log_error(logger,"NO HAY SUFICIENTES BLOQUES LIBRES PARA REALIZAR LA COMPACTACIÓN DE LA TABLA %s", tabla);
                return 0;
            }

            FILE* f;
            char* pathBin = string_new();
            string_append_with_format(&pathBin,"%sTables/%s/%d.bin",procesoConfig->puntoMontaje,tabla,i);

            f = fopen(pathBin,"wb+");
            if(f == NULL){
                log_error(logger,"NO PUDO ABRIRSE LA PARTICIÓN %d", i);
                log_error(logger,"PATH", i);
            }

            if(fputs(sizeParticion,f) != NULL && fputs(blocksParticion,f) != NULL){
                fclose(f);
                log_info(logger,"PARTICIÓN %d DE LA TABLA %s ACTUALIZADA", i, tabla);
                fsEscribirEnBloques(particionEnString, bloquesAEscribir, cantBloques);
                free(sizeParticion);
                free(blocksParticion);
            }else{
                fclose(f);
                free(sizeParticion);
                free(blocksParticion);
                log_error(logger,"NO SE PUDO ESCRIBIR EN EL ARCHIVO TEMPORAL %d DE LA TABLA %s", i, tabla);
            }

        }else{
            char* sizeNulo = string_new();
            string_append(&sizeNulo,"SIZE=0\n");

            pthread_mutex_lock(&mutexBitmap);
            int bloque = fsBuscarBloqueLibreYOcupar();
            pthread_mutex_unlock(&mutexBitmap);
            printf("\nBLOQUE A OCUPAR POR PARTICION VACIA: %d", bloque);
            
            char* blockUnico = string_new();
            string_append_with_format(&blockUnico,"BLOCKS=[%d]",bloque);

            FILE* f;
            char* pathBinVacio = string_new();
            string_append_with_format(&pathBinVacio,"%sTables/%s/%d.bin",procesoConfig->puntoMontaje,tabla,i);

            f = fopen(pathBinVacio,"wb+");
            if(f == NULL){
                log_error(logger,"NO PUDO ABRIRSE LA PARTICIÓN %d", i);
                log_error(logger,"PATH: %s",pathBinVacio);
            }

            if(fputs(sizeNulo,f) != NULL && fputs(blockUnico,f) != NULL){
                fclose(f);
                log_info(logger,"PARTICIÓN %d DE LA TABLA %s ACTUALIZADA", i, tabla);
                fsVaciarBloque(bloque);
                free(sizeNulo);
                free(blockUnico);
            }else{
                fclose(f);
                free(sizeNulo);
                free(blockUnico);
                log_error(logger,"NO SE PUDO ESCRIBIR EN EL ARCHIVO TEMPORAL %d DE LA TABLA %s", i, tabla);
            }

        }  
    }
    return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsBorrarTemporales(char* tabla){

    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX];

    if (path == NULL) {
        log_error(logger,"ERROR DEL PATH");
        return 0;
    }

    char* pathTabla = string_new();
    string_append_with_format(&pathTabla, "%sTables/%s", procesoConfig->puntoMontaje, tabla);

    dir = opendir(pathTabla);
    if (dir == NULL) {
        log_error(logger,"ERROR EN OPENDIR");
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            snprintf(path, (size_t) PATH_MAX, "%s/%s", pathTabla, entry->d_name);
            if (string_ends_with(entry->d_name, ".tmpc")) {
                remove(path);
                log_info(logger,"BORRANDO: %s",path);
            }
        }
    }
    closedir(dir);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsLiberarBloquesDeTabla(char* tabla){

    DIR* dir;
    struct dirent* entry;
    char path[PATH_MAX];

    char* pathTabla = string_new();
    string_append_with_format(&pathTabla,"%sTables/%s",procesoConfig->puntoMontaje,tabla);

    infoBloques_t* infoBloques;
    t_config* bloquesConfig;
    int cantBloques = 0;

    dir = opendir(pathTabla);

    if (dir == NULL) {
        log_error(logger,"ERROR EN OPENDIR");
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {

            snprintf(path, (size_t) PATH_MAX, "%s/%s", pathTabla, entry->d_name);

            if(string_ends_with(entry->d_name,"tmpc") || string_ends_with(entry->d_name,"bin")){ 

                log_info(logger,"INGRESANDO AL ARCHIVO CON PATH: %s", path);
                
                bloquesConfig = config_create(path);

	            if (bloquesConfig == NULL) {
	                log_error(logger,"ERROR: NO SE PUDO LEER LA INFORMACION DEL ARCHIVO");
	                exit(1);
	            }else{

                    infoBloques = fsCrearEstructuraInfoBloques(bloquesConfig);

                    for (int i = 0; infoBloques->blocks[i] != NULL; i++){
                        int bloqueALiberar = atoi(infoBloques->blocks[i]);
                        pthread_mutex_lock(&mutexBitmap);
                        bitarray_clean_bit(bitarray,bloqueALiberar);
                        pthread_mutex_unlock(&mutexBitmap);

                    }

                    free(infoBloques);         
                }      
            }
        }
    }
    free(pathTabla);
    free(bloquesConfig);
    closedir(dir);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

infoBloques_t* fsCrearEstructuraInfoBloques(t_config* bloquesConfig){

    infoBloques_t* infoBloques = malloc(sizeof(infoBloques_t));

    infoBloques->size = config_get_int_value(bloquesConfig, "SIZE");
	infoBloques->blocks = config_get_array_value(bloquesConfig, "BLOCKS");

    return infoBloques;

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsIniciarCompactacionesTablasExistentes(void* pathTablas){

    char* pathMetadatas = (char*)pathTablas;
    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX];

    char* tabla;

    metadata_t *m = malloc(sizeof(metadata_t));
    t_config *metaConfig = NULL;

    if (path == NULL) {
        log_error(logger,"ERROR DEL PATH");
    }
    dir = opendir(pathMetadatas);
    if (dir == NULL) {
        log_error(logger,"ERROR EN OPENDIR"); 
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {

            snprintf(path, (size_t) PATH_MAX, "%s/%s", pathMetadatas, entry->d_name);

            if (entry->d_type == DT_DIR) {
                log_info(logger,"INICIANDO COMPACTACION DE LA TABLA %s:", entry->d_name);
                fsIniciarCompactacionesTablasExistentes((void*)path);

            }else if(strcmp(entry->d_name,"MetaData") == 0){

                log_info(logger,"INGRESANDO A METADATA, PATH: %s", path);
                
                metaConfig = config_create(path);

	            if (metaConfig == NULL) {
	                log_error(logger,"ERROR: NO SE PUDO IMPORTAR LA METADATA");
	                exit(1);
	            }
                fsLlenarEstructuraMeta(m, metaConfig);
                tabla = string_new();
                tabla = string_substring(path,string_length(procesoConfig->puntoMontaje)+7,string_length(path)); // EL +7 ES POR EL TABLES/
                char** vectorPath = string_split(tabla,"/");
                free(tabla);
                fsCompactarTabla(m,vectorPath[0]);
                free(vectorPath);
            }
        }
    }   

    if(metaConfig != NULL){
        config_destroy(metaConfig);
    }

    closedir(dir);
    free(m);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsCompactarTabla(metadata_t* unaMetadata, char* tabla){

    infoCompactador_t* unaInfo = malloc(sizeof(infoCompactador_t));

    hiloCompactador_t* unNodoCompactador;

    unaInfo->tabla = string_new();
    string_append(&unaInfo->tabla, tabla);

    unaInfo->metadata = *unaMetadata;

    pthread_t hiloCompactador;

    if(pthread_create(&hiloCompactador, NULL, (void*)fsCompactador, (void*)unaInfo)) {
        log_error(logger,"ERROR CREANDO EL THREAD DE COMPACTADOR");
    }

    unNodoCompactador = crearNodoHiloCompactador(tabla, hiloCompactador);
    pthread_mutex_lock(&mutexListaHilos);
    list_add(hilosCompactador, unNodoCompactador);
    pthread_mutex_unlock(&mutexListaHilos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

hiloCompactador_t* crearNodoHiloCompactador(char* tablaRecibida, pthread_t hiloRecibido){

    hiloCompactador_t* nodoHiloCompactador = malloc(sizeof(hiloCompactador_t));
    nodoHiloCompactador->tabla = string_new();
    string_append(&nodoHiloCompactador->tabla, tablaRecibida);
    nodoHiloCompactador->estado = 0;
    nodoHiloCompactador->hilo = hiloRecibido;

    return nodoHiloCompactador;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsBorrarTablaHilosCompactador(char* tabla){

    pthread_mutex_lock(&mutexTablaABuscar);
    tablaABuscar = string_new();
    string_append(&tablaABuscar, tabla);
    list_remove_and_destroy_by_condition(hilosCompactador, (void*)mismoNombreCompactador, eliminarTablasHilosCompactador);
    pthread_mutex_unlock(&mutexTablaABuscar);


}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsBloquearTabla(char* tabla){
    
    pthread_mutex_lock(&mutexTablaABuscar);

    tablaABuscar = string_new();
    string_append(&tablaABuscar, tabla);
    hiloCompactador_t* nodoHilo = list_find(hilosCompactador,(void*)mismoNombreCompactador);
    
    pthread_mutex_unlock(&mutexTablaABuscar);
    
    nodoHilo->estado = 1;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsDesbloquearTabla(char* tabla){
    pthread_mutex_lock(&mutexTablaABuscar);

    tablaABuscar = string_new();
    string_append(&tablaABuscar, tabla);
    hiloCompactador_t* nodoHilo = list_find(hilosCompactador,(void*)mismoNombreCompactador);
    
    pthread_mutex_unlock(&mutexTablaABuscar);

    nodoHilo->estado = 0;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int fsTablaBloqueada(char* tabla){

    pthread_mutex_lock(&mutexTablaABuscar);

    tablaABuscar = string_new();
    string_append(&tablaABuscar, tabla);
    hiloCompactador_t* nodoHilo = list_find(hilosCompactador,(void*)mismoNombreCompactador);
    
    pthread_mutex_unlock(&mutexTablaABuscar);

    return nodoHilo->estado;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fsRenombrarTemporales(char* tabla){

    DIR* dir;
    struct dirent* entry;
    char path[PATH_MAX];

    char* pathTabla = string_new();
    string_append_with_format(&pathTabla,"%sTables/%s",procesoConfig->puntoMontaje,tabla);

    dir = opendir(pathTabla);

    if (dir == NULL) {
        log_error(logger,"ERROR EN OPENDIR");
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {

            snprintf(path, (size_t) PATH_MAX, "%s/%s", pathTabla, entry->d_name);
            
            if(string_ends_with(entry->d_name,"tmp")){
                char* nuevoNombre = string_new();
                string_append_with_format(&nuevoNombre,"%sc",path);
                int resultado = rename(path,nuevoNombre);

                if(resultado == 0 ){
                    log_info(logger,"ARCHIVO TEMPORAL %s RENOMBRADO CON ÉXITO.", entry->d_name);
                } else {
                    log_error(logger,"ERROR AL RENOMBRAR ARCHIVO TEMPORAL %s", entry->d_name);

                }
                free(nuevoNombre);
            }
            
        }
    }

    closedir(dir);
    free(pathTabla);

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AdministradorDeConexiones(void* infoAdmin){

    infoAdminConexiones_t* unaInfoAdmin = (infoAdminConexiones_t*) infoAdmin;

    int idCliente = 0;
    int resultado;

    while((resultado = recibirInt(unaInfoAdmin->socketCliente,&idCliente)) > 0){

        printf("\nMensaje recibido! Se me conectó una %s. Procedo a responderle...", devuelveNombreProceso(idCliente));
        
        if(idCliente != -1){
            manejarRespuestaAMemoria(unaInfoAdmin->socketCliente,idCliente,unaInfoAdmin->log);
        }
    }

    if(resultado == 0){
        printf("\nCliente desconectado");
        fflush(stdout);
        close(unaInfoAdmin->socketCliente);
        
    }else if(resultado < 0){
        printf("\nError al recibir");
        close(unaInfoAdmin->socketCliente);
       
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void manejarRespuestaAMemoria(int socket, int idSocket, t_log* log){

	int* tipoMensaje = malloc(sizeof(int));
	int* tamanioMensaje = malloc(sizeof(int));

	void* buffer = recibirPaquete(socket, tipoMensaje, tamanioMensaje, log);

		switch(*tipoMensaje){

			case tSelect: {

				t_select* unSelect = (t_select*) buffer;

				sleep(procesoConfig->retardo/1000);

                if(fsTablaExiste(unSelect->tabla)){

                    if(!fsTablaBloqueada(unSelect->tabla)){

                        registro_t* registro;
                        registro = fsEncontrarRegistro(unSelect->tabla,unSelect->key);

                        if(registro != NULL){
                            int tamanioRegistro = 0;
                            log_info(log,"Se encontró el registro solicitado por una memoria:");
                            log_info(log,"Valor: %s",registro->value);
                            log_info(log,"Timestamp: %d", registro->timestamp);
                            enviarInt(socket, 7); ////FALTARIA DEFINIR QUE EL VALOR 8 SIGNIFICA REGISTRO SELECCIONADO CON EXITO
                            enviarPaquete(socket, tRegistro, registro, tamanioRegistro, log);
                        }else{
                            log_warning(log,"No se encontró el registro solicitado por una memoria.");
                            enviarInt(socket, 8); //FALTARIA DEFINIR QUE EL VALOR 8 SIGNIFICA REGISTRO SELECCIONADO SIN EXITO
                        }
            	    }else{
                        log_warning(log,"La tabla %s está bloqueada.", unSelect->tabla); 
                        enviarInt(socket, 8); //FALTARIA DEFINIR QUE EL VALOR 8 SIGNIFICA REGISTRO SELECCIONADO SIN EXITO
            	    }
                }else{
                    log_warning(log,"La tabla %s no existe en el sistema.", unSelect->tabla);
                    enviarInt(socket, 8);
                }
				break;
            }

			case tInsert: {

				t_insert* unInsert = (t_insert*) buffer;

				sleep(procesoConfig->retardo/1000);
                
                if(fsTablaExiste(unInsert->tabla)){

                    if(!fsTablaBloqueada(unInsert->tabla)){
                        if(fsCrearRegistroEnMemtable(unInsert->tabla,unInsert->key,unInsert->valor,unInsert->timestamp)){
                            log_info(log,"Insert de registro pedido por memoria finalizada. Key: %d, Valor: %s", unInsert->key, unInsert->valor);
                            enviarInt(socket, 9); //FALTARIA DEFINIR QUE EL VALOR 9 SIGNIFICA REGISTRO INSERTADO CON EXITO
                        }else{
                            log_error(log,"Error al insertar el registro pedido por memoria.");
                            enviarInt(socket, 10); //FALTARIA DEFINIR QUE EL VALOR 9 SIGNIFICA REGISTRO INSERTADO SIN EXITO
                        }
                    }else{ 
                        log_warning(log,"La tabla %s está bloqueada.", unInsert->tabla);
                        enviarInt(socket, 10); //FALTARIA DEFINIR QUE EL VALOR 9 SIGNIFICA REGISTRO INSERTADO SIN EXITO
                    }
                }else{
                    log_warning(log,"La tabla %s no existe.", unInsert->tabla);
                    enviarInt(socket, 10);
                }
				break;
			}

			case tDrop: {

				t_drop* unDrop = (t_drop*) buffer;

				sleep(procesoConfig->retardo/1000);

				if(fsTablaExiste(unDrop->tabla)){

        			if(!fsTablaBloqueada(unDrop->tabla)){
            			char* pathTabla = string_new();

            			string_append_with_format(&pathTabla,"%sTables/%s",procesoConfig->puntoMontaje, unDrop->tabla);

            			fsLiberarBloquesDeTabla(unDrop->tabla);

            			if(fsBorrarTabla(pathTabla) != 0){
                
                			pthread_mutex_lock(&mutexMemtable);
                			fsBorrarTablaMemtable(unDrop->tabla);
                			pthread_mutex_unlock(&mutexMemtable);

                			pthread_mutex_lock(&mutexListaHilos);
                			fsBorrarTablaHilosCompactador(unDrop->tabla);
                			pthread_mutex_unlock(&mutexListaHilos);
                
                			log_info(log,"Pedido de memoria de borrado de tabla finalizado correctamente. Tabla: %s",unDrop->tabla);
							enviarInt(socket, 11); //FALTARIA DEFINIR QUE EL VALOR 11 SIGNIFICA TABLA BORRADA CON EXITO
            			}else{
                			log_warning(log,"Error al borrar la tabla pedida por memoria");
							enviarInt(socket, 12); //FALTARIA DEFINIR QUE EL VALOR 12 SIGNIFICA TABLA BORRADA SIN EXITO
            			}
        			}else{
            			log_warning(log,"La tabla %s está bloqueada.", unDrop->tabla);
						enviarInt(socket, 12); //FALTARIA DEFINIR QUE EL VALOR 12 SIGNIFICA TABLA BORRADA SIN EXITO
        			}
    			}else{
        			log_warning(log,"La tabla pedida por memoria no existe en el sistema");
					enviarInt(socket, 12); //FALTARIA DEFINIR QUE EL VALOR 12 SIGNIFICA TABLA BORRADA SIN EXITO
    			}
				break;
			}

			case tCreate: {

				t_create* unCreate = (t_create*) buffer;

				sleep(procesoConfig->retardo/1000);

				if(!fsTablaExiste(unCreate->tabla)){
                    char* particiones = string_new();
                    string_append(&particiones, string_itoa(unCreate->cantParticiones));
                    char* tiempoCompactaciones = string_new();
                    string_append(&tiempoCompactaciones, string_itoa(unCreate->tiempo_entre_compactaciones));
					fsCrearTabla(unCreate->tabla,unCreate->tipoConsistencia,particiones,tiempoCompactaciones);
                    log_info(log,"Pedido de memoria de creación de tabla finalizado correctamente. Tabla: %s",unCreate->tabla);
                    free(particiones);
                    free(tiempoCompactaciones);
					enviarInt(socket, 13); //FALTARIA DEFINIR QUE EL VALOR 13 SIGNIFICA TABLA CREADA CON EXITO
                }else{
                	log_warning(log,"La tabla pedida por memoria ya existe en el sistema");
					enviarInt(socket, 14); //FALTARIA DEFINIR QUE EL VALOR 14 SIGNIFICA TABLA CREADA SIN EXITO
            	}
				break;
			}

			case tDescribe: {

				t_describe* unDescribe = (t_describe*) buffer;

				sleep(procesoConfig->retardo/1000);

                if (!strcmp(unDescribe->tabla,"")) {
                    log_info(log,"Una memoria pidió un describe sin tabla especificada.");
                    char* pathMetadatas = string_new();
                    string_append_with_format(&pathMetadatas,"%sTables",procesoConfig->puntoMontaje);
                    char* unNombreTabla;
                    t_list* unaListaMetadatas = list_create();
                    fsDameMetadatas(pathMetadatas, unaListaMetadatas, unNombreTabla);
                    if(list_size(unaListaMetadatas) > 0){
                        for (int i = 0;i<list_size(unaListaMetadatas);i++){
                            nodoMetadata_t* nodo = list_get(unaListaMetadatas,i);
                            log_info(log,"Tabla del nodo: %s",nodo->tabla);
                            fsListarMetadata(nodo->metadata);
                        }
                        int tamanioDescribe = 0;
                        log_info(log,"Enviamos a la memoria todas las metadatas existentes.");
                        enviarInt(socket, 15);
                        enviarPaquete(socket, tMetadata, unaListaMetadatas, tamanioDescribe, log);
                    }else{
                        log_info(log,"No existen tablas en el sistema.");
                        enviarInt(socket, 16);
                    }
                    free(pathMetadatas);
                }else if(fsTablaExiste(unDescribe->tabla)){
                    int tamanioDescribe = 0;
                    log_info(log,"TABLA ESPECIFICADA: %s", unDescribe->tabla);
                    t_list* unaListaMetadatas = list_create();
                    metadata_t* unaMetadata;
            	    unaMetadata = fsObtenerMetadata(unDescribe->tabla);
                    nodoMetadata_t* unNodo = fsCrearNodoMetadata(unDescribe->tabla,unaMetadata);
                    log_info(log,"Tabla del nodo: %s",unNodo->tabla);
                    fsListarMetadata(unNodo->metadata);
                    list_add(unaListaMetadatas, unNodo);
                    enviarInt(socket, 15);
                    enviarPaquete(socket, tMetadata, unaListaMetadatas, tamanioDescribe, log);
                }else{
                    log_warning(log,"Una memoria pidió una tabla que no existe en el sistema: %s", unDescribe->tabla);
                    enviarInt(socket, 16);
                }
				break;
			}

            case tAdministrativo: {

                t_administrativo* unAdministrativo = (t_administrativo*) buffer;

                int tamanioAdministrativo = 0;

                if(procesoConfig->tamanioValue > 0){

                    unAdministrativo->valor = procesoConfig->tamanioValue;

                    log_info(log,"Enviamos el tamaño maximo del registro a memoria: %d",procesoConfig->tamanioValue);
                    enviarInt(socket, 17);
                    enviarPaquete(socket, tAdministrativo, unAdministrativo, tamanioAdministrativo, log);
                }else{
                    log_info(log,"El archivo de configuracion tenia un tamaño máximo de value: %d. No se lo enviamos a memoria",procesoConfig->tamanioValue);
                    enviarInt(socket, 18);
                }

   				break;

            }


			default:{
				log_error(log,"Recibimos algo de memoria que no sabemos manejar: %d",*tipoMensaje);
				abort();
				break;
			}
	}
    free(tipoMensaje);
    free(tamanioMensaje);
	free(buffer);
}