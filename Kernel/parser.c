#include "parser.h"
#include "api.h"
#include <inttypes.h>
#include "kernel.h"

#define HASH(STR) HashStr(STR)

//CUANDO TERMINARON SU CUANTUM SE PONEN EN READY Y FINALIZAN DEL ARCHIVO PASAR LAS LINEAS DEL SCRIPT A CADA
//THREAD MEDIANTE LOS JOBS(job_t->args);

LQLFile ArchivoLQL;
char **pScript=NULL;

uint16_t HashStr(const char*);
unsigned short int argc(char **);

#ifdef DEBUG
void _debug_print_(char**);
#endif

int ParseSelect(char **);
valorRegistro_t* ParseInsert(char **);
int ParseCreate(char **);
int ParseDrop(char **);
int ParseJournal(char **);
int ParseDescribe(char **);
int ParseAdd(char **);
int ParseRun(char **);
int ParseMetrics(char **);
//unsigned short int argc(char **args);

void PaFinalizar()
{
	int n=0;
	if(pScript == NULL)
	{
		return;
	}
	while(pScript[n] != NULL)
	{
		free(pScript[n]);
		n++;
	}
	free(pScript);
}
int PaAbrirArchivoLQL(const char *filename)
{
	pScript = malloc(sizeof(char*)*1024);
	ArchivoLQL = fopen(filename,"r");	
	if(ArchivoLQL == NULL)
	{
		return 0;
	}
	return 1;
}

void PaCerrarArchivoLQL()
{
	if(ArchivoLQL != NULL)
	{
		fclose(ArchivoLQL);
		return;
	}
}

char **PaParseArchivoLQL()
{
	char *line = NULL;
	size_t len = 0 ;
	ssize_t lectura = 0;
	int lnctr=0;
	int n = 0;

	while( (lectura = getline(&line,&len,ArchivoLQL)) != EOF)
	{
		pScript[lnctr] = malloc(strlen(line) + 1);
		strcpy(pScript[lnctr],line);
		lnctr++;
	}
	free(line);
	pScript[lnctr] = NULL;
	return pScript;	 
}

int PaParseLineaYEjecutar(char *linea)
{
		char **args=PaParseArgs(linea);
		
		switch(HASH(args[0]))
		{
			case SELECT:
				if(!ParseSelect(args))
				{
					if(!KeSelect(args))
					{
						return 0;
					}
				}
				free(args);
				break;
			case RUN:
				if(!ParseRun(args))
				{
					if(!KeRun(args))
					{
						return 0;
					}
				}
				free(args);
				break;
			case INSERT:
				if(ParseInsert(args) == NULL)
				{
					if(!KeInsert(args))
					{
						return 0;
					}
				}
				free(args);
				break;
			case CREATE:
				if(!ParseCreate(args))
				{
					if(!KeCreate(args))
					{
						return 0;
					}
				}
				free(args);
				break;
			case DROP:
				if(!ParseDrop(args))
				{
					if(!KeDrop(args))
					{
						return 0;
					}
				}
				free(args);
				break;
			case ADD:
				if(!ParseAdd(args))
				{
					if(!KeAdd(args))
					{
						return 0;
					}
				}
				free(args);
				break;
			case METRICS:
				if(!ParseMetrics(args))
				{
					if(!KeMetrics(args))
					{
						return 0;
					}
				}
				free(args);
				break;
			case JOURNAL:
				if(!ParseJournal(args))
				{
					if(!KeJournal(args))
					{
						return 0;
					}
				}
				free(args);
				break;
			case DESCRIBE:
				if(!ParseDescribe(args))
				{
					if(!KeDescribe(args))
					{
						return 0;
					}
				}
				free(args);
				break;
			default:
				break;
		}
		return 1;

}

char** PaParseArgs(char *linea)
{
	char **args=malloc(sizeof(char**)*64);
	int index=0;

	while(isspace(*linea)) linea++;
	while(*linea)
	{
		if(args) args[index] = linea;
		while(*linea && !isspace(*linea)) ++linea;
		if(args && *linea) *linea++ = '\0';
		while(isspace(*linea)) linea++;
		index++;
	}
	args[index] = NULL;
	return args;
}

#ifdef DEBUG
void __debug__print(char** args)
{
	int index=0;
	while(args[index]!=NULL)
	{
		printf("%s\n",args[index]);
		index++;
	}
}
#endif

//sdbm hash
uint16_t HashStr(const char *str)
{
	unsigned long hash = 0;
	int ch;

	while( ch = *str++)
	{
		hash = ch + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

unsigned short int FinalizaConComillas(char *str)
{
	//printf("str->%s",str);
	return str[strlen(str)-1] == '"';
}
unsigned short int argc(char **args)
{
	unsigned short int counter = 0;
	unsigned short int quotes = 0;
	unsigned short int argcount = 0;

	while(args[counter] != NULL)
	{
		if(args[counter][0] == '"')
		{
			while(!FinalizaConComillas(args[counter]))
			{
				quotes++;
				counter++;
			}	
		}
		counter++;
	}
	//printf("quotes-> %d",quotes);
	//counter = counter - (quotes+1);
	//printf("counter-> %d",counter);
	argcount = counter - quotes;
	return argcount;	
}

int ParseSelect(char **args)
{
	unsigned short int argcount = argc(args);
	
	if(argcount != 3 )
	{
		printf("\nNumero de argumentos erroneo!\n");
		return 0;
	}
	return 1;
}

valorRegistro_t* ParseInsert(char **args)
{
	unsigned short int argcount = argc(args);
	//printf("argc-> %d",argcount);
	if(!(argcount >=4 && argcount <=5) )
	{
		printf("\nNumero de argumentos erroneo!\n");
		return NULL;
	}

	valorRegistro_t* valorRegistro = chequearValorEntreComillas(args, TAM_MAX_VALUE);

	if(!(valorRegistro->proximoParametro > 0)){

        log_warning(logger,"EL VALOR DEBE INGRESARSE ENTRE COMILLAS Y NO DEBE SUPERAR EL MAXIMO DE CARACTERES PERMITIDO.");
		return NULL;

    }else{

		return valorRegistro;

	}
}

int ParseCreate(char **args)
{
	if(argc(args) < 5)
	{
		return 0;
	}
}
int ParseDescribe(char **args)
{
	unsigned short int argcount = argc(args);
	if(!(argcount>=1 && argcount <= 2))
	{
		return 0;
	}
	return 1;
}
int ParseDrop(char **args)
{
	if(argc(args) < 2)
	{
		return 0;
	}
	return 1;
}
int ParseJournal(char **args)
{
	if(argc(args) != 1)
	{
		return 0;
	}
	return 1;
}
int ParseRun(char **args)
{
     if(argc(args) < 2)
	 {
		return 0;
	 }
        return 1;
}
int ParseAdd(char **args)
{
    if(argc(args) < 5)
	{
		log_info(logger,"Error argumentos");
		return 0;
	}
	if(atoi(args[2]) <= list_size(lista_memorias) && atoi(args[2]) >= 0  )
	{
		return 1;
	}else
	{
		log_info(logger,"Error numero memoria ");
		return 0;
	}
	

}
int ParseMetrics(char **args)
{
     if(argc(args))
	{
		return 0;
	}
	return 1;
}

