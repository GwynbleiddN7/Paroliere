#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include "../Utility/Utility.h"
#include "GameState.h"

#define DEFAULT_DICT_FILE "Data/dizionario.txt"
#define DEFAULT_GAME_DURATION 3

GameInfo *initGameInfo(char* newName, int newPort) //Funzione per inizializzare le info
{
    //Inizializzo la struct con le informazioni di Default
    GameInfo* gameInfo = malloc(sizeof(GameInfo));

    gameInfo->serverPort = newPort;
    gameInfo->serverName = NULL;
    copyString(&gameInfo->serverName, newName);

    gameInfo->matrixLine = 0;
    gameInfo->customMatrixType = Default;
    gameInfo->matrixFile = NULL;
    gameInfo->dictionaryFile = NULL;

    gameInfo->seed = time(0); //Seed di default
    gameInfo->gameDuration = DEFAULT_GAME_DURATION; //Durata game default
    gameInfo->currentSession = NULL;
    gameInfo->dictionary= NULL;

    copyString(&gameInfo->dictionaryFile, DEFAULT_DICT_FILE); //Path dizionario default

    return gameInfo;
}

bool updateMatrixFile(GameInfo* info, char* newFile) //Funzione per aggiornare il file della matrice
{
    if(!regularFileExists(newFile)) return false; //Controllo che sia un file esistente e regolare
    info->customMatrixType = File; //Aggiorno la flag per sapere in quale caso mi trovo

    copyString(&info->matrixFile, newFile); //Aggiorno il valore del path nella struct
    return true;
}

bool updateDictionaryFile(GameInfo* info, char* newFile) //Funzione per aggiornare il file del dizionario
{
    if(!regularFileExists(newFile)) return false; //Controllo che sia un file esistente e regolare

    copyString(&info->dictionaryFile, newFile); //Aggiorno il valore del path nella struct
    return true;
}

bool loadMatrixFile(GameInfo* gameInfo) //Funzione per caricare la matrice dal file
{
    int bytes_read, fd;

    //Controllo che il file esista e che sia di tipo corretto (già controllato precedentemente, solo per sicurezza)
    if(!regularFileExists(gameInfo->matrixFile)) return false;

    //Apro il file matrice in lettura con una chiamata di sistema
    if(syscall_fails_get(fd, open(gameInfo->matrixFile, O_RDONLY))) return false;

    char newLetter;
    int rowIndex=0, colIndex=0, currentLine=0;
    while(true) //Leggo un byte alla volta dal file (sizeof(char))
    {
        if(syscall_fails_get(bytes_read, read(fd, &newLetter, sizeof(char)))) return false;

        if(bytes_read == 0) //Se arrivo a fine file ricomincio dalla prima riga
        {
            printf("EOF\n");
            lseek(fd, 0, SEEK_SET);
            gameInfo->matrixLine = 0;
            currentLine = 0;
        }

        //Raggiungo la riga di file che devo leggere in base al round
        if(currentLine < gameInfo->matrixLine) { //Se non sto alla riga corrispondente al round continuo a leggere
            if(newLetter == '\n') currentLine++; //Se arrivo a fine riga incremento il counter
            continue;
        }
        if(newLetter == '\n') break; //Se arrivo a fine riga interrompo la lettura
        if(newLetter == ' '){
            colIndex++; //Se leggo uno spazio incremento il numero di colonna
            if(colIndex == MATRIX_SIZE) //Se sono alla quarta colonna letta ricomincio da 0 e incremento la riga di 1
            {
                colIndex = 0;
                rowIndex++;
            }
            continue;
        }
        char letter = (char)tolower(newLetter);
        gameInfo->currentSession->currentMatrix[rowIndex][colIndex] = letter; //Salvo la lettera letta nella posizione corrispondente della matrice
        if(letter == 'q') lseek(fd, sizeof(char), SEEK_CUR); //Se leggo una 'q', salto una lettera per evitare di leggere la 'u'
    }

    if(syscall_fails(close(fd))) return false; //Provo a chiudere il file
    return true;
}

Player* createPlayer(int fd_client) //Funzione per inizializzare un nuovo giocatore
{
    Player* player = malloc(sizeof(Player));
    player->socket_fd = fd_client;
    player->foundWords = createStringArray();
    player->score = 0;
    player->name = NULL;
    player->bRegistered = false;
    return player;
}

//Enum per le direzioni della ricerca
enum Directions{
    UP,
    DOWN,
    LEFT,
    RIGHT
};

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

bool generateMatrix(GameInfo* gameInfo) //Funzione per generare la matrice di gioco
{
    switch (gameInfo->customMatrixType) {
        case File:
            return loadMatrixFile(gameInfo); //Se la flag indica File, carico dal file
        case Random: //Altrimenti genero random
        case Default:
            for(int i=0; i<MATRIX_SIZE; i++)
            {
                for(int j=0; j<MATRIX_SIZE; j++)
                {
                    //Genero una lettera (codice ascii) random nel range da ascii('a') a ascii('z')
                    char randomLetter = (char)((rand() % TOTAL_LETTERS) + FIRST_LETTER);
                    gameInfo->currentSession->currentMatrix[i][j] = randomLetter;
                }
            }
            break;
    }
    return true;
}

bool initGameSession(GameInfo* info) //Funzione per inizializzare la sessione di gioco
{
    //Alloco lo spazio per la struct e la inizializzo
    GameSession* session = malloc(sizeof(GameSession));
    for(int i=0; i<MAX_CLIENTS; i++) session->players[i] = NULL;
    session->timeToNextPhase = 0;
    session->numPlayers = 0;
    session->gamePhase = Off;
    session->scores = NULL;

    info->currentSession = session; //Salvo il puntatore nella struct GameInfo
    return generateMatrix(info); //Genero la matrice e ritorno il risultato
}

bool nextGameSession(GameInfo* gameInfo) //Funzione per inizializzare la nuova sessione di gioco
{
    if(gameInfo->currentSession == NULL) return false; //Failsafe

    //Se passo da Paused a Playing genero la nuova matrice e resetto gli score
    freeScoreArray(gameInfo->currentSession->scores);
    gameInfo->currentSession->scores = createScoreArray();
    return generateMatrix(gameInfo); //Genero la matrice e ritorno il risultato
}