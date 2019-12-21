#include "kernel.h"

#define MAX_BUILTINS 9
#define MAX_TABLES 4096

pthread_t configuration_thread;
pthread_t metadata_thread;
pthread_t describe_thread;
pthread_mutex_t metadata_lock;

void *KeConfigurationManagerThread();
void *KeGetTableMetadata();
void *KeActualizarMetadata();

void KeInicializar()
{
	builtins_t builtins[] = {
								{"RUN",&KeRun},
								{"SELECT",&KeSelect},
								{"INSERT",&KeInsert},
								{"METRICS",&KeMetrics},
								{"CREATE",&KeCreate},
								{"DESCRIBE",&KeDescribe},
								{"DROP",&KeDrop},
								{"JOURNAL",&KeJournal},
								{"ADD",&KeAdd}
							};

	ConsolaInicializar("kernel>",builtins,MAX_BUILTINS);
	logger = KeCrearNuevoLogger();
	kernel_conf = KeCargarConfiguracion(tconfig);

	tables_metadata = list_create();
	lista_memorias = list_create();
	
	KeConectarAMemoria();

	CriteriosInicializar();


	PlInicializar(kernel_conf->quantum,
				  kernel_conf->multiprocesamiento,
				  kernel_conf->retardo_ciclo_instruccion);
	pthread_create(&configuration_thread,NULL,KeConfigurationManagerThread,NULL);
    pthread_create(&metadata_thread,NULL,KeActualizarMetadata,NULL);
	
}

void KeTerminar()
{
	free(kernel_conf);
	free(logger);
	PlFinalizar();
	
}

void main(void)
{
	KeInicializar();
	ConsolaMain();
	KeTerminar();
	exit(0);
}

kernelConf *KeCargarConfiguracion(t_config *tconfig)
{
	kernelConf *config = (kernelConf*)malloc(sizeof(kernelConf));
    tconfig = config_create(KERNEL_CONFIG_PATH);
	//tconfig = config_create("kernel.config");
    
    if (tconfig == NULL) 
	{
        log_error(logger,"- NO SE PUDO IMPORTAR LA CONFIGURACION");
    	free(config);
        exit(1);
    }

    //log_info(logger, "- CONFIGURACION IMPORTADA");

    config->ip_memoria = config_get_string_value(tconfig, IP_MEMORIA);
    config->puerto_memoria = config_get_int_value(tconfig, PUERTO_MEMORIA);
    config->quantum = config_get_int_value(tconfig, QUANTUM);
    config->multiprocesamiento = config_get_int_value(tconfig, MULTIPROCESAMIENTO);
    config->metadata_refresh = config_get_int_value(tconfig, METADATA_REFRESH);
    config->retardo_ciclo_instruccion = config_get_int_value(tconfig, RETARDO_CICLO_EJECUCION);

    //log_info(logger, "· IP Memoria = %s", config->ip_memoria);
    //log_info(logger, "· Puerto Memoria = %d", config->puerto_memoria);
    //log_info(logger, "· Quantum = %d", config->quantum);
    //log_info(logger, "· Multiprocesamiento = %d", config->multiprocesamiento);
    //log_info(logger, "· Metadata Refresh = %d", config->metadata_refresh);
    //log_info(logger, "· Retardo Ciclo Instrucción = %d", config->retardo_ciclo_instruccion);

	free(tconfig->path);
	free(tconfig->properties);
	free(tconfig);

	tconfig = NULL;

	return config;
}

t_log *KeCrearNuevoLogger()
{
    t_log *log =  log_create("logs/kernel.log", "KERNEL", true, LOG_LEVEL_TRACE); 
    log_info(log, "- LOG INICIADO");
	return log;
}

void KeConectarAMemoria()
{
	
	log_info(logger,"IP DE LA MEMORIA: %s", kernel_conf->ip_memoria);
	log_info(logger,"PUERTO DE LA MEMORIA: %d", kernel_conf->puerto_memoria);
	mem_socket = cliente(kernel_conf->ip_memoria,kernel_conf->puerto_memoria,2,logger);
	pedirTamanioMaximo(mem_socket,3);
	lista_memorias = KeGetMemorias(mem_socket,3);
	lista_sockets = KeGetSockets(lista_memorias);
	pthread_create(&describe_thread,NULL,KeGetTableMetadata,NULL);

}

void *KeConfigurationManagerThread()
{
	while(1)
	{
		sleep(120);
		kernelConf *conf = KeCargarConfiguracion(tconfig);
		PlanificadorRestart(conf->quantum,conf->multiprocesamiento,conf->retardo_ciclo_instruccion);
	}
}

