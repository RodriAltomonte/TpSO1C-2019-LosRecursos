#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

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
#include "sockets.h"

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

typedef enum t_protocolo {
	tSelect = 1,
	tInsert,
	tDrop,
	tCreate,
	tDescribe,
	tRegistro,
	tJournal,
	tMetadata,
	tAdministrativo,
	tPedidoMemorias,
	tPoolMemorias,
	tFinDeProtocolo //NO SACAR Y DEJAR A LO ULTIMO!!!
} t_protocolo;

typedef enum {SC=1,SHC,EC} TIPO_CONSISTENCIA;


typedef struct t_select{
	char* comando;
	char* tabla;
	uint16_t key;
} __attribute__((packed)) t_select;


typedef struct t_insert{
	char* comando;
	char* tabla;
	uint16_t key;
	char* valor;
	uint32_t timestamp;
} __attribute__((packed)) t_insert;

typedef struct t_drop{
	char* comando;
	char* tabla;
} __attribute__((packed)) t_drop;

typedef struct t_create{
	char* comando;
	char* tabla;
	char* tipoConsistencia;
	uint16_t cantParticiones;
	uint16_t tiempo_entre_compactaciones;
} __attribute__((packed)) t_create;

typedef struct t_describe{
	char* comando;
	char* tabla;
} __attribute__((packed)) t_describe;

typedef struct metadata_s{
	TIPO_CONSISTENCIA consistencia;
	uint16_t num_particiones;
	uint16_t tiempo_entre_compactaciones;
}__attribute__((packed)) metadata_t;

typedef struct nodoMetadata_s{
	metadata_t* metadata;
	char* tabla;
}__attribute__((packed)) nodoMetadata_t;

typedef struct {
	int16_t codigo;
	uint32_t valor;
} __attribute__((packed)) t_administrativo;

typedef struct registro_s{
	uint32_t timestamp;
	uint16_t key;
	char* value;
} __attribute__((packed)) registro_t;

typedef struct infoAdminConexiones_s{
    t_log* log;
	int socketCliente;
} __attribute__((packed)) infoAdminConexiones_t;

typedef struct infoServidor_s{
    t_log* log;
	int puerto;
	char* ip;
} __attribute__((packed)) infoServidor_t;

typedef struct t_gossip{
	int puerto;
	int id;
	char* ip;
} __attribute__((packed)) t_gossip;

typedef struct valorRegistro_s{ 
	int proximoParametro;
	char* valor;
}valorRegistro_t;


void* recibirPaquete(int fdCliente, int* tipoMensaje, int* tamanioMensaje, t_log *log);
void  enviarPaquete(int fdCliente, int tipoMensaje, void * mensaje, int tamanioMensaje, t_log *log);

void* serializar(int tipoMensaje, void* mensaje, int* tamanio);
void* deserializar(uint16_t tipoMensaje, void* mensaje);

void* serializarSelect(t_select* select, int* tamanio);
t_select* deserializarSelect(void* buffer);

void* serializarInsert(void* insert, int* tamanio);
t_insert* deserializarInsert(void* buffer);

void* serializarDrop(void* drop, int* tamanio);
t_drop* deserializarDrop(void* buffer);

void* serializarCreate(void* create, int* tamanio);
t_create* deserializarCreate(void* buffer);

void* serializarDescribe(void* describe, int* tamanio);
t_describe* deserializarDescribe(void* buffer);

void* serializarGossip(void* gossip, int* tamanio);
t_list* deserializarGossip(void* buffer);

void* serializarMetadata(void* metadatas, int* tamanio);
t_list* deserializarMetadata(void* buffer);
void fsListarMetadata(metadata_t *m);
void eliminarNodoMetadata(nodoMetadata_t* unNodo);

void* serializarRegistro(void* registro,int* tamanio);
registro_t* deserializarRegistro(void* buffer);

void* serializarAdministrativo(void* administrativo, int* tamanio);
t_administrativo* deserializarAdministrativo(void* buffer);

void pedirTamanioMaximo(int socketServidor, int idCliente);
t_list* pedirMetadatasAlServidor(int socketServidor, t_describe* unDescribe,int idCliente);
registro_t* pedirRegistroAlServidor(int socketServidor, char* tabla, uint16_t clave, int idCliente);

valorRegistro_t* chequearValorEntreComillas(char** query, int tamMaxRegistro);
int fsTransformarTipoConsistencia(char* tipoConsistencia);

void pruebaSelect();
void pruebaInsert();
void pruebaDrop();
void pruebaCreate();
void pruebaDescribe();
//void pruebaMetadata();

#endif /* SERIALIZACION_H_ */
