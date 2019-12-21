#include "api.h"
#include "parser.h"
#include "kernel.h"
#include "criterio.h"
#include <commons/collections/list.h>
#include "metrics.h"

#define RAND_MAX 100

int randm(int size)
{
	int div = RAND_MAX /(size + 1 );
	int ret;

	do
	{
		ret = rand() / div;
	}while(ret > size);
	return ret;
}

int KeSelect(char **args)
{
	struct metrics_data_s data;
    if(!ParseSelect(args))
	{
		return 0;
	}

	for(int size = 0; args[size]!=NULL; size++){
		printf("%s ",args[size]);
	}

	t_list *consistency = CriterioGetTableConsistency(args[1]);
	
	if(consistency != NULL){

		if(!list_is_empty(consistency)){
	
			pthread_mutex_lock(&mutex_Nodito);
			tablaMetadata = string_new();
			string_append(&tablaMetadata,args[1]);
			nodoMetadata_t* unNodito = list_find(tables_metadata,esLaTablaBuscada);
			pthread_mutex_unlock(&mutex_Nodito);

			if(unNodito != NULL){


				t_gossip *tg = list_get(consistency,randm(list_size(consistency)-1));
				
				data.type = unNodito->metadata->consistencia;
				data.command = SEL;
				//data.memoria_id = tg->id; 
				data.time_init = GetTimeInMs();

				int sock = KeSocketDeMemoria(tg->id);

				registro_t* nuevoRegistro = pedirRegistroAlServidor(sock, args[1], (uint16_t)atoi(args[2]), 3);
				if(nuevoRegistro != NULL){
					log_info(logger,"Registro encontrado: ");
					log_info(logger,"Key: %d", nuevoRegistro->key);
					log_info(logger,"Valor: %s", nuevoRegistro->value);
					free(nuevoRegistro);
				}
				data.time_end = GetTimeInMs();
				MetricsAddCommand(data);
				return 1;

			} else {
				log_warning(logger,"No hay memorias disponibles correspondientes a la consistencia de la tabla ingresada.");
				return 0;
			}
		}else{
			log_warning(logger,"No existe la tabla");
			return 0;
		}
	} else {
		log_warning(logger,"No existe la tabla");
		return 0;
	}

}

int KeDrop(char **args)
{
	if(!ParseDrop(args))
	{
		return 0;
	}
	for(int size = 0; args[size]!=NULL; size++){
		printf("%s ",args[size]);
	}

	t_list *consistency = CriterioGetTableConsistency(args[1]);
	if(consistency != NULL){
		if(!list_is_empty(consistency))
		{
			pthread_mutex_lock(&mutex_Nodito);
			tablaMetadata = string_new();
			string_append(&tablaMetadata,args[1]);
			nodoMetadata_t* unNodito = list_remove_by_condition(tables_metadata,esLaTablaBuscada);
			pthread_mutex_unlock(&mutex_Nodito);

			if(unNodito != NULL){

				free(unNodito->metadata);
				free(unNodito);

				t_gossip *tg = list_get(consistency,randm(list_size(consistency)-1));
				int sock = KeSocketDeMemoria(tg->id);

				int tamanioDropCreado = 0;
				t_drop* unDropCreado = malloc(sizeof(t_drop));

				unDropCreado->comando = string_new();
				string_append(&unDropCreado->comando ,args[0]);

				unDropCreado->tabla = string_new();
				string_append(&unDropCreado->tabla ,args[1]);

				enviarInt(sock, 3);
				enviarPaquete(sock, tDrop, unDropCreado, tamanioDropCreado, logger);

				free(unDropCreado);

				int resultado;
				int tipoResultado = 0;

				if((resultado = recibirInt(sock,&tipoResultado)) > 0){
					if(tipoResultado == 11){

						log_info(logger,"Tabla borrada con éxito en FS");

					}else if(tipoResultado == 12){

						log_info(logger,"No se pudo borrar la tabla en FS");

					}
					return 1;
				}
			}else{
				printf("\nNo existe la tabla");
				log_info(logger,"No existe la tabla");
				return 0;
			}

		}else{

			log_error(logger,"No hay memorias disponibles correspondientes a la consistencia de la tabla ingresada.");
			return 0;
		}

	}else{
		log_error(logger, "El kernel no conoce la tabla ingresada.");
		return 0;
	}	
}

