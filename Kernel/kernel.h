/*
 * ===================================================
 * @Name: kernel.h
 * @Author : Juan Manuel Canosa
 * @Description : Implementacion Proceso Kernel 
 * ===================================================
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include "../libreriasCompartidas/sockets.h"
#include "../libreriasCompartidas/serializacion.h"
#include "../libreriasCompartidas/consola.h"
#include <commons/config.h>
#include <stdio.h>
#include <string.h>
#include "api.h"
#include "planificador.h"
#include "parser.h"

#define KERNEL_CONFIG_PATH "configs/kernel.config"
#define IP_MEMORIA "IP_MEMORIA"
#define PUERTO_MEMORIA "PUERTO_MEMORIA"
#define QUANTUM "QUANTUM"
#define MULTIPROCESAMIENTO "MULTIPROCESAMIENTO"
#define METADATA_REFRESH "METADATA_REFRESH"
#define RETARDO_CICLO_EJECUCION "SLEEP_EJECUCION"

typedef struct kernel_conf_s{

	char* ip_memoria;
	int32_t puerto_memoria;
	int32_t quantum;
	int32_t multiprocesamiento;
	int32_t metadata_refresh;
	int32_t retardo_ciclo_instruccion;

}kernelConf;

typedef struct poolMemorias_t{
	t_gossip* gossip;
	int socket;
}poolMemorias_t;

/*
 *	@brief: Inicializa el proceso kernel
 *	@params : Nada
 *	@return : Nada
 */
void KeInicializar();

/*
 *	@brief : Establece conexion con el proceso memoria
 *	@params Ip : Ip del proceso memoria
 *	@params puerto : Puerto de conexion
 *	@return : Nada
 */
void KeConectarAMemoria();

/*
 *	@brief : Carga el archivo de configuracion y el logger 
 *	@params t_config : Puntero a estructura de configuracion 
 *	@return : Nada
 */
kernelConf *KeCargarConfiguracion(t_config*);

/*
 *	@brief : Crea un nuevo logger
 *	@params : Nada
 *	@return : Puntero al nuevo logger
 */
t_log *KeCrearNuevoLogger();

/*
 *	@brief : Termina el proceso y libera la memoria utilizada
 *	@params : Nada
 *	@return : Nada
 */
void KeTerminar();
void AdministradorDeConexiones(void* );
void KeLockDescribe();
void KeUnlockDescribe();
void criteriosInicializar();
bool esLaMemoria(void* nodi);
bool esElSocket(void* nodi);
bool esLaMemoria2(void* nodi);
bool esElSocket2(void* nodi);
t_list* KeGetSockets(t_list* listaMemorias);

t_config *tconfig;
t_log *logger;
kernelConf *kernel_conf;
t_list* lista_memorias;
t_list* tables_metadata;
t_list* lista_sockets;
t_list *STRONGC;
t_list *STRONGHC;
t_list *EVENTUALC;

int SOCKET_A_BUSCAR;
int MEMORIA_A_BUSCAR;

int mem_socket;

int mem_id;

int32_t TAM_MAX_VALUE;
pthread_mutex_t  mutex_Nodito;
pthread_mutex_t  mutex_MEMORIA_A_BUSCAR;
pthread_mutex_t mutex_SOCKET_A_BUSCAR;

#endif 
