#include <stdbool.h>
#include "../Utility/Utility.h"

//Struttura Trie dizionario
typedef struct TrieNode{
    struct TrieNode* nextLetters[TOTAL_LETTERS];
    bool wordEnd;
} TrieNode;

//Dichiaro le funzioni esposte
TrieNode* createTrie(); //Alloca un nodo del Trie
void freeTrie(TrieNode* head); //Funzione per liberare la memoria occupata dal Trie
bool findInTrie(TrieNode* head, const char* word, int currentIndex); //Funzione per verificare la presenza di una parola
TrieNode* loadDictionary(const char* path); //Funzione per caricare il dizionario dal file