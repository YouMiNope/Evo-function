#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>

#include "analysis.hpp"


void load_sequence(Sequence &p_seq, const char *path);
void exec_sequence(Sequence &p_seq, void *param);

template <typename T>
void gen_test_data(T *target, size_t len);



int __handle_no_args();
int __handle_one_args(const char *cmd);
int __handle_two_args(const char *cmd, const char *val);
int __handle_three_args(const char *cmd, const char *val_a, const char *val_b);

int __cmd_test(const char *val);
int __cmd_align(const char *, const char *);

int main(int arg_count, const char *args[])
{
    int ret = 0;
    if(arg_count == 1)
    {
        ret = __handle_no_args();
    }
    else if(arg_count == 2)
    {
        ret = __handle_one_args(args[1]);
    }
    else if(arg_count == 3)
    {
        ret = __handle_two_args(args[1], args[2]);
    }
    else if(arg_count == 4)
    {
        ret = __handle_three_args(args[1], args[2], args[3]);
    }

    return ret;
}

int __handle_no_args()
{

    return 0;
}

int __handle_one_args(const char *cmd)
{

    return 0;
}

int __handle_two_args(const char *cmd, const char *val)
{
    if(strcmp(cmd, "test") == 0)
    {
        __cmd_test(val);
    }
    return 0;
}

int __handle_three_args(const char *cmd, const char *val_a, const char *val_b)
{
    if(strcmp(cmd, "align") == 0)
    {
        __cmd_align(val_a, val_b);
    }

    return 0;
}




int __cmd_test(const char *val)
{
    Sequence target;

    double tester[16 * 3];
    memset(tester, 0x00, 16 * 3 * sizeof(double));
    gen_test_data(tester, 16 * 2);
    puts("before_execute:");
    for(size_t i = 0; i < 16 * 3; i++)
    {
        printf("%- 4.2lf ", tester[i]);
        if(i % 16 == 15)    putchar('\n');
    }

    load_sequence(target, val);
    exec_sequence(target, tester);
    puts("after_execute:");
    for(size_t i = 0; i < 16 * 3; i++)
    {
        printf("%- 4.2lf ", tester[i]);
        if(i % 16 == 15)    putchar('\n');
    }
    return 0;
}

int __cmd_align(const char *val_a, const char *val_b)
{
    Sequence seq_a;
    Sequence seq_b;
    BasicAnalyser a_analyser;

    load_sequence(seq_a, val_a);
    load_sequence(seq_b, val_b);

    GeneLibrary::mutate_prob tmp_prob;
    tmp_prob.flip = 14;
    tmp_prob.add = 14;
    tmp_prob.del = 15;

    a_analyser.comp(seq_a, seq_b, tmp_prob);
    a_analyser.print();
    return 0;
}



template <typename T>
void gen_test_data(T *target, size_t len)
{
    for(size_t i = 0; i < len; i++)
    {
        target[i] = ((T)random() * 2 / (T)0x7fffffff) - 1.;
    }
}

void load_sequence(Sequence &p_seq, const char *path)
{
    int _fd = open(path, O_RDONLY);
    struct stat statbuf;
    fstat(_fd, &statbuf);
    p_seq.len = statbuf.st_size;
    p_seq.p_context = (uint8_t *)malloc(statbuf.st_size);

    read(_fd, p_seq.p_context, p_seq.len);
    close(_fd);
}

void exec_sequence(Sequence &p_seq, void *param)
{
    uint8_t *code = (uint8_t *)mmap(NULL, 4096UL, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    memcpy(code, p_seq.p_context, p_seq.len);

    ((void (*)(void *))code)(param);
    munmap(code, 4096UL);
}