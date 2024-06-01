#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>
#include "../CommandsHandler/CommsHandler.h"
#include "../Utility/Utility.h"
#include "../Trie/Trie.h"
#include "GameState.h"

#define ADDR_INDEX 1
#define PORT_INDEX 2

//Utilizzo questa struct per definire i possibili parametri con double-dash e il relativo valore per lo switch()
static struct option long_options[] =
{
    {"matrici", required_argument, NULL, 'm'},
    {"durata", required_argument, NULL, 't'},
    {"seed", required_argument, NULL, 's'},
    {"diz", required_argument, NULL, 'd'},
    {NULL, 0, NULL, 0} //Possibile aggiunti di altri parametri che gestisco nello switch
};

int main(int argc, char** argv)
{
    if(argc-1 < 2) exitMessage("Nome host e numero porta sono obbligatori"); //Controllo che ci siamo almeno i primi 2 parametri (-1 per il nome del file)

    char* addr = argv[ADDR_INDEX]; //Prendo il primo parametro che corrisponde all'hostname
    int port;
    if(!validatePort(argv[PORT_INDEX], &port)) exitMessage("Numero di porta non corretto"); //Controllo con una funzione se la porta inserita è valida, se non lo è interrompo

    GameInfo* gameInfo = initGameInfo(addr, port); //Inizializzo la struct che contiene le informazioni del gioco

    int option; opterr = 0;
    while ((option = getopt_long_only(argc, argv, "m:t:s:d:", long_options, NULL)) != -1) //Uso gli opt_long per prendere i 4 possibili parametri opzionali m t s d (m:t:s:d: indica che hanno un valore dopo)
    {
        switch (option)
        {
            case 'm':
                if(gameInfo->customMatrixType == Random) exitMessage("Seed e File matrice non possono essere presenti contemporaneamente"); //Se Random era settato ho già un'opzione e interrompo
                if(!updateMatrixFile(gameInfo, optarg)) exitMessage("File matrice non esistente o non corretto"); //Se il file non esiste interrompo, altrimenti lo salvo nella struct
                break;
            case 't':
                if(!strToInt(optarg, &gameInfo->gameDuration)) exitMessage("Durata di gioco non valido"); //Se il valore della durata è un intero lo converto e lo salvo nella struct, altrimento interrompo
                break;
            case 's':
                if(gameInfo->customMatrixType == File) exitMessage("Seed e File matrice non possono essere presenti contemporaneamente"); //Se File era settato ho già un'opzione e interrompo
                if(!strToInt(optarg, &gameInfo->seed)) exitMessage("Seed non valido"); //Se il valore del seed è un intero lo converto e lo salvo nella struct, altrimento interrompo
                gameInfo->customMatrixType = Random; //Aggiorno la flag per sapere in quale caso mi trovo
                break;
            case 'd':
                if(!updateDictionaryFile(gameInfo, optarg)) exitMessage("File dizionario non esistente o non corretto"); //Se il file non esiste interrompo, altrimenti lo salvo nella struct
                break;
            default:
                exitMessage("Parametri aggiuntivi non riconosciuti"); //Se trovo altri parametri interrompo
        }
    }
    if (optind != argc-2) exitMessage("Parametri aggiuntivi non riconosciuti"); //Se trovo altri parametri interrompo

    srand(gameInfo->seed); //Inizializzo srand con il seed time(0) o con quello inserito come parametro

    TrieNode* head = loadDictionary(gameInfo->dictionaryFile);
    while(true)
    {
        char* word = malloc(20);
        scanf("%s", word);
        bool found = findWord(head, word, 0);
        printf("%d\n", found);
    }
}