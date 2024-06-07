#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/wait.h>
#include "../CommsHandler/CommsHandler.h"
#include "../Utility/Utility.h"

//Indice parametri
#define ADDR_INDEX 1
#define PORT_INDEX 2

//Definizione di keyword per gestire i comandi di input
#define HELP_IN 'h'
#define REGISTER_IN 'r'
#define MATRIX_IN 'm'
#define WORD_IN 'p'
#define END_IN 'q'
#define SCORE_IN 's'
#define UNKNOWN_IN '?'

//Definizione per la grafica
#define INPUT_SPACE "\n\n---------------------------------------\n"

//Dichiaro le funzioni
bool decodeInput(char* string, char* cmd, char** arg); //Funzione per decodificare l'input dei comandi
void printMatrix(char** matrix); //Funzione per stampare a schermo la matrice di gioco
void* readBuffer(void* arg); //Funzione thread per leggere il buffer dal server
void* readInput(void* input); //Funzione thread per leggere l'input da tastiera
void signalHandler(int signum); //Funzione callback per i segnali

//Variabili di condizione per l'I/O
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t output_available = PTHREAD_COND_INITIALIZER;

int client_fd; //Canale di comunicazione client/server
bool bGameGoing = true; //Stato del gioco
volatile sig_atomic_t user_interrupted = 0; //Flag utilizzata nel callback dei segnali

int main(int argc, char** argv)
{
    if(argc-1 != 2) exitWithMessage("Numero parametri non corretto"); //Controllo che i parametri in ingresso siano esattamente 2 (argc-1 per il nome file)

    char* addr = NULL;
    int port;
    if(!validatePort(argv[PORT_INDEX], &port)) exitWithMessage("Numero di porta non corretto"); //Controllo con una funzione se la porta inserita è valida, se non lo è interrompo
    if(!validateAddr(argv[ADDR_INDEX], &addr)) exitWithMessage("Indirizzo non valido"); //Controllo con una funzione se l'indirizzo è corretto, se non lo è interrompo

    //Creo il socket per connettermi al server
    struct sockaddr_in server_addr;

    //Imposto i dati per la connessione
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(addr);
    free(addr); //A questo punto non ho più bisogno dell'indirizzo

    if(syscall_fails_get(client_fd, socket(AF_INET, SOCK_STREAM, 0)))  //Creo il socket per la connessione al server
        exitWithMessage("Errore creazione del socket");

    //Provo a connettermi al server
    if(syscall_fails(connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))))
    {
        close(client_fd); //Se non riesco a connettermi chiudo il socket senza controllare il risultato perché sto chiudendo il processo
        exitWithMessage("Impossibile connettersi al server"); //Chiudo il processo con un errore
    }

    //Mask e handler per intercettare SIGINT
    struct sigaction new_action;
    new_action.sa_handler = signalHandler; //Handler personalizzato
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0; //Rimuovo la flag per ripristinare le chiamate di sistema
    sigaction(SIGINT, &new_action, NULL);

    pthread_t* input_thread = malloc(sizeof(pthread_t)); //Creo l'allocazione di memoria per condividere il TID del thread input
    pthread_t read_thread;
    if (pthread_create(&read_thread, NULL, readBuffer, input_thread) != 0) //Creo il thread per la lettura dei messaggi dal server
    {
        free(input_thread); //Libero la memoria prima di uscire
        close(client_fd); //Se non riesco a creare il thread di lettura chiudo il socket senza controllare il risultato perché sto chiudendo il processo
        exitWithMessage("Errore creazione thread lettura"); //Chiudo il processo con un errore
    }

    printf("Connesso al server! Scrivi 'aiuto' per sapere i comandi\n"); //Log

    while(bGameGoing) //Game Loop
    {
        pthread_mutex_lock(&output_mutex); //Acquisisco il lock dell'I/O

        char* input = NULL;
        if (pthread_create(input_thread, NULL, readInput, &input) != 0) //Creo il thread per la lettura da tastiera
        {
            printf("Errore creazione thread lettura"); //Se non riesco a creare il thread invio un messaggio
            sendTextMessage(client_fd, MSG_CLOSE_CLIENT, NULL); //E richiedo la disconnessione dal gioco
            break;
        }

        pthread_join(*input_thread, NULL); //Attendo un input da tastiera

        //Gestione arrivo messaggio asincrono dal server
        if(user_interrupted == 1){ //Se il thread di input utente è stato interrotto da un SIGUSR1 proveniente dal thread di lettura dal server
            if(input != NULL) free(input); //Provo a liberare la memoria per sicurezza
            printf("\n\n");
            pthread_cond_wait(&output_available, &output_mutex); //Attendo il completamento del risultato gestito dal thread di lettura
            user_interrupted = 0; //Resetto la flag
            pthread_mutex_unlock(&output_mutex); //Sblocco il lock
            continue; //Ripropongo il prompt all'utente

        }

        if(input == NULL){ //Se c'è stato un errore di lettura o se l'utente ha premuto solamente invio
            pthread_mutex_unlock(&output_mutex); //Sblocco il lock
            continue; //Ripropongo il prompt all'utente
        }

        char cmd;
        char* arg;
        if(!decodeInput(input, &cmd, &arg)) //Provo a decodificare l'input e assegno i dati a cmd e arg
        {
            printf("Comando non valido, scrivi 'aiuto' per sapere i comandi possibili"); //In caso di input errato
            free(input); //Libero la memoria
            pthread_mutex_unlock(&output_mutex); //Libero il lock
            continue; //Ripropongo il prompt all'utente
        }

        switch (cmd) {
            case HELP_IN:
                //Comando di aiuto e stampo i comandi possibili
                printf("COMANDI DISPONIBILI:\n"
                       "- aiuto -> Stampa questo messaggio\n"
                       "- registra_utente *nome* -> Registrati alla partita con un nome di una parola\n"
                       "- matrice -> Richiedi la matrice di gioco e/o il tempo di partita/attesa\n"
                       "- punti -> Scopri quanti punti hai guadagnato finora\n"
                       "- p *parola* -> Prova ad indovinare una parola\n"
                       "- fine -> Esci dal gioco"
                );
                break;
            case REGISTER_IN:
                printf("Richiesta registrazione in corso...\n"); //Testo da stampare in caso gli slot di gioco siano pieni e rimane in attesa
                sendTextMessage(client_fd, MSG_REGISTRA_UTENTE, arg); //Mando il messaggio per richiedere la registrazione, risultato gestito dal thread di lettura
                pthread_cond_wait(&output_available, &output_mutex); //Attendo il completamento del risultato gestito dal thread di lettura
                break;
            case MATRIX_IN:
                sendTextMessage(client_fd, MSG_MATRICE, NULL); //Richiedo la matrice con un messaggio, risultato gestito dal thread di lettura
                pthread_cond_wait(&output_available, &output_mutex); //Attendo il completamento del risultato gestito dal thread di lettura
                break;
            case WORD_IN:
                sendTextMessage(client_fd, MSG_PAROLA, arg); //Invio la parola da controllare, risultato gestito dal thread di lettura
                pthread_cond_wait(&output_available, &output_mutex); //Attendo il completamento del risultato gestito dal thread di lettura
                break;
            case SCORE_IN:
                sendTextMessage(client_fd, MSG_PUNTI_FINALI, NULL); //Invio la richiesta per visualizzare la scoreboard
                pthread_cond_wait(&output_available, &output_mutex); //Attendo il completamento del risultato gestito dal thread di lettura
                break;
            case END_IN:
                sendTextMessage(client_fd, MSG_CLOSE_CLIENT, NULL); //Richiedo la disconnessione dal gioco
                bGameGoing = false; //Esco dal Game Loop
                break;
            default:
                printf("Comando non valido, scrivi 'aiuto' per sapere i comandi possibili"); //Failsafe per altri comandi
                break;
        }
        free(input); //Libero la memoria del comando di input
        pthread_mutex_unlock(&output_mutex); //Sblocco il lock
    }
    close(client_fd); //Chiudo il canale di comunicazione con il server (e di conseguenza il thread se ne accorgerà ed eseguirà una procedura di uscita)
    pthread_join(read_thread, NULL); //Attendo la chiusura del thread causata dalla chiusura del socket
    free(input_thread); //Libero la memoria allocata
}


