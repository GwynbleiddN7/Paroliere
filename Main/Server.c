#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <getopt.h>
#include <signal.h>
#include "../CommsHandler/CommsHandler.h"
#include "../Utility/Utility.h"
#include "../GameState/GameState.h"

//Indice parametri
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

//Dichiaro le funzioni
static void freeGameMem(); //Funzione per liberare la memoria occupata dalle strutture di gioco
static void getOptionalParameters(int argc, char** argv); //Funzione per leggere i parametri opzionali
static void* start_server(); //Funzione del thread per avviare il server
static void* handle_player(void* playerArg); //Funzione del thread per gestire una nuova connessione
static bool addPlayerToGame(Player* player); //Funzione per aggiungere un player al game
static void removePlayerFromGame(Player* player); //Funzione per rimuovere un player dal game
static void deletePlayer(Player* player); //Funzione per interrompere il thread giocatore liberando correttamente la memoria
static void sendMatrix(Player* player); //Funzione per inviare un messaggio con la matrice e/o i tempi di gioco/attesa
static long getWordScore(char* word);
void* scorer();

static GameInfo* gameInfo; //Struct condivisa che contiene tutte le informazioni del gioco

//Variabili di condizione per gestire la lista di giocatori
pthread_mutex_t players_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t players_not_full = PTHREAD_COND_INITIALIZER;

pthread_mutex_t score_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t score_cond = PTHREAD_COND_INITIALIZER;

int main(int argc, char** argv)
{
    //Validazione parametri obbligatori
    if(argc-1 < 2) exitWithMessage("Nome host e numero porta sono obbligatori"); //Controllo che ci siamo almeno i primi 2 parametri (-1 per il nome del file)
    char* addr = argv[ADDR_INDEX]; //Prendo il primo parametro che corrisponde all'hostname
    int port;
    if(!validatePort(argv[PORT_INDEX], &port)) exitWithMessage("Numero di porta non corretto"); //Controllo con una funzione se la porta inserita è valida, se non lo è interrompo

    //Inizializzazione struttura di gioco
    gameInfo = initGameInfo(addr, port); //Inizializzo la struct che contiene le informazioni del gioco
    getOptionalParameters(argc, argv); //Aggiorno la struct con i parametri opzionali
    srand(gameInfo->seed); //Inizializzo srand con il seed time(0) o con quello inserito come parametro

    //Carico il dizionario da file
    gameInfo->dictionary = loadDictionary(gameInfo->dictionaryFile);
    if(gameInfo->dictionary == NULL)
    {
        freeGameMem();
        exitWithMessage("Errore nella lettura del dizionario");
    }

    //Inizializzazione sessione di gioco
    if(!initGameSession(gameInfo))
    {
        freeGameMem();
        exitWithMessage("Errore nella creazione della matrice");
    }

    //Gestione del segnale SIGINT per terminare
    sigset_t set; int sig;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_SETMASK, &set, NULL);

    //Creazione di un thread che gestisce il server
    pthread_t server_thread;
    if (pthread_create(&server_thread, NULL, start_server, NULL) != 0) {
        freeGameMem(); //Se non riesco a creare il thread libero la memoria e chiudo il processo
        exitWithMessage("Errore creazione thread server");
    }

    sigwait(&set,&sig);
    printf("\nInterruzione del gioco in corso...\n");
    fflush(stdout);

    //Chiusura del socket che induce l'interruzione del server_thread (con shutdown mi assicuro che la accept venga interrotta)
    shutdown(gameInfo->serverSocket, SHUT_RDWR); //Non eseguo controlli sul risultato perché sto chiudendo il programma
    close(gameInfo->serverSocket); //Non eseguo controlli sul risultato perché sto chiudendo il programma
    pthread_join(server_thread, NULL); //Attendo la chiusura del server_thread (se è ancora attivo)

    freeGameMem(); //Libero tutta la memoria occupata dalle strutture di gioco
}

