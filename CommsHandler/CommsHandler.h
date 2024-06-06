//Comandi default
#define MSG_OK 'K'
#define MSG_ERR 'E'
#define MSG_REGISTRA_UTENTE 'R'
#define MSG_MATRICE 'M'
#define MSG_TEMPO_PARTITA 'T'
#define MSG_TEMPO_ATTESA 'A'
#define MSG_PAROLA 'W'
#define MSG_PUNTI_FINALI 'F'
#define MSG_PUNTI_PAROLA 'P'

//Comandi personali aggiunti
#define MSG_CLOSE_CLIENT 'C' //Per gestire meglio la chiusura di un client
#define MSG_VINCITORE 'V' //Per mandare un messaggio personalizzato con il nome del vincitore

//Struttura messaggio
typedef struct{
    unsigned int length;
    char type;
    char data[];
} Message;

//Dichiaro le funzioni esposte
void sendTextMessage(int fd, char type, const void* data); //Funzione per inviare un messaggio di testo tramite un file descriptor
void sendNumMessage(int fd, char type, long message); //Funzione per inviare un messaggio numerico tramite un file descriptor
Message* readMessage(int fd); //Funzione per leggere i campi di un messaggio
char** getMatrix(const char* data); //Funzione per estrarre una matrice in un char** a partire da dati char* in un messaggio
long getNumber(const char* data); //Funzione per estrarre un long a partire da un char* in un messaggio
void deleteMessage(Message* msg); //Funzione per liberare la memoria di un messaggio