void *KeActualizarMetadata()
{
	poolMemorias_t* memPrincipal;
	while(1)
	{
		if(!list_is_empty(lista_sockets)){
			
			int sizeMemoria =  list_size(lista_sockets)-1;
			//t_gossip* unGossip;
			int* valorAEnviar = malloc(sizeof(int));
			*valorAEnviar = -1;

			while(write(mem_socket,valorAEnviar,sizeof(int)) == -1)
			{

				close(mem_socket);

				pthread_mutex_lock(&mutex_SOCKET_A_BUSCAR);
				SOCKET_A_BUSCAR = mem_socket;
				t_gossip* memoria = list_remove_by_condition(lista_memorias,esLaMemoria);
				poolMemorias_t* poolMemoria = list_remove_by_condition(lista_sockets,esElSocket2);
				pthread_mutex_lock(&mutex_SOCKET_A_BUSCAR);

				free(memoria->ip);
				free(memoria);
				free(poolMemoria);
				
				memPrincipal = list_get(lista_sockets,0);
				mem_socket = memPrincipal->socket;
				mem_id = memPrincipal->gossip->id;
			}
			printf("\n\nSOCKET %d, ID: %d", mem_socket, mem_id);
			lista_memorias = KeGetMemorias(mem_socket,3);
			lista_sockets = KeGetSockets(lista_memorias);
			
			sleep(30);

		}
	}
}

bool esLaMemoria(void* nodi) {

  t_gossip* s = (t_gossip*) nodi;
  bool existe = false;
  
  if (s->id == MEMORIA_A_BUSCAR) {
    existe = true;
  }

  return existe;

}

bool esLaMemoria2(void* nodi) {

  t_gossip* s = (t_gossip*) nodi;
  bool existe = false;
  
  if (KeSocketDeMemoria(s->id) == SOCKET_A_BUSCAR) {
    existe = true;
  }

  return existe;

}



bool esElSocket(void* nodi) {

  poolMemorias_t* s = (poolMemorias_t*) nodi;
  bool existe = false;
  
  if (s->gossip->id == MEMORIA_A_BUSCAR) {
    existe = true;
  }

  return existe;

}

bool esElSocket2(void* nodi) {

  poolMemorias_t* s = (poolMemorias_t*) nodi;
  bool existe = false;
  
  if (s->socket == SOCKET_A_BUSCAR) {
    existe = true;
  }

  return existe;

}


t_list* KeGetMetadata()
{
		return lista_memorias;
}

void AdministradorDeConexiones(void* s)
{
	return;
}

//ESTO NO ANDA
void* KeGetTableMetadata()
{	
	while(1)
	{

	t_describe* unDescribeCreado = malloc(sizeof(t_describe));

  	unDescribeCreado->comando = string_new();
  	string_append(&unDescribeCreado->comando ,"DESCRIBE");

 	unDescribeCreado->tabla = string_new();

    string_append(&unDescribeCreado->tabla ,"");

  	tables_metadata = pedirMetadatasAlServidor(mem_socket, unDescribeCreado, 3);
	
	sleep(kernel_conf->metadata_refresh);

  	} 
}

void KeLockDescribe()
{
	pthread_mutex_lock(&metadata_lock);
}

void KeUnlockDescribe()
{
	pthread_mutex_unlock(&metadata_lock);
}

void CriteriosInicializar()
{
        STRONGC = list_create();
        STRONGHC = list_create();
        EVENTUALC = list_create();        
}

t_list* KeGetSockets(t_list* listaMemorias){

	if(listaMemorias != NULL){

		if(!list_is_empty(listaMemorias)){

			int cantMemorias = list_size(listaMemorias);
			t_list* listitaSockets;

			if(lista_sockets == NULL || list_is_empty(lista_sockets)){

				listitaSockets = list_create();

			} else {

				listitaSockets = lista_sockets;

			}

				for(int i=0; i<cantMemorias;i++){

					poolMemorias_t* memoria = malloc(sizeof(poolMemorias_t));
					memoria->gossip = list_get(listaMemorias,i);
					
					pthread_mutex_lock(&mutex_MEMORIA_A_BUSCAR);
					MEMORIA_A_BUSCAR = memoria->gossip->id;

					if(list_find(listitaSockets,esElSocket) == NULL){
						memoria->socket = cliente(memoria->gossip->ip, memoria->gossip->puerto, 2, logger);
						list_add(listitaSockets, memoria);
					} else {
						free(memoria);
					}

					pthread_mutex_unlock(&mutex_MEMORIA_A_BUSCAR);

				}

				return listitaSockets;

		} else{
			log_info(logger,"La lista de memorias estaba vacía, no altero lista de sockets");
			t_list* listaNueva = list_create();
			return listaNueva;
		}
	} else {
		log_info(logger,"La lista de memorias estaba vacía, no altero lista de sockets");
		t_list* listaNueva = list_create();
		return listaNueva;
	}
}

int KeSocketDeMemoria(int memoriaId){

	pthread_mutex_lock(&mutex_MEMORIA_A_BUSCAR);
	MEMORIA_A_BUSCAR = memoriaId;
	poolMemorias_t* memoria = list_find(lista_sockets,esElSocket);
	pthread_mutex_unlock(&mutex_MEMORIA_A_BUSCAR);

	return memoria->socket;


}