int KeMetrics(char **args)
{
	MetricsGetInfo();
	return 1;
}

int KeInsert(char **args)
{
	struct metrics_data_s data;

    valorRegistro_t* valorRegistro = ParseInsert(args);

	if(valorRegistro == NULL)
	{
		return 0;
	}

	for(int size = 0; args[size]!=NULL; size++){
		printf("%s ",args[size]);
	}

	t_list *consistency = CriterioGetTableConsistency(args[1]);

	if(consistency != NULL){

		if(!list_is_empty(consistency)){

			t_gossip *tg = list_get(consistency,randm(list_size(consistency)-1));
 
				
			pthread_mutex_lock(&mutex_Nodito);
			tablaMetadata = string_new();
			string_append(&tablaMetadata,args[1]);
			nodoMetadata_t* unNodito = list_find(tables_metadata,esLaTablaBuscada);
			
			data.type = unNodito->metadata->consistencia;
			data.command = INS;
			data.memoria_id = tg->id;
			data.time_init = GetTimeInMs();

			
			pthread_mutex_unlock(&mutex_Nodito);

			if(unNodito	!= NULL){

				int sock = KeSocketDeMemoria(tg->id);

				int tamanioInsertCreado = 0;

				log_info(logger,"COMANDO A EJECUTAR: INSERT");

				t_insert* unInsertCreado = malloc(sizeof(t_insert));

				unInsertCreado->comando = string_new();
				string_append(&unInsertCreado->comando ,args[0]);

				unInsertCreado->tabla = string_new();  
				string_append(&unInsertCreado->tabla ,args[1]);

				unInsertCreado->key = (uint16_t) atoi(args[2]);

				unInsertCreado->valor = string_new();
				string_append(&unInsertCreado->valor,valorRegistro->valor);

				if (args[valorRegistro->proximoParametro] == NULL){
					unInsertCreado->timestamp = (uint32_t)time(NULL);
				}else{
					unInsertCreado->timestamp = (uint32_t)atoi(args[valorRegistro->proximoParametro]);
				}

				free(valorRegistro);

				enviarInt(sock, 3);
				enviarPaquete(sock, tInsert, unInsertCreado, tamanioInsertCreado, logger);

				int resultado;
				int tipoResultado = 0;

				if((resultado = recibirInt(sock,&tipoResultado)) > 0){

					if(tipoResultado == 22){

						log_info(logger,"Tabla actualizada correctamente");
						data.time_end = GetTimeInMs();
						MetricsAddCommand(data);

					}else{

						log_info(logger,"No se pudo actualizar la tabla");

					}
				}

				free(unInsertCreado);
				
				return 1;
			}else{
				log_info(logger,"No existe la tabla");
				return 0;
			}
		}else{
			log_error(logger,"No hay memorias disponibles correspondientes a la consistencia de la tabla ingresada.");
			return 0;
		}
	}else{
		log_error(logger, "El kernel no conoce la tabla ingresada.");
		return 0;
	}
	//TODO EN CASO DE QUE FALLE DEBIDO A UN DROP PREVIO EN EL FS SE DEBE 
	//INFORMAR POR ARCHIVO DE LOG Y FINALIZAR LA EJECUCION
	//TODO:Enviar request a memoria
	//puts("INSERT llamado");
}

