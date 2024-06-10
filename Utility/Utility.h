#include <stdbool.h>
#include <arpa/inet.h>

#define FIRST_LETTER 'a'
#define TOTAL_LETTERS 26
#define PLAYER_NAME_SIZE 10
#define MATRIX_SIZE 4

//Macro per controllare se una chiamata di sistema (con errore -1) fallisce
#define syscall_fails(syscall) (syscall == -1)
//Macro per controllare se una chiamata di sistema (con errore -1) fallisce (con assegnazione del risultato ad una variabile)
#define syscall_fails_get(val, syscall) ((val = syscall) == -1)
//Macro che wrappa i parametri di una read e controlla se c'è stato un errore di lettura o una lettura vuota
#define read_fails(arg1, arg2, arg3) (read(arg1, arg2, arg3) <= 0)
//Macro che wrappa i parametri di una read e controlla se c'è stato un errore di lettura o una lettura vuota e aggiunge un return NULL per le funzioni che ritornano un puntatore, interrompendole
#define try_read(arg1, arg2, arg3) if(read(arg1, arg2, arg3) <= 0) { return NULL; }

//Dichiaro le funzioni esposte
bool regularFileExists(const char* file); //Funzione per controllare l'esistenza di un file
void exitWithMessage(char* message); //Funzione per terminare il programma con un messaggio
void copyString(char** dest, char* src); //Funzione per creare una stringa con il valore di un'altra
bool strToInt(char* src, int* out); //Funzione per convertire stringa in intero
bool validatePort(char* portString, int* portInt); //Funzione per validare la porta
bool validateAddr(char* addrInput, in_addr_t* addrOutput); //Funzione per validare l'indirizzo
int secondsToMinutes(long* seconds); //Funzione per convertire i secondi in minuti + secondi