void getOptionalParameters(int argc, char** argv) //Funzione per leggere i parametri opzionali
{
    int option; opterr = 0;
    while ((option = getopt_long_only(argc, argv, "m:t:s:d:", long_options, NULL)) != -1) //Uso gli opt_long per prendere i 4 possibili parametri opzionali m t s d (m:t:s:d: indica che hanno un valore dopo)
    {
        switch (option)
        {
            case 'm':
                if(gameInfo->customMatrixType == Random)
                    exitWithMessage("Seed e File matrice non possono essere presenti contemporaneamente"); //Se Random era settato ho già un'opzione e interrompo
                if(!updateMatrixFile(gameInfo, optarg)) exitWithMessage("File matrice non esistente o non corretto"); //Se il file non esiste interrompo, altrimenti lo salvo nella struct
                break;
            case 't':
            {
                int duration;
                if(!strToInt(optarg, &duration) || duration <= 0) exitWithMessage("Durata di gioco non valido"); //Se il valore della durata è un intero lo converto e lo salvo nella struct, altrimenti interrompo
                gameInfo->gameDuration = duration * 60;
                break;
            }
            case 's':
                if(gameInfo->customMatrixType == File)
                    exitWithMessage("Seed e File matrice non possono essere presenti contemporaneamente"); //Se File era settato ho già un'opzione e interrompo
                if(!strToInt(optarg, &gameInfo->seed)) exitWithMessage("Seed non valido"); //Se il valore del seed è un intero lo converto e lo salvo nella struct, altrimenti interrompo
                gameInfo->customMatrixType = Random; //Aggiorno la flag per sapere in quale caso mi trovo
                break;
            case 'd':
                if(!updateDictionaryFile(gameInfo, optarg))
                    exitWithMessage("File dizionario non esistente o non corretto"); //Se il file non esiste interrompo, altrimenti lo salvo nella struct
                break;
            default:
                exitWithMessage("Parametri aggiuntivi non riconosciuti"); //Se trovo altri parametri interrompo
        }
    }
    if (optind != argc-2) exitWithMessage("Parametri aggiuntivi non riconosciuti"); //Se trovo altri parametri interrompo
}

void timeTick(int signum) //Funzione callback per SIGALARM
{
    if(gameInfo == NULL || gameInfo->currentSession == NULL) return;
    if(gameInfo->currentSession->gamePhase == Off) return;

    gameInfo->currentSession->timeToNextPhase--; //Decremento il counter del tempo
    if(gameInfo->currentSession->timeToNextPhase <= 0)
    {
        //Aggiorno la fase di gioco
        if(gameInfo->currentSession->gamePhase == Paused)
        {
            gameInfo->currentSession->gamePhase = Playing;
            gameInfo->currentSession->timeToNextPhase = 10;//gameInfo->gameDuration;
        }
        else
        {
            gameInfo->currentSession->gamePhase = Paused;
            gameInfo->currentSession->timeToNextPhase = 10;//PAUSE_TIME;
        }

        //Avvio il thread che si occupa degli score
        pthread_t scorer_thread;
        pthread_create(&scorer_thread, NULL, scorer, NULL);
        pthread_join(scorer_thread, NULL);

        if(gameInfo->currentSession->gamePhase == Paused) nextGameSession(gameInfo); //Se il gioco entrato in pausa creo la nuova sessione
    }
    alarm(1); //Riavvio il counter
}

void* start_server() //Funzione del thread per avviare il server
{
    struct sockaddr_in server_addr, client_addr;

    //Creo il socket del server
    if(syscall_fails_get(gameInfo->serverSocket, socket(AF_INET, SOCK_STREAM, 0))) {
        perror("Errore creazione server socket"); //Invio il messaggio di errore
        kill(getpid(), SIGINT); //Lascio gestire la chiusura all'intercept personalizzato del SIGINT
        pthread_exit(NULL);
    }

    //Imposto i dati per la connessione
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(gameInfo->serverPort);
    server_addr.sin_addr.s_addr = inet_addr(gameInfo->serverName);

    //Avvio il server e rimango in attesa di MAX_CLIENTS
    if(syscall_fails(bind(gameInfo->serverSocket, (struct sockaddr *) &server_addr, sizeof(server_addr))) || syscall_fails(listen(gameInfo->serverSocket, MAX_CLIENTS))){
        perror("Errore avvio del server"); //Invio il messaggio di errore
        kill(getpid(), SIGINT); //Lascio gestire la chiusura all'intercept personalizzato del SIGINT
        pthread_exit(NULL);
    }

    gameInfo->currentSession->gamePhase = Paused; //Se il server stato creato avvio il gioco in pausa

    //Gestione per intercettare SIGALARM
    struct sigaction act;
    act.sa_handler = &timeTick;
    act.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &act, NULL);

    alarm(1); //Avvio il gioco tramite il segnale SIGALARM intercettato da timeTick

    socklen_t client_addr_len = sizeof(client_addr);
    while(true){
        //Accetto una nuova connessione da un client
        int fd_client;
        if(syscall_fails_get(fd_client,accept(gameInfo->serverSocket, (struct sockaddr *) &client_addr, &client_addr_len))) break;

        //Alloco la memoria per il nuovo player e lo inizializzo
        Player* player = malloc(sizeof(Player));
        player->socket_fd = fd_client;
        player->foundWords = createArray();
        player->score = 0;
        player->bRegistered = false;

        //Lascio gestire il player a un nuovo thread
        if (pthread_create(&player->thread[Main], NULL, handle_player, (void*) player) != 0)
        {
            //Se non riesco a creare un thread per un client continuo a giocare ma avviso con un errore e libero la memoria del player
            free(player);
            perror("Errore creazione thread client");
        }
    }

    pthread_exit(NULL);
}

