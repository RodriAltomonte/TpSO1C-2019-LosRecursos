#ifndef FYLESYSTEM_H_
#define FYLESYSTEM_H_

#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "../libreriasCompartidas/sockets.h"
#include "../libreriasCompartidas/consola.h"
#include "../libreriasCompartidas/serializacion.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#define LFS_CONFIG_PATH "configs/fileSystem.config"
#define PUERTO_LFS "PUERTO_ESCUCHA"
#define PUNTO_MONTAJE "PUNTO_MONTAJE"
#define RETARDO "RETARDO"
#define TAMANIO_VALUE "TAMANIO_VALUE"
#define TIEMPO_DUMP "TIEMPO_DUMP"

#define MAGIC_NUMBER "MAGIC_NUMBER"
#define BLOCKS "BLOCKS"
#define BLOCK_SIZE "BLOCK_SIZE"

#define CANTIDAD_DE_COMANDOS_CONSOLA_FS 5


////////////////////////////////STRUCTS/////////////////////////////

typedef struct {
    char* nombreTabla;
	t_list* listaRegistros;
} tablaMemtable_t;

typedef struct{
	int particion;
	t_list* listaRegistros;
} nodoParticion_t;

typedef struct hiloCompactador_s{
    char* tabla;
	int estado; //0 para normal, 1 para bloqueada
	pthread_t hilo;
} hiloCompactador_t;

typedef struct {

	int32_t puertoEscucha;
	char* puntoMontaje;
	int32_t retardo;
	int32_t tamanioValue;
	int32_t tiempoDump;
	char* magic_number;
	int32_t cantidadDeBloques;
	size_t tamanioBloques;

} fileSystemConfig;

typedef struct infoCompactador_s{
	metadata_t metadata;
	char* tabla;
}infoCompactador_t;

typedef struct infoBloques_s{
	int size;
	char** blocks;
}infoBloques_t;

///////////////////VARIABLES GLOBALES///////////////////////////

char* tablaABuscar;
//char* tablaHiloCompactador;
unsigned char valorArchivoConfig;

t_list* MEMTABLE;
t_list* hilosCompactador;

t_bitarray* bitarray;

t_config* configArchivoConfiguracion;
t_config* configMetadata;
fileSystemConfig* procesoConfig;

t_log* logger;

int32_t online;

pthread_mutex_t mutexBitmap;
pthread_mutex_t mutexMemtable;
pthread_mutex_t mutexListaHilos;
pthread_mutex_t mutexTablaABuscar;

///////////////////////ARRANQUE///////////////////////////////

fileSystemConfig* cargarConfiguracionFS(t_config* configArchivoConfiguracion, t_config* configMetadata);
void configurarLoggerFS();
void inicializar();
void conectarFS();
void actualizarConfiguracionFS();
//void consola();
void finalizar();

//////////////////////LISSANDRA///////////////////////////////

void fsSelect(char** args);
void fsInsert(char** args);
void fsDescribe(char** args);
void fsDrop(char** args);
void fsCreate(char** args);
registro_t* fsEncontrarRegistro(char *tabla, uint16_t key);

////////////////////////METADATA///////////////////////////////

metadata_t* fsObtenerMetadata(char *tabla);
int  fsCalcularParticion(metadata_t* meta, uint16_t key);
void fsListarTodasLasMetadatas(char* pathMetadatas);
void fsListarMetadata(metadata_t *m);
void fsLlenarEstructuraMeta(metadata_t* m, t_config* metaConfig);
//int fsTransformarTipoConsistencia(char* tipoConsistencia);
int fsCheckearConsistencia(char* consistenciaQuery);
nodoMetadata_t* fsCrearNodoMetadata (char* tabla, metadata_t* meta);

//////////////////////////DUMP/////////////////////////////////

void fsDump();
int hayEspacioParaDump();
void crearArchivoTemporal(tablaMemtable_t* unaTabla, int cantidadDeDumps);

