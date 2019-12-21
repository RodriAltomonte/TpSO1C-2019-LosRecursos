#ifndef _PLANIFICADOR_H
#define _PLANIFICADOR_H_

/*
 *	Author : Juan Manuel Canosa
 *	Description : Planificador Round Robin del Kernel
 */

#define USHORT unsigned short int 

void PlInicializar(int,int,int);
void PlFinalizar();
void PlCrearTrabajos(char**);
void PlanificadorRestart(int,int,int);
#endif