void* scorer()
{
    pthread_mutex_lock(&players_mutex); //Evito che altri player possano unirsi al game durante il rilascio dei punti

    //Invio il messaggio asincrono ai player
    for(int i = 0; i<MAX_CLIENTS; i++)
        if(gameInfo->currentSession->players[i] != NULL) pthread_kill(gameInfo->currentSession->players[i]->thread[Write], SIGUSR1);

    if(gameInfo->currentSession->gamePhase == Playing) pthread_exit(NULL); //Se non devo mandare gli score esco (cioè da Paused è passata a Playing)

    pthread_mutex_lock(&score_mutex);
    int listSize;
    while(listSize < gameInfo->currentSession->numPlayers) pthread_cond_wait(&score_cond, &score_mutex);
    //get winner
    pthread_cond_broadcast(&score_cond);
    pthread_mutex_unlock(&score_mutex);
    pthread_mutex_unlock(&players_mutex);
    pthread_exit(NULL);
}

void* player_write(void* playerArg)
{
    Player* player = (Player*) playerArg; //Casting del parametro a Player*
    sigset_t set; int sig;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    pthread_sigmask(SIG_SETMASK, &set, NULL);

    while (true)
    {
        sigwait(&set, &sig);
        if(!player->bRegistered) break;
        if(gameInfo->currentSession->gamePhase == Paused) //Se il gioco sta terminando (cioè da Playing è passato a Paused)
        {
            pthread_mutex_lock(&score_mutex);
            //add to list
            pthread_cond_wait(&score_cond, &score_mutex);
            //send score
            pthread_mutex_unlock(&score_mutex);
        }
        else sendMatrix(player); //Se il gioco sta ricominciando mando la matrice
    }
    pthread_exit(NULL);
}

