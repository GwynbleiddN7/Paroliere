#include <stdbool.h>

//Struct per l'array dinamico
typedef struct DynamicArray{
    int size;
    char** elements;
}DynamicArray;

DynamicArray* createArray();  //Funzione per creare e allocare un array dinamico
void addToArray(DynamicArray* array, char* string); //Funzione per aggiungere un elemento all'array
void freeArray(DynamicArray* array); //Funzione per liberare la memoria occupata dall'array
bool findInArray(DynamicArray* array, char* word); //Funzione per cercare un elemento nell'array