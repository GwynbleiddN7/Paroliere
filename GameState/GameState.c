#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "../CommsHandler/CommsHandler.h"
#include "../Utility/Utility.h"
#include "GameState.h"

#define ITALIAN_LETTERS "abcdefghilmnopqrstuvz" //Lettere italiane
#define DEFAULT_DICT_FILE "Data/dizionario.txt" //File dizionario default
#define DEFAULT_GAME_DURATION 3*60 //3 minuti in secondi

//Enum per le direzioni della ricerca nella matrice
enum Directions{
    UP,
    DOWN,
    LEFT,
    RIGHT
};

GameInfo *initGameInfo(in_addr_t newAddr, int newPort) //Funzione per inizializzare le info
{
    //Alloco la memoria per la struct e la inizializzo con valori di default o temporanei
    GameInfo* gameInfo = malloc(sizeof(GameInfo));

    gameInfo->serverPort = newPort;
    gameInfo->serverAddr = newAddr;

    for(int i=0; i<LOBBY_SIZE; i++) gameInfo->lobby[i] = NULL;
    gameInfo->currentLobbySize = 0;

    gameInfo->dictionaryFile = NULL;
    gameInfo->dictionary= NULL;
    gameInfo->customMatrixType = Default;
    gameInfo->matrixFile = NULL;
    gameInfo->matrix = NULL;
    gameInfo->matrixLine = 0;

    gameInfo->seed = time(0); //Seed di default
    gameInfo->gameDuration = DEFAULT_GAME_DURATION; //Durata game default
    gameInfo->currentSession = NULL;

    copyString(&gameInfo->dictionaryFile, DEFAULT_DICT_FILE); //Path dizionario default

    return gameInfo;
}


bool initGameSession(GameInfo* info) //Funzione per inizializzare la sessione di gioco
{
    //Alloco lo spazio per la struct e la inizializzo temporaneamente
    GameSession* session = malloc(sizeof(GameSession));
    for(int i=0; i<MAX_CLIENTS; i++) session->players[i] = NULL;
    session->timeToNextPhase = 0;
    session->numPlayers = 0;
    session->gamePhase = Off;
    session->scores = NULL;
    info->currentSession = session; //Salvo il puntatore nella struct GameInfo

    return loadMatrixFile(info); //Carico la matrice dal file se devo e ritorno il risultato dell'operazione
}


bool updateDictionaryFile(GameInfo* info, char* newFile) //Funzione per aggiornare il file del dizionario
{
    if(!regularFileExists(newFile)) return false; //Controllo che sia un file esistente e regolare

    copyString(&info->dictionaryFile, newFile); //Aggiorno il valore del path nella struct
    return true;
}


bool updateMatrixFile(GameInfo* info, char* newFile) //Funzione per aggiornare il file della matrice
{
    if(!regularFileExists(newFile)) return false; //Controllo che sia un file esistente e regolare
    info->customMatrixType = File; //Aggiorno la flag per sapere in quale caso mi trovo

    copyString(&info->matrixFile, newFile); //Aggiorno il valore del path nella struct
    return true;
}