void* readInput(void* input) //Funzione thread per leggere l'input da tastiera
{
    char** string_input = (char**)input;
    struct sigaction new_action;
    new_action.sa_handler = signalHandler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGUSR1, &new_action, NULL);

    printf("%s\n", INPUT_SPACE);
    printf("[PROMPT PAROLIERE]-->");
    fflush(stdout);

    size_t input_size;
    size_t bytes_read = getline(string_input, &input_size, stdin); //getline() per prendere una riga da stdin
    printf("\n");
    if(bytes_read <= 1) {
        if(*string_input != NULL) free(*string_input);
        *string_input = NULL;
    }
    pthread_exit(NULL);
}


void* readBuffer(void* arg) //Funzione thread per leggere il buffer dal server
{
    pthread_t* thread_t = (pthread_t*)arg; //Casting del parametro
    while(true)
    {
        Message* msg = readMessage(client_fd); //Leggo il messaggio

        pthread_kill(*thread_t, SIGUSR1); //Interrompo la lettura da input, inviando il segnale SIGUSR1 al thread che si occupata dell'input
        pthread_mutex_lock(&output_mutex); //Acquisisco il lock dell'I/O

        if(msg == NULL) {
            //Nel caso di disconnessione con il server si ha un errore di lettura (ad esempio alla chiusura del socket)
            bGameGoing = false; //Interrompo il gioco
            pthread_cond_broadcast(&output_available); //In caso di errore avvenuto in precedenza sblocco eventuali task in attesa che andranno a terminare
            pthread_mutex_unlock(&output_mutex); //Sblocco il lock
            pthread_exit(NULL); //Esco dal thread
        }

        switch (msg->type) {
            case MSG_ERR:
                printf("%s", msg->data); //Stampo il messaggio
                pthread_cond_signal(&output_available); //Ultimo comando della catena quindi invio un segnale ai task in attesa
                break;
            case MSG_OK:
                printf("%s\n\n", msg->data); //Stampo il messaggio
                break;
            case MSG_PUNTI_FINALI:
                printf("SCOREBOARD: \n%s\n", msg->data); //Visualizzo la scoreboard
                break;
            case MSG_PUNTI_PAROLA:
            {
                //Gestione del risultato della parola
                long num = getNumber(msg->data);
                if(num == 0) printf("Avevi già trovato questa parola!\n");
                else printf("Parola trovata!\n");
                printf("Punti: %ld", num);
                pthread_cond_signal(&output_available); //Ultimo comando della catena quindi invio un segnale ai task in attesa
                break;
            }
            case MSG_TEMPO_PARTITA:
            {
                //Il gioco è in corso quindi trovo il tempo di rimanente e lo stampo
                long seconds = getNumber(msg->data);
                int minutes = secondsToMinutes(&seconds);
                printf("Tempo rimanente alla fine della partita: %ld secondi (%d min e %ld sec)", seconds+(minutes*60), minutes, seconds);
                pthread_cond_signal(&output_available); //Ultimo comando della catena quindi invio un segnale ai task in attesa
                break;
            }
            case MSG_VINCITORE:
                printf("VINCITORE: %s\n", msg->data); //Stampo il tempo rimanente
                break;
            case MSG_TEMPO_ATTESA:
            {
                //Il gioco è in pausa quindi trovo il tempo di attesa e lo stampo
                long seconds = getNumber(msg->data);
                int minutes = secondsToMinutes(&seconds);
                printf("Il gioco è in pausa\nTempo di attesa per la prossima partita: %ld secondi (%d min e %ld sec)", seconds+(minutes*60), minutes, seconds);
                pthread_cond_signal(&output_available); //Ultimo comando della catena quindi invio un segnale ai task in attesa
                break;
            }
            case MSG_MATRICE:
                printMatrix(getMatrix(msg->data)); //Stampo la matrice
                break;
        }
        deleteMessage(msg); //Elimino il messaggio
        pthread_mutex_unlock(&output_mutex); //Sblocco il lock
    }
}


