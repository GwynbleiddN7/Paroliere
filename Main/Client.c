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

#define INPUT_SPACE "\n\n---------------------------------------\n"

//Dichiaro le funzioni
bool decodeInput(char* string, char* cmd, char** arg); //Funzione per decodificare l'input dei comandi
void printMatrix(char** matrix); //Funzione per stampare a schermo la matrice di gioco
void* read_buffer(void* arg); //Funzione thread per leggere il buffer dal server

pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t output_available = PTHREAD_COND_INITIALIZER;

int client_fd;
bool bGameGoing = true;

volatile sig_atomic_t user_interrupted = 0;
void signal_handler(int signum) {
    if(signum == SIGUSR1) {
        user_interrupted = 1;
        pthread_exit(NULL);
    }
    else if(signum == SIGINT) {
        shutdown(client_fd, SHUT_RDWR);
    }
}

void* read_input(void* input)
{
    char** string_input = (char**)input;
    struct sigaction new_action;
    new_action.sa_handler = signal_handler;
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
    free(addr);

    if(syscall_fails_get(client_fd, socket(AF_INET, SOCK_STREAM, 0))) exitWithMessage("Errore creazione del socket");

    //Chiamata di sistema per connettermi al server
    if(syscall_fails(connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))))
    {
        close(client_fd); //Se non riesco a connettermi chiudo il socket senza controllare il risultato perché sto chiudendo il processo
        exitWithMessage("Impossibile connettersi al server"); //Chiudo il processo con un errore
    }

    struct sigaction new_action;
    new_action.sa_handler = signal_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGINT, &new_action, NULL);

    pthread_t* input_thread = malloc(sizeof(pthread_t));
    pthread_t read_thread;
    if (pthread_create(&read_thread, NULL, read_buffer, input_thread) != 0)
    {
        free(input_thread); //Libero la memoria appena allocata
        close(client_fd); //Se non riesco a creare il thread di lettura chiudo il socket senza controllare il risultato perché sto chiudendo il processo
        exitWithMessage("Errore creazione thread lettura"); //Chiudo il processo con un errore
    }

    printf("Connesso al server! Scrivi 'aiuto' per sapere i comandi\n");

    while(bGameGoing) //Game Loop
    {
        pthread_mutex_lock(&output_mutex);

        char* input = NULL;
        if (pthread_create(input_thread, NULL, read_input, &input) != 0)
        {
            printf("Errore creazione thread lettura"); //Chiudo il processo con un errore
            sendTextMessage(client_fd, MSG_CLOSE_CLIENT, NULL); //Richiedo la disconnessione dal gioco
            break;
        }

        pthread_join(*input_thread, NULL);

        if(user_interrupted == 1){
            if(input != NULL) free(input);
            printf("\n\n");
            pthread_cond_wait(&output_available, &output_mutex); //Attendo il completamento del risultato gestito dal thread di lettura
            user_interrupted = 0;
            pthread_mutex_unlock(&output_mutex);
            continue; //Se sono uscito correttamente ho un input altrimenti il processo è stato interrotto e non leggo

        }

        if(input == NULL){
            pthread_mutex_unlock(&output_mutex);
            continue; //Se sono uscito correttamente ho un input altrimenti il processo è stato interrotto e non leggo

        }

        char cmd;
        char* arg;
        if(!decodeInput(input, &cmd, &arg)) //Provo a decodificare l'input e assegno i dati a cmd e arg
        {
            printf("Comando non valido, scrivi 'aiuto' per sapere i comandi possibili");
            free(input);
            pthread_mutex_unlock(&output_mutex);
            continue;
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
                sendTextMessage(client_fd, MSG_PUNTI_FINALI, NULL); //Invio la parola da controllare
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
        free(input);
        pthread_mutex_unlock(&output_mutex);
    }
    close(client_fd); //Chiudo il canale di comunicazione con il server (e di conseguenza il thread se ne accorgerà ed eseguirà una procedura di uscita)
    pthread_join(read_thread, NULL); //Attendo la chiusura del thread causata dalla chiusura del socket
    free(input_thread); //Libero la memoria allocata
}

void* read_buffer(void* arg) //Funzione thread per leggere il buffer dal server
{
    pthread_t* thread_t = (pthread_t*)arg;
    while(true)
    {
        Message* msg = readMessage(client_fd); //Leggo il messaggio

        pthread_kill(*thread_t, SIGUSR1); //Interrompo la lettura da input, fermando il processo che si sta occupando della lettura
        pthread_mutex_lock(&output_mutex); //Acquisisco il lock

        if(msg == NULL) {
            //Nel caso di disconnessione con il server si ha un errore di lettura (ad esempio alla chiusura del socket)
            bGameGoing = false;
            pthread_cond_signal(&output_available);
            pthread_mutex_unlock(&output_mutex);
            pthread_exit(NULL);
        }

        switch (msg->type) {
            case MSG_ERR:
                printf("%s", msg->data);
                pthread_cond_signal(&output_available);
                break;
            case MSG_OK:
                printf("%s\n\n", msg->data);
                break;
            case MSG_PUNTI_FINALI:
                printf("SCOREBOARD: \n%s\n", msg->data);
                break;
            case MSG_PUNTI_PAROLA:
            {
                long num = getNumber(msg->data);
                if(num == 0) printf("Avevi già trovato questa parola!\n");
                else printf("Parola trovata!\n");
                printf("Punti: %ld", num);
                pthread_cond_signal(&output_available);
                break;
            }
            case MSG_TEMPO_PARTITA:
            {
                long seconds = getNumber(msg->data);
                int minutes = 0;
                while(seconds>=60 && (seconds = seconds - 60)) minutes++;
                printf("Tempo rimanente alla fine della partita: %d min e %ld sec", minutes, seconds); //Stampo il tempo rimanente
                pthread_cond_signal(&output_available);
                break;
            }
            case MSG_VINCITORE:
                printf("VINCITORE: %s\n", msg->data); //Stampo il tempo rimanente
                break;
            case MSG_TEMPO_ATTESA:
            {
                long seconds = getNumber(msg->data);
                int minutes = 0;
                while(seconds>=60 && (seconds = seconds - 60)) minutes++;
                printf("Il gioco è in pausa\nTempo di attesa per la prossima partita: %d min e %ld sec", minutes, seconds); //Il gioco è in pausa quindi trovo il tempo di attesa e lo stampo
                pthread_cond_signal(&output_available);
                break;
            }
            case MSG_MATRICE:
                printMatrix(getMatrix(msg->data)); //Stampo la matrice
                break;
        }
        deleteMessage(msg);
        pthread_mutex_unlock(&output_mutex);
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
    for(int i=0; i<MATRIX_SIZE; i++) free(matrix[i]);
    free(matrix);
}