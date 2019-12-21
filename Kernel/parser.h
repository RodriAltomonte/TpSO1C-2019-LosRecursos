#ifndef _PARSER_H_
#define _PARSER_H_

/*
 * @Author: Juan Manuel Canosa
 * @Description: Parser para archivos LQL
 */

#include <stdio.h>
#include <commons/log.h>
#include <stdlib.h>

typedef FILE* LQLFile;

#define SELECT 23036
#define RUN 3211
#define INSERT 40313 
#define CREATE 10748
#define DROP 32207
#define ADD 193
#define METRICS 30947
#define JOURNAL 9687
#define DESCRIBE 15787

/*
 * @brief: Cuando se ejecuta el comando RUN se abre un archivo mediante esta
 *		   funcion para ser parseado y mandar los requests a memoria
 * @params filename: Nombre del archivo a abrir
 * @return: Exito al abrir(1) o fallo al abrir(0)
 */
int PaAbrirArchivoLQL(const char *filename);

/*
 * @brief: Parsea un archivo con scripts y verifica que no contenga errores
 * @params: Nada
 * @return: Lista de lineas del archivo
 */
char ** PaParseArchivoLQL();

/*
 * @brief: Cierra el archivo actualmente abierto
 * @params: Nada
 * @return: Nada
 */
void PaCerrarArchivoLQL();

/*
 * @brief: Parsea una linea y chequea que la estructura este bien y luego ejecuta el request
 * @params linea: Linea a parsear
 * @return : 1->OK , 0->Error
 */
int PaParseLineaYEjecutar(char *linea);

/*
 * @brief: Convierte una linea en una lista de argumentos
 * @params linea: Linea a convertir
 * @return: Array de argumentos
 */
char** PaParseArgs(char *linea);

#endif
