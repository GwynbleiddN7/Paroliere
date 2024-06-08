# Compilatore
CC = cc

# Flag per la compilazione
CFLAGS = -Wall -g -pedantic -pthread

# File eseguibili finali
SERVER_EXE = paroliere_srv
CLIENT_EXE = paroliere_cl

# Percorsi per ogni file sorgente
SERVER = Main/Server
CLIENT = Main/Client
TRIE = DataStructures/Trie
ARRAY = DataStructures/DynamicArray
UTILITY = Utility/Utility
COMMS = CommsHandler/CommsHandler
GAMESTATE = GameState/GameState

# Insieme dei file sorgente .c
SRCS = $(SERVER).c $(CLIENT).c $(TRIE).c $(ARRAY).c $(UTILITY).c $(COMMS).c $(GAMESTATE).c

# Insieme dei file oggetto .o
OBJS = $(SRCS:.c=.o)

# Unico target di compilazione default
all: $(SERVER_EXE) $(CLIENT_EXE)

# Compilazione del Server con i relativi file oggetto da legare [$0 = target, $^ = lista dei prerequisiti]
$(SERVER_EXE):  $(SERVER).o $(TRIE).o $(ARRAY).o $(UTILITY).o $(COMMS).o $(GAMESTATE).o
	$(CC) $(CFLAGS) -o $@ $^

# Compilazione del Client con i relativi file oggetto da legare [$0 = target, $^ = lista dei prerequisiti]
$(CLIENT_EXE): $(CLIENT).o $(UTILITY).o $(COMMS).o
	$(CC) $(CFLAGS) -o $@ $^

# Pattern per compilare i file .c nei relativi file .c [$0 = target, $< = primo elemento della lista dei prerequisiti]
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pulizia dei file di compilazione
clean:
	rm -f $(OBJS) $(SERVER_EXE) $(CLIENT_EXE)

# Target Phony
.PHONY: all clean