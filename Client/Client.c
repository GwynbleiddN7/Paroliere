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
#define MAX_INPUT_BUFF 50

#define HELP_IN 'h'
#define REGISTER_IN 'r'
#define MATRIX_IN 'm'
#define WORD_IN 'p'
#define END_IN 'q'
#define UNKNOWN_IN '?'

char decodeInput(const char* string);

int main(int argc, char** argv)
{
    if(argc-1 != 2) exitMessage("Numero parametri non corretto");

    char* addr = argv[ADDR_INDEX]; //Prendo il primo parametro che corrisponde all'hostname
    int port;
    if(!validatePort(argv[PORT_INDEX], &port)) exitMessage("Numero di porta non corretto"); //Controllo con una funzione se la porta inserita è valida, se non lo è interrompo

    int client_fd, ret;
    struct sockaddr_in server_addr;
    SYSC(client_fd, socket(AF_INET, SOCK_STREAM, 0), "Nella socket");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(addr);

    SYSC(ret, connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)), "nella connect");

    bool bRegistered = false;
    bool bGameGoing = true;
    while(bGameGoing)
    {
        char* input;
        size_t line_size;
        int n_read, msg_size;
        getline(&input, &line_size, stdin);

        char* trimmed = strtok(input, "\n"); //Rimuovo lo \n salvato nel buffer da getline
        char* token = strtok(trimmed, " "); //Divido la stringa tra parole

        char cmd = decodeInput(token); //La prima parola corrisponde al comando e lo decodifico in un char per usarlo nello switch

        char* arg = strtok(NULL, " "); //Prendo il parametro
        if(arg != NULL && (cmd != REGISTER_IN && cmd != WORD_IN)) //Se ci sono parametri e non sta eseguendo un comando che non li prevede ricomincia
        {
            printf("Errore 1\n");
            continue;
        }
        if(arg == NULL && (cmd == REGISTER_IN || cmd == WORD_IN)) //Se NON ci sono parametri e sta eseguendo un comando che li prevede ricomincia
        {
            printf("Errore 2\n");
            continue;
        }
        if(arg != NULL && strtok(NULL, " ") != NULL) //Se ci sono parametri e ne trova un'altro non troppi e ricomincia
        {
            printf("Errore 3\n");
            continue;
        }

        switch (cmd) {
            case HELP_IN:
            {
                printf("Manda i comandi\n");
                break;
            }
            case REGISTER_IN:
            {
                void *out;
                msg_size = buildTextMsg(&out, MSG_REGISTRA_UTENTE, arg);
                SYSC(n_read, write(client_fd, out, msg_size), "Nella write");

                Message msg;
                SYSC(n_read, read(client_fd, &msg.length, sizeof(int)), "Nella write");
                SYSC(n_read, read(client_fd, &msg.type, sizeof(char)), "Nella write");
                if(msg.length > 0) SYSC(n_read, read(client_fd, &msg.data, msg.length), "Nella write");


                if(msg.type == MSG_ERR) printf("Errore nella registrazione, prova con un altro nome");
                else {
                    printf("Utente registrato");
                    bRegistered = true;
                }

                break;
            }
            case MATRIX_IN:
                if(!bRegistered) continue;
                break;
            case WORD_IN:
                if(!bRegistered) continue;
                break;
            case END_IN:
            {
                void *out;
                msg_size = buildTextMsg(&out, MSG_CLOSE_CLIENT, NULL);
                SYSC(n_read, write(client_fd, out, msg_size), "Nella write");
                bGameGoing = false;
                break;
            }
            default:
                //Err
                break;
        }
    }

    SYSC(ret, close(client_fd), "Nella close");
}


char decodeInput(const char* string)
{
    if(strcmp(string, "aiuto") == 0) return HELP_IN;
    if(strcmp(string, "registra_utente") == 0) return REGISTER_IN;
    if(strcmp(string, "matrice") == 0) return MATRIX_IN;
    if(strcmp(string, "p") == 0) return WORD_IN;
    if(strcmp(string, "fine") == 0) return END_IN;
    return UNKNOWN_IN;
}