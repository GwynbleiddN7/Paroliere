#include <stdbool.h>
#include "../Utility/Utility.h"

//Struttura Trie dizionario
typedef struct TrieNode{
    struct TrieNode* nextLetters[TOTAL_LETTERS];
    bool wordEnd;
} TrieNode;

//Rendo "pubbliche" alcune funzioni del file Trie.c
bool findWord(TrieNode* head, const char* word, int currentIndex); //Funzione per verificare la presenza di una parola
TrieNode* loadDictionary(const char* path); //Funzione per caricare il dizionario dal file
void freeTrie(TrieNode* head); //Funzione per liberare la memoria