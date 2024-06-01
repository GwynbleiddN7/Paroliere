#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>
#include "../CommandsHandler/CommsHandler.h"
#include "../Utility/Utility.h"

#define ADDR_INDEX 1
#define PORT_INDEX 2

int main(int argc, char** argv)
{
    if(argc-1 != 2) exitMessage("Numero parametri non corretto");

    char* addr = argv[ADDR_INDEX]; //Prendo il primo parametro che corrisponde all'hostname
    int port;
    if(!validatePort(argv[PORT_INDEX], &port)) exitMessage("Numero di porta non corretto"); //Controllo con una funzione se la porta inserita è valida, se non lo è interrompo
}