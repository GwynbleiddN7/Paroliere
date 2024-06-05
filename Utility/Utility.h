#include <stdbool.h>

#define FIRST_LETTER 'a'
#define TOTAL_LETTERS 26
#define PLAYER_NAME_SIZE 10
#define MATRIX_SIZE 4

#define syscall_fails(syscall) (syscall == -1)
#define syscall_fails_get(val, syscall) ((val = syscall) == -1)
#define read_fails(arg1, arg2, arg3) (read(arg1, arg2, arg3) <= 0)
#define try_read(arg1, arg2, arg3) if(read(arg1, arg2, arg3) <= 0) { return NULL; }

//Dichiaro le funzioni esposte
bool regularFileExists(const char* file); //Funzione per controllare l'esistenza di un file
void exitWithMessage(char* message); //Funzione per terminare il programma con un messaggio
void copyString(char** dest, char* src); //Funzione per creare una stringa con il valore di un'altra
bool strToInt(char* src, int* out); //Funzione per convertire stringa in intero
bool validatePort(char* portString, int* portInt); //Funzione per validare la porta