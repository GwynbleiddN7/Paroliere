#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <getopt.h>
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

static void getOptionalParameters(int argc, char** argv);
static void* start_server();
static void* handle_player(void* playerArg);
void exitThread(Message* msg, Player* player);
void sendMatrix(Player* player);

static GameInfo* gameInfo;

int main(int argc, char** argv)
{
    //Validazione parametri obbligatori
    if(argc-1 < 2) exitMessage("Nome host e numero porta sono obbligatori"); //Controllo che ci siamo almeno i primi 2 parametri (-1 per il nome del file)
    char* addr = argv[ADDR_INDEX]; //Prendo il primo parametro che corrisponde all'hostname
    int port;
    if(!validatePort(argv[PORT_INDEX], &port)) exitMessage("Numero di porta non corretto"); //Controllo con una funzione se la porta inserita è valida, se non lo è interrompo

    //Inizializzazione struttura di gioco
    gameInfo = initGameInfo(addr, port); //Inizializzo la struct che contiene le informazioni del gioco
    getOptionalParameters(argc, argv); //Aggiorno la struct con i parametri opzionali
    srand(gameInfo->seed); //Inizializzo srand con il seed time(0) o con quello inserito come parametro

    //Inizializzazione sessione di gioco
    initGameSession(gameInfo);

    pthread_t server_thread;
    if (pthread_create(&server_thread, NULL, start_server, NULL) != 0) exitMessage("Errore creazione thread server");

    TrieNode* head = loadDictionary(gameInfo->dictionaryFile);
    while(true)
    {
        sleep(10);

    }
}

void getOptionalParameters(int argc, char** argv)
{
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
}

void* start_server() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;

    SYSC(server_fd, socket(AF_INET, SOCK_STREAM, 0), "Nella socket");

    int opt=1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
        perror("Nella setsockopt");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))){
        perror("Nella setsockopt");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(gameInfo->serverPort);
    server_addr.sin_addr.s_addr = inet_addr(gameInfo->serverName);

    SYSCALL(bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)), "Nella bind");
    SYSCALL(listen(server_fd, MAX_CLIENTS), "Nella listen");

    socklen_t client_addr_len = sizeof(client_addr);

    while(true){
        int fd_client;
        SYSC(fd_client, accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len), "Nella accept");
        Player* player = malloc(sizeof(Player));
        player->socket_fd = fd_client;
        player->score = 0;
        if (pthread_create(&player->thread, NULL, handle_player, (void*) player) != 0) exitMessage("Errore creazione thread client");
    }

    //Handler chiusura clients/join


    SYSCALL(close(server_fd), "Errore nella chiusura del server");
}

pthread_mutex_t players_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t players_not_full = PTHREAD_COND_INITIALIZER;

void addPlayerToGame(Player* player)
{
    pthread_mutex_lock(&players_mutex);

    while (gameInfo->currentSession->numPlayers >= MAX_CLIENTS) {
        pthread_cond_wait(&players_not_full, &players_mutex);
    }

    for(int i=0; i<MAX_CLIENTS; i++)
    {
        if(gameInfo->currentSession->players[i] == NULL)
        {
            gameInfo->currentSession->players[i] = player;
            gameInfo->currentSession->numPlayers++;
        }
    }
    pthread_mutex_unlock(&players_mutex);
}

void removePlayerFromGame(Player* player)
{
    pthread_mutex_lock(&players_mutex);

    if (gameInfo->currentSession->numPlayers == 0) return;

    for(int i=0; i<MAX_CLIENTS; i++)
    {
        if(gameInfo->currentSession->players[i] != NULL)
        {
            if(gameInfo->currentSession->players[i]->name == player->name)
            {
                free(player);
                gameInfo->currentSession->numPlayers--;
                gameInfo->currentSession->players[i] = NULL;
                pthread_cond_signal(&players_not_full);
                break;
            }
        }
    }
    pthread_mutex_unlock(&players_mutex);
}

void* handle_player(void* playerArg)
{
    Player* player = (Player*) playerArg;
    bool userRegistered;

    do{
        Message* msg = readMessage(player->socket_fd);

        switch(msg->type)
        {
            case MSG_REGISTRA_UTENTE:
            {
                bool foundUsername = false;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (gameInfo->currentSession->players[i] != NULL) {
                        if (strcmp(gameInfo->currentSession->players[i]->name, msg->data) == 0) {
                            sendTextMessage(player->socket_fd, MSG_ERR, NULL);
                            foundUsername = true;
                        }
                    }
                }
                if(!foundUsername){
                    copyString(&player->name, msg->data);
                    userRegistered = true;
                }
                free(msg);
                break;
            }
            case MSG_CLOSE_CLIENT:
                exitThread(msg, player);
                pthread_exit(NULL);
            default:
                free(msg);
                sendTextMessage(player->socket_fd, MSG_ERR, NULL);
                break;
        }
    } while(!userRegistered);

    addPlayerToGame(player);
    sendTextMessage(player->socket_fd, MSG_OK, NULL);
    sendMatrix(player);

    while(true)
    {
        Message* msg = readMessage(player->socket_fd);

        switch(msg->type)
        {
            case MSG_PAROLA:
                break;
            case MSG_MATRICE:
                sendMatrix(player);
                break;
            case MSG_CLOSE_CLIENT:
                exitThread(msg, player);
                pthread_exit(NULL);
            default:
                sendTextMessage(player->socket_fd, MSG_ERR, NULL);
                break;
        }

        free(msg);
    }
}

void sendMatrix(Player* player)
{
    if(gameInfo->currentSession->bIsPaused)
        sendNumMessage(player->socket_fd, MSG_TEMPO_ATTESA, 6);
    else{
        sendTextMessage(player->socket_fd, MSG_MATRICE, gameInfo->currentSession->currentMatrix);
        sendNumMessage(player->socket_fd, MSG_TEMPO_PARTITA, 3);
    }
}

void exitThread(Message* msg, Player* player)
{
    if(msg != NULL) free(msg);
    SYSCALL(close(player->socket_fd), "Errore nella chiusura del client");
    removePlayerFromGame(player);
}
