#include <unistd.h>
#include <iostream>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <math.h>

#include "GeneLibrary.hpp"

// Exit codes:
#define EXIT_FORK_ERROR -1
#define EXIT_FREE_BEATEN -2
#define EXIT_MEM_FULL -3

#define LIFE 5

const size_t CODE_SIZE = 4096UL;

// real function here
void m_func(void * p_data);

// get cost
double m_func_cost(double *real, double *test);

// SIGCHLD handle
void handle_child(int);
void handle_sig(int);

//  On exit
void handle_exit(int, void *);

typedef struct
{
    uint8_t *p_code;
    GeneLibrary::shm_head_data *p_shm_head;
}share_args;

struct gene
{
    size_t pos = 0;
    size_t len = 0;
    double fitness = 0.;
};

int main(int args_count, const char *args[])
{
    GeneLibrary::link(2);
    GeneLibrary::init();
    
    gene my_gene;
    int shm_id = shmget(ftok("./", 0), GeneLibrary::max_size, 0666);
    share_args my_args;

    uint8_t *code = (uint8_t *)mmap(NULL, CODE_SIZE, PROT_EXEC | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    GeneLibrary::shm_head_data *shm_head = (GeneLibrary::shm_head_data *)shmat(shm_id, (void *)0, 0);

    my_args.p_code = code;
    my_args.p_shm_head = shm_head;

    signal(SIGCHLD, handle_child);
    signal(SIGSEGV, handle_sig);
    signal(SIGBUS, handle_sig);
    signal(SIGALRM, handle_sig);
    signal(SIGILL, handle_sig);
    signal(SIGABRT, handle_sig);

    on_exit(handle_exit, &my_args);

    my_gene.len = GeneLibrary::load_seed_code(args[1]);
    my_gene.fitness = 1.;
    my_gene.pos = GeneLibrary::smalloc(getpid(), (float)my_gene.fitness);
    if(my_gene.pos == -1)   exit(EXIT_MEM_FULL);
    memcpy(my_args.p_code, (uint8_t *)(my_args.p_shm_head + 1), my_gene.len);
    size_t life = 0;

Load:
    // usleep(50000);
    // Load the function code
    //      copy code from sharemem to mmap
    //      set fitness of code in sharemem to 0    (give up)

Fork:
    // Fork
    //      Self: do nothing
    //      Chld: mutate code
    int fork_id = fork();
    if(fork_id < 0)         exit(EXIT_FORK_ERROR);
    if(fork_id > 0)
    {
        life++;
    }
    else if(fork_id == 0)
    {
        my_args.p_shm_head->gene_count_total++;
        my_gene.pos = GeneLibrary::smalloc(getpid(), (float)my_gene.fitness);
        if(my_gene.pos == -1)   exit(EXIT_MEM_FULL);
        // mutate
        life = 0;
        GeneLibrary::mutate_prob a_prob;
        a_prob.flip = 14;
        a_prob.add  = 14;
        a_prob.del  = 14;

        // a_prob.flip = 32;
        // a_prob.add  = 32;
        // a_prob.del  = 32;
        my_gene.len = GeneLibrary::mutate(my_args.p_code, my_gene.len, a_prob);
    }

Exec:
    // Execute
    //      Self: always executeable.
    //      Chld: very chance to fail.              alarm(1); fall ? exit;
    double test_data[16 * 3];
    double real_data[16 * 3];
    GeneLibrary::gen_test_data(test_data, 16 * 2);
    memcpy(real_data, test_data, 16 * 2 * sizeof(double));

    int ret = GeneLibrary::sfree(my_gene.pos, getpid(), (float)my_gene.fitness);
    if(ret == -1)           exit(EXIT_FREE_BEATEN);
    my_args.p_shm_head->func_fail_count++;
    alarm(1);
    ((void (*)(void *))my_args.p_code)(test_data);
    alarm(0);
    my_args.p_shm_head->func_fail_count--;
    my_gene.pos = GeneLibrary::smalloc(getpid(), (float)my_gene.fitness);
    if(my_gene.pos == -1)   exit(EXIT_MEM_FULL);

    m_func(real_data);

    // Collect data
    //      fitness.
    double cost = m_func_cost(real_data, test_data);
    my_gene.fitness = 1. / (cost + 1.);
    // printf("%lf\t%lf\n", my_gene.fitness, cost);

End:
    if(!my_args.p_shm_head->is_terminate && life < LIFE)
        goto Load;

    if(my_args.p_shm_head->is_terminate)
    {
        char path[64];
        sprintf(path, "./save/%d", getpid());
        int _fd = open(path, O_CREAT | O_WRONLY, S_IRWXU);
        write(_fd, my_args.p_code, CODE_SIZE);
        close(_fd);
    }

    GeneLibrary::sfree(my_gene.pos, getpid(), (float)my_gene.fitness);
    exit(0);
}

double m_func_cost(double *real, double *test)
{
    double cost = 0.;
    for(size_t i = 2 * 16; i < 3 * 16; i++)
    {
        cost += pow(real[i] - test[i], 2);
    }
    return cost;
}

void handle_exit(int e_code, void *args)
{
    share_args *m_args = (share_args *)args;

    switch (e_code)
    {
    case 0:
        m_args->p_shm_head->life_end_count++;
        break;
    case EXIT_FORK_ERROR:
        m_args->p_shm_head->fork_fail_count++;
        break;
    case EXIT_FREE_BEATEN:
        m_args->p_shm_head->sfree_fail_count++;
        break;
    case EXIT_MEM_FULL:
        m_args->p_shm_head->smalc_fail_count++;
        break;
    }

    shmdt(m_args->p_shm_head);
    munmap(m_args->p_code, CODE_SIZE);
}

void handle_child(int _sig)
{
    wait(nullptr);
}
void handle_sig(int _sig)
{
    exit(-_sig);
}