void signalHandler(int signum) //Funzione callback per i segnali
{
    if(signum == SIGUSR1) { //Se ricevo un SIGUSR1 devo interrompere la lettura da tastiera
        user_interrupted = 1; //Setto la flag
        pthread_exit(NULL); //Esco dal thread (segnale inviato e intercettato solamente al thread che si occupa della lettura da tastiera)
    }
    else if(signum == SIGINT) { //Se ricevo un SIGINT (CTRL+C)
        shutdown(client_fd, SHUT_RDWR); //Interrompo il canale di comunicazione con il server in modo che il programma inizi la procedura di terminazione
    }
}


bool decodeInput(char* string, char* cmd, char** arg) //Funzione per decodificare l'input dei comandi
{
    char* trimmed = strtok(string, "\n"); //Rimuovo lo \n salvato nel buffer dalla getline()
    char* command = strtok(trimmed, " "); //Tokenizzo la stringa in parole

    //La prima parola corrisponde al comando e lo decodifico in un char per usarlo nello switch
    char c = UNKNOWN_IN;
    if(strcmp(command, "aiuto") == 0) c = HELP_IN;
    else if(strcmp(command, "registra_utente") == 0) c = REGISTER_IN;
    else if(strcmp(command, "matrice") == 0) c = MATRIX_IN;
    else if(strcmp(command, "punti") == 0) c = SCORE_IN;
    else if(strcmp(command, "p") == 0) c = WORD_IN;
    else if(strcmp(command, "fine") == 0) c = END_IN;

    *cmd = c; //Assegno alla variabile per reference il comando
    *arg = strtok(NULL, " "); //Assegno alla variabile per reference il parametro (token successivo)

    //Se ci sono parametri e non sta eseguendo un comando che non li prevede ritorno false
    if(*arg != NULL && (c != REGISTER_IN && c != WORD_IN)) return false;
    //Se NON ci sono parametri e sta eseguendo un comando che li prevede ritorno false
    if(*arg == NULL && (c == REGISTER_IN || c == WORD_IN)) return false;
    //Se ci sono parametri e ne trova un altro sono troppi (max comando + parametro) e ritorno false
    if(*arg != NULL && strtok(NULL, " ") != NULL) return false;

    return true; //Se passo i controlli ritorno true
}


void printMatrix(char** matrix) //Funzione per stampare a schermo la matrice di gioco
{
    printf("GAMEBOARD:\n----------\n");
    for(int i=0; i<MATRIX_SIZE; i++)
    {
        for(int j=0; j<MATRIX_SIZE; j++)
        {
            if(matrix[i][j] == 'q') printf("Qu "); //Se trovo q scrivo Qu
            else printf("%c  ", toupper(matrix[i][j])); //Altrimenti scrivo l'elemento della matrice corrispondente
        }
        printf("\n"); //Cambio riga
    }
    printf("----------\n");

    //Libero la memoria occupata dalla matrice
    for(int i=0; i<MATRIX_SIZE; i++) free(matrix[i]);
    free(matrix);
}