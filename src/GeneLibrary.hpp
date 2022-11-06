#ifndef GENE_LIBRARY_H
#define GENE_LIBRARY_H

#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>
#include <atomic>
#include <signal.h>

#define MAX_SIZE (32 * 8)

namespace GeneLibrary
{
    struct shm_head_data
    {
        size_t max_size;
        size_t p_data;
        size_t p_fitness;
        std::atomic_uint64_t gene_count_total;
        std::atomic_uint64_t sfree_fail_count;
        std::atomic_uint64_t smalc_fail_count;
        std::atomic_uint64_t life_end_count;
        std::atomic_uint64_t func_fail_count;
        std::atomic_uint64_t fork_fail_count;

        std::atomic_uint64_t last_posit;
        // std::atomic_uint64_t mem_count;
        
        pthread_spinlock_t mem_lock;
        std::atomic_bool is_terminate;
    };
    struct pid_mem
    {
        std::atomic_int32_t pid;
        std::atomic<double> ffitness;
    };
    size_t page_size;
    size_t code_size;
    size_t max_size;
    int shm_id;
    shm_head_data *shm_head;
    uint8_t *shm_data;
    pid_mem *shm_fitness;

    sigset_t block_set;

    void link(size_t pages);
    void init();

    // int sfree(posit, len, fitness):
    //      return 0 if success
    //      fail if fitness in shmem is biger than param fitness
    int sfree(size_t posit, int pid, double ffitness);

    // size_t smalloc(len, fitness):
    //      return posit, -1 if failed
    //      fail when no place for alloc
    size_t smalloc(int pid, double ffitness);

    // mutate
    //      return new lenth of mutated code
    struct mutate_prob
    {
        uint8_t flip = 32;
        uint8_t add  = 32;
        uint8_t del  = 32;
        void never()
        {
            flip = 32;
            add  = 32;
            del  = 32;
        }
    };
    size_t mutate(uint8_t *target, size_t len, mutate_prob m_probs);

    // generate test data
    void gen_test_data(double *target, size_t len);

    // load the origin code into shmem at posit 0
    size_t load_seed_code(const char *path, uint8_t *dst);

    void set_terminate(bool);
    void get_head_data(shm_head_data *);
}
#endif