int KeCreate(char **args)
{
	if(!ParseCreate(args))
	{
		return 0;
	}
	for(int size = 0; args[size]!=NULL; size++){
		printf("%s ",args[size]);
	}

	nodoMetadata_t *nodo = malloc(sizeof(nodoMetadata_t));

	nodo->metadata = malloc(sizeof(metadata_t));
	nodo->metadata->consistencia = fsTransformarTipoConsistencia(args[2]);
	nodo->metadata->tiempo_entre_compactaciones = (uint16_t) atoi(args[4]);
	nodo->metadata->num_particiones = (uint16_t) atoi(args[3]);
	nodo->tabla = string_new();
	string_append(&nodo->tabla, args[1]);

	list_add(tables_metadata,nodo);

	t_list* consistency = CriterioGetTableConsistency(args[1]);

	if(list_is_empty(consistency)){
		
		log_info(logger,"\nNo es posible realizar la operación, no hay una memoria asociada al criterio");
		nodoMetadata_t *nodo2 = list_remove(tables_metadata,list_size(tables_metadata)-1);
		free(nodo2->metadata);
		free(nodo2);
		return 0;

	}else{

		t_gossip* nod = list_get(consistency,randm(list_size(consistency)-1));

		int sock = KeSocketDeMemoria(nod->id);

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

		enviarInt(sock, 3);
		enviarPaquete(sock, tCreate, unCreateCreado, tamanioCreateCreado, logger);

		int resultado;
		int tipoResultado = 0;

		if((resultado = recibirInt(sock,&tipoResultado)) > 0){
			if(tipoResultado == 13){

				log_info(logger,"Tabla creada con éxito en FS: %s",unCreateCreado->tabla);
				return 0;

			}else if(tipoResultado == 14){

				log_info(logger,"No se pudo crear la tabla en FS: %d",unCreateCreado->tabla);
				return  0;

			}
		}
		free(unCreateCreado);
		return 1;
	}
}

