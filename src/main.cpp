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
#define EXIT_SIGALRM -4
#define EXIT_SIG_ERROR -5
#define EXIT_LIFE_END -6

#define ALARM_USEC 500000

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

struct gene
{
    size_t pos = 0;
    size_t len = 0;
    double fitness = 0.;
};

typedef struct
{
    gene *p_gene;
    GeneLibrary::shm_head_data *p_shm_head;
    uint8_t *p_code;
}share_args;

int main(int args_count, const char *args[])
{
    GeneLibrary::link(1);
    GeneLibrary::init();
    
    gene my_gene;
    share_args my_args;

    int shm_id = shmget(ftok("./", 0), GeneLibrary::max_size, 0666);
    my_args.p_shm_head = (GeneLibrary::shm_head_data *)shmat(shm_id, (void *)0, 0);
    my_args.p_code     = (uint8_t *)mmap(NULL, CODE_SIZE, PROT_EXEC | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    my_args.p_gene     = &my_gene;

    signal(SIGCHLD, handle_child);

    on_exit(handle_exit, &my_args);

    my_gene.fitness = 1.;
    my_gene.len = GeneLibrary::load_seed_code(args[1], my_args.p_code);
    my_gene.pos = GeneLibrary::smalloc(getpid(), my_gene.fitness);
    if(my_gene.pos == -1)   exit(EXIT_MEM_FULL);

Load:
    usleep(400);
    // Load the function code
    //      copy code from sharemem to mmap
    //      set fitness of code in sharemem to 0    (give up)

Fork:
    // Fork
    //      Self: do nothing
    //      Chld: mutate code
    pid_t fork_id = fork();
    if(fork_id < 0)
    {
        GeneLibrary::sfree(my_gene.pos, getpid(), my_gene.fitness);
        exit(EXIT_FORK_ERROR);
    }

    GeneLibrary::mutate_prob a_prob;
    if(fork_id > 0)
    {
        a_prob.flip = 16;
        a_prob.add  = 16;
        a_prob.del  = 16;

        // a_prob.never();
    }
    else if(fork_id == 0)
    {
        my_args.p_shm_head->gene_count_total++;

        a_prob.flip = 15;
        a_prob.add  = 15;
        a_prob.del  = 15;

        // a_prob.never();
    }
    my_gene.len = GeneLibrary::mutate(my_args.p_code, my_gene.len, a_prob);

PreExec:
    if(fork_id > 0)
    {
        int ret;
        ret = GeneLibrary::sfree(my_gene.pos, getpid(), my_gene.fitness);
        if(ret < 0)         exit(EXIT_FREE_BEATEN);
    }

Exec:
    // Execute
    //      Self: always executeable.
    //      Chld: very chance to fail.              alarm(1); fall ? exit;
    my_args.p_shm_head->func_fail_count++;

    double test_data[16 * 3];
    double real_data[16 * 3];
    memset(test_data, 0x00, 16 * 3 * sizeof(double));
    memset(real_data, 0x00, 16 * 3 * sizeof(double));
    GeneLibrary::gen_test_data(test_data, 16 * 2);
    memcpy(real_data, test_data, 16 * 2 * sizeof(double));

    ualarm(ALARM_USEC, 0);
    ((void (*)(void *))my_args.p_code)(test_data);
    ualarm(0, 0);

    m_func(real_data);

    // Collect data
    //      fitness.
    double cost = m_func_cost(real_data, test_data);
    my_gene.fitness = 1. / (cost + 1.);

    my_args.p_shm_head->func_fail_count--;

PostExec:
    my_gene.pos = GeneLibrary::smalloc(getpid(), my_gene.fitness);
    if(my_gene.pos == -1)   exit(EXIT_MEM_FULL);

End:
    if(!my_args.p_shm_head->is_terminate)
        goto Load;

    if(my_args.p_shm_head->is_terminate && fork_id > 0 && my_gene.fitness > .8f)
    {
        char path[64];
        sprintf(path, "./save/%d", getpid());
        int _fd = open(path, O_CREAT | O_WRONLY, S_IRWXU);
        ssize_t ret = write(_fd, my_args.p_code, my_gene.len);
        if(ret == -1)
        {
            close(_fd);
            printf("error writing file: %s, errno: %d\n", path, errno);
            exit(EXIT_LIFE_END);
        }
        close(_fd);
    }

    exit(EXIT_LIFE_END);
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
    gene *p_gene = m_args->p_gene;

    switch (e_code)
    {
    case 0:
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
        // m_args->p_shm_head->func_fail_count--;
    case EXIT_LIFE_END:
        m_args->p_shm_head->life_end_count++;
        GeneLibrary::sfree(p_gene->pos, getpid(), p_gene->fitness);
        break;
    case EXIT_SIGALRM:
        break;
    case EXIT_SIG_ERROR:
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
    switch (_sig)
    {
    case SIGALRM:
        exit(EXIT_SIGALRM);
        break;
    
    default:
        exit(EXIT_SIG_ERROR);
        break;
    }
}