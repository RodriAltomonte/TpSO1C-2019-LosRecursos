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
#include "serializacion.h"


void* serializar(int tipoMensaje, void* mensaje, int* tamanio){
	void* buffer;

	switch(tipoMensaje){

		case tSelect: {
			buffer = serializarSelect(mensaje, tamanio);
			break;
		}

		case tInsert: {
			buffer = serializarInsert(mensaje, tamanio);
			break;
		}

		case tDrop: {
			buffer = serializarDrop(mensaje, tamanio);
			break;
		}

		case tCreate: {
			buffer = serializarCreate(mensaje,tamanio);
			break;
		}

		case tDescribe: {
			buffer = serializarDescribe(mensaje,tamanio);
			break;

		}

		case tRegistro: {
			buffer = serializarRegistro(mensaje, tamanio);
			break;

		}

		case tAdministrativo: {
			buffer = serializarAdministrativo(mensaje, tamanio);
			break;
		}

		case tMetadata: {
			buffer = serializarMetadata(mensaje,tamanio);
			break;
		}

		case tPedidoMemorias:{
			buffer = serializarAdministrativo(mensaje, tamanio);
			break;
		}

		case tPoolMemorias:{
			buffer = serializarGossip(mensaje, tamanio);
			break;
		}

		case tJournal:{
			buffer = serializarAdministrativo(mensaje, tamanio);
			break;
		}

		case 0: {
			//printf("Desconexion. \n");
			break;
		}

		default: {
			//perror("Se recibio un mensaje que no esta en el protocolo.");
			abort();
			break;
		}
	}
	return buffer;
}


void* deserializar(uint16_t tipoMensaje, void* mensaje){

	void* buffer;

		switch(tipoMensaje){
			case tSelect: {
				buffer = deserializarSelect(mensaje);
				break;
			}

			case tInsert: {
				buffer = deserializarInsert(mensaje);
				break;
			}

			case tDrop: {
				buffer = deserializarDrop(mensaje);
				break;
			}

			case tCreate: {
				buffer = deserializarCreate(mensaje);
				break;
			}

			case tDescribe: {
				buffer = deserializarDescribe(mensaje);
				break;

			}

			case tRegistro: {
				buffer = deserializarRegistro(mensaje);
				break;

			}

			case tMetadata: {
				buffer = deserializarMetadata(mensaje);
				break;
			}

			case tAdministrativo: {
				buffer = deserializarAdministrativo(mensaje);
				break;
			}

			case tPedidoMemorias:{
				buffer = deserializarAdministrativo(mensaje);
				break;
			}

			case tPoolMemorias:{
				buffer = deserializarGossip(mensaje);
				break;
			}

			case tJournal:{
				buffer = deserializarAdministrativo(mensaje);
				break;
			}

			case 0: {
				//printf("Desconexion. \n");
				break;
			}


		default:{
				//perror("Se recibio un mensaje que no esta en el protocolo.");
				abort();
				break;
			}
		}
		return buffer;
}

void enviarPaquete(int fdCliente, int tipoMensaje, void * mensaje, int tamanioMensaje, t_log *log){

	int desplazamiento = 0;

	void* mensajeSerializado = serializar(tipoMensaje,mensaje,&tamanioMensaje);
	int tamanioTotal = sizeof(int) * 2 + tamanioMensaje;
	
	// printf("\nTamaño total del paquete serializado dentro del enviarPaquete: %d", tamanioTotal);
	// printf("\nTipo del paquete dentro del enviarPaquete: %d", tipoMensaje);

	void* buffer = malloc(tamanioTotal);
	memcpy(buffer + desplazamiento, &tipoMensaje, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buffer + desplazamiento, &tamanioMensaje, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buffer + desplazamiento, mensajeSerializado, tamanioMensaje);

	enviarPorSocket(fdCliente, buffer, tamanioTotal, log);

	free(buffer);
	free(mensajeSerializado);
}


void* recibirPaquete(int fdCliente, int* tipoMensaje, int* tamanioMensaje, t_log *log){

	int recibido = recibirPorSocket(fdCliente, tipoMensaje, sizeof(int), log);

	// printf("\n--->Tipo de mensaje en recibir paquete: %d",*tipoMensaje);
	// printf("\n--->Bytes recibidos en primera iteracion: %d",recibido);


	if (*tipoMensaje < 1 || *tipoMensaje > tFinDeProtocolo || recibido <= 0){
		return NULL;
	}

	recibido = recibirPorSocket(fdCliente, tamanioMensaje, sizeof(int), log);
	void* mensaje = malloc(*tamanioMensaje);

	// printf("\n--->Bytes recibidos en segunda iteracion: %d",recibido);
	// printf("\n--->Tamaño del paquete a recibir: %d",*tamanioMensaje);

	recibido = recibirPorSocket(fdCliente, mensaje, *tamanioMensaje, log);

 	// printf("\n--->Bytes recibidos en tercera iteracion: %d",recibido);

	void* buffer = deserializar(*tipoMensaje, mensaje);

	free(mensaje);
	return buffer;

} // Recordar castear


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////SERIALIZACION PAQUETES//////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////SELECT//////////////////////////////////////////////////////

