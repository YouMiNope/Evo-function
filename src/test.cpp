#include <unistd.h>
#include <iostream>
// #include <sys/shm.h>
// #include <sys/ipc.h>
#include <sys/mman.h>
// #include <sys/signal.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
// #include <math.h>

#include "GeneLibrary.hpp"

void m_func(void * p_data);

void m_memcpy(double *dst, double *src, size_t len);

int main(int a, const char * args[])
{
    int _hex_fd = open(args[1], O_RDWR);
    uint8_t *code = (uint8_t *)mmap(NULL, 4096UL, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

    read(_hex_fd, code, 4096UL);

    double test_data[16 * 3];
    double real_data[16 * 3];

    GeneLibrary::gen_test_data(test_data, 16 * 2);
    ((void (*)(void *))code)(test_data);
    for(size_t i = 0; i < 12; i++)
    {
        for(size_t j = 0; j < 4; j++)
        {
            printf("\t%.2lf", test_data[i * 4 + j]);
        }
        putchar('\n');
    }

    putchar('\n');

    m_memcpy(real_data, test_data, 16 * 2);
    m_func(real_data);
    for(size_t i = 0; i < 12; i++)
    {
        for(size_t j = 0; j < 4; j++)
        {
            printf("\t%.2lf", real_data[i * 4 + j]);
        }
        putchar('\n');
    }

    return 0;
}

void m_memcpy(double *dst, double *src, size_t len)
{
    for(size_t i = 0; i < len; i++)
    {
        dst[i] = src[i];
    }
}