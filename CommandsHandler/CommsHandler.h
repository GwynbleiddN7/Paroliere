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

void sendTextMessage(int fd, char type, const void* data);
void sendNumMessage(int fd, char type, long message);
Message* readMessage(int fd);
char** getMatrix(const char* data);
long getNumber(const char* data);