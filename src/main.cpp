#include <unistd.h>
#include <iostream>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
#include <sys/proc.h>

#include "GeneLibrary.hpp"

const size_t CODE_SIZE = 4096UL;

// real function here
void m_func(void * p_data);

// get cost
double m_func_cost(double *real, double *test);

// SIGCHLD handle
void handle_child(int);
void handle_sig(int);

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
    uint8_t *code = (uint8_t *)mmap(NULL, CODE_SIZE, PROT_EXEC | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    int shm_id = shmget(ftok("./", 0), GeneLibrary::max_size, 0666);
    GeneLibrary::shm_head_data *shm_head = (GeneLibrary::shm_head_data *)shmat(shm_id, (void *)0, 0);

    signal(SIGCHLD, handle_child);
    signal(SIGSEGV, handle_sig);
    signal(SIGBUS, handle_sig);
    signal(SIGALRM, handle_sig);
    signal(SIGILL, handle_sig);
    signal(SIGABRT, handle_sig);

    

    my_gene.len = GeneLibrary::load_seed_code(args[1]);
    my_gene.fitness = 1.;
    my_gene.pos = GeneLibrary::smalloc(getpid(), (float)my_gene.fitness);
    memcpy(code, (uint8_t *)(shm_head + 1), my_gene.len);
    size_t life = 0;

Load:
    usleep(200000);
    // Load the function code
    //      copy code from sharemem to mmap
    //      set fitness of code in sharemem to 0
    // memcpy(code, shm_codes + my_gene.pos, my_gene.len);
    int ret = GeneLibrary::sfree(my_gene.pos, getpid(), (float)my_gene.fitness);
    if(ret != 0)
    {
        shm_head->sfree_fail_count++;
        shmdt(shm_head);
        munmap(code, CODE_SIZE);
        exit(-2);
    }

Fork:
    // Fork
    //      Self: do nothing
    //      Chld: mutate code
    int fork_id = fork();
    if(fork_id < 0)
    {
        // sleep(10);
        // goto Fork;
        shm_head->fork_fail_count++;
        // printf("fork error: %d", errno);
        shmdt(shm_head);
        munmap(code, CODE_SIZE);
        exit(-1);
    }
    if(fork_id > 0)
    {
        // nothing
        life++;
    }
    else if(fork_id == 0)
    {
        // mutate
        life = 0;
        shm_head->gene_count_total++;
        GeneLibrary::mutate_prob a_prob;
        a_prob.flip = 13;
        a_prob.add  = 12;
        a_prob.del  = 12;
        my_gene.len = GeneLibrary::mutate(code, my_gene.len, a_prob);
    }

Exec:
    // Execute
    //      Self: always executeable.
    //      Chld: very chance to fail.              alarm(1); fall ? exit;
    double test_data[16 * 3];
    double real_data[16 * 3];
    GeneLibrary::gen_test_data(test_data, 16 * 2);
    memcpy(real_data, test_data, 16 * 2);

    shm_head->func_fail_count++;
    shmdt(shm_head);
    alarm(1);
    ((void (*)(void *))code)(test_data);
    alarm(0);
    shm_head = (GeneLibrary::shm_head_data *)shmat(shm_id, (void *)0, 0);
    shm_head->func_fail_count--;
    m_func(real_data);

    // Collect data
    //      fitness.
    double cost = m_func_cost(real_data, test_data);
    my_gene.fitness = 1. / cost + 1.;

Upload:
    // Update code to sharemem
    //      Can return: update gene posit.
    //      bad return: exit.
    my_gene.pos = GeneLibrary::smalloc(getpid(), (float)my_gene.fitness);
    if(my_gene.pos == -1)
    {
        shm_head->smalc_fail_count++;
        shmdt(shm_head);
        munmap(code, CODE_SIZE);
        exit(-3);
    }
    // memcpy(shm_codes+ my_gene.pos, code, my_gene.len);

    if(!shm_head->is_terminate && life < 10)
        goto Load;

    shm_head->life_end_count++;
    GeneLibrary::sfree(my_gene.pos, getpid(), (float)my_gene.fitness);
    shmdt(shm_head);
    munmap(code, CODE_SIZE);
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

void handle_child(int _sig)
{
    wait(nullptr);
}
void handle_sig(int _sig)
{
    exit(-_sig);
}
void handle_exit()
{

}