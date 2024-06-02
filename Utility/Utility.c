#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "Utility.h"

bool regularFileExists(const char* file)
{
    struct stat statbuf;
    if(stat(file,  &statbuf) == -1) return false; //Controllo se il file esiste
    if(!S_ISREG(statbuf.st_mode)) return false; //Controllo se il file è un file regolare
    return true;
}

void exitMessage(char* message)
{
    printf("%s", message); //Esco con un messaggio
    fflush(NULL); //Flush del buffer output
    exit(EXIT_FAILURE);
}

void copyString(char** dest, char* src)
{
    if(*dest != NULL) free(*dest); //Libero la memoria in caso non fosse NULL
    *dest = malloc(strlen(src)); //Alloco lo spazio per la nuova stringa
    strcpy(*dest, src); //Copio la stringa
}

bool strToInt(char* src, int* out)
{
    char* end;
    int num = strtol(src, &end, 10); //Converto la stringa in intero base 10
    if(*end != '\0') return false; //Se è rimasto qualcosa da convertire (non era una cifra) esco
    *out = num; //Ritorno per reference il numero
    return true;
}

bool validatePort(char* portString, int* portInt)
{
    strToInt(portString, portInt); //Converto in intero
    //Controllo che sia una porta nel range valido
    if(*portInt > 65535  || *portInt < 1024) return false;
    return true;
}