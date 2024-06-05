#include <stdbool.h>
#include "../Utility/Utility.h"
#include "../DataStructures/Trie.h"
#include "../DataStructures/DynamicArray.h"

//Massimo numero di player
#define MAX_CLIENTS 2
#define PAUSE_TIME 60

//Enum per salvare la scelta della generazione della matrice
enum MatrixGen
{
    Default,
    Random,
    File
};

enum GamePhase
{
    Off,
    Playing,
    Paused
};

enum ThreadFunction
{
    Main,
    Read,
    Write
};

//Struct per il player
typedef struct Player
{
    int socket_fd;
    pthread_t thread[3];
    bool bRegistered;
    char* name;
    int score;
    DynamicArray* foundWords;
} Player;

//Struct per la sessione corrente di gioco
typedef struct GameSession
{
    char currentMatrix[MATRIX_SIZE][MATRIX_SIZE];
    Player* players[MAX_CLIENTS];
    int numPlayers;
    long timeToNextPhase;
    enum GamePhase gamePhase;
} GameSession;

//Struct per le informazioni base del gioco
typedef struct GameInfo
{
    char* serverName;
    int serverPort;
    int serverSocket;

    enum MatrixGen customMatrixType;
    char* matrixFile;
    char* dictionaryFile;
    TrieNode* dictionary;

    int round;
    int seed;
    int gameDuration;
    GameSession* currentSession;
} GameInfo;


//Dichiaro le funzioni esposte
GameInfo *initGameInfo(char* newName, int newPort); //Funzione per inizializzare le info
bool initGameSession(GameInfo* info); //Funzione per inizializzare la sessione di gioco

bool updateMatrixFile(GameInfo* info, char* newFile); //Funzione per aggiornare il file della matrice
bool updateDictionaryFile(GameInfo* info, char* newFile); //Funzione per aggiornare il file del dizionario
bool nextGameSession(GameInfo* info); //Funzione per inizializzare la nuova sessione di gioco
bool findInMatrix(char matrix[MATRIX_SIZE][MATRIX_SIZE], char* word); //Funzione per cercare una parola nella matrice

