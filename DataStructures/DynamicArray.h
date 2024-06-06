#include <stdbool.h>

//Struct per l'array dinamico
typedef struct StringList{
    int size;
    char** strings;
}StringList;

StringList* createStringArray();  //Funzione per creare e allocare un array dinamico
void addStringToArray(StringList* array, char* string); //Funzione per aggiungere un elemento all'array
void freeStringArray(StringList* array); //Funzione per liberare la memoria occupata dall'array
bool findStringInArray(StringList* array, char* word); //Funzione per cercare una string nell'array (cast a string sottointeso)

typedef struct ScoreData{
    char* playerName;
    int score;

}ScoreData;

typedef struct ScoreList{
    int size;
    char* textCSV;
    ScoreData* scores;
}ScoreList;

ScoreList* createScoreArray();  //Funzione per creare e allocare un array dinamico
void addScoreToArray(ScoreList* array, char* playerName, int score); //Funzione per aggiungere un elemento all'array
void freeScoreArray(ScoreList* array); //Funzione per liberare la memoria occupata dall'array