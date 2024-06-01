#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "Trie.h"
#include "../Utils/ErrorHandler.h"

#define FIRST_LETTER 'a'

static TrieNode* createTrieNode()
{
    //Alloco lo spazio per un nuovo nodo e lo inizializzo
    TrieNode* node = malloc(sizeof(TrieNode));
    for(int i=0; i<TOTAL_LETTERS; i++) node->nextLetters[i] = NULL;
    node->wordEnd = false;
    return node;
}

static TrieNode* findLetter(TrieNode* node, char letter)
{
    int asciiDiff = letter - FIRST_LETTER; //Calcolo offset relativo della lettera
    if(asciiDiff >= 0 && asciiDiff < TOTAL_LETTERS) //Controllo se è una lettera dell'alfabeto valida [a -> z]
    {
        return node->nextLetters[asciiDiff]; //Ritorno il puntatore alla prossima lettera
    }
    return NULL;
}

static TrieNode* addLeaf(TrieNode* node, char letter)
{
    int asciiDiff = letter - FIRST_LETTER; //Calcolo offset relativo della lettera
    if(asciiDiff >= 0 && asciiDiff < TOTAL_LETTERS) //Controllo se è una lettera dell'alfabeto valida [a -> z]
    {
        //Alloco lo spazio per una nuova lettera e ritorno il puntatore
        if(node->nextLetters[asciiDiff] == NULL) node->nextLetters[asciiDiff] = createTrieNode();
        return node->nextLetters[asciiDiff];
    }
    return NULL;
}

bool findWord(TrieNode* head, const char* word, int currentIndex)
{
    if(currentIndex == strlen(word)) //Se sto alla fine della parola, controllo la flag wordEnd e ritorno il suo valore (true se parola trovata)
    {
        if(head != NULL) return head->wordEnd;
        else return false;
    }
    TrieNode* node = findLetter(head, word[currentIndex]); //Cerco la lettera all'interno del nodo corrente head
    if(node != NULL) return findWord(node, word, currentIndex+1); //Avanzo ricorsivamente nell'albero se ho trovato la lettera
    else return false; //Altrimenti la parola non è presente e ritorno false
}

TrieNode* loadDictionary(const char* path)
{
    int retvalue, fd;
    struct stat statbuf;

    //Controllo che il file esista e che sia di tipo corretto
    SYSC(retvalue, stat(path,  &statbuf), "Errore lettura file, file non esistente");
    if(!S_ISREG(statbuf.st_mode))
    {
        printf("Errore nel tipo del file dizionario");
        exit(EXIT_FAILURE);
    }

    //Apro il file dizionario in lettura
    SYSC(fd, open(path, O_RDONLY), "Errore nell'apertura del file dizionario");

    TrieNode* head = createTrieNode(); //Creo il puntatore al root node del Trie

    TrieNode* currentPointer = head;
    char newLetter;
    while( (retvalue = read(fd, &newLetter, sizeof(char))) ) //Leggo una lettera alla volta e aggiorno l'albero
    {
        if(newLetter == '\n') { //Se ho letto \n la parola è terminata e setto la flag wordEnd
            currentPointer->wordEnd = true;
            currentPointer = head; //Ricomincia dal nodo radice ogni volta che finisce la lettura di una parola
            continue;
        }

        currentPointer = addLeaf(currentPointer, newLetter); //Provo ad aggiungere una foglia se non esiste la lettera corrispondente
        if(currentPointer == NULL) //Lettura lettera non valida e chiudo il programma
        {
            printf("Errore lettura del file, lettera non valida: %c", newLetter);
            exit(EXIT_FAILURE);
        }
    }

    return head;
}