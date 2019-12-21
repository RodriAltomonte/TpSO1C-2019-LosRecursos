#include "consola.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

builtins_t Builtins[8];

#define BUILTIN(I) (Builtins[I])
#define BUILTIN_NOMBRE(I) BUILTIN(I).builtin_nombre
#define BUILTIN_FUNC(I,ARGS) BUILTIN(I).builtin_func(args)

char* consolaNombre; //char* ConsolaNombre = NULL;
int ConsolaCantidadDeComandos;

void ConsolaInicializar(const char* nombre, builtins_t bs[], int cantidadDeComandos)
{
	consolaNombre = nombre;
	ConsolaCantidadDeComandos = cantidadDeComandos;
	ConsolaAgregarBuiltins(bs, ConsolaCantidadDeComandos);
}

void ConsolaMain()
{
	char *line;
	char **args;
   
start:
	line = ConsolaLeerLinea(consolaNombre);
	args = ConsolaParsearArgs(line);
    
    if(*args == NULL || strcmp(args[0]," ") == 0)
    {
        free(line);
        free(args);
        goto start;
    }
 
	while(strcmp(args[0],"exit"))
	{
		ConsolaEjecutarComando(args, ConsolaCantidadDeComandos);
		free(args);
		free(line);

		line = ConsolaLeerLinea(consolaNombre);
		args = ConsolaParsearArgs(line);

		  if(*args == NULL || strcmp(args[0]," ") == 0)
   		 {
        		free(line);
        		free(args);
        		goto start;
   		 }
	}

	free(line);
	free(args);
	//exit(0);
	return;
}	

char **ConsolaParsearArgs(char *line)
{
	uint8_t bufsize = CONSOLE_TOK_BUFSIZE;
	unsigned short int position = 0;
	char **tokens = malloc(bufsize*sizeof(char*));
	char* token;

	if(!tokens)
	{
		fprintf(stderr,CONSOLE_MEM_ALLOC_ERR_MSG);
		exit(EXIT_FAILURE);
	}		
	token = (char*) strtok(line,CONSOLE_TOK_DELIM);
	while(token != NULL)
	{
		tokens[position] = token;
		position++;
		
		//realocar el buffer si se nos quedo chico	
		if(position >= bufsize )
		{
			bufsize += CONSOLE_TOK_BUFSIZE;
			tokens = realloc(tokens,bufsize * sizeof(char*));
			if(!tokens)
			{
				fprintf(stderr,CONSOLE_MEM_ALLOC_ERR_MSG);
				exit(EXIT_FAILURE);
			}	
		}
			token = (char*) strtok(NULL,CONSOLE_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;

}

unsigned short int ConsolaGetArgc(char **args)
{
			uint8_t cnt = 0;
			for(cnt = 0 ; args[cnt] != NULL; cnt++);
			return cnt;
}

char *ConsolaLeerLinea(const char *name)
{
		int bufsize= CONSOLE_TOK_BUFSIZE;
		int position = 0;
		char *buffer = malloc(sizeof(char)*bufsize);
		int c;

		if(!buffer)
		{
			fprintf(stderr,CONSOLE_MEM_ALLOC_ERR_MSG);
			exit(EXIT_FAILURE);
		}

			printf("%s",name);

		while(1)
		{
			c = getchar();

			if(c == EOF || c== '\n')
			{
				buffer[position] = '\0';
				return buffer;
			}else
			{
				buffer[position] = c;
			}
			position++;
		}

		//Si nos excedimos del tamaño del buffer ,realocar
		if(position >=bufsize)
		{
			bufsize+=CONSOLE_TOK_BUFSIZE;
			buffer = realloc(buffer,bufsize);
			if(!buffer)
			{
				fprintf(stderr,CONSOLE_MEM_ALLOC_ERR_MSG);
				exit(EXIT_FAILURE);
			}
		}
}

unsigned short int ConsolaEjecutarComando(char **args, int ConsolaCantidadDeComandos)
{
		uint16_t index = 0;
		//puts("Ejecutando comando\n");
		//for(index=0;index<sizeof(Builtins)/sizeof(Builtins[4]);index++)
		for(index=0;index<ConsolaCantidadDeComandos;index++)
		{
			if(strcmp(BUILTIN_NOMBRE(index),args[0]) == 0)
			{
				//printf("Llamado el comando : %s\n",BUILTIN_NOMBRE(index));
				BUILTIN_FUNC(index,args);
				return 0;
			}
			
		}	
	printf("Comando no reconocido %s \n",args[0]);
		return 1;
}

void ConsolaAgregarBuiltins(builtins_t bs[], int ConsolaCantidadDeComandos)
{
	uint8_t i = 0;
	for(i=0; i<ConsolaCantidadDeComandos;i++)
	{
			Builtins[i].builtin_nombre = bs[i].builtin_nombre;
			Builtins[i].builtin_func= bs[i].builtin_func;
	}
}


/////////////////////////////////////////////////////////////////////CONSOLA VIEJA//////////////////////////////////////////////////////////

char **parseaLinea(char *linea) {

	uint8_t tamanioBuffer = CONSOLE_TOK_BUFSIZE;
	unsigned short int posicion = 0;
	char **tokens = malloc(tamanioBuffer * sizeof(char*));
	char *token;

	if (!tokens) {
		fprintf(stderr, CONSOLE_MEM_ALLOC_ERR_MSG);
		exit(EXIT_FAILURE);
	}	

	token = (char*) strtok(linea, CONSOLE_TOK_DELIM);
	
	while (token != NULL) {

		tokens[posicion] = token;
		posicion++;
		
		//realocar el buffer si se nos quedo chico	
		if (posicion >= tamanioBuffer) {
			tamanioBuffer += CONSOLE_TOK_BUFSIZE;
			tokens = realloc(tokens, tamanioBuffer * sizeof(char*));
			if (!tokens) {
				fprintf(stderr, CONSOLE_MEM_ALLOC_ERR_MSG);
				exit(EXIT_FAILURE);
			}	
		}

		token = (char*) strtok(NULL, CONSOLE_TOK_DELIM);
	
	}
	
	tokens[posicion] = NULL;
	return tokens;

}

unsigned short int console_get_argc(char **args) {

	unsigned short int cnt = 0;
	for (cnt = 0 ; args[cnt] != NULL; cnt++);
	return cnt;

}

char *leeLinea(const char *nombre) {

	int tamanioBuffer= CONSOLE_TOK_BUFSIZE;
	int posicion = 0;
	char *buffer = malloc(tamanioBuffer * sizeof(char));
	int c;

	if (!buffer) {
		fprintf(stderr, CONSOLE_MEM_ALLOC_ERR_MSG);
		exit(EXIT_FAILURE);
	}

	printf("%s", nombre);

	while(1) {
	
		c = getchar();

		if(c == EOF || c == '\n') {
			buffer[posicion] = '\0';
			return buffer;
		} else {
			buffer[posicion] = c;
		}
		posicion++;
	}
		
	//Si nos excedimos del tamaño del buffer ,realocar
	if (posicion >= tamanioBuffer) {
		tamanioBuffer += CONSOLE_TOK_BUFSIZE;
		buffer = realloc(buffer, tamanioBuffer);
		if (!buffer) {
			fprintf(stderr, CONSOLE_MEM_ALLOC_ERR_MSG);
			exit(EXIT_FAILURE);
		}
	}

}

unsigned short int ejecutarConsola(char **args) {

	//TODO
	return 1;

}