#include <stdbool.h>

#define MATRIX_SIZE 4

//Enum per salvare la scelta della generazione della matrice
enum MatrixGen
{
    Default,
    Random,
    File
};

//Struct per le informazioni base del gioco
typedef struct GameInfo
{
    char* serverName;
    int serverPort;

    enum MatrixGen customMatrixType;
    char* matrixFile;
    char* dictionaryFile;
    int gameDuration;
    int seed;
} GameInfo;

typedef struct GameSession
{
    char currentMatrix[MATRIX_SIZE][MATRIX_SIZE];
    int round;
} GameSession;

//Rendo "pubbliche" alcune funzioni del file GameState.c
GameInfo *initGameInfo(char* newName, int newPort); //Funzione per inizializzare le info

bool updateMatrixFile(GameInfo* info, char* newFile); //Funzione per aggiornare il file della matrice
bool updateDictionaryFile(GameInfo* info, char* newFile); //Funzione per aggiornare il file del dizionario

void generateMatrix(GameSession* gameSession, GameInfo* gameInfo);
