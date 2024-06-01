#include <stdio.h>
#include "../CommandsHandler/CommsHandler.h"

int main(int argc, char** argv)
{
    char* x = "ciao";
    buildMessage(NULL, MSG_TEMPO_ATTESA, x);
}