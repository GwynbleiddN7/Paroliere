#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "Utility.h"

bool regularFileExists(const char* file) //Funzione per controllare l'esistenza di un file
{
    struct stat stats;
    if(stat(file,  &stats) == -1) return false; //Controllo se il file esiste
    if(!S_ISREG(stats.st_mode)) return false; //Controllo se il file è un file regolare
    return true;
}

void exitWithMessage(char* message) //Funzione per terminare il programma con un messaggio
{
    printf("%s", message); //Scrivo un messaggio di errore
    exit(EXIT_FAILURE); //Esco
}

void copyString(char** dest, char* src) //Funzione per creare una stringa con il valore di un'altra
{
    if(*dest != NULL) free(*dest); //Libero la memoria in caso non fosse NULL
    *dest = malloc(strlen(src)+1); //Alloco lo spazio per la nuova stringa e il \0
    strcpy(*dest, src); //Copio la stringa
}

bool strToInt(char* src, int* out) //Funzione per convertire stringa in intero
{
    char* end;
    int num = strtol(src, &end, 10); //Converto la stringa in intero base 10
    if(*end != '\0') return false; //Se è rimasto qualcosa da convertire (non era una cifra) esco
    *out = num; //Ritorno per reference il numero
    return true;
}

bool validatePort(char* portString, int* portInt) //Funzione per validare la porta
{
    strToInt(portString, portInt); //Converto in intero
    return (*portInt >= 1024 && *portInt <= 65535); //Controllo che sia una porta nel range valido
}