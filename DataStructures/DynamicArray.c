#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../Utility/Utility.h"
#include "DynamicArray.h"

DynamicArray* createArray() //Funzione per creare e allocare un array dinamico
{
    //Alloco la memoria necessaria e inizializzo l'array
    DynamicArray* array = malloc(sizeof(DynamicArray));
    array->size = 0;
    array->elements = NULL;
    return array;
}

void addToArray(DynamicArray* array, char* string) //Funzione per aggiungere un elemento all'array
{
    if(string == NULL) return;
    array->size++; //Incremento la dimensione dell'array
    array->elements = realloc(array->elements, array->size * sizeof(char*)); //Ri-alloco lo spazio necessario
    copyString(&array->elements[array->size-1], string); //Copio il valore della stringa nell'array
}

void freeArray(DynamicArray* array) //Funzione per liberare la memoria occupata dall'array
{
    if(array == NULL) return;
    for(int i=0; i<array->size; i++)
    {
        if(array->elements[i] == NULL) continue;
        free(array->elements[i]); //Libero la memoria occupata dall'elemento
    }
    free(array); //Libero la memoria occupata dalla struct
}

bool findInArray(DynamicArray* array, char* word) //Funzione per cercare un elemento nell'array
{
    for(int i=0; i<array->size; i++)
    {
        if(array->elements[i] == NULL) continue;
        if(strcmp(array->elements[i], word) == 0) return true; //Controllo se esiste l'elemento
    }
    return false;
}