#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
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

char** getMatrix(const char* data)
{
    char** matrix = (char**) malloc(MATRIX_SIZE * sizeof(char*));

    for(int i=0; i < MATRIX_SIZE; i++)
    {
        matrix[i] = (char*) malloc(MATRIX_SIZE * sizeof(char));
        for(int j=0; j < MATRIX_SIZE; j++)
        {
            matrix[i][j] = data[i * MATRIX_SIZE + j];
        }
    }
    return matrix;
}

long getNumber(const char* data)
{
    return *(long*)(data);
}

void sendTextMessage(int fd, const char type, const void* data)
{
    void* msg;
    int msg_size = buildTextMsg(&msg, type, data);
    SYSCALL(write(fd, msg, msg_size), "Errore nell'invio di un messaggio");
}

void sendNumMessage(int fd, char type, long message)
{
    void* msg;
    int msg_size = buildNumMsg(&msg, type, message);
    SYSCALL(write(fd, msg, msg_size), "Errore nell'invio di un messaggio");
}

Message* readMessage(int fd)
{
    int size;
    SYSCALL(read(fd, &size, sizeof(int)), "Errore nella lettura di un messaggio");
    Message* msg = malloc(size + sizeof(int) + sizeof(char));
    msg->length = size;
    SYSCALL(read(fd, &msg->type, sizeof(char)), "Errore nella lettura di un messaggio");
    if(msg->length > 0) SYSCALL(read(fd, &msg->data, msg->length), "Errore nella lettura di un messaggio");
    return msg;
}