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

//Dichiaro le funzioni
bool decodeInput(char* string, char* cmd, char** arg); //Funzione per decodificare l'input dei comandi
void printMatrix(char** matrix); //Funzione per stampare a schermo la matrice di gioco
void* read_buffer(void* arg); //Funzione thread per leggere il buffer dal server

pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t output_available = PTHREAD_COND_INITIALIZER;

int client_fd;
bool bGameGoing = true;

int main(int argc, char** argv)
{
    if(argc-1 != 2) exitWithMessage("Numero parametri non corretto"); //Controllo che i parametri in ingresso siano esattamente 2 (argc-1 per il nome file)

    char* addr = argv[ADDR_INDEX]; //Prendo il primo parametro che corrisponde all'hostname
    int port;
    if(!validatePort(argv[PORT_INDEX], &port)) exitWithMessage("Numero di porta non corretto"); //Controllo con una funzione se la porta inserita è valida, se non lo è interrompo

    //Creo il socket per connettermi al server

    struct sockaddr_in server_addr;
    if(syscall_fails_get(client_fd, socket(AF_INET, SOCK_STREAM, 0))) exitWithMessage("Errore creazione del socket");

    //Imposto i dati per la connessione
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(addr);

    //Chiamata di sistema per connettermi al server
    if(syscall_fails(connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))))
    {
        close(client_fd); //Se non riesco a connettermi chiudo il socket senza controllare il risultato perché sto chiudendo il processo
        exitWithMessage("Impossibile connettersi al server"); //Chiudo il processo con un errore
    }

    pid_t* reader_pid = malloc(sizeof(pid_t));
    pthread_t read_thread;
    if (pthread_create(&read_thread, NULL, read_buffer, reader_pid) != 0)
    {
        free(reader_pid); //Libero la memoria appena allocata
        close(client_fd); //Se non riesco a creare il thread di lettura chiudo il socket senza controllare il risultato perché sto chiudendo il processo
        exitWithMessage("Errore creazione thread lettura"); //Chiudo il processo con un errore
    }

    printf("Connesso al server! Scrivi 'aiuto' per sapere i comandi\n");

    while(bGameGoing) //Game Loop
    {
        char* input = NULL;
        size_t input_size;

        int input_pipe[2];
        if(syscall_fails(pipe(input_pipe)))
        {
            perror("Errore nella creazione della pipe");
            break;
        }

        int pid;
        if(syscall_fails_get(pid, fork()))
        {
            perror("Errore nella creazione della processo di lettura input");
            break;
        }

        if(pid == 0) {
            close(input_pipe[0]); //Se non riesco a chiudere la pipe in lettura provo comunque a continuare il gioco perché non è un errore non compromettente

            printf("\n[PROMPT PAROLIERE]-->");
            fflush(stdout);
            getline(&input, &input_size, stdin); //getline() per prendere una riga da stdin

            //Invio al processo padre il risultato della lettura con due write differenti (se le write falliscono esco con errore in modo che la lettura venga ignorata e il processo riavviato)
            if(syscall_fails(write(input_pipe[1], &input_size, sizeof(size_t)))) exit(EXIT_FAILURE);
            if(syscall_fails(write(input_pipe[1], input, input_size))) exit(EXIT_FAILURE);

            close(input_pipe[1]); //Se non riesco a chiudere la pipe in scrittura provo comunque a continuare il gioco perché non è un errore non compromettente
            exit(EXIT_SUCCESS);
        }

        close(input_pipe[1]); //Se non riesco a chiudere la pipe in scrittura provo comunque a continuare il gioco perché non è un errore non compromettente

        int status;
        *reader_pid = pid; //Aggiorno il pid del nuovo processo per il thread
        waitpid(pid, &status, 0); //Attendo la scrittura da parte del processo figlio
        if(!WIFEXITED(status)) continue; //Se sono uscito correttamente ho un input altrimenti il processo è stato interrotto e non leggo

        if(read_fails(input_pipe[0], &input_size, sizeof(size_t))) { //Se il thread è stato interrotto o c'è stato un errore di lettura riprovo
            close(input_pipe[0]); //Se non riesco a chiudere la pipe in lettura provo comunque a continuare il gioco perché non è un errore non compromettente
            continue;
        }
        input = malloc(input_size);
        if(read_fails(input_pipe[0], input, input_size)) { //Se il thread è stato interrotto o c'è stato un errore di lettura riprovo
            close(input_pipe[0]); //Se non riesco a chiudere la pipe in lettura provo comunque a continuare il gioco perché non è un errore non compromettente
            continue;
        }

        close(input_pipe[0]); //Se non riesco a chiudere la pipe in lettura provo comunque a continuare il gioco perché non è un errore non compromettente

        pthread_mutex_lock(&output_mutex);

        char cmd;
        char* arg;
        if(!decodeInput(input, &cmd, &arg)) //Provo a decodificare l'input e assegno i dati a cmd e arg
        {
            printf("Comando non valido, scrivi 'aiuto' per sapere i comandi possibili\n");
            pthread_mutex_unlock(&output_mutex);
            continue;
        }

        switch (cmd) {
            case HELP_IN:
                //Comando di aiuto e stampo i comandi possibili
                printf("\nCOMANDI DISPONIBILI:\n"
                       "- aiuto -> Stampa questo messaggio\n"
                       "- registra_utente *nome* -> Registrati alla partita con un nome di una parola\n"
                       "- matrice -> Richiedi la matrice di gioco e/o il tempo di partita/attesa\n"
                       "- punti -> Scopri quanti punti hai guadagnato finora\n"
                       "- p *parola* -> Prova ad indovinare una parola\n"
                       "- fine -> Esci dal gioco\n"
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
                printf("Comando non valido, scrivi 'aiuto' per sapere i comandi possibili\n"); //Failsafe per altri comandi
                break;
        }
        pthread_mutex_unlock(&output_mutex);
    }
    free(reader_pid); //Libero la memoria allocata
    close(client_fd); //Chiudo il canale di comunicazione con il server (e di conseguenza il thread se ne accorgerà ed eseguirà una procedura di uscita)
    pthread_join(read_thread, NULL); //Attendo la chiusura del thread causata dalla chiusura del socket
}

