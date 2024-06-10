#include <stdbool.h>
#include "../Utility/Utility.h"
#include "../DataStructures/Trie.h"
#include "../DataStructures/DynamicArray.h"

//Massimo numero di player
#define MAX_CLIENTS 3
#define LOBBY_SIZE (MAX_CLIENTS*2)
#define PAUSE_TIME 60

//Variabile di condizione per gestire la lista di giocatori (condivisa con Server.c)
extern pthread_mutex_t players_mutex;

//Enum per salvare la scelta della generazione della matrice
enum MatrixGen
{
    Default,
    Random,
    File
};

//Enum per le diverse fai di gioco
enum GamePhase
{
    Off,
    Playing,
    Paused
};

//Enum per gestire i thread dei giocatori
enum ThreadFunction
{
    Main,
    Sync,
    Async
};

//Struct per il player
typedef struct Player
{
    int socket_fd; //Socket di connessione
    pthread_t thread[3]; //Lista dei thread per ogni client (Main Read Write)
    bool bRegistered; //Flag registrazione
    char* name; //Nome
    int score; //Punteggio attuale
    StringList* foundWords; //Parole trovate
} Player;

//Struct per la sessione corrente di gioco
typedef struct GameSession
{
    char currentMatrix[MATRIX_SIZE][MATRIX_SIZE]; //Matrice di gioco
    Player* players[MAX_CLIENTS]; //Giocatore registrati
    int numPlayers; //Numero giocatori registrati
    ScoreList* scores; //Punteggi
    long timeToNextPhase; //Tempo di gioco rimasto
    enum GamePhase gamePhase; //Fase di gioco
} GameSession;

//Struct per le informazioni base del gioco
typedef struct GameInfo
{
    in_addr_t serverAddr; //Indirizzo server
    int serverPort; //Porta di connessione
    int serverSocket; //Socket di comunicazione

    Player* lobby[LOBBY_SIZE]; //Lista dei client connessi al server (anche quelli ancora non registrati alla partita) -> Massimo consentito = doppio dei player totali
    int currentLobbySize; //Numero di client connessi in totale alla lobby

    enum MatrixGen customMatrixType; //Tipo di generazione matrice
    char* matrixFile; //File con le matrici
    char* dictionaryFile; //File dizionario

    TrieNode* dictionary; //Dizionario caricato in memoria
    StringList* matrix; //Matrici caricate in memoria

    int matrixLine; //Numero matrice da leggere
    int seed; //Seed per il random
    int gameDuration; //Durata delle partite
    GameSession* currentSession; //Sessione corrente
} GameInfo;


//Dichiaro le funzioni esposte
GameInfo *initGameInfo(in_addr_t newAddr, int newPort); //Funzione per inizializzare le info
bool initGameSession(GameInfo* info); //Funzione per inizializzare la sessione di gioco
bool updateDictionaryFile(GameInfo* info, char* newFile); //Funzione per aggiornare il file del dizionario
bool updateMatrixFile(GameInfo* info, char* newFile); //Funzione per aggiornare il file della matrice
bool loadMatrixFile(GameInfo* gameInfo); //Funzione per caricare la matrice dal file
void generateMatrix(GameInfo* gameInfo); //Funzione per generare la matrice di gioco
bool findInMatrix(char matrix[MATRIX_SIZE][MATRIX_SIZE], char* word); //Funzione per cercare una parola nella
Player* createPlayer(int fd_client); //Funzione per inizializzare un nuovo giocatore
bool addPlayerToGame(GameInfo* gameInfo, Player* player); //Funzione per aggiungere un player al game
bool addPlayerToLobby(GameInfo* gameInfo, Player* player); //Funzione per aggiungere un client alla lobby
void deletePlayer(GameInfo* gameInfo, Player* player); //Funzione per eliminare un giocatore
void freeGameMem(GameInfo* gameInfo); //Funzione per liberare la memoria occupata dalle strutture di gioco
void sendCurrentGameInfo(GameInfo* gameInfo, Player* player); //Funzione per inviare un messaggio con la matrice e/o i tempi di gioco/attesa
long getWordScore(char* word); //Funzione per calcolare il punteggio di una parola
