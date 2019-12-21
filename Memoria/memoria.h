#ifndef MEMORIA_H_

#define MEMORIA_H_

#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "../libreriasCompartidas/sockets.h"
#include "../libreriasCompartidas/consola.h"
#include "../libreriasCompartidas/serializacion.h"
//incluyo time.h para sacar el timestamp
#include <time.h>
//incluyo para sacar el usleep
#include <unistd.h>

#define MEM_CONFIG_PATH "configs/memoria.config"
#define PUERTO_MEMORIA "PUERTO"
#define IP_FS "IP_FS"
#define PUERTO_FS "PUERTO_FS"
#define IP_SEEDS "IP_SEEDS"
#define PUERTO_SEEDS "PUERTO_SEEDS"
#define RETARDO_MEMORIA "RETARDO_MEM"
#define RETARDO_FS "RETARDO_FS"
#define TAMANIO_MEMORIA "TAM_MEM"
#define RETARDO_JOURNAL "RETARDO_JOURNAL"
#define RETARDO_GOSSIPING "RETARDO_GOSSIPING"
#define NUMERO_MEMORIA "MEMORY_NUMBER"

#define ID_CLIENTE 1
#define MAX_BUILTINS_MEMORIA 6

t_log *logger;

typedef struct {
	int puertoEscucha;
	char* ipFileSystem;
	int puertoFileSystem;
	char** ipSeeds;
	char** puertoSeeds;
	int retardoMemoria;
	int retardoFileSystem;
	int tamanioMemoria;
	int retardoJournal;
	int retardoGossiping;
	int idMemoria;
} memoriaConfig;

typedef struct {
	int nroPagina;
	int flag; //1 == modificado
	registro_t* reg;
} tPagina;

typedef struct {
	char* path;
	t_list* paginas;
} tSegmento;

typedef struct { 
	int id;
	int puerto;
	char* ip;
	int flag;
	int socket;
}tSeed;

// typedef struct{ 
// 	int proximoParametro;
// 	char* valor;
// }sgteParam;

t_list* TABLA_SEGMENTOS; //AKA Memoria
t_list* TABLA_PAGINAS; //CREO QUE NO ES NECESARIO REVISAR
t_list* TABLA_MARCOS_LIBRES;
t_list* TABLA_GOSSIPING;

t_config *tconfig;
memoriaConfig *PROCESO_CONFIG;
int SOCKET_LFS;
int32_t online;

int32_t TAM_MAX_VALUE; //dato obtenido de LFS
int32_t  TAM_MAX_REGISTRO;
char * MEMORIA_PRINCIPAL; //se debe reservar la memoria al principio

void configurarLogger();
memoriaConfig* cargarConfiguracion(t_config* config);
void conectar();
void inicializarMemoria();
void* consola(void * arg);
void* gossiping(void * arg);
void* ejecutarJournaling(void *arg);
void memSelect(char** args);
void memInsert(char** args);
void memCreate(char** args);
void memDescribe(char** args);
void memDrop(char** args);
void memJournal();
void finalizar();
void AdministradorDeConexiones(void* infoAdmin);

void actualizarFrame(tPagina* pagina);
tPagina* llenarRegistro(tPagina* pagina);
void eliminarSegmentoDeMemoria(tSegmento* segmento);
tSegmento* traerSegmento(char* nombreTabla); //devuelve un segmento de la tabla de segmentos
tSegmento* crearSegmento(char *nombreTabla);
bool existePath(void *tSegmento);
char *pathABuscar;
pthread_mutex_t  mutex_pathABuscar;
void liberarPaginas(t_list* paginas);//libera las paginas de un segmento
tPagina* traerPagina(tSegmento* segmento, uint16_t clave); //devuelve una pagina de la lista de paginas del segmento enviadpo por parámetro
bool existeKey(void *tPagina);
uint16_t keyABuscar;
bool existeIdMemoria(void *tseed);
char* ipMemoriaABuscar;
int puertoMemoriaABuscar;

pthread_mutex_t mutex_keyABuscar;
pthread_mutex_t mutex_LRU;
pthread_mutex_t mutex_Config;
pthread_mutex_t mutex_ipMemoriaABuscar;
pthread_mutex_t mutex_puertoMemoriaABuscar;

pthread_mutex_t mutex_Gossip;

void almacenarValor(uint16_t clave, char* valor, uint32_t timeStamp, tSegmento* unSegmento);//si time 0 entonces se lo asigno yo 
tPagina* pedirPagina(t_list* tablaPaginasLibres);
void ejecutarAlgoritmoReemplazo();
void test();


void manejarRespuestaAMemoria(int socket, int idSocket, t_log* log);
void manejarRespuestaAKernel(int socket, int idSocket, t_log* log);

//sgteParam* checkInsertValue(char** args);
t_list* seedsRunning();
void ActualizarTablaGossiping(t_list* tabla);
void eliminarSeedMemoria(t_gossip* unGoss);

pthread_mutex_t  mutex_memSelect;
pthread_mutex_t  mutex_memInsert;
pthread_mutex_t  mutex_memCreate;
pthread_mutex_t  mutex_memDescribe;
pthread_mutex_t  mutex_memDrop;
pthread_mutex_t  mutex_memJournal;

//hilos
pthread_t hiloServidor;
pthread_t hiloCliente;
pthread_t hiloGossiping;
pthread_t hiloConsola;
pthread_t hilojournal;

#endif


/*
bool estaFull(segmento *segmento);

pagina *pedirPagina(segmento *segmento); //devuelve una página libre
bool estaLibre(void *pagina);
bool esConsistenteConFS(void *pagina);
// proceso gossiping
void *gossiping(void *arg);
void ejecutarJournaling();
*/