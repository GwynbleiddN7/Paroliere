# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g -pedantic -pthread

# Executable files
SERVER_EXE = paroliere_srv
CLIENT_EXE = paroliere_cl

# Paths
SERVER = Main/Server
CLIENT = Main/Client
TRIE = DataStructures/Trie
UTILITY = Utility/Utility
COMMS = CommsHandler/CommsHandler
GAMESTATE = GameState/GameState

# Source files
SRCS = $(SERVER).c $(CLIENT).c $(TRIE).c $(UTILITY).c $(COMMS).c $(GAMESTATE).c

# Object files
OBJS = $(SRCS:.c=.o)

# Targets
all: $(SERVER_EXE) $(CLIENT_EXE)

# Rule to link object files to create the server executable
$(SERVER_EXE):  $(SERVER).o $(TRIE).o $(UTILITY).o $(COMMS).o $(GAMESTATE).o
	$(CC) $(CFLAGS) -o $@ $^

# Rule to link object files to create the client executable
$(CLIENT_EXE): $(CLIENT).o $(UTILITY).o $(COMMS).o
	$(CC) $(CFLAGS) -o $@ $^

# Pattern rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target to remove object files and executables
clean:
	rm -f $(OBJS) $(SERVER_EXE) $(CLIENT_EXE)

# Phony targets
.PHONY: all clean