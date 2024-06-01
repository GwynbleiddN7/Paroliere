#include <errno.h>

//Controllo per chiamate di sistema che ritornano -1 come errore
#define SYSC(v,c,m) if((v=c)==-1){ perror(m); exit(errno); }

//Controllo per chiamate di sistema che ritornano NULL come errore
#define SYSCN(v,c,m) if((v=c)==NULL){ perror(m);exit(errno); }