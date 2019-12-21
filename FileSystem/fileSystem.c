#include "fileSystem.h"

int32_t main() {

    pthread_t hiloConsola;
	pthread_t hiloServidor;
	pthread_t hiloDump;
	pthread_t hiloCompactaciones;
	pthread_t hiloArchivoConfig;
	
	inicializar();

	char* pathTablas = string_new();
	string_append_with_format(&pathTablas,"%sTables", procesoConfig->puntoMontaje);
	
	pthread_create(&hiloCompactaciones, NULL, (void*)fsIniciarCompactacionesTablasExistentes, (void*)pathTablas);
	pthread_create(&hiloDump, NULL, (void*)fsDump, NULL);
    pthread_create(&hiloConsola, NULL, (void*)ConsolaMain, NULL);
	pthread_create(&hiloArchivoConfig, NULL, (void*)actualizarConfiguracionFS, NULL);

	infoServidor_t * unaInfoServidor = malloc(sizeof(infoServidor_t));
	unaInfoServidor->log = logger;
	unaInfoServidor->puerto = procesoConfig->puertoEscucha;
	unaInfoServidor->ip = string_new();
	string_append(&unaInfoServidor->ip,"0");
	
	pthread_create(&hiloServidor,NULL,(void*)servidor_inicializar,(void*)unaInfoServidor);
	pthread_join(hiloConsola, NULL);
	finalizar();
	return 0;

}

void inicializar() {

	configurarLoggerFS();
    log_info(logger, "- LOG INICIADO");

    procesoConfig = cargarConfiguracionFS(configArchivoConfiguracion, configMetadata);
    log_info(logger, "- PROCESO CONFIGURADO CON EXITO!");
	
	MEMTABLE = list_create();
	hilosCompactador = list_create();

	inicializarBitMap();
	crearBloquesFileSystem();

    builtins_t builtins[] = {
								{"SELECT",&fsSelect},
	 							{"INSERT",&fsInsert},
	 							{"CREATE",&fsCreate},
	 							{"DESCRIBE",&fsDescribe},
								{"DROP",&fsDrop}
							};
    ConsolaInicializar("\nFile System>",builtins, CANTIDAD_DE_COMANDOS_CONSOLA_FS);
}

void finalizar() {
	free(procesoConfig);
	free(logger);
}

void actualizarConfiguracionFS(){
	while(1){
		sleep(10);
		FILE *fp = fopen(LFS_CONFIG_PATH,"rb");
		unsigned char checksum = 0;
		while (!feof(fp) && !ferror(fp)) {
   			checksum ^= fgetc(fp);
		}
		fclose(fp);

		if (checksum != valorArchivoConfig) {

			procesoConfig = cargarConfiguracionFS(configArchivoConfiguracion, configMetadata);
			valorArchivoConfig = checksum;

		}

	}

}

