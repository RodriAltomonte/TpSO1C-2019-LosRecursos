sin_color=\x1b[0m
verde=\x1b[32;01m
amarillo=\x1b[33;01m
negrita := $(shell tput bold)
sin_negrita := $(shell tput sgr0)

all:
	mkdir -p logs
	gcc -g -w FileSystem/fileSystem.c FileSystem/fileSystemFunciones.c libreriasCompartidas/sockets.c libreriasCompartidas/consola.c libreriasCompartidas/serializacion.c -o fileSystem -lcommons -lpthread
	@printf '$(negrita)$(amarillo)FileSystem$(sin_color) ..... $(verde)ok!$(sin_color)$(sin_negrita)\n'
	gcc -g -w Kernel/kernel.c Kernel/api.c Kernel/criterio.c Kernel/parser.c Kernel/thread_pool.c Kernel/planificador.c Kernel/metrics.c libreriasCompartidas/sockets.c libreriasCompartidas/consola.c libreriasCompartidas/serializacion.c -o kernel -lcommons -lpthread -lm
	@printf '$(negrita)$(amarillo)Kernel$(sin_color) ..... $(verde)ok!$(sin_color)$(sin_negrita)\n'

	gcc -g -w Memoria/memoria.c Memoria/memoriaFunciones.c libreriasCompartidas/sockets.c libreriasCompartidas/consola.c libreriasCompartidas/serializacion.c -o memoria -lcommons -lpthread
	@printf '$(negrita)$(amarillo)Memoria$(sin_color) ..... $(verde)ok!$(sin_color)$(sin_negrita)\n'

# Clean
clean:
	rm -f memoria kernel fileSystem *.o
	rm logs/fileSystem.log logs/memoria.log logs/kernel.log
