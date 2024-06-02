#include <errno.h>
#include <stdbool.h>

#define FIRST_LETTER 'a'
#define TOTAL_LETTERS 26
#define PLAYER_NAME_SIZE 10
#define MATRIX_SIZE 4

//Controllo distruttivo per chiamate di sistema che ritornano -1 come errore
#define SYSC(v,c,m) if((v=c)==-1){ perror(m); exit(errno); }

//Controllo distruttivo per chiamate di sistema che ritornano NULL come errore
#define SYSCN(v,c,m) if((v=c)==NULL){ perror(m);exit(errno); }

bool regularFileExists(const char* file); //Funzione per controllare l'esistenza di un file
void exitMessage(char* message); //Funzione per terminare il programma con un messaggio
void copyString(char** dest, char* src); //Funzione per creare una stringa con il valore di un'altra
bool strToInt(char* src, int* out); //Funzione per convertire stringa in intero
bool validatePort(char* portString, int* portInt); //Funzione per validare la porta