void* player_read(void* playerArg)
{
    Player* player = (Player*) playerArg; //Casting del parametro a Player*

    //Game Loop
    while(true)
    {
        Message* msg = readMessage(player->socket_fd); //Rimango in attesa di messaggi da parte del client

        if(msg == NULL) //Errore di lettura, probabile crash del client quindi chiudo il thread correttamente rimuovendo il giocatore dal game
        {
            deleteMessage(msg); //Elimino il messaggio
            pthread_exit(NULL); //Chiudo il thread
        }

        if(gameInfo->currentSession->gamePhase == Off) //Nel caso in cui arrivi un messaggio mentre il server sta chiudendo (molto improbabile)
        {
            sendTextMessage(player->socket_fd, MSG_ERR, "Il gioco sta terminando"); //Provo ad avvisare il client se non ho già chiuso il socket
            deleteMessage(msg);
            continue;
        }

        switch(msg->type)
        {
            case MSG_REGISTRA_UTENTE:
            {
                if(player->bRegistered) sendTextMessage(player->socket_fd, MSG_ERR, "Sei già stato registrato nella partita"); //Utente già registrato, invio un errore al client
                else{
                    int nameLen = (int)strlen(msg->data);
                    bool usernameValid = nameLen <= 10;
                    //Controllo validità caratteri
                    for(int i=0; i<nameLen && usernameValid; i++)
                    {
                        if(!isalnum(msg->data[i]))
                        {
                            usernameValid = false;
                            break;
                        }
                        msg->data[i] = (char)tolower(msg->data[i]);
                    }
                    if(!usernameValid)
                    {
                        sendTextMessage(player->socket_fd, MSG_ERR, "Nome non valido, usa massimo 10 caratteri alfanumerici"); //Invio un messaggio di errore al client
                        break;
                    }
                    //Richiesta di registrazione
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (gameInfo->currentSession->players[i] != NULL) {
                            if (strcmp(gameInfo->currentSession->players[i]->name, msg->data) == 0) { //Se trovo un utente con lo stesso nome interrompo
                                usernameValid = false;
                                break;
                            }
                        }
                    }
                    if(!usernameValid)
                    {
                        sendTextMessage(player->socket_fd, MSG_ERR, "Nome già presente, riprova con uno differente"); //Invio un messaggio di errore al client
                        break;
                    }
                    //Se i controlli sono stati passati registro
                    copyString(&player->name, msg->data); //Copiando il nome inviato nel messaggio nella struct Player
                    //Aggiungo il player al game
                    if(!addPlayerToGame(player)) //Ritorna false se gli slot sono tutti occupati
                    {
                        sendTextMessage(player->socket_fd, MSG_ERR, "Non ci sono altri slot disponibili, riprova più tardi"); //Invio un messaggio di errore al client
                        break;
                    }
                    player->bRegistered = true; //Setto la flag di controllo
                    sendTextMessage(player->socket_fd, MSG_OK, "Utente registrato correttamente!"); //Invio un messaggio di conferma
                    sendMatrix(player); //Invio la matrice e/o il tempo di gioco/attesa
                }
                break;
            }
            case MSG_PAROLA:
            {
                if(!player->bRegistered) sendTextMessage(player->socket_fd, MSG_ERR, "Devi registrarti per usare questo comando");
                else{
                    if(gameInfo->currentSession->gamePhase == Paused) sendTextMessage(player->socket_fd, MSG_ERR, "Il gioco è attualmente in pausa");
                    else
                    {
                        char* word = msg->data;
                        for(int i=0; i<strlen(msg->data); i++) word[i] = (char)tolower(word[i]);

                        bool wordFound = findInMatrix(gameInfo->currentSession->currentMatrix, word);
                        if(!wordFound) {
                            sendTextMessage(player->socket_fd, MSG_ERR, "Parola non presente nella matrice");
                            break;
                        }
                        bool wordExists = findInTrie(gameInfo->dictionary, word, 0);
                        if(!wordExists) {
                            sendTextMessage(player->socket_fd, MSG_ERR, "Parola non valida");
                            break;
                        }
                        bool wordAlreadyFound = findInArray(player->foundWords, word);
                        if(wordAlreadyFound) sendNumMessage(player->socket_fd, MSG_PUNTI_PAROLA, 0);
                        else{
                            addToArray(player->foundWords, word);
                            long score = getWordScore(msg->data);
                            sendNumMessage(player->socket_fd, MSG_PUNTI_PAROLA, score);
                            player->score += (int)score;
                        }
                    }
                }
                break;
            }
            case MSG_PUNTI_FINALI:
                if(!player->bRegistered) sendTextMessage(player->socket_fd, MSG_ERR, "Devi registrarti per usare questo comando");
                else sendNumMessage(player->socket_fd, MSG_PUNTI_FINALI, (long) player->score);
                break;
            case MSG_MATRICE:
                if(!player->bRegistered) sendTextMessage(player->socket_fd, MSG_ERR, "Devi registrarti per usare questo comando");
                else sendMatrix(player); //Se viene richiesta un matrice mando la matrice e/o il tempo di gioco/attesa
                break;
            case MSG_CLOSE_CLIENT:
                deleteMessage(msg);
                pthread_exit(NULL); //Esco dal thread
            default:
                sendTextMessage(player->socket_fd, MSG_ERR, "Comando non valido"); //Se il messaggio non è valido invio un messaggio di errore
                break;
        }
        deleteMessage(msg); //Libero la memoria occupata dal messaggio
    }
}

void* handle_player(void* playerArg) //Funzione del thread per gestire una nuova connessione
{
    Player* player = (Player*) playerArg; //Casting del parametro a Player*

    pthread_create(&player->thread[Read], NULL, player_read, playerArg);
    pthread_create(&player->thread[Write], NULL, player_write, playerArg);

    pthread_join(player->thread[Read], NULL);
    player->bRegistered = false; //Se si chiude il thread di lettura il player non è più registrato

    pthread_kill(player->thread[Write], SIGUSR1); //Invio un segnale per far chiudere il thread di lettura
    pthread_join(player->thread[Write], NULL); //Aspetto che si chiuda

    deletePlayer(player); //Elimino la memoria occupata dal player
    pthread_exit(NULL);
}

