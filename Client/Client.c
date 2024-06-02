#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "../CommandsHandler/CommsHandler.h"
#include "../Utility/Utility.h"

#define ADDR_INDEX 1
#define PORT_INDEX 2

#define HELP_IN 'h'
#define REGISTER_IN 'r'
#define MATRIX_IN 'm'
#define WORD_IN 'p'
#define END_IN 'q'
#define UNKNOWN_IN '?'

bool decodeInput(char* string, char* cmd, char** arg);
void printMatrix(char** matrix);
void receiveMatrix(int fd);

int main(int argc, char** argv)
{
    if(argc-1 != 2) exitMessage("Numero parametri non corretto");

    char* addr = argv[ADDR_INDEX]; //Prendo il primo parametro che corrisponde all'hostname
    int port;
    if(!validatePort(argv[PORT_INDEX], &port)) exitMessage("Numero di porta non corretto"); //Controllo con una funzione se la porta inserita è valida, se non lo è interrompo

    int client_fd;
    struct sockaddr_in server_addr;
    SYSC(client_fd, socket(AF_INET, SOCK_STREAM, 0), "Errore nella creazione del socket client");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(addr);

    SYSCALL(connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)), "Errore nella connessione con il server");

    bool bRegistered = false;
    bool bGameGoing = true;
    while(bGameGoing)
    {
        char* input;
        size_t line_size;

        printf("\n[PROMPT PAROLIERE]-->");
        getline(&input, &line_size, stdin);

        char cmd;
        char* arg;
        if(!decodeInput(input, &cmd, &arg))
        {
            printf("Comando non valido, scrivi aiuto per sapere i comandi possibili\n");
            continue;
        }

        switch (cmd) {
            case HELP_IN:
            {
                printf("aiuto\nregistra_utente nome\nmatrice\np parola\nfine\n");
                break;
            }
            case REGISTER_IN:
            {
                printf("Richiesta registrazione in corso...\n");
                sendTextMessage(client_fd, MSG_REGISTRA_UTENTE, arg);

                Message* msg = readMessage(client_fd);
                if(msg->type == MSG_ERR) printf("Errore nella registrazione, prova con un altro nome\n");
                else{
                    printf("Registrazione avvenuta!\n");
                    bRegistered = true;
                    receiveMatrix(client_fd);
                }
                free(msg);
                break;
            }
            case MATRIX_IN:
                if(!bRegistered)
                {
                    printf("Devi registrarti prima di poter eseguire questo comando\n");
                    continue;
                }
                sendTextMessage(client_fd, MSG_MATRICE, NULL);
                receiveMatrix(client_fd);
                break;
            case WORD_IN:
                if(!bRegistered)
                {
                    printf("Devi registrarti prima di poter eseguire questo comando\n");
                    continue;
                }
                printf("Parola inserita: %s\n", arg);
                break;
            case END_IN:
            {
                sendTextMessage(client_fd, MSG_CLOSE_CLIENT, NULL);
                bGameGoing = false;
                break;
            }
            default:
                printf("Comando non valido, scrivi aiuto per sapere i comandi possibili\n");
                break;
        }
    }

    SYSCALL(close(client_fd), "Errore nella chiusura del socket client");
}

bool decodeInput(char* string, char* cmd, char** arg)
{
    char* trimmed = strtok(string, "\n"); //Rimuovo lo \n salvato nel buffer da getline
    char* command = strtok(trimmed, " "); //Divido la stringa tra parole

    //La prima parola corrisponde al comando e lo decodifico in un char per usarlo nello switch
    char c = UNKNOWN_IN;
    if(strcmp(command, "aiuto") == 0) c = HELP_IN;
    else if(strcmp(command, "registra_utente") == 0) c = REGISTER_IN;
    else if(strcmp(command, "matrice") == 0) c = MATRIX_IN;
    else if(strcmp(command, "p") == 0) c = WORD_IN;
    else if(strcmp(command, "fine") == 0) c = END_IN;

    *cmd = c;
    *arg = strtok(NULL, " "); //Prendo il parametro

    //Se ci sono parametri e non sta eseguendo un comando che non li prevede ricomincia
    if(*arg != NULL && (c != REGISTER_IN && c != WORD_IN)) return false;
    //Se NON ci sono parametri e sta eseguendo un comando che li prevede ricomincia
    if(*arg == NULL && (c == REGISTER_IN || c == WORD_IN)) return false;
    //Se ci sono parametri e ne trova un'altro non troppi e ricomincia
    if(*arg != NULL && strtok(NULL, " ") != NULL) return false;

    return true;
}

void printMatrix(char** matrix)
{
    for(int i=0; i<MATRIX_SIZE; i++)
    {
        for(int j=0; j<MATRIX_SIZE; j++)
        {
            if(matrix[i][j] == 'q') printf("Qu");
            else printf("%c ", matrix[i][j]);
        }
        printf("\n");
    }
}

void receiveMatrix(int fd)
{
    Message* msg = readMessage(fd);
    if(msg->type == MSG_MATRICE){
        printMatrix(getMatrix(msg->data));
        free(msg);
        msg = readMessage(fd);
        if(msg->type == MSG_TEMPO_PARTITA) printf("Tempo rimanente: %ld", getNumber(msg->data));
    }
    else if(msg->type == MSG_TEMPO_ATTESA) printf("Tempo attesa: %ld", getNumber(msg->data));
    else printf("Errore nella ricezione della matrice, prova ad usare il comando apposito");
    free(msg);
}