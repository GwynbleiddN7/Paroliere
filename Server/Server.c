#include <stdio.h>
#include <stdbool.h>
#include "../CommandsHandler/CommsHandler.h"
#include "../Trie/Trie.h"

int main(int argc, char** argv)
{
    TrieNode* head = loadDictionary("../Data/dizionario.txt");
    while(true)
    {
        char *word = NULL;
        scanf("%s", word);
        bool found = findWord(head, word, 0);
        printf("%d\n", found);
    }
}