#ifndef GENE_LIBRARY_H
#define GENE_LIBRARY_H

#include <unistd.h>
#include <inttypes.h>

namespace GeneLibrary
{
    struct shm_head_data
    {
        size_t max_size;
        size_t p_data;
        size_t p_fitness;
        size_t gene_count_total;
        size_t sfree_fail_count;
        size_t smalc_fail_count;
        size_t life_end_count;
        size_t func_fail_count;
        size_t fork_fail_count;
        bool is_terminate;
    };
    size_t page_size;
    size_t code_size;
    size_t max_size;
    int shm_id;
    shm_head_data *shm_head;
    uint8_t *shm_data;
    double  *shm_fitness;

    void link(size_t pages);
    void init();

    struct pid_mem
    {
        int pid;
        float ffitness;
    };
    // int sfree(posit, len, fitness):
    //      return 0 if success
    //      fail if fitness in shmem is biger than param fitness
    int sfree(size_t posit, size_t len, double fitness);
    int sfree(size_t posit, int pid, float ffitness);

    // size_t smalloc(len, fitness):
    //      return posit, -1 if failed
    //      fail when no place for alloc
    size_t smalloc(size_t len, double fitness);
    size_t smalloc(int pid, float ffitness);

    // mutate
    //      return new lenth of mutated code
    struct mutate_prob
    {
        uint8_t flip = 32;
        uint8_t add  = 32;
        uint8_t del  = 32;
    };
    size_t mutate(uint8_t *target, size_t len, mutate_prob m_probs);

    // generate test data
    void gen_test_data(double *target, size_t len);

    // load the origin code into shmem at posit 0
    size_t load_seed_code(const char *path);

    void set_terminate(bool);
    void get_head_data(shm_head_data *);
}
#endif