bool loadMatrixFile(GameInfo* gameInfo) //Funzione per caricare la matrice dal file
{
    if(gameInfo->customMatrixType != File) return true;

    int bytes_read, fd;

    //Controllo che il file esista e che sia di tipo corretto (già controllato precedentemente, solo per sicurezza)
    if(!regularFileExists(gameInfo->matrixFile)) return false;

    //Apro il file matrice in lettura con una chiamata di sistema
    if(syscall_fails_get(fd, open(gameInfo->matrixFile, O_RDONLY))) return false;

    gameInfo->matrix = createStringArray(); //Inizializzo l'array

    //Inizializzo la nuova parola vuota con il null termination character
    char* newWord = malloc(sizeof(char));
    newWord[0] = '\0';

    char newLetter;
    int counterCheck=0; //Counter per validare il numero di lettere in ogni riga del file
    while((bytes_read = read(fd, &newLetter, sizeof(char)))) //Leggo un byte alla volta dal file (sizeof(char))
    {
        if(bytes_read == -1) { //Se ho un errore esco
            free(newWord);
            return false;
        }

        if(newLetter == '\n') { //Se arrivo a fine riga incremento il counter
            if(counterCheck != MATRIX_SIZE*MATRIX_SIZE-1) { //Devono esserci 16 elementi per riga, quindi 15 spazi non considerando l'ultimo che non è presente
                free(newWord); //Libero lo spazio occupato e interrompo
                return false;
            }
            addStringToArray(gameInfo->matrix, newWord); //Aggiungo la parola completata alla lista
            counterCheck = 0; //Resetto il counter di controllo

            free(newWord); //Libero la memoria della parola
            newWord = malloc(sizeof(char)); //Alloco il nuovo spazio per la successiva parola
            newWord[0] = '\0'; //Assegno il \0 allo spazio allocato

            continue;
        }
        if(newLetter == ' ') {
            counterCheck++; //Se ho trovato uno spazio aumento il counter delle lettere
            continue;
        }
        if(!isalpha(newLetter)) {
            free(newWord); //Se ho trovato un carattere non valido libero la memoria ed esco
            return false;
        }
        char letter = (char)tolower(newLetter); //Converto in minuscolo
        if(letter == 'q') lseek(fd, sizeof(char), SEEK_CUR); //Se trovo una q salto di 1 per non leggere la u

        int len = strlen(newWord);
        newWord = realloc(newWord, (len + 2) * sizeof(char)); //Ri-alloco lo spazio per la nuova lettera e il \0
        newWord[len] = letter; //Aggiungo la lettera alla stringa nel nuovo spazio appena allocato
        newWord[len+1] = '\0'; //Aggiungo il null termination character per delineare la fine della stringa
    }

    //Se l'ultima riga del file non finisce con \n sono uscito dal while prima di salvare l'ultima parola quindi lo faccio qui
    if(strlen(newWord) > 0){
        if(counterCheck == MATRIX_SIZE*MATRIX_SIZE-1) addStringToArray(gameInfo->matrix, newWord);
        else{
            free(newWord);
            return false;
        }
    }
    free(newWord); //Libero lo spazio occupato dalla parola

    if(syscall_fails(close(fd))) return false; //Provo a chiudere il file
    return true;
}


void generateMatrix(GameInfo* gameInfo) //Funzione per generare la matrice di gioco
{
    switch (gameInfo->customMatrixType) {
        case File:
            if(gameInfo->matrixLine >= gameInfo->matrix->size) gameInfo->matrixLine = 0; //Se sono arrivato alla fine delle matrici disponibili ricomincio dalla prima

            int rowIndex=0, colIndex=0;
            for(int i=0; i < MATRIX_SIZE*MATRIX_SIZE; i++, colIndex++)
            {
                char letter = gameInfo->matrix->strings[gameInfo->matrixLine][i]; //Prendo la lettera attuale
                if(i > 0 && i % MATRIX_SIZE == 0) //Ogni 4 lettere cambio riga e resetto la colonna
                {
                    colIndex = 0;
                    rowIndex++;
                }
                gameInfo->currentSession->currentMatrix[rowIndex][colIndex] = letter; //Salvo la lettera letta nella posizione corrispondente della matrice
            }
            gameInfo->matrixLine++; //Aumento il counter della prossima riga da leggere
            break;
        case Random: //Altrimenti genero random
        case Default:
        {
            char* italianLetters = ITALIAN_LETTERS; //Lettere possibili
            for(int i=0; i<MATRIX_SIZE; i++)
            {
                for(int j=0; j<MATRIX_SIZE; j++)
                {
                    char randomLetter = italianLetters[rand() % strlen(italianLetters)]; //Genero una lettera random presa dalla lista di lettere possibili
                    gameInfo->currentSession->currentMatrix[i][j] = randomLetter; //Assegno la lettera alla posizione della matrice
                }
            }
            break;
        }
    }
}


bool recursiveFindMatrix(char matrix[MATRIX_SIZE][MATRIX_SIZE], bool foundMatrix[MATRIX_SIZE][MATRIX_SIZE], int i, int j, char* word, int currentIndex) //Funzione per cercare ricorsivamente la parola nella matrice
{
    if(matrix[i][j] == word[currentIndex] && !foundMatrix[i][j]) //Se la lettera corrisponde e non ho già visitato la cella
    {
        //Gestione Qu
        if(word[currentIndex] == 'q')
        {
            if(currentIndex == strlen(word) - 1) return false; //La 'q' non può essere l'ultima lettera
            if(word[currentIndex+1] != 'u') return false; //Dopo la 'u' ci deve essere la 'u'
            currentIndex++; //Salto la u
        }

        if(currentIndex == strlen(word) - 1) return true; //Se la lettera corrisponde e sono alla fine della parola ho trovato il risultato
        foundMatrix[i][j] = true; //Flag della cella come letta

        bool bFound = false;
        for(int k=0; k<4; k++)
        {
            switch (k) {
                case UP:
                    if(i>0) bFound = recursiveFindMatrix(matrix, foundMatrix, i-1, j, word, currentIndex+1); //Se posso andare sopra cerco da sopra
                    break;
                case DOWN:
                    if(i<MATRIX_SIZE-1) bFound = recursiveFindMatrix(matrix, foundMatrix, i+1, j, word, currentIndex+1); //Se posso andare sotto cerco da sotto
                    break;
                case LEFT:
                    if(j>0) bFound = recursiveFindMatrix(matrix, foundMatrix, i, j-1, word, currentIndex+1); //Se posso andare a sinistra cerco da sinistra
                    break;
                case RIGHT:
                    if(j<MATRIX_SIZE-1) bFound = recursiveFindMatrix(matrix, foundMatrix, i, j+1, word, currentIndex+1); //Se posso andare a destra cerco da destra
                    break;
            }
            if(bFound) break;
        }
        foundMatrix[i][j] = false; //Resetto il flag visitato della cella per le successive ricorsioni
        return bFound;
    }
    else return false;
}


