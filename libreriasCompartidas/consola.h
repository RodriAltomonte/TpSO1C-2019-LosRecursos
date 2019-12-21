#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#define CONSOLE_TOK_BUFSIZE 32
#define CONSOLE_TOK_DELIM " "
#define CONSOLE_MEM_ALLOC_ERR_MSG "Consola: Error en malloc!\n"

typedef struct builtins_s
{
	char *builtin_nombre;
	int (*builtin_func)(char**);
}builtins_t;

void ConsolaInicializar(const char *nombre ,builtins_t builtins[], int cantidadDeComandos);

/*
 * @brief: Lee una linea de stdin
 * @params name: String para imprimir en pantalla
 * @return: Puntero a string leida
 */
char *ConsolaLeerLinea(const char *name);

/*	@brief: Loop que lee los comandos introducidos en stdin y los ejecuta
 *	@params: Nada
 *	@return: Nada
 */
void ConsolaMain();

/*
 *	@brief: Funcion que permite leer los parametros de un comando recibido por stdin
 *	@param line: puntero que contiene el string del comando
 *	@return: array de parametros terminado por un NULL
 */
char **ConsolaParsearArgs(char *line);


/*
 *	@brief: Calcula la cantidad de argumentos que tiene un comando
 *	@param args: Array con los argumentos
 *	@return : La cantidad de argumentos
 */
unsigned short int ConsolaGetArgc(char **args);

/*
 *	@brief:Ejecuta un comando introducido por stdin
 *	@param args: Array que contiene los argumentos del request
 *	@return: 1 si se ejecuto correctamente 0 si no
 */
unsigned short int ConsolaEjecutarComando(char **args, int cantidadDeComandos);

/*
  *
  */
void ConsolaAgregarBuiltins(builtins_t[], int cantidadDeComandos);

////////////////////////////////////////////////////////////////////CONSOLA VIEJA/////////////////////////////////////////////////////////////////

char *leeLinea(const char *name);
char **parseaLinea(char *line);
unsigned short int console_get_argc(char **args);
unsigned short int ejecutarConsola(char **args);


#endif