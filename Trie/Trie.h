#include <stdbool.h>
#define TOTAL_LETTERS 26

//Struttura Trie dizionario
typedef struct{
    struct TrieNode* nextLetters[TOTAL_LETTERS];
    bool wordEnd;
} TrieNode;

bool findWord(TrieNode* head, const char* word, int currentIndex); //Funzione per verificare la presenza di una parola
TrieNode* loadDictionary(const char* path); //Funzione per caricare il dizionario dal file