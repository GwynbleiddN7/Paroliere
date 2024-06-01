#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "GameState.h"
#include "../Utility/Utility.h"

#define DEFAULT_DICT_FILE "../Data/dizionario.txt"
#define DEFAULT_GAME_DURATION 3

GameInfo *initGameInfo(char* newName, int newPort)
{
    //Inizializzo la struct con le informazioni di Default
    GameInfo* gameInfo = malloc(sizeof(GameInfo));
    copyString(&gameInfo->serverName, newName);
    gameInfo->serverPort = newPort;
    gameInfo->customMatrixType = Default;
    gameInfo->matrixFile = NULL;
    copyString(&gameInfo->dictionaryFile, DEFAULT_DICT_FILE); //Path dizionario default
    gameInfo->gameDuration = DEFAULT_GAME_DURATION; //Durata game default
    gameInfo->seed = time(0); //Seed di default
    return gameInfo;
}

bool updateMatrixFile(GameInfo* info, char* newFile)
{
    if(!regularFileExists(newFile)) return false; //Controllo che sia un file esistente e regolare
    info->customMatrixType = File; //Aggiorno la flag per sapere in quale caso mi trovo

    copyString(&info->matrixFile, newFile); //Aggiorno il valore del path nella struct
    return true;
}

bool updateDictionaryFile(GameInfo* info, char* newFile)
{
    if(!regularFileExists(newFile)) return false; //Controllo che sia un file esistente e regolare

    copyString(&info->dictionaryFile, newFile); //Aggiorno il valore del path nella struct
    return true;
}

void loadMatrixFile(GameSession* session, char* path)
{
    int retvalue, fd;

    //Controllo che il file esista e che sia di tipo corretto (già controllato precedentemente, solo per sicurezza)
    if(!regularFileExists(path)) exitMessage("File matrice non esistente o di formato non corretto");

    //Apro il file matrice in lettura con una chiamata di sistema
    SYSC(fd, open(path, O_RDONLY), "Errore nell'apertura del file matrice");

    char newLetter;
    int rowIndex=0, colIndex=0, currentLine=0;
    while( (retvalue = read(fd, &newLetter, sizeof(char))) )
    {
        if(currentLine < session->round) {
            if(newLetter == '\n') currentLine++;
            if(newLetter == EOF) {
                session->round = 0;
                currentLine = 0;
            }
            continue;
        }
        if(newLetter == '\n') break;
        if(newLetter == ' '){
            colIndex++;
            if(colIndex == MATRIX_SIZE)
            {
                colIndex = 0;
                rowIndex++;
            }
            continue;
        }
        session->currentMatrix[rowIndex][colIndex] = newLetter;
        if(newLetter == 'q') lseek(fd, sizeof(char), SEEK_CUR);
    }

    //Chiudo il file con una chiamata di sistema
    SYSC(retvalue, close(fd), "Errore nella chiusura del file dizionario");
}

void generateMatrix(GameSession* gameSession, GameInfo* gameInfo)
{
    switch (gameInfo->customMatrixType) {
        case File:
            loadMatrixFile(gameSession, gameInfo->matrixFile);
            break;
        case Random:
        case Default:
            for(int i=0; i<MATRIX_SIZE; i++)
            {
                for(int j=0; j<MATRIX_SIZE; j++)
                {
                    gameSession->currentMatrix[i][j] = (char)((rand() % TOTAL_LETTERS) + FIRST_LETTER);
                }
            }
            break;
    }
}

