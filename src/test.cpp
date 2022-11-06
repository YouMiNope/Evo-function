
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>


void call_exit(int c, void *)
{
    printf("%d\n", c);
}

int main(int a, const char * args[])
{
    on_exit(call_exit, nullptr);
    alarm(1);
    int fork_id = fork();
    if(fork_id == 0)
    {
        sleep(3);
        exit(-1);
    }
    else if(fork_id > 0)
    {
        sleep(3);
        exit(-2);
    }
}
