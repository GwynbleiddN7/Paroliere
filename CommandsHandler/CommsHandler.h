#define MSG_OK 'K'
#define MSG_ERR 'E'
#define MSG_REGISTRA_UTENTE 'R'
#define MSG_MATRICE 'M'
#define MSG_TEMPO_PARTITA 'T'
#define MSG_TEMPO_ATTESA 'A'
#define MSG_PAROLA 'W'
#define MSG_PUNTI_FINALI 'F'
#define MSG_PUNTI_PAROLA 'P'
#define MSG_CLOSE_CLIENT 'C'

//Struttura messaggio
typedef struct{
    unsigned int length;
    char type;
    char data[];
} Message;

int buildTextMsg(void** out, char type, const void* data);
int buildNumMsg(void** out, char type, long num);
char* getMessage(Message* message);
char** getMatrix(Message* message);
long getNumber(Message* message);