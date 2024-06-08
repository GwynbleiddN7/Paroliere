//Comandi default
#define MSG_OK 'K' //messaggio di oK
#define MSG_ERR 'E' //messaggio di Errore
#define MSG_REGISTRA_UTENTE 'R' //messaggio per Registrare l'utente
#define MSG_MATRICE 'M' //messaggio per ricevere/inviare la Matrice
#define MSG_TEMPO_PARTITA 'T' //messaggio per il Tempo della partita attuale
#define MSG_TEMPO_ATTESA 'A' //messaggio per il tempo di Attesa
#define MSG_PAROLA 'W' //messaggio per mandare la parola (Word)
#define MSG_PUNTI_FINALI 'F' //messaggio per ricevere i punti Finali
#define MSG_PUNTI_PAROLA 'P' //messaggio per ricevere i punti della Parola indovinata

//Comandi personali aggiunti
#define MSG_CLOSE_CLIENT 'C' //messaggio per gestire meglio la chiusura di un Client
#define MSG_VINCITORE 'V' //messaggio per mandare un messaggio personalizzato con il nome del Vincitore
#define MSG_PUNTI_PERSONALI 'G' //messaggio per far ricevere al client i punti personali durante il Game

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