void* serializarSelect(t_select* select, int* tamanio){

	t_select* unSelect = (t_select*) select;

	int desplazamiento = 0;
	int tamanioComando = string_length(unSelect->comando);
	int tamanioTabla = string_length(unSelect->tabla);

	*tamanio = 2 * sizeof(int) + tamanioComando + tamanioTabla + sizeof(uint16_t);

	void* selectSerializado = malloc(*tamanio);

	memcpy(selectSerializado + desplazamiento, &tamanioComando, sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(selectSerializado + desplazamiento, unSelect->comando, tamanioComando);
	desplazamiento += tamanioComando;

	memcpy(selectSerializado + desplazamiento, &tamanioTabla,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(selectSerializado + desplazamiento, unSelect->tabla,tamanioTabla);
	desplazamiento += tamanioTabla;

	memcpy(selectSerializado + desplazamiento, &unSelect->key, sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	///////////////////////////////////////CASO DE PRUEBA SERIALIZACION SELECT///////////////////////////////////////////////
	
	// printf("\n\nSELECT A SERIALIZAR: \n");
	// printf("\nComando: %s", unSelect->comando);
	// printf("\nTabla: %s", unSelect->tabla);
	// printf("\nKey: %d", unSelect->key);
	// printf("\nTamaño del select: %d", *tamanio);
	
	// t_select* selectDeserializado = deserializarSelect(selectSerializado);

	// printf("\n\nSELECT DESERIALIZADO: \n");
	// printf("\nComando: %s", selectDeserializado->comando);
	// printf("\nTabla: %s", selectDeserializado->tabla);
	// printf("\nKey: %d\n", selectDeserializado->key);

	// free(selectDeserializado);
	
	///////////////////////////////////////FIN CASO DE PRUEBA SERIALIZACION SELECT////////////////////////////////////////////

	return selectSerializado;
}

t_select* deserializarSelect(void* buffer){

	t_select* unSelect = malloc(sizeof(t_select));

	int desplazamiento = 0;
	int tamanioComando = 0;
	int tamanioTabla = 0;

	memcpy(&tamanioComando, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* bufferComando = malloc(tamanioComando+1);
	memcpy(bufferComando, buffer + desplazamiento, tamanioComando);
	bufferComando[tamanioComando] = '\0';
	desplazamiento += tamanioComando;

	memcpy(&tamanioTabla, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* bufferTabla = malloc(tamanioTabla+1);
	memcpy(bufferTabla, buffer + desplazamiento, tamanioTabla);
	bufferTabla[tamanioTabla] = '\0';
	desplazamiento += tamanioTabla;

	memcpy(&unSelect->key, buffer + desplazamiento, sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	unSelect->comando = string_new();
	unSelect->tabla = string_new();

	string_append(&unSelect->comando, bufferComando);
	string_append(&unSelect->tabla, bufferTabla);

	free (bufferComando);
	free (bufferTabla);

	return unSelect;
}

///////////////////////////////////////////////////////INSERT//////////////////////////////////////////////////

void* serializarInsert(void* insert, int* tamanio){

	t_insert* unInsert = (t_insert*) insert;
	
	int desplazamiento = 0;
	int tamanioComando = string_length(unInsert->comando);
	int tamanioTabla = string_length(unInsert->tabla);
	int tamanioValor = string_length(unInsert->valor);

	*tamanio = 3 * sizeof(int) + tamanioComando + tamanioTabla + tamanioValor + sizeof(uint16_t) + sizeof(uint32_t);

	void* insertSerializado = malloc(*tamanio);

	memcpy(insertSerializado + desplazamiento, &tamanioComando,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(insertSerializado + desplazamiento, unInsert->comando,tamanioComando);
	desplazamiento += tamanioComando;

	memcpy(insertSerializado + desplazamiento, &tamanioTabla,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(insertSerializado + desplazamiento, unInsert->tabla,tamanioTabla);
	desplazamiento += tamanioTabla;

	memcpy(insertSerializado + desplazamiento, &unInsert->key, sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	memcpy(insertSerializado + desplazamiento, &tamanioValor,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(insertSerializado + desplazamiento, unInsert->valor,tamanioValor);
	desplazamiento += tamanioValor;

	memcpy(insertSerializado + desplazamiento, &unInsert->timestamp, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	///////////////////////////////////////CASO DE PRUEBA SERIALIZACION INSERT////////////////////////////////////////////

	// printf("\n\nINSERT A SERIALIZAR: \n");
	// printf("\nComando: %s", unInsert->comando);
	// printf("\nTabla: %s", unInsert->tabla);
	// printf("\nValor: %s", unInsert->valor);
	// printf("\nKey: %d", unInsert->key);
	// printf("\nTimestamp: %d", unInsert->timestamp);
	// printf("\nTamaño del insert: %d", *tamanio);
	
	// t_insert* insertDeserializado = deserializarInsert(insertSerializado);

	// printf("\n\nINSERT DESERIALIZADO: \n");
	// printf("\nComando: %s", insertDeserializado->comando);
	// printf("\nTabla: %s", insertDeserializado->tabla);
	// printf("\nValor: %s", insertDeserializado->valor);
	// printf("\nKey: %d", insertDeserializado->key);
	// printf("\nTimestamp: %d", insertDeserializado->timestamp);

	// free(insertDeserializado);

	///////////////////////////////////////FIN CASO DE PRUEBA SERIALIZACION INSERT////////////////////////////////////////////

	return insertSerializado;
}

t_insert* deserializarInsert(void* buffer){

	t_insert* unInsert = malloc(sizeof(t_insert));
	
	int desplazamiento = 0;
	int tamanioComando = 0;
	int tamanioTabla   = 0;
	int tamanioValor   = 0;

	memcpy(&tamanioComando, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* bufferComando = malloc(tamanioComando+1);
	memcpy(bufferComando, buffer + desplazamiento, tamanioComando);
	bufferComando[tamanioComando] = '\0';
	desplazamiento += tamanioComando;

	memcpy(&tamanioTabla, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* bufferTabla = malloc(tamanioTabla+1);
	memcpy(bufferTabla, buffer + desplazamiento, tamanioTabla);
	bufferTabla[tamanioTabla] = '\0';
	desplazamiento += tamanioTabla;

	memcpy(&unInsert->key, buffer + desplazamiento, sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	memcpy(&tamanioValor, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* bufferValor = malloc(tamanioValor+1);
	memcpy(bufferValor, buffer + desplazamiento, tamanioValor);
	bufferValor[tamanioValor] = '\0';
	desplazamiento += tamanioValor;

	memcpy(&unInsert->timestamp, buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	unInsert->comando = string_new();
	unInsert->tabla   = string_new();
	unInsert->valor   = string_new();

	string_append(&unInsert->comando, bufferComando);
	string_append(&unInsert->tabla, bufferTabla);
	string_append(&unInsert->valor, bufferValor);

	free (bufferComando);
	free (bufferTabla);
	free (bufferValor);

	return unInsert;
}

///////////////////////////////////////////////////////DROP//////////////////////////////////////////////////

void* serializarDrop(void* drop, int* tamanio){

	t_drop* unDrop = (t_drop*) drop;

	int desplazamiento = 0;
	int tamanioComando = string_length(unDrop->comando);
	int tamanioTabla = string_length(unDrop->tabla);
	
	*tamanio = 2 * sizeof(int) + tamanioComando + tamanioTabla;

	void* dropSerializado = malloc(*tamanio);

	memcpy(dropSerializado + desplazamiento, &tamanioComando,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(dropSerializado + desplazamiento, unDrop->comando, tamanioComando);
	desplazamiento += tamanioComando;

	memcpy(dropSerializado + desplazamiento, &tamanioTabla,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(dropSerializado + desplazamiento, unDrop->tabla, tamanioTabla);
	desplazamiento += tamanioTabla;

	///////////////////////////////////////CASO DE PRUEBA SERIALIZACION DROP////////////////////////////////////////////

	// printf("\n\nDROP A SERIALIZAR: \n");
	// printf("\nComando: %s", unDrop->comando);
	// printf("\nTabla: %s", unDrop->tabla);
	// printf("\nTamaño del drop: %d", *tamanio);
	
	// t_drop* dropDeserializado = deserializarDrop(dropSerializado);

	// printf("\n\nDROP DESERIALIZADO: \n");
	// printf("\nComando: %s", dropDeserializado->comando);
	// printf("\nTabla: %s", dropDeserializado->tabla);

	// free(dropDeserializado);

	// ///////////////////////////////////////FIN CASO DE PRUEBA SERIALIZACION DROP////////////////////////////////////////////

	return dropSerializado;
}

t_drop* deserializarDrop(void* buffer){

	t_drop* unDrop = malloc(sizeof(t_drop));

	int desplazamiento = 0;
	int tamanioComando = 0;
	int tamanioTabla = 0;

	memcpy(&tamanioComando, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* bufferComando = malloc(tamanioComando+1);
	memcpy(bufferComando, buffer + desplazamiento, tamanioComando);
	bufferComando[tamanioComando] = '\0';
	desplazamiento += tamanioComando;

	memcpy(&tamanioTabla, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* bufferTabla = malloc(tamanioTabla+1);
	memcpy(bufferTabla, buffer + desplazamiento, tamanioTabla);
	bufferTabla[tamanioTabla] = '\0';
	desplazamiento += tamanioTabla;

	unDrop->comando = string_new();
	unDrop->tabla = string_new();

	string_append(&unDrop->comando, bufferComando);
	string_append(&unDrop->tabla, bufferTabla);

	free (bufferComando);
	free (bufferTabla);

	return unDrop;
}

///////////////////////////////////////////////////////CREATE//////////////////////////////////////////////////

void* serializarCreate(void* create, int* tamanio){

	t_create* unCreate = (t_create*) create;

	int desplazamiento = 0;
	int tamanioComando = string_length(unCreate->comando);
	int tamanioTabla   = string_length(unCreate->tabla);
	int tamanioCompactacion = string_length(unCreate->tipoConsistencia);

	*tamanio = 3 * sizeof(int) + tamanioComando + tamanioTabla + tamanioCompactacion + 2 * sizeof(uint16_t);

	void* createSerializado = malloc(*tamanio);

	memcpy(createSerializado + desplazamiento, &tamanioComando, sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(createSerializado + desplazamiento, unCreate->comando, tamanioComando);
	desplazamiento += tamanioComando;

	memcpy(createSerializado + desplazamiento, &tamanioTabla,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(createSerializado + desplazamiento, unCreate->tabla, tamanioTabla);
	desplazamiento += tamanioTabla;

	memcpy(createSerializado + desplazamiento, &tamanioCompactacion,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(createSerializado + desplazamiento, unCreate->tipoConsistencia, tamanioCompactacion);
	desplazamiento += tamanioCompactacion;

	memcpy(createSerializado + desplazamiento, &unCreate->cantParticiones, sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	memcpy(createSerializado + desplazamiento, &unCreate->tiempo_entre_compactaciones,sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	///////////////////////////////////////CASO DE PRUEBA SERIALIZACION CREATE////////////////////////////////////////////

	// printf("\n\nCREATE A SERIALIZAR: \n");
	// printf("\nComando: %s", unCreate->comando);
	// printf("\nTabla: %s", unCreate->tabla);
	// printf("\nTipo de compactacion: %s", unCreate->tipoConsistencia);
	// printf("\nCantidad de particiones: %d", unCreate->cantParticiones);
	// printf("\nTiempo entre compactaciones: %d", unCreate->tiempo_entre_compactaciones);
	// printf("\nTamaño del create: %d", *tamanio);
	
	// t_create* createDeserializado = deserializarCreate(createSerializado);

	// printf("\n\nCREATE DESERIALIZADO: \n");
	// printf("\nComando: %s", createDeserializado->comando);
	// printf("\nTabla: %s", createDeserializado->tabla);
	// printf("\nTipo de compactacion: %s", createDeserializado->tipoConsistencia);
	// printf("\nCantidad de particiones: %d", createDeserializado->cantParticiones);
	// printf("\nTiempo entre compactaciones: %d", createDeserializado->tiempo_entre_compactaciones);

	//free(createDeserializado);

	///////////////////////////////////////FIN CASO DE PRUEBA SERIALIZACION CREATE////////////////////////////////////////////

	return createSerializado;
}

t_create* deserializarCreate(void* buffer){

	t_create* unCreate = malloc(sizeof(t_create));

	int desplazamiento = 0;
	int tamanioComando = 0;
	int tamanioTabla = 0;
	int tamanioCompactacion = 0;

	memcpy(&tamanioComando, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* bufferComando = malloc(tamanioComando+1);
	memcpy(bufferComando, buffer + desplazamiento, tamanioComando);
	bufferComando[tamanioComando] = '\0';
	desplazamiento += tamanioComando;

	memcpy(&tamanioTabla, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* bufferTabla = malloc(tamanioTabla+1);
	memcpy(bufferTabla, buffer + desplazamiento, tamanioTabla);
	bufferTabla[tamanioTabla] = '\0';
	desplazamiento += tamanioTabla;

	memcpy(&tamanioCompactacion, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* bufferCompactacion = malloc(tamanioCompactacion+1);
	memcpy(bufferCompactacion, buffer + desplazamiento, tamanioCompactacion);
	bufferCompactacion[tamanioCompactacion] = '\0';
	desplazamiento += tamanioCompactacion;

	memcpy(&unCreate->cantParticiones, buffer + desplazamiento, sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	memcpy(&unCreate->tiempo_entre_compactaciones, buffer + desplazamiento, sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	unCreate->comando = string_new();
	unCreate->tabla   = string_new();
	unCreate->tipoConsistencia = string_new();

	string_append(&unCreate->comando, bufferComando);
	string_append(&unCreate->tabla, bufferTabla);
	string_append(&unCreate->tipoConsistencia, bufferCompactacion);

	free (bufferComando);
	free (bufferTabla);
	free (bufferCompactacion);

	return unCreate;
}

///////////////////////////////////////////////////////DESCRIBE//////////////////////////////////////////////////

void* serializarDescribe(void* describe, int* tamanio){

	t_describe* unDescribe = (t_describe*) describe;

	int desplazamiento = 0;
	int tamanioComando = string_length(unDescribe->comando);
	int tamanioTabla   = string_length(unDescribe->tabla);

	*tamanio = sizeof(int) * 2 + tamanioComando + tamanioTabla; 

	void* describeSerializado = malloc(*tamanio);

	memcpy(describeSerializado + desplazamiento, &tamanioComando, sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(describeSerializado + desplazamiento, unDescribe->comando, tamanioComando);
	desplazamiento += tamanioComando;

	memcpy(describeSerializado + desplazamiento, &tamanioTabla,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(describeSerializado + desplazamiento, unDescribe->tabla,tamanioTabla);
	desplazamiento += tamanioTabla;

	///////////////////////////////////////CASO DE PRUEBA SERIALIZACION DESCRIBE////////////////////////////////////////////

	// printf("\n\nDESCRIBE A SERIALIZAR: \n");
	// printf("\nComando: %s", unDescribe->comando);
	// printf("\nTabla: %s", unDescribe->tabla);
	// printf("\nTamaño del describe: %d", *tamanio);
	
	// t_describe* describeDeserializado = deserializarDescribe(describeSerializado);

	// printf("\n\nDESCRIBE DESERIALIZADO: \n");
	// printf("\nComando: %s", describeDeserializado->comando);
	// printf("\nTabla: %s\n", describeDeserializado->tabla);

	// free(describeDeserializado);

	///////////////////////////////////////FIN CASO DE PRUEBA SERIALIZACION DESCRIBE////////////////////////////////////////////

	return describeSerializado;
}

t_describe* deserializarDescribe(void* buffer){

	t_describe* unDescribe = malloc(sizeof(t_describe));

	int desplazamiento = 0;
	int tamanioComando = 0;
	int tamanioTabla = 0;

	memcpy(&tamanioComando, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* bufferComando = malloc(tamanioComando+1);
	memcpy(bufferComando, buffer + desplazamiento, tamanioComando);
	bufferComando[tamanioComando] = '\0';
	desplazamiento += tamanioComando;

	memcpy(&tamanioTabla, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* bufferTabla = malloc(tamanioTabla+1);
	memcpy(bufferTabla, buffer + desplazamiento, tamanioTabla);
	bufferTabla[tamanioTabla] = '\0';
	desplazamiento += tamanioTabla;

	unDescribe->comando = string_new();
	unDescribe->tabla = string_new();

	string_append(&unDescribe->comando, bufferComando);
	string_append(&unDescribe->tabla, bufferTabla);

	free (bufferComando);
	free (bufferTabla);

	return unDescribe;
}

////////////////////////////////////////////////////ADMINISTRATIVO////////////////////////////////////////////////////

void* serializarAdministrativo(void* administrativo, int* tamanio) {

	t_administrativo* unAdministrativo = (t_administrativo*) administrativo;

	*tamanio = sizeof(uint16_t) + sizeof(uint32_t);

	printf("\n \n ******Tamanio administrativo %d",*tamanio);

	int desplazamiento = 0; 

	void* paqueteAdministrativo = malloc(*tamanio);

	memcpy(paqueteAdministrativo + desplazamiento, &unAdministrativo->codigo, sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	memcpy(paqueteAdministrativo + desplazamiento, &unAdministrativo->valor, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	///////////////////////////////////////CASO DE PRUEBA SERIALIZACION ADMINISTRATIVO////////////////////////////////////////////

	// printf("\n\nADMINISTRATIVO A SERIALIZAR: \n");
	// printf("\nCodigo: %d", unAdministrativo->codigo);
	// printf("\nValor: %d", unAdministrativo->valor);
	// printf("\nTamaño del administrativo: %d", *tamanio);
	
	// t_administrativo* administrativoDeserializado = deserializarAdministrativo(unAdministrativo);

	// printf("\n\nADMINISTRATIVO DESERIALIZADO: \n");
	// printf("\nCodigo: %d", administrativoDeserializado->codigo);
	// printf("\nValor: %d\n", administrativoDeserializado->valor);

	// free(administrativoDeserializado);

	///////////////////////////////////////FIN CASO DE PRUEBA SERIALIZACION ADMINISTRATIVO////////////////////////////////////////////

	return paqueteAdministrativo;

}

t_administrativo* deserializarAdministrativo(void* buffer) {

	int desplazamiento = 0;
	
	t_administrativo* unAdministrativo = malloc(sizeof(t_administrativo));

	memcpy(&unAdministrativo->codigo, buffer + desplazamiento, sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	memcpy(&unAdministrativo->valor, buffer + desplazamiento, sizeof(uint32_t));

	return unAdministrativo;

}


//////////////////////////////////////////////////////GOSSIPING//////////////////////////////////////////////////////

void* serializarGossip(void* gossips, int* tamanio){

	t_list* listaGossips = (t_list*)gossips;

	int cantidadGossips = list_size(listaGossips);
	int desplazamiento = 0;
	int i = 0;
	*tamanio = sizeof(int) + (3 * sizeof(int)) * cantidadGossips; //El primer sizeof son la cantidad de gossips
	int tamanioIp = 0;

	while(i<cantidadGossips){
		t_gossip* goss = list_get(listaGossips,i);
		tamanioIp = string_length(goss->ip);
		*tamanio += tamanioIp;
		i++;
	}

	void* paqueteGossips = malloc(*tamanio);

	memcpy(paqueteGossips + desplazamiento, &cantidadGossips, sizeof(int));
	desplazamiento += sizeof(int);

	i=0;

	while(i<cantidadGossips){
	
		t_gossip* unGossip = list_get(listaGossips,i);
		tamanioIp = string_length(unGossip->ip);

		memcpy(paqueteGossips + desplazamiento, &unGossip->id,sizeof(int));
		desplazamiento += sizeof(int);

		memcpy(paqueteGossips + desplazamiento, &unGossip->puerto,sizeof(int));
		desplazamiento += sizeof(int);

		memcpy(paqueteGossips + desplazamiento, &tamanioIp,sizeof(int));
		desplazamiento += sizeof(int);

		memcpy(paqueteGossips + desplazamiento, unGossip->ip,tamanioIp);
		desplazamiento += tamanioIp;
		
		i++;
	}
	return paqueteGossips;
}

t_list* deserializarGossip(void* buffer){

	int desplazamiento = 0;
	
	t_list* listaGossips = list_create();

	int cantGossips = 0;
	memcpy(&cantGossips, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	int i = 0;
	int tamanioIp = 0;

	while(i<cantGossips){

		t_gossip* goss = malloc(sizeof(t_gossip));

		memcpy(&goss->id, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);

		memcpy(&goss->puerto, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);

		memcpy(&tamanioIp, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);

		char* bufferIp = malloc(tamanioIp+1);
		memcpy(bufferIp, buffer + desplazamiento, tamanioIp);
		bufferIp[tamanioIp] = '\0';
		desplazamiento += tamanioIp;

		goss->ip = string_new();
		string_append(&goss->ip, bufferIp);

		list_add(listaGossips,goss);
		free (bufferIp);

		i+=1;
	}
	
	return listaGossips;
}

////////////////////////////////////////////////////METADATA////////////////////////////////////////////////////

void* serializarMetadata(void* metadatas, int* tamanio){
	
	t_list* listaMetadatas = (t_list*)metadatas;
	int cantMetadatas = list_size(listaMetadatas);
	int tamanioTabla = 0; 
	int i = 0;

	int desplazamiento = 0;
	*tamanio = sizeof(int) + sizeof(metadata_t) * cantMetadatas + cantMetadatas * sizeof(int); // le sumo la cantidad de metadatas (indicado en el 1er int) que va a recibir el otro

	while(i<cantMetadatas){
		nodoMetadata_t* nodito = list_get(listaMetadatas,i);
		tamanioTabla = string_length(nodito->tabla);
		*tamanio += tamanioTabla;
		i++;
	}

	void* paqueteMetadata = malloc(*tamanio);

	memcpy(paqueteMetadata + desplazamiento, &cantMetadatas, sizeof(int));
	desplazamiento += sizeof(int);

	i=0;
	
	while(i<cantMetadatas){

		nodoMetadata_t* nodo = list_get(listaMetadatas,i);
		tamanioTabla = string_length(nodo->tabla);

		memcpy(paqueteMetadata + desplazamiento, &tamanioTabla, sizeof(int));
		desplazamiento += sizeof(int);

		memcpy(paqueteMetadata + desplazamiento, nodo->tabla, tamanioTabla);
		desplazamiento += tamanioTabla;

		memcpy(paqueteMetadata + desplazamiento, &nodo->metadata->consistencia, sizeof(TIPO_CONSISTENCIA));
		desplazamiento += sizeof(TIPO_CONSISTENCIA);

		memcpy(paqueteMetadata + desplazamiento, &nodo->metadata->num_particiones, sizeof(uint16_t));
	 	desplazamiento += sizeof(uint16_t);

	 	memcpy(paqueteMetadata + desplazamiento, &nodo->metadata->tiempo_entre_compactaciones, sizeof(uint16_t));
	 	desplazamiento += sizeof(uint16_t);

		i+=1;
	}
	list_destroy_and_destroy_elements(listaMetadatas,eliminarNodoMetadata);
	return paqueteMetadata;
}

t_list* deserializarMetadata(void* buffer){

	int desplazamiento = 0;
	
	t_list* listaMetadatas = list_create();

	int cantMetadatas = 0;
	memcpy(&cantMetadatas, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	int i = 0;
	int tamanioTabla = 0;

	while(i<cantMetadatas){

		nodoMetadata_t* nodo = malloc(sizeof(nodoMetadata_t));

		memcpy(&tamanioTabla, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);

		char* bufferTabla = malloc(tamanioTabla+1);
		memcpy(bufferTabla, buffer + desplazamiento, tamanioTabla);
		bufferTabla[tamanioTabla] = '\0';
		desplazamiento += tamanioTabla;

		metadata_t* metita = malloc(sizeof(metadata_t));

		memcpy(&metita->consistencia, buffer + desplazamiento, sizeof(TIPO_CONSISTENCIA));
		desplazamiento += sizeof(TIPO_CONSISTENCIA);

		memcpy(&metita->num_particiones, buffer + desplazamiento, sizeof(uint16_t));
		desplazamiento += sizeof(uint16_t);

		memcpy(&metita->tiempo_entre_compactaciones, buffer + desplazamiento, sizeof(uint16_t));
		desplazamiento += sizeof(uint16_t);

		nodo->metadata = metita;
		
		nodo->tabla = string_new();
		string_append(&nodo->tabla, bufferTabla);

		list_add(listaMetadatas,nodo);

		free(bufferTabla);
		
		i+=1;
	}

	return listaMetadatas;
}


void fsListarMetadata(metadata_t *m){

    switch (m->consistencia){
        case 1:
            printf("\nCONSISTENCIA: SC\n");
            log_info(logger,"CONSISTENCIA: SC");
            break;
        case 2: 
            printf("\nCONSISTENCIA: SHC\n");
            log_info(logger,"CONSISTENCIA: SHC");
            break;
        case 3:
            printf("\nCONSISTENCIA: EC\n");
            log_info(logger,"CONSISTENCIA: EC");
            break;
    }

    printf("PARTICIONES: %d\n", m->num_particiones);
    log_info(logger,"PARTICIONES: %d",m->num_particiones);
    
    printf("TIEMPO DE COMPACTACIÓN: %d\n", m->tiempo_entre_compactaciones);
    log_info(logger,"TIEMPO DE COMPACTACIÓN: %d",m->tiempo_entre_compactaciones);
}


void eliminarNodoMetadata(nodoMetadata_t* unNodo){
    free(unNodo->tabla);
	//free(unNodo->metadata);
    free(unNodo);
}


////////////////////////////////////////////////////REGISTRO////////////////////////////////////////////////////

void* serializarRegistro(void* registro, int* tamanio){

	registro_t* unRegistro = (registro_t*) registro;
	
	int desplazamiento = 0;
	int tamanioValor = string_length(unRegistro->value);

	*tamanio = sizeof(int) + sizeof(uint32_t) + sizeof(uint16_t) + tamanioValor;

	void* registroSerializado = malloc(*tamanio);

	memcpy(registroSerializado + desplazamiento, &tamanioValor,sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(registroSerializado + desplazamiento, unRegistro->value,tamanioValor);
	desplazamiento += tamanioValor;

	memcpy(registroSerializado + desplazamiento, &unRegistro->key, sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	memcpy(registroSerializado + desplazamiento, &unRegistro->timestamp, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);


	///////////////////////////////////////CASO DE PRUEBA SERIALIZACION REGISTRO////////////////////////////////////////////

	// printf("\n\nREGISTRO A SERIALIZAR: \n");
	// printf("\nValor: %s", unRegistro->value);
	// printf("\nKey: %d", unRegistro->key);
	// printf("\nTimestamp: %d", unRegistro->timestamp);
	// printf("\nTamaño del registro: %d", *tamanio);
	
	// registro_t* registroDeserializado = deserializarRegistro(registroSerializado);

	// printf("\n\nREGISTRO DESERIALIZADO: \n");
	// printf("\nValor: %s", registroDeserializado->value);
	// printf("\nKey: %d", registroDeserializado->key);
	// printf("\nTimestamp: %d", registroDeserializado->timestamp);

	// free(registroDeserializado);

	///////////////////////////////////////FIN CASO DE PRUEBA SERIALIZACION REGISTRO/////////////////////////////////////////

	return registroSerializado;
}

registro_t* deserializarRegistro(void* buffer){

	registro_t* unRegistro = malloc(sizeof(registro_t));

	int desplazamiento = 0;
	int tamanioValor = 0;

	memcpy(&tamanioValor, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* bufferValor = malloc(tamanioValor+1);
	memcpy(bufferValor, buffer + desplazamiento, tamanioValor);
	bufferValor[tamanioValor] = '\0';
	desplazamiento += tamanioValor;

	memcpy(&unRegistro->key, buffer + desplazamiento, sizeof(uint16_t));
	desplazamiento += sizeof(uint16_t);

	memcpy(&unRegistro->timestamp, buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	unRegistro->value = string_new();
	string_append(&unRegistro->value, bufferValor);

	free (bufferValor);

	return unRegistro;
}

void pedirTamanioMaximo(int socketServidor, int idCliente) {

  t_administrativo* administrativo = malloc(sizeof(t_administrativo));

  administrativo->codigo = 1;
  administrativo->valor = 0;

  int tamanioAdmin = 0;

  enviarInt(socketServidor, idCliente);
  enviarPaquete(socketServidor, tAdministrativo, administrativo, tamanioAdmin, logger);

  int resultado;
  int tipoResultado = 0;

  int* tipoMensaje = malloc(sizeof(int));
  int* tamanioMensaje = malloc(sizeof(int));

  if((resultado = recibirInt(socketServidor,&tipoResultado)) > 0){

    if(tipoResultado == 17){

      void* buffer = recibirPaquete(socketServidor, tipoMensaje, tamanioMensaje, logger);

      t_administrativo* tamanioMaximo = (t_administrativo*) buffer;

      TAM_MAX_VALUE = tamanioMaximo->valor;

      log_info(logger,"Recibimos el tamaño máximo de registro correctamente");

    }else if(tipoResultado == 18){

      log_info(logger,"No se recibió el tamaño máximo de registro correctamente");

    }
  }
  free(tipoMensaje);
  free(tamanioMensaje);
  free(administrativo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

t_list* pedirMetadatasAlServidor(int socketServidor, t_describe* unDescribe, int idCliente){

  //CHEQUEAR SI HAY QUE HACER FREE'S (CASI SEGURO QUE SI) DE unDescribe, tipoMensaje y tamanioMensaje (ya estan hechos ya hechos).

  int tamanioDescribe = 0;

  enviarInt(socketServidor, idCliente);
  enviarPaquete(socketServidor, tDescribe, unDescribe, tamanioDescribe, logger);

  int resultado;
  int tipoResultado = 0;

  int* tipoMensaje = malloc(sizeof(int));
  int* tamanioMensaje = malloc(sizeof(int));

  if((resultado = recibirInt(socketServidor,&tipoResultado)) > 0){

   if(tipoResultado == 15){

      void* buffer = recibirPaquete(socketServidor, tipoMensaje, tamanioMensaje, logger);
      t_list* listaMetadatas = (t_list*) buffer;

	  int cantMetadatas = 0;

	   cantMetadatas = list_size(listaMetadatas);

      for(int i = 0; i < cantMetadatas; i++){

        nodoMetadata_t* nodo = list_get(listaMetadatas,i);
        log_info(logger,"Tabla: %s",nodo->tabla);
        fsListarMetadata(nodo->metadata);
      }
      
    return listaMetadatas;

    }else if(tipoResultado == 16){

      log_info(logger,"No hay tablas en FS");
	  t_list* listaMetadatas = list_create();
      return listaMetadatas;

    }
  }
  free(tipoMensaje);
  free(tamanioMensaje);
  free(unDescribe);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

t_list* KeGetMemorias(int socketMemPrincipal ,int idKernel){

	int tamanioPedidoMemorias = 0;

	t_administrativo* unPedidoMemorias = malloc(sizeof(t_administrativo));

  	unPedidoMemorias->codigo = 2;
  	unPedidoMemorias->valor = 1;

  	enviarInt(socketMemPrincipal, idKernel);
  	enviarPaquete(socketMemPrincipal, tPedidoMemorias, unPedidoMemorias, tamanioPedidoMemorias, logger);

	int resultado;
  	int tipoResultado = 0;

  	int* tipoMensaje = malloc(sizeof(int));
  	int* tamanioMensaje = malloc(sizeof(int));

  	if((resultado = recibirInt(socketMemPrincipal,&tipoResultado)) > 0){

   		if(tipoResultado == 30){

        	void* buffer = recibirPaquete(socketMemPrincipal, tipoMensaje, tamanioMensaje, logger);
        	t_list* listaMemorias = (t_list*) buffer;

      		int cantMemorias = list_size(listaMemorias);

			for(int i = 0; i < cantMemorias; i++){

				t_gossip* nodo = list_get(listaMemorias,i);
				log_info(logger,"Puerto de la memoria: %d",nodo->puerto);
				log_info(logger,"Ip de la memoria: %s",nodo->ip);
				log_info(logger,"ID de la memoria: %d",nodo->id);
			}
      
    		return listaMemorias;

    	}else if(tipoResultado == 31){
			log_info(logger,"Error al recibir las memorias");
			t_list* listaMemorias = list_create();
			return listaMemorias;
    	}
  	}
  	free(tipoMensaje);
  	free(tamanioMensaje);
  	free(unPedidoMemorias);
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

registro_t* pedirRegistroAlServidor(int socketServidor, char* tabla, uint16_t clave, int idCliente) {

  //CHEQUEAR SI HAY QUE HACER FREE'S (CASI SEGURO QUE SI) DE unSelectCreado, tipoMensaje y tamanioMensaje (ya estan hechos ya hechos).

  int tamSelect = 0;
  t_select* unSelectCreado = malloc(sizeof(t_select));

  unSelectCreado->comando = string_new();
  string_append(&unSelectCreado->comando ,"SELECT");

  unSelectCreado->tabla = string_new();
  string_append(&unSelectCreado->tabla ,tabla);

  unSelectCreado->key = clave;

  enviarInt(socketServidor, idCliente);
  enviarPaquete(socketServidor, tSelect, unSelectCreado, tamSelect, logger);

  int resultado;
  int tipoResultado = 0;

  int* tipoMensaje = malloc(sizeof(int));
  int* tamanioMensaje = malloc(sizeof(int));

  if((resultado = recibirInt(socketServidor,&tipoResultado)) > 0){

    if(tipoResultado == 7){

      void* buffer = recibirPaquete(socketServidor, tipoMensaje, tamanioMensaje, logger);

      registro_t* unRegistro = (registro_t*) buffer;

      log_info(logger,"Valor del registro encontrado en FS: %s",unRegistro->value);
      log_info(logger,"Key del registro encontrado en FS: %d",unRegistro->key);
      log_info(logger,"Timestamp del registro encontrado en FS: %d",unRegistro->timestamp);

	  return unRegistro;

    }else if(tipoResultado == 8){

      log_info(logger,"No se encontró el registro con key %d en la tabla %s", unSelectCreado->key, unSelectCreado->tabla);
      return NULL;

    }
  }
  free(tipoMensaje);
  free(tamanioMensaje);
  free(unSelectCreado);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

valorRegistro_t* chequearValorEntreComillas(char** query, int tamMaxRegistro){

    int cantidadDeElementos = 0;
    int contador = 3;
    int posicionProximoParametro = 0;
    int hayComillaAbierta = 0;
    int hayComillaCerrada = 0;
    valorRegistro_t* valorRegistro = malloc(sizeof(valorRegistro_t));

    valorRegistro->proximoParametro = 0;
    valorRegistro->valor = string_new();
    string_append(&valorRegistro->valor,"");

    while(query[cantidadDeElementos] != NULL){
        cantidadDeElementos+=1;
    }

    for(contador; contador<cantidadDeElementos; contador++){

        if((string_starts_with(query[contador], "\"")) && (!string_ends_with(query[contador], "\""))){
            hayComillaAbierta = 1;
            string_append(&valorRegistro->valor,query[contador]);
        }else if((hayComillaAbierta == 1) && (hayComillaCerrada == 0) && (!string_ends_with(query[contador], "\""))){
            string_append_with_format(&valorRegistro->valor," %s",query[contador]);
        }else if((!string_starts_with(query[contador], "\"")) && (string_ends_with(query[contador], "\""))){
            hayComillaCerrada = 1;
            string_append_with_format(&valorRegistro->valor," %s",query[contador]);
            valorRegistro->proximoParametro = contador + 1;
        }else if((string_starts_with(query[contador], "\"")) && (string_ends_with(query[contador], "\""))){
            hayComillaAbierta = 1;
            hayComillaCerrada = 1;
            string_append(&valorRegistro->valor,query[contador]);
            valorRegistro->proximoParametro = contador + 1;
        }
    }

    if(string_length(valorRegistro->valor) > 0){
        char* aux = string_new();
        string_append(&aux,string_substring(valorRegistro->valor,1,string_length(valorRegistro->valor)-2));
        if(string_length(aux) <= tamMaxRegistro){
            valorRegistro->valor = string_new();
            string_append(&valorRegistro->valor, aux);
            free(aux);
        }else{
            free(aux);
            valorRegistro->valor = string_new();
            string_append(&valorRegistro->valor,"");
            valorRegistro->proximoParametro = 0;
        }
    }
    return valorRegistro;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int fsTransformarTipoConsistencia(char* tipoConsistencia){

    if(strcmp(tipoConsistencia,"SC") == 0){
        return 1;
    }else if (strcmp(tipoConsistencia,"SHC") == 0){
        return 2;
    }else{
        return 3;
    }  
    
}