bool findInMatrix(char matrix[MATRIX_SIZE][MATRIX_SIZE], char* word) //Funzione per cercare una parola nella matrice
{
    if(strlen(word) == 0) return false; //Failsafe
    bool bFound = false; //Flag risultato
    for(int i=0; i<MATRIX_SIZE && !bFound; i++)
    {
        for(int j=0; j<MATRIX_SIZE && !bFound; j++)
        {
            if(word[0] == matrix[i][j]) { //Se l'iniziale della parola corrisponde alla lettera attuale della matrice
                bool foundMatrix[MATRIX_SIZE][MATRIX_SIZE] = {false}; //Inizializzo una matrice di boolean che indicano se ho già letto una cella
                bFound = recursiveFindMatrix(matrix, foundMatrix, i, j, word, 0); //Inizio la ricorsione per trovare la parola
            }
        }
    }
    return bFound;
}


Player* createPlayer(int fd_client) //Funzione per inizializzare un nuovo giocatore
{
    Player* player = malloc(sizeof(Player)); //Alloco la memoria per il giocatore
    player->socket_fd = fd_client; //Imposto il socket di comunicazione
    player->foundWords = createStringArray(); //Inizializzo l'array di parole trovate
    player->score = 0;
    player->name = NULL;
    player->bRegistered = false;
    return player;
}


bool addPlayerToLobby(GameInfo* gameInfo, Player* player) //Funzione per aggiungere un client alla lobby
{
    pthread_mutex_lock(&players_mutex); //Inizio della sezione critica con l'acquisizione di un lock

    if(gameInfo->currentLobbySize >= LOBBY_SIZE) { //Se non c'è più spazio nella lobby
        pthread_mutex_unlock(&players_mutex); //Sblocco il lock alla fine della sezione critica
        return false;
    }

    for(int i=0; i<LOBBY_SIZE; i++)
    {
        if(gameInfo->lobby[i] == NULL) //Trovo la prima posizione libera nella lista della lobby
        {
            gameInfo->lobby[i] = player; //Assegno il puntatore di player alla posizione della lista
            gameInfo->currentLobbySize++; //Incremento il numero di client nella lobby
            break;
        }
    }
    pthread_mutex_unlock(&players_mutex); //Sblocco il lock alla fine della sezione critica
    return true;
}


bool addPlayerToGame(GameInfo* gameInfo, Player* player) //Funzione per aggiungere un player al game
{
    pthread_mutex_lock(&players_mutex); //Inizio della sezione critica con l'acquisizione di un lock

    if(gameInfo->currentSession->numPlayers >= MAX_CLIENTS) {
        pthread_mutex_unlock(&players_mutex); //Sblocco il lock alla fine della sezione critica
        return false;
    }

    for(int i=0; i<MAX_CLIENTS; i++)
    {
        if(gameInfo->currentSession->players[i] == NULL) //Trovo la prima posizione libera nella lista di giocatori
        {
            gameInfo->currentSession->players[i] = player; //Assegno il puntatore di player alla posizione della lista
            gameInfo->currentSession->numPlayers++; //Incremento il numero di player in partita
            break;
        }
    }
    pthread_mutex_unlock(&players_mutex); //Sblocco il lock alla fine della sezione critica
    return true;
}