bool addPlayerToGame(Player* player) //Funzione per aggiungere un player al game
{
    pthread_mutex_lock(&players_mutex); //Inizio della sezione critica con l'acquisizione di un lock

    while (gameInfo->currentSession->numPlayers >= MAX_CLIENTS) {
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

void removePlayerFromGame(Player* player) //Funzione per rimuovere un player dal game
{
    pthread_mutex_lock(&players_mutex); //Inizio della sezione critica con l'acquisizione di un lock

    for(int i=0; i<MAX_CLIENTS && gameInfo->currentSession->numPlayers > 0; i++)
    {
        if(gameInfo->currentSession->players[i] != NULL) //Per ogni player non NULL nella lista
        {
            if(gameInfo->currentSession->players[i] == player) //Controllo che abbia lo stesso nome di quello che cerco
            {
                gameInfo->currentSession->players[i] = NULL; //Rendo disponibile lo slot
                gameInfo->currentSession->numPlayers--; //Decremento il numero di player in game
                pthread_cond_signal(&players_not_full); //Segnalo a un eventuale thread in pausa nella wait che si è liberato uno slot
                break;
            }
        }
    }

    pthread_mutex_unlock(&players_mutex); //Sblocco il lock alla fine della sezione critica
}

void deletePlayer(Player* player)
{
    if(player == NULL) return; //Failsafe
    if(gameInfo->currentSession->gamePhase != Off) { //Se devo rendere disponibile un nuovo slot (false quando viene chiamata da freeGameMem ~ non rischio di chiudere due volte il socket)
        removePlayerFromGame(player); //Se il game è in corso (quindi non sto chiudendo il server), rendo disponibile lo slot
        close(player->socket_fd); //Chiudo la connessione con il client del player in questione (se c'è un errore il socket è già chiuso o comunque procedo a liberare la memoria)
    }
    freeArray(player->foundWords); //Libero la memoria occupata dal Trie delle parole trovate
    free(player->name); //Libero la memoria occupata dal nome
    free(player); //Libero la memoria occupata dalla struct
}

void freeGameMem()
{
    if(gameInfo == NULL) return; //Failsafe
    if(gameInfo->currentSession != NULL)
    {
        gameInfo->currentSession->gamePhase = Off;
        for(int i=0; i<MAX_CLIENTS; i++) {
            Player* player = gameInfo->currentSession->players[i];
            if(player == NULL) continue;
            //Chiudo la connessione con il client del player che induce la terminazione del thread (con shutdown mi assicuro che la read venga interrotta e quindi il thread interrotto)
            shutdown(player->socket_fd, SHUT_RDWR); //Non eseguo controlli sul risultato perché sto chiudendo il programma
            close(player->socket_fd); //Non eseguo controlli sul risultato perché sto chiudendo il programma
            pthread_join(player->thread[Main], NULL); //Attendo la chiusura del thread che si occupa di liberare la memoria se il socket era ancora aperto
        }
        free(gameInfo->currentSession); //Libero la memoria occupata dalla struct della sessione
    }
    if(gameInfo->dictionary != NULL) freeTrie(gameInfo->dictionary); //Libero la memoria occupata dal Trie del dizionario
    if(gameInfo->serverName != NULL) free(gameInfo->serverName); //Libero la memoria occupata dal nome del server
    if(gameInfo->matrixFile != NULL) free(gameInfo->matrixFile); //Libero la memoria occupata dal file matrice
    if(gameInfo->dictionaryFile != NULL) free(gameInfo->dictionaryFile); //Libero la memoria occupata dal file dizionario

    free(gameInfo); //Libero la memoria occupata dalla struct del gioco
}

void sendMatrix(Player* player) //Funzione per inviare un messaggio con la matrice e/o i tempi di gioco/attesa
{
    if(gameInfo->currentSession->gamePhase == Paused)
        sendNumMessage(player->socket_fd, MSG_TEMPO_ATTESA, gameInfo->currentSession->timeToNextPhase); //Se il gioco è in pausa invio il tempo di attesa
    else{
        sendTextMessage(player->socket_fd, MSG_MATRICE, gameInfo->currentSession->currentMatrix); //Se il gioco è in corso invio la matrice
        sendNumMessage(player->socket_fd, MSG_TEMPO_PARTITA, gameInfo->currentSession->timeToNextPhase); //E il tempo rimanente
    }
}

long getWordScore(char* word) //Funzione per contare lo score di una parola
{
    int counter = 0;
    for(int i=0; i<strlen(word); i++) if(word[i] != 'q') counter++; //Gestisco il caso Qu che deve valere 1
    return counter;
}