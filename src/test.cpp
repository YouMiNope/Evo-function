#include <unistd.h>
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "GeneLibrary.hpp"

int main(int a, const char * args[])
{
    alarm(1);

    auto id = fork();

    if(id == 0)
    {
        sleep(2);
        puts("chld ok");
    }
    else if(id > 0)
    {
        sleep(2);
        puts("self ok");
    }

    return 0;
}