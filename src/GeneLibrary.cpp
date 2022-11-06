
#include "GeneLibrary.hpp"

#include <sys/mman.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <cstdio>
#include <errno.h>
#include <memory.h>
#include <random>
#include <sys/stat.h>

const size_t INIT_FACTOR_LEN = 1;
const uint8_t init_factor[] = {
    // 0x55,                // push rbp
    // 0x48, 0x89, 0xe5,    // mov rbp rsp
    // 0xf3, 0x0f, 0x1e, 0xfa, // endbr64
    0x48,
};
const size_t ENDL_FACTOR_LEN = 5;
const uint8_t endl_factor[] = {
    // 0x5d,               // pop rbp
    0x48, 0x83, 0xc4, 0x30,
    0xc3,               // ret
};
size_t loc_init_factor(uint8_t *target, size_t begin, size_t end)
{
    size_t loc_init = 0;
    size_t loc_out = begin;
    for(; loc_out < end; loc_out++)
    {
        if(target[loc_out] == init_factor[loc_init])
        {
            loc_init++;
            if(loc_init == INIT_FACTOR_LEN)   return loc_out - INIT_FACTOR_LEN + 1;
        }
        else
        {
            loc_init = 0;
        }
    }
    return end;
}

size_t loc_endl_factor(uint8_t *target, size_t begin, size_t end)
{
    size_t loc_endl = 0;
    size_t loc_out = begin;
    for(; loc_out < end; loc_out++)
    {
        if(target[loc_out] == endl_factor[loc_endl])
        {
            loc_endl++;
            if(loc_endl == ENDL_FACTOR_LEN)   return loc_out + 1;
        }
        else
        {
            loc_endl = 0;
        }
    }
    return end;
}

uint8_t get_fault(uint8_t level)
{
    uint8_t fault = 0xFF;
    for(uint8_t i = 0; i < level; i++)
    {
        fault &= random() & 0xFF;
    }
    return fault;
}

namespace GeneLibrary
{
    void link(size_t pages)
    {
        page_size = sysconf(_SC_PAGE_SIZE);
        code_size = page_size * pages;
        max_size  = sizeof(shm_head_data) + code_size + code_size * sizeof(pid_mem);

        shm_id = shmget(ftok("./", 0), max_size, 0666 | IPC_CREAT);
        if(shm_id < 0)
        {
            printf("ERROR: share memory get failed, errno: %d\n", errno);
            return;
        }
        shm_head = (shm_head_data *)shmat(shm_id, (void *)0, 0);
        if((void *)shm_head == (void *)-1)
        {
            printf("ERROR: shmat failed with errno: %d\n", errno);
            return;
        }
        printf("share mem linked! shm_id: %d\n", shm_id);
        shm_data = (uint8_t *)(shm_head + 1);
        shm_fitness = (pid_mem *)(shm_data + code_size);
    }

    void init()
    {
        srandom(time(0));
        shm_head->is_terminate      = false;
        shm_head->max_size          = max_size;
        shm_head->p_data            = sizeof(shm_head_data);
        shm_head->p_fitness         = shm_head->p_data + code_size;
        shm_head->gene_count_total  = 1;
        shm_head->sfree_fail_count  = 0;
        shm_head->smalc_fail_count  = 0;
        shm_head->fork_fail_count   = 0;
        shm_head->func_fail_count   = 0;
        shm_head->life_end_count    = 0;
        shm_head->last_posit        = 0;
        // shm_head->mem_count         = 0;
        pthread_spin_init(&shm_head->mem_lock, PTHREAD_PROCESS_SHARED);

        sigemptyset(&block_set);
        sigaddset(&block_set, SIGALRM);
    }

    size_t load_seed_code(const char *path, uint8_t *dst)
    {
        int fd = open(path, O_RDONLY);
        if(fd < 0)
        {
            printf("ERROR: opening %s\n", path);
            return -1;
        }
        
        struct stat statbuf;
        fstat(fd, &statbuf);

        ssize_t ret = read(fd, dst, statbuf.st_size);
        if(ret == -1)
        {
            printf("error loading seed code: %s, errno: %d\n", path, errno);
        }
        close(fd);

        memset(shm_fitness, 0x00, statbuf.st_size * sizeof(pid_mem));

        printf("len:%ld\n", statbuf.st_size);
        return statbuf.st_size;
    }

    int sfree(size_t posit, int pid, double)
    {
        pid_mem *a_pid = (pid_mem *)(shm_fitness + posit);
        auto tmp = std::atomic_exchange(&a_pid->ffitness, 10.);
        if(tmp > 1.0001)
        {
            return -2;
        }
        else if(a_pid->pid != pid)
        {
            a_pid->ffitness = tmp;
            return -1;
        }
        a_pid->ffitness = 0.;

        return 0;
    }

    size_t smalloc(int pid, double ffitness)
    {

        // if(shm_head->active_count >= MAX_SIZE)  return -1;
        for(size_t i = 0; i < MAX_SIZE; i++)
        {
            size_t pos = (i + shm_head->last_posit) % MAX_SIZE;
            pid_mem *a_pid = (pid_mem *)(shm_fitness + pos);

            auto tmp = std::atomic_exchange(&a_pid->ffitness, 10.);
            if(tmp < ffitness)
            {
                // shm_head->mem_count++;
                shm_head->last_posit = (pos + 1) % MAX_SIZE;
                a_pid->pid = pid;
                a_pid->ffitness = ffitness;

                return pos;
            }
            else if(tmp <= 1.0001)
            {
                a_pid->ffitness = tmp;
            }
        }
        return -1;
    }

    void gen_test_data(double *target, size_t len)
    {
        for(size_t i = 0; i < len; i++)
        {
            target[i] = ((double)random() * 2 / (double)0x7fffffff) - 1.;
        }
    }

    size_t mutate(uint8_t *target, size_t len, mutate_prob m_probs)
    {
        size_t out_len = len;
        for(size_t i = 0; i < len; i++)
        {
            if(m_probs.flip < 32)
            {
                target[i] ^= get_fault(m_probs.flip);
            }
            if(m_probs.add  < 32 && get_fault(m_probs.add) > 0)
            {
                for(size_t j = len; j > i; j--)
                {
                    target[j] = target[j - 1];
                }
                target[i] = random() & 0xFF;
                out_len++;
            }
            if(m_probs.del  < 32 && get_fault(m_probs.del) > 0)
            {
                for(size_t j = i; j < len - 1; j++)
                {
                    target[j] = target[j + 1];
                }
                target[len - 1] = 0;
                out_len--;
            }
        }
        return out_len;
    }

    void set_terminate(bool is_terminate)
    {
        shm_head->is_terminate = is_terminate;
    }
}