//////////////////////MEMTABLE/////////////////////////////////

void fsBorrarTablaMemtable(char* tabla);
int fsCrearRegistroEnMemtable(char* tablaRecibida, uint16_t keyRecibido, char* valorRecibido, uint32_t timeStampRecibido);
tablaMemtable_t* buscarTablaEnMemtable(char* tablaAEvaluar);
tablaMemtable_t* crearNodoTablaMemtable(char* tablaRecibida);
registro_t* buscarRegistroEnTabla(t_list* registrosTablaBuscada, uint16_t keyRecibida);
registro_t* crearNodoRegistro(char* valor, uint16_t keyRecibido, uint32_t timestampRecibido);
registro_t* fsBuscarMayorTimestampMemtable (char* tabla, uint16_t key);

///////////////FUNCIONES PARA MANEJO DE LISTAS/////////////////

void eliminarListaParticiones(nodoParticion_t* unaParticion);
void eliminarRegistrosMemtable(registro_t* unRegistro);
void eliminarTablasMemtable(tablaMemtable_t* unaTabla);
bool mismoNombre(tablaMemtable_t* unaTabla);
bool mayorTimestamp(registro_t* menorRegistro, registro_t * mayorRegistro);
void eliminarTablasHilosCompactador(hiloCompactador_t* unHiloCompactador);
bool mismoNombreCompactador(hiloCompactador_t* unHiloCompactador);

//////////////////////////////BITMAP///////////////////////////

void inicializarBitMap();
void crearArchivoBitmap(char* pathBitmap);
void verificarBitmapPrevio(char* pathBitmap);
int validarArchivo(char* path);
int fsCantidadBloquesLibres();
int fsBuscarBloqueLibreYOcupar();
void crearBloquesFileSystem();
void fsMostrarEstadoBitmap();

////////////////////////////FILE SYSTEM////////////////////////

int fsTablaExiste(char *tabla);
void fsCrearTabla(char* tabla, char* consistencia, char* particiones, char* tiempoCompactacion);
int fsBorrarTabla(char* pathTabla);
int fsCantBloquesParaSize(int size);
int fsEscribirEnBloques(char* registros, int arregloBloques[], int cantBloques);
registro_t* fsBuscarRegistroEnBloques (char* tabla, uint16_t keyRecibida);
registro_t* fsBuscarRegistroEnString(char* todosLosRegistros, uint16_t keyRecibida);
char* fsCrearCharDeRegistrosDeTabla(char* tabla, int particion);
void fsVaciarBloque(int bloque);

////////////////////////////COMPACTACION////////////////////////

void fsCompactador(void* infoCompactador);
t_list* fsCrearListaParticiones(int particiones);
int keyEstaEnArreglo(uint16_t key, uint16_t arregloKeys[], int tamArreglo);
int fsEscribirNuevasParticiones(t_list* listaParticiones, char* tabla);
void fsBorrarTemporales(char* tabla);
void fsLiberarBloquesDeTabla(char* tabla);
infoBloques_t* fsCrearEstructuraInfoBloques(t_config* bloquesConfig);
void fsCompactarTabla(metadata_t* unaMetadata, char* tabla);
void fsIniciarCompactacionesTablasExistentes(void* pathTablas);
void fsBorrarTablaHilosCompactador(char* tabla);
hiloCompactador_t* crearNodoHiloCompactador(char* tablaRecibida, pthread_t hiloRecibido);
int fsObtenerCantidadDeTemporalesACompactar(char* tabla);

void fsBloquearTabla(char* tabla);
int fsTablaBloqueada(char* tabla);
void fsDesbloquearTabla(char* tabla);


////////////////////////////CONEXIONES///////////////////////////

void manejarRespuestaAMemoria(int socket, int idSocket, t_log* log);
void AdministradorDeConexiones(void* infoAdmin);

#endif
