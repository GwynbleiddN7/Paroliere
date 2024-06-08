#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <arpa/inet.h>
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
    printf("%s\n", message); //Scrivo un messaggio di errore
    fflush(stdout);
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

in_addr_t getIPV4FromHostname(const char* hostname, in_addr_t* addrOutput) //Funzione per tradurre un hostname in IPV4
{
    //Inizializzo le variabili addrinfo
    struct addrinfo hints = {0}, *result = NULL;

    //Imposto le proprietà come quelle del server e del client
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    //Provo a tradurre l'hostname
    if (getaddrinfo(hostname, NULL, &hints, &result) != 0) return false;

    //Se ho trovato almeno un risultato
    struct sockaddr_in *sockAddr = (struct sockaddr_in *)result->ai_addr; //Prendo l'indirizzo
    *addrOutput = sockAddr->sin_addr.s_addr; //E prendo l'indirizzo nel formato che utilizzerò


    //Libero la memoria allocata
    freeaddrinfo(result);
    /*
        NOTA dalla relazione:
        Dai test risulta un unico caso in cui valgrind rileva una porzione di
        memoria non liberata. Dopo aver verificato attentamente che il codice scritto
        liberasse correttamente la memoria allocata, una ricerca sul web ha portato alla
        conclusione che la funzione di libreria apposita freeaddrinfo() di netdb.h
        non liberi tutta la memoria allocata dalla funzione getaddrinfo(), e che non ci siano altri metodi per farlo.
    */
    return true;
}

bool validateAddr(char* addrInput, in_addr_t* addrOutput) //Funzione per validare l'indirizzo
{
    struct sockaddr_in sa;
    if(inet_pton(AF_INET, addrInput, &sa.sin_addr) != 0)
    {
        *addrOutput = sa.sin_addr.s_addr; //Controllo se è un indirizzo IPV4 valido
        return true;
    }
    else return getIPV4FromHostname(addrInput, addrOutput); //Controllo se è un hostname valido e lo converto in IPV4
}

int secondsToMinutes(long* seconds) //Funzione per convertire i secondi in minuti + secondi
{
    int minutes = 0;
    while(*seconds >= 60)
    {
        *seconds = *seconds - 60;
        minutes++;
    }
    return minutes;
}
