
#include "GeneLibrary.hpp"

#include <sys/mman.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <cstdio>
#include <errno.h>
#include <memory.h>
#include <random>

const size_t INIT_FACTOR_LEN = 4;
const uint8_t init_factor[] = {
    0x55,               // push rbp
    0x48, 0x89, 0xe5,   // mov rbp rsp
};
const size_t ENDL_FACTOR_LEN = 2;
const uint8_t endl_factor[] = {
    0x5d,               // pop rbp
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
        max_size  = sizeof(shm_head_data) + code_size + code_size * sizeof(double);

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
        shm_fitness = (double *)(shm_data + code_size);
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
        pthread_mutexattr_t _attr;
        pthread_mutexattr_init(&_attr);
        pthread_mutexattr_setpshared(&_attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&shm_head->mem_lock, &_attr);
    }

    size_t load_seed_code(const char *path)
    {
        int fd = open(path, O_RDONLY);
        if(fd < 0)
        {
            printf("ERROR: opening %s\n", path);
            return -1;
        }
        uint8_t *file_code = (uint8_t *)mmap(NULL, 1 * page_size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);
        close(fd);

        if((void *)file_code == MAP_FAILED)
        {
            printf("ERROR: createing mmap, errno %d\n", errno);
            return -1;
        }

        size_t _begin, _end;
        _begin = loc_init_factor(file_code, 0, 1 * page_size);
        if(_begin == 1 * page_size)
        {
            printf("ERROR: can't locate initfactor of file %s\n", path);
            return -1;
        }
        _end = loc_endl_factor(file_code, _begin, 1 * page_size);
        if(_end == 1 * page_size)
        {
            printf("ERROR: can't locate endlfactor of file %s\n", path);
            return -1;
        }
        memcpy(shm_data, file_code + _begin, _end - _begin);
        for(size_t i = 0; i < code_size; i++)
        {
            shm_fitness[i] = 0;
        }

        munmap(file_code, 1 * page_size);
        return _end - _begin;
    }

    int sfree(size_t posit, size_t len, double fitness)
    {
        for(size_t i = 0; i < len; i++)
        {
            if(shm_fitness[posit + i] > fitness + 0.000001)    return -1;
            else if(shm_fitness[posit + i] <= fitness + 0.000001)
            {
                shm_fitness[posit + i] = 0.;
            }
        }
        return 0;
    }
    int sfree(size_t posit, int pid, float fitness)
    {
        pid_mem *a_pid = (pid_mem *)(shm_fitness + posit);
        if(a_pid->pid != pid)
        {
            return -1;
        }
        a_pid->ffitness = 0.f;
        return 0;
    }

    size_t smalloc(size_t len, double fitness)
    {
        size_t count = 0;
        for(size_t i = 0; i < code_size; i++)
        {
            if(shm_fitness[i] + 0.000001 < fitness)
            {
                shm_fitness[i] += 1.;
                count++;
            }
            else
            {
                for(size_t j = i + 1 - count; j < i + 1; j++)
                {
                    shm_fitness[j] -= 1.;
                }
                count = 0;
            }

            if(count >= len)
            {
                for(size_t j = i + 1 - count; j < i + 1; j++)
                {
                    shm_fitness[j] = fitness;
                }
                return i + 1 - count;
            }
        }
        return -1;
    }
    size_t smalloc(int pid, float ffitness)
    {
        for(size_t i = 0; i < MAX_SIZE; i++)
        {
            pid_mem *a_pid = (pid_mem *)(shm_fitness + i);
            if(a_pid->ffitness + 0.00001f < ffitness)
            {
                pthread_mutex_lock(&shm_head->mem_lock);
                a_pid->pid = pid;
                a_pid->ffitness = ffitness;
                pthread_mutex_unlock(&shm_head->mem_lock);
                return i;
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
                target[i] = 0;
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