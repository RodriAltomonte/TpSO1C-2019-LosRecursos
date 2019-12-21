#ifndef _API_H_
#define _API_H

#define bool int
int KeSelect(char**);
int KeMetrics(char**);
int KeInsert(char**);
int KeCreate(char**);
int KeDescribe(char**);
int KeDrop(char**);
int KeJournal(char**);
int KeAdd(char**);
int KeRun(char**);
bool esLaTablaBuscada(void* nodi);

char * tablaMetadata;

#endif
