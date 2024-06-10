#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include "Trie.h"

TrieNode* createTrie() //Funzione per creare un nodo
{
    //Alloco lo spazio per un nuovo nodo e lo inizializzo
    TrieNode* node = malloc(sizeof(TrieNode));
    for(int i=0; i<TOTAL_LETTERS; i++) node->nextLetters[i] = NULL;
    node->wordEnd = false;
    return node;
}

static TrieNode* findLetter(TrieNode* node, char letter) //Funzione per cercare una lettera in un nodo
{
    if(!isalpha(letter)) return NULL; //Controllo che sia una lettera
    letter = tolower(letter); //Converto la lettere in minuscolo per comodità

    int asciiDiff = letter - FIRST_LETTER; //Calcolo offset relativo della lettera
    return node->nextLetters[asciiDiff]; //Ritorno il puntatore alla prossima lettera
}

static TrieNode* addLeaf(TrieNode* node, char letter) //Funzione per provare ad aggiungere una foglia al Trie
{
    if(!isalpha(letter)) return NULL; //Controllo che sia una lettera
    letter = tolower(letter); //Converto la lettere in minuscolo per comodità

    int asciiDiff = letter - FIRST_LETTER; //Calcolo offset relativo della lettera
    //Alloco lo spazio per una nuova lettera e ritorno il puntatore
    if(node->nextLetters[asciiDiff] == NULL) node->nextLetters[asciiDiff] = createTrie(); //Inizializzo una nuovo nodo
    return node->nextLetters[asciiDiff]; //Ritorno il puntatore al nodo relativo alla lettera cercata
}

void freeTrie(TrieNode* head) //Funzione per liberare la memoria occupata dal Trie
{
    if(head == NULL) return;
    for(int i=0; i<TOTAL_LETTERS; i++)
    {
        if(head->nextLetters[i] != NULL) freeTrie(head->nextLetters[i]); //Ricorsiva per liberare lo spazio più interno all'albero
    }
    free(head); //Libero la memoria
}


bool findInTrie(TrieNode* head, const char* word, int currentIndex) //Funzione per verificare la presenza di una parola
{
    if(currentIndex == strlen(word)) //Se sto alla fine della parola, controllo la flag wordEnd e ritorno il suo valore (true se parola trovata)
    {
        if(head != NULL) return head->wordEnd;
        else return false;
    }
    TrieNode* node = findLetter(head, word[currentIndex]); //Cerco la lettera all'interno del nodo corrente head
    if(node != NULL) return findInTrie(node, word, currentIndex+1); //Avanzo ricorsivamente nell'albero se ho trovato la lettera
    else return false; //Altrimenti la parola non è presente e ritorno false
}

TrieNode* loadDictionary(const char* path) //Funzione per caricare il dizionario dal file
{
    int bytes_read, fd;

    //Controllo che il file esista e che sia di tipo corretto
    if(!regularFileExists(path)) return NULL;

    //Apro il file dizionario in lettura con una chiamata di sistema
    if(syscall_fails_get(fd, open(path, O_RDONLY))) return NULL;

    TrieNode* head = createTrie(); //Creo il puntatore al root node del Trie
    TrieNode* currentPointer = head; //Puntatore temporaneo
    char newLetter;
    while( (bytes_read = read(fd, &newLetter, sizeof(char))) ) //Leggo una lettera alla volta e aggiorno l'albero
    {
        if(newLetter == '\n') { //Se ho letto \n la parola è terminata e setto la flag wordEnd
            currentPointer->wordEnd = true;
            currentPointer = head; //Ricomincia dal nodo radice ogni volta che finisce la lettura di una parola
            continue;
        }

        currentPointer = addLeaf(currentPointer, newLetter); //Provo ad aggiungere una foglia se non esiste la lettera corrispondente
        if(currentPointer == NULL || bytes_read == 0) //Lettura lettera non valida, elimino il Trie ed esco dal ciclo
        {
            freeTrie(head); //Libero la memoria dell'albero generato finora
            head = NULL; //Imposto a NULL ed esco
            break;
        }
    }

    //Chiudo il file e se fallisce ritorno NULL
    if(syscall_fails(close(fd))) return NULL;

    return head; //Ritorno albero generato o NULL in caso di errore
}