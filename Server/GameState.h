#include <stdbool.h>
#include "../Utility/Utility.h"
#define MAX_CLIENTS 32

//Enum per salvare la scelta della generazione della matrice
enum MatrixGen
{
    Default,
    Random,
    File
};

//Struct per il player
typedef struct Player
{
    int socket_fd;
    pthread_t thread;
    char* name;
    int score;
} Player;

//Struct per la sessione corrente di gioco
typedef struct GameSession
{
    char currentMatrix[MATRIX_SIZE][MATRIX_SIZE];
    Player* players[MAX_CLIENTS];
    int numPlayers;
    int round;
    bool bIsPaused;
} GameSession;

//Struct per le informazioni base del gioco
typedef struct GameInfo
{
    char* serverName;
    int serverPort;

    enum MatrixGen customMatrixType;
    char* matrixFile;
    char* dictionaryFile;

    int seed;
    int gameDuration;
    GameSession* currentSession;
} GameInfo;


//Rendo "pubbliche" alcune funzioni del file GameState.c
GameInfo *initGameInfo(char* newName, int newPort); //Funzione per inizializzare le info
void initGameSession(GameInfo* info); //Funzione per inizializzare la sessione di gioco

bool updateMatrixFile(GameInfo* info, char* newFile); //Funzione per aggiornare il file della matrice
bool updateDictionaryFile(GameInfo* info, char* newFile); //Funzione per aggiornare il file del dizionario
void generateMatrix(GameInfo* gameInfo); //Funzione per generare la matrice di gioco