void* read_buffer(void* arg) //Funzione thread per leggere il buffer dal server
{
    pid_t* pid = (pid_t*)arg;
    while(true)
    {
        Message* msg = readMessage(client_fd); //Leggo il messaggio
        pthread_mutex_lock(&output_mutex); //Acquisisco il lock
        kill(*pid, SIGTERM); //Interrompo la lettura da input, fermando il processo che si sta occupando della lettura

        if(msg == NULL) {
            //Nel caso di disconnessione con il server si ha un errore di lettura (ad esempio alla chiusura del socket)
            bGameGoing = false;
            pthread_mutex_unlock(&output_mutex);
            pthread_exit(NULL);
        }

        switch (msg->type) {
            case MSG_ERR:
                pthread_cond_signal(&output_available);
            case MSG_OK:
                if(msg->length > 0) printf("%s\n", msg->data);
                break;
            case MSG_PUNTI_FINALI:
                printf("Punti totali: %ld\n", getNumber(msg->data));
                pthread_cond_signal(&output_available);
                break;
            case MSG_PUNTI_PAROLA:
            {
                long num = getNumber(msg->data);
                if(num == 0) printf("Avevi già trovato questa parola!\n");
                else printf("Parola trovata!\n");
                printf("Punti: %ld\n", num);
                pthread_cond_signal(&output_available);
                break;
            }
            case MSG_TEMPO_PARTITA:
                printf("Tempo rimanente: %ld\n", getNumber(msg->data)); //Stampo il tempo rimanente
                pthread_cond_signal(&output_available);
                break;
            case MSG_TEMPO_ATTESA:
                printf("Tempo attesa: %ld\n", getNumber(msg->data)); //Il gioco è in pausa quindi trovo il tempo di attesa e lo stampo
                pthread_cond_signal(&output_available);
                break;
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
    printf("\n----------\n");
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
}