int KeDescribe(char **args)
{
	if(!ParseDescribe(args))
	{
		return 0;
	}
	for(int size = 0; args[size]!=NULL; size++){
		printf("%s ",args[size]);
	}
	
	if(args[1] == NULL){

		log_info(logger,"COMANDO A EJECUTAR: DESCRIBE");

		t_describe* unDescribeCreado = malloc(sizeof(t_describe));

		unDescribeCreado->comando = string_new();
		string_append(&unDescribeCreado->comando ,args[0]);

		unDescribeCreado->tabla = string_new();

		string_append(&unDescribeCreado->tabla ,"");
	
  		tables_metadata = pedirMetadatasAlServidor(mem_socket, unDescribeCreado, 3);
		return 1;

	}else{

		t_list *consistency = CriterioGetTableConsistency(args[1]);

		if(consistency != NULL){

			log_info(logger,"COMANDO A EJECUTAR: DESCRIBE");

			t_describe* unDescribeCreado = malloc(sizeof(t_describe));

			unDescribeCreado->comando = string_new();
			string_append(&unDescribeCreado->comando ,args[0]);

			unDescribeCreado->tabla = string_new();
    
			string_append(&unDescribeCreado->tabla ,args[1]);
  
			pthread_mutex_lock(&mutex_Nodito);
			tablaMetadata = string_new();
			string_append(&tablaMetadata,args[1]);
 			nodoMetadata_t* unNodito = list_find(tables_metadata,esLaTablaBuscada);
			pthread_mutex_unlock(&mutex_Nodito);

			 if(unNodito == NULL){
				 list_add(tables_metadata,list_get(pedirMetadatasAlServidor(mem_socket, unDescribeCreado,3),0));
			 } else{
				 printf("\nTABLA: %s", unNodito->tabla);
				 log_info(logger,"TABLA: %s", unNodito->tabla);
				 fsListarMetadata(unNodito->metadata);
			 }
			return 1;
			
		}else{
			printf("\nNo existe la tabla");
			log_info(logger,"No existe la tabla");
			return 0;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////

bool esLaTablaBuscada(void* nodi) {

  nodoMetadata_t* s = (nodoMetadata_t*) nodi;
  bool existe = false;
  
  if (strcmp(s->tabla, tablaMetadata) == 0) {
    existe = true;
  }

  return existe;

}

int KeJournal(char **args)
{
	if(!ParseJournal(args))
	{
		return 0;
	}
	
	printf("\nPROCEDO A EJECUTAR JOURNAL");
	int i = 0;

	int tamanioJournalCreado = 0;
	int resultado;
	int tipoResultado = 0;

	t_administrativo* unAdministrativo = malloc(sizeof(t_administrativo));

	int cantMemSC = list_size(STRONGC);
	int cantMemShc = list_size(STRONGHC);
	int cantMemEc = list_size(EVENTUALC);

	t_gossip* mem;

	while(i < cantMemSC)
	{

		mem = list_get(STRONGC,i);

		unAdministrativo->valor = 1;
		unAdministrativo->codigo = 1;

		int fd = KeSocketDeMemoria(mem->id);

		enviarInt(fd, 3);
		enviarPaquete(fd, tJournal, unAdministrativo, tamanioJournalCreado, logger);

		if((resultado = recibirInt(fd,&tipoResultado)) > 0){

			if(tipoResultado == 27){

				log_info(logger,"Journal realizado con éxito en la memoria %d", i);
				return 1;

			}else{

				log_info(logger,"No se pudo Realizar el journal en la memoria %d", i);
				return 0;

			}
		}
		i++;
	}

	i=0;

	while(i< cantMemShc)
	{

		mem = list_get(STRONGHC,i);

		int fd = KeSocketDeMemoria(mem->id);

		unAdministrativo->valor = 1;
		unAdministrativo->codigo = 1;

		enviarInt(fd, 3);
		enviarPaquete(fd, tJournal, unAdministrativo, tamanioJournalCreado, logger);

		if((resultado = recibirInt(fd,&tipoResultado)) > 0){

			if(tipoResultado == 27){

				log_info(logger,"Journal realizado con éxito en la memoria %d", i);
				return 1;

			}else{

				log_info(logger,"No se pudo Realizar el journal en la memoria %d", i);
				return 0;

			}
		}
		i++;
	}

	i = 0;

	while(i< cantMemEc)
	{

		mem = list_get(EVENTUALC,i);

		int fd = KeSocketDeMemoria(mem->id);

		unAdministrativo->valor = 1;
		unAdministrativo->codigo = 1;

		enviarInt(fd, 3);
		enviarPaquete(fd, tJournal, unAdministrativo, tamanioJournalCreado, logger);

		if((resultado = recibirInt(fd,&tipoResultado)) > 0){

			if(tipoResultado == 27){

				log_info(logger,"Journal realizado con éxito en la memoria %d", i);
				return 1;

			}else{

				log_info(logger,"No se pudo Realizar el journal en la memoria %d", i);
				return 0;

			}
		}
		i++;
	}
}

int KeAdd(char **args)
{
	if(!ParseAdd(args))
	{
		return;
	}

	int i = 0;
	int id_memoria = atoi(args[2]);
	CONSISTENCY_TYPE ct = CriterioGetConsistency(args[4]);
	 
	t_gossip *memoria = NULL;
	memoria = list_get(lista_memorias,i);
	while(memoria->id != id_memoria)
	{
		i++;
		memoria = list_get(lista_memorias,i);
	}
	CriterioAgregarMemoria(memoria,ct);
	return 1;
}

int KeRun(char **args)
{
	char **script;
	if(!PaAbrirArchivoLQL(args[1]))
	{
		perror("PaAbrirArchivoLQL(): Error no se pudo abrir el archivo");
		return;
	}

	script = PaParseArchivoLQL();
	PlCrearTrabajos(script);
	PaCerrarArchivoLQL();
	return 1;
}