void removePlayerFromGameAndLobby(GameInfo* gameInfo, Player* player) //Funzione per rimuovere un client dalla lobby
{
    pthread_mutex_lock(&players_mutex); //Inizio della sezione critica con l'acquisizione di un lock

    for(int i=0; i<LOBBY_SIZE && gameInfo->currentLobbySize > 0; i++)
    {
        if(gameInfo->lobby[i] != NULL) //Per ogni player non NULL nella lista
        {
            if(gameInfo->lobby[i] == player) //Controllo che sia il player che cerco
            {
                gameInfo->lobby[i] = NULL; //Rendo disponibile lo slot
                gameInfo->currentLobbySize--; //Decremento il numero di client nella lobby
                break;
            }
        }
    }

    //Oltre a eliminarlo dalla lobby provo a eliminarlo dalla partita se si era registrato
    for(int i=0; i<MAX_CLIENTS && gameInfo->currentSession->numPlayers > 0; i++)
    {
        if(gameInfo->currentSession->players[i] != NULL) //Per ogni player non NULL nella lista
        {
            if(gameInfo->currentSession->players[i] == player) //Controllo che sia il player che cerco
            {
                gameInfo->currentSession->players[i] = NULL; //Rendo disponibile lo slot
                gameInfo->currentSession->numPlayers--; //Decremento il numero di player in game
                break;
            }
        }
    }

    pthread_mutex_unlock(&players_mutex); //Sblocco il lock alla fine della sezione critica
}


void deletePlayer(GameInfo* gameInfo, Player* player) //Funzione per eliminare un giocatore
{
    if(player == NULL) return; //Failsafe
    if(gameInfo->currentSession->gamePhase != Off) { //Se devo rendere disponibile un nuovo slot (false quando viene chiamata da freeGameMem ~ non rischio di chiudere due volte il socket e non spreco cicli inutili)
        pthread_detach(player->thread[Main]); //Se non sto chiudendo il server non ho bisogno di attendere che il thread principale si chiuda e posso far liberare la memoria automaticamente
        removePlayerFromGameAndLobby(gameInfo, player); //Rimuovo il player dalla lobby e dalla partita se è registrato
        close(player->socket_fd); //Chiudo la connessione con il client del player in questione (se c'è un errore il socket è già chiuso o comunque procedo a liberare la memoria)
    }
    if(player->name != NULL) free(player->name); //Libero la memoria occupata dal nome
    freeStringArray(player->foundWords); //Libero la memoria occupata dal Trie delle parole trovate
    free(player); //Libero la memoria occupata dalla struct
}


void freeGameMem(GameInfo* gameInfo) //Funzione per liberare la memoria occupata dalle strutture di gioco
{
    if(gameInfo == NULL) return; //Failsafe
    if(gameInfo->currentSession != NULL)
    {
        //Prendo tutti i client connessi alla lobby
        for(int i=0; i<LOBBY_SIZE; i++) {
            Player* player = gameInfo->lobby[i];
            if(player == NULL) continue;
            //Chiudo la connessione con il client del player che induce la terminazione del thread (con shutdown mi assicuro che la read venga interrotta e quindi il thread interrotto)
            shutdown(player->socket_fd, SHUT_RDWR); //Non eseguo controlli sul risultato perché sto chiudendo il programma
            close(player->socket_fd); //Non eseguo controlli sul risultato perché sto chiudendo il programma
            pthread_join(player->thread[Main], NULL); //Attendo la chiusura del thread che si occupa di liberare la memoria se il socket era ancora aperto
        }
        freeScoreArray(gameInfo->currentSession->scores); //Libero la memoria occupata dalla lista dei punteggi
        free(gameInfo->currentSession); //Libero la memoria occupata dalla struct della sessione
    }
    if(gameInfo->dictionary != NULL) freeTrie(gameInfo->dictionary); //Libero la memoria occupata dal Trie del dizionario
    if(gameInfo->matrixFile != NULL) free(gameInfo->matrixFile); //Libero la memoria occupata dal file matrice
    if(gameInfo->matrix != NULL) freeStringArray(gameInfo->matrix); //Libero la memoria occupata dal file matrice
    if(gameInfo->dictionaryFile != NULL) free(gameInfo->dictionaryFile); //Libero la memoria occupata dal file dizionario

    free(gameInfo); //Libero la memoria occupata dalla struct del gioco
}


void sendCurrentGameInfo(GameInfo* gameInfo, Player* player) //Funzione per inviare un messaggio con la matrice e/o i tempi di gioco/attesa
{
    if(gameInfo->currentSession->gamePhase == Paused)
        sendNumMessage(player->socket_fd, MSG_TEMPO_ATTESA, gameInfo->currentSession->timeToNextPhase); //Se il gioco è in pausa invio il tempo di attesa
    else{
        sendTextMessage(player->socket_fd, MSG_MATRICE, gameInfo->currentSession->currentMatrix); //Se il gioco è in corso invio la matrice
        sendNumMessage(player->socket_fd, MSG_TEMPO_PARTITA, gameInfo->currentSession->timeToNextPhase); //E il tempo rimanente
    }
}


long getWordScore(char* word) //Funzione per calcolare il punteggio di una parola
{
    int counter = 0;
    for(int i=0; i<strlen(word); i++) if(word[i] != 'q') counter++; //Gestisco il caso Qu che deve valere 1
    return counter;
}