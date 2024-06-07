#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "CommsHandler.h"
#include "../Utility/Utility.h"

int buildTextMsg(void** out, const char type, const void* data) //Funzione per costruire un messaggio di testo
{
    int dataSize = (data != NULL) ? strlen((const char*) data)+1 : 0; //Prendo la lunghezza dei dati (strlen + 1 per considerare il \0)
    int length = sizeof(dataSize) + sizeof(type) + dataSize; //Calcolo lo spazio effettivo da allocare per la struct

    //Alloco lo spazio per il messaggio e assegno i dati
    Message* message = malloc(length);
    message->length = dataSize;
    message->type = type;
    if(dataSize != 0) {
        memcpy(message->data, data, dataSize-1); //Copio i dati nello spazio di memoria che parte dalla locazione di message->data se sono presenti dati
        message->data[dataSize-1] = '\0'; //Aggiungi il null termination character alla fine della stringa
    }

    *out = message; //Assegno a out il puntatore per il messaggio
    return length; //Ritorno la dimensione effettiva del messaggio
}

int buildNumMsg(void** out, const char type, long num) //Funzione per costruire un messaggio numerico
{
    int dataSize = sizeof(num); //Prendo la lunghezza dei dati
    int length = sizeof(dataSize) + sizeof(type) + dataSize; //Calcolo lo spazio effettivo da allocare per la struct

    //Alloco lo spazio per il messaggio e assegno i dati
    Message* message = malloc(length);
    message->length = dataSize;
    message->type = type;
    memcpy(message->data, (char*)&num, dataSize); //Copio i dati nello spazio di memoria che parte dalla locazione di message->data

    *out = message; //Assegno a out il puntatore per il messaggio
    return length; //Ritorno la dimensione effettiva del messaggio
}

char** getMatrix(const char* data) //Funzione per estrarre una matrice in un char** a partire da dati char* in un messaggio
{
    char** matrix = (char**) malloc(MATRIX_SIZE * sizeof(char*)); //Alloco lo spazio per la nuova matrice [righe]

    for(int i=0; i < MATRIX_SIZE; i++)
    {
        matrix[i] = (char*) malloc(MATRIX_SIZE * sizeof(char)); //Alloco lo spazio per la nuova matrice [colonne]
        for(int j=0; j < MATRIX_SIZE; j++)
        {
            matrix[i][j] = data[i * MATRIX_SIZE + j]; //Assegno a ogni posizione il valore corrispondente nella lista dati
        }
    }
    return matrix;
}

long getNumber(const char* data) //Funzione per estrarre un long a partire da un char* in un messaggio
{
    return *(long*)(data); //Cast a long e dereference dei dati
}

void sendTextMessage(int fd, const char type, const void* data) //Funzione per inviare un messaggio di testo tramite un file descriptor
{
    void* msg;
    int msg_size = buildTextMsg(&msg, type, data); //Creo il messaggio di testo
    write(fd, msg, msg_size); //Invio il messaggio attraverso il fd (eventuali errori nel canale di comunicazione vengono gestiti dalla read)
    deleteMessage(msg);
}

void sendNumMessage(int fd, char type, long message) //Funzione per inviare un messaggio numerico tramite un file descriptor
{
    void* msg;
    int msg_size = buildNumMsg(&msg, type, message); //Creo il messaggio numerico
    write(fd, msg, msg_size); //Invio il messaggio attraverso il fd (eventuali errori nel canale di comunicazione vengono gestiti dalla read)
    deleteMessage(msg);
}

Message* readMessage(int fd) //Funzione per leggere i campi di un messaggio
{
    //read_fails() in caso di errore lettura, quindi probabile crash del client

    int size;
    if(read_fails(fd, &size, sizeof(int))) return NULL; //Leggo i primi sizeof(int) byte dal fd, che corrispondono al primo elemento della struct, cioè la lunghezza dei dati

    Message* msg = malloc(sizeof(int) + sizeof(char) + size); //Alloco lo spazio necessario per leggere tutto il messaggio
    msg->length = size; //Definisco la lunghezza dei dati nella struct
    if(read_fails(fd, &msg->type, sizeof(char))) //Leggo i secondi sizeof(char) byte dal fd, che corrispondo al tipo del messaggio
    {
        free(msg);
        return NULL;
    };
    if(msg->length > 0) { //Leggo il resto del messaggio, cioè i dati allocati in coda
        if(read_fails(fd, &msg->data, size))
        {
            free(msg);
            return NULL;
        }
    }

    return msg;
}

void deleteMessage(Message* msg){ //Funzione per liberare la memoria di un messaggio
    free(msg); //Libero la memoria occupata dal messaggio
}