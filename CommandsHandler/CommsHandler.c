#include <stdlib.h>
#include <string.h>
#include "CommsHandler.h"

int buildMessage(void** out, const char type, const char* data)
{
    int dataSize = strlen(data); //Prendo la lunghezza dei dati
    char* newData = dataSize == 0 ? NULL : malloc(dataSize); //Se ho dati alloco lo spazio altrimenti assegno NULL
    strcpy(newData,data); //Copio i dati nella variabile temporanea
    int length = sizeof(dataSize) + sizeof(type) + dataSize; //Calcolo lo spazio effettivo da allocare per la struct

    //Alloco lo spazio per il messaggio e assegno i dati
    Message* message = malloc(length);
    message->length = dataSize;
    message->type = type;
    message->data = newData;

    *out = message; //Assegno ad out il puntatore per il messaggio
    return length; //Ritorno la dimensione effettiva del messaggio
}

char* getData(void* msg)
{
    Message* message = (Message*) msg;
    if(message->length == 0) return NULL;
    else return message->data;
}