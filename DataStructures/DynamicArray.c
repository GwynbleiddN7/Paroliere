#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../Utility/Utility.h"
#include "DynamicArray.h"

StringList* createStringArray() //Funzione per creare e allocare un array dinamico
{
    //Alloco la memoria necessaria e inizializzo l'array
    StringList* array = malloc(sizeof(StringList));
    array->size = 0;
    array->strings = NULL;
    return array;
}

void addStringToArray(StringList* array, char* string) //Funzione per aggiungere un elemento all'array
{
    if(string == NULL) return;
    array->size++; //Incremento la dimensione dell'array
    array->strings = realloc(array->strings, array->size * sizeof(char*)); //Ri-alloco lo spazio necessario
    array->strings[array->size-1] = NULL; //Inizializzo il nuovo blocco di memoria
    copyString(&array->strings[array->size-1], string); //Copio il valore della stringa nell'array
}

void freeStringArray(StringList* array) //Funzione per liberare la memoria occupata dall'array
{
    if(array == NULL) return;
    for(int i=0; i<array->size; i++)
    {
        if(array->strings[i] == NULL) continue;
        free(array->strings[i]); //Libero la memoria occupata dall'elemento
    }
    free(array->strings);
    free(array); //Libero la memoria occupata dalla struct
}

bool findStringInArray(StringList* array, char* word) //Funzione per cercare un elemento nell'array
{
    for(int i=0; i<array->size; i++)
    {
        if(array->strings[i] == NULL) continue;
        if(strcmp(array->strings[i], word) == 0) return true; //Controllo se esiste l'elemento
    }
    return false;
}

ScoreList* createScoreArray()  //Funzione per creare e allocare un array dinamico
{
    //Alloco la memoria necessaria e inizializzo l'array
    ScoreList* array = malloc(sizeof(ScoreList));
    array->size = 0;
    array->textCSV = NULL;
    array->scores = NULL;
    return array;
}

void addScoreToArray(ScoreList* array, char* playerName, int score) //Funzione per aggiungere un elemento all'array
{
    ScoreData newScoreData;
    newScoreData.score = score;
    newScoreData.playerName = NULL;
    copyString(&newScoreData.playerName, playerName);

    array->size++; //Incremento la dimensione dell'array
    array->scores = realloc(array->scores, array->size * sizeof(ScoreData)); //Ri-alloco lo spazio necessario
    array->scores[array->size-1] = newScoreData; //Aggiungo lo score all'array
}

void freeScoreArray(ScoreList* array) //Funzione per liberare la memoria occupata dall'array
{
    if(array == NULL) return;
    for(int i=0; i<array->size; i++) free(array->scores[i].playerName); //Libero la memoria occupata dal nome del player
    free(array->scores);
    free(array->textCSV); //Libero la memoria occupata dal testo
    free(array); //Libero la memoria occupata dalla struct
}