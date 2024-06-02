#include <stdlib.h>
#include <string.h>
#include "CommsHandler.h"
#include "../Utility/Utility.h"

int buildTextMsg(void** out, const char type, const void* data)
{
    int dataSize = (data != NULL) ? strlen((const char*) data) : 0; //Prendo la lunghezza dei dati
    int length = sizeof(dataSize) + sizeof(type) + dataSize; //Calcolo lo spazio effettivo da allocare per la struct

    //Alloco lo spazio per il messaggio e assegno i dati
    Message* message = malloc(length);
    message->length = dataSize;
    message->type = type;
    if(data != NULL) memcpy(message->data, data, dataSize);

    *out = message; //Assegno ad out il puntatore per il messaggio
    return length; //Ritorno la dimensione effettiva del messaggio
}

int buildNumMsg(void** out, const char type, long num)
{
    int dataSize = sizeof(num); //Prendo la lunghezza dei dati
    int length = sizeof(dataSize) + sizeof(type) + dataSize; //Calcolo lo spazio effettivo da allocare per la struct

    //Alloco lo spazio per il messaggio e assegno i dati
    Message* message = malloc(length);
    message->length = dataSize;
    message->type = type;
    memcpy(message->data, (char*)&num, dataSize);

    *out = message; //Assegno ad out il puntatore per il messaggio
    return length; //Ritorno la dimensione effettiva del messaggio
}

char* getMessage(Message* message)
{
    char* msg;
    copyString(&msg, message->data);
    return msg;
}

char** getMatrix(Message* message)
{
    char** matrix = (char**) malloc(MATRIX_SIZE * sizeof(char*));

    if(message->length == sizeof(char)*16)
    {
        for(int i=0; i < MATRIX_SIZE; i++)
        {
            matrix[i] = (char*) malloc(MATRIX_SIZE * sizeof(char));
            for(int j=0; j < MATRIX_SIZE; j++)
            {
                matrix[i][j] = message->data[i * MATRIX_SIZE + j];
            }
        }
        return matrix;
    }
    else return NULL;
}

long getNumber(Message* message)
{
    if(message->length == sizeof(long)) return *(long*)(message->data);
    else return 0;
}