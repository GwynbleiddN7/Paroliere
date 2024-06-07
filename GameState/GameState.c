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

#define DEFAULT_DICT_FILE "Data/dizionario.txt" //File dizionario default
#define DEFAULT_GAME_DURATION 3*60 //3 minuti

GameInfo *initGameInfo(char* newName, int newPort) //Funzione per inizializzare le info
{
    //Inizializzo la struct con le informazioni di Default
    GameInfo* gameInfo = malloc(sizeof(GameInfo));

    gameInfo->serverPort = newPort;
    gameInfo->serverName = newName;

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


Player* createPlayer(int fd_client) //Funzione per inizializzare un nuovo giocatore
{
    Player* player = malloc(sizeof(Player));
    player->socket_fd = fd_client; //Imposto il socket di comunicazione
    player->foundWords = createStringArray(); //Inizializzo l'array di parole trovate
    player->score = 0;
    player->name = NULL;
    player->bRegistered = false;
    return player;
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


bool loadMatrixFile(GameInfo* gameInfo) //Funzione per caricare la matrice dal file
{
    if(gameInfo->customMatrixType != File) return true;

    int bytes_read, fd;

    //Controllo che il file esista e che sia di tipo corretto (già controllato precedentemente, solo per sicurezza)
    if(!regularFileExists(gameInfo->matrixFile)) return false;

    //Apro il file matrice in lettura con una chiamata di sistema
    if(syscall_fails_get(fd, open(gameInfo->matrixFile, O_RDONLY))) return false;

    gameInfo->matrix = createStringArray();

    char* newWord = malloc(sizeof(char));
    newWord[0] = '\0';

    char newLetter;
    int counterCheck=0;
    while((bytes_read = read(fd, &newLetter, sizeof(char)))) //Leggo un byte alla volta dal file (sizeof(char))
    {
        if(bytes_read == -1) { //Se ho un errore esco
            free(newWord);
            return false;
        }

        if(newLetter == '\n') { //Se arrivo a fine riga incremento il counter
            if(counterCheck != MATRIX_SIZE*MATRIX_SIZE-1) { //Devono esserci 16 elementi per riga, quindi 15 spazi non considerando l'ultimo che non è presente
                free(newWord);
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
        newWord[len] = letter;
        newWord[len+1] = '\0';
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

    return loadMatrixFile(info);
}