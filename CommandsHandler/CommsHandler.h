#define MSG_OK 'K'
#define MSG_ERR 'E'
#define MSG_REGISTRA_UTENTE 'R'
#define MSG_MATRICE 'M'
#define MSG_TEMPO_PARTITA 'T'
#define MSG_TEMPO_ATTESA 'A'
#define MSG_PAROLA 'W'
#define MSG_PUNTI_FINALI 'F'
#define MSG_PUNTI_PAROLA 'P'

//Struttura messaggio
typedef struct{
    unsigned int length;
    char type;
    char* data;
} Message;

int buildMessage(void** out, char type, const char* data); //Funzione per creare il messaggio
char* getData(void* msg);  //MIGHT NOT NEED THIS!