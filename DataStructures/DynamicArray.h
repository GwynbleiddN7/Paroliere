#include <stdbool.h>

//Struct per l'array dinamico dedicato alle stringhe
typedef struct StringList{
    int size; //Numero di stringhe presenti
    char** strings; //Lista delle stringhe
}StringList;

//Dichiaro le funzione esposte
StringList* createStringArray();  //Funzione per creare e allocare un array dinamico
void addStringToArray(StringList* array, char* string); //Funzione per aggiungere un elemento all'array
void freeStringArray(StringList* array); //Funzione per liberare la memoria occupata dall'array
bool findStringInArray(StringList* array, char* word); //Funzione per cercare una string nell'array (cast a string sottointeso)

//Struct per il singolo score
typedef struct ScoreData{
    char* playerName; //Nome del giocatore
    int score; //Score del giocatore

}ScoreData;

//Struct per l'array dinamico dedicato agli score
typedef struct ScoreList{
    int size; //Numero di score presenti
    char* textCSV; //Classifica
    char* winner; //Nome ultimo vincitore
    ScoreData* scores; //Lista degli score
}ScoreList;

//Dichiaro le funzione esposte
ScoreList* createScoreArray();  //Funzione per creare e allocare un array dinamico
void addScoreToArray(ScoreList* array, char* playerName, int score); //Funzione per aggiungere un elemento all'array
void freeScoreArray(ScoreList* array); //Funzione per liberare la memoria occupata dall'array