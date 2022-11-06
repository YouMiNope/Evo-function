#include "GeneLibrary.hpp"

#include <printf.h>
#include <sys/shm.h>
#include <errno.h>
#include <curses.h>
#include <strings.h>
#include <pthread.h>


class Viewer
{
public:
    static const int command_len = 128;

    int shm_id;
    uint8_t *data;
    GeneLibrary::shm_head_data *my_head;
    uint8_t *my_data;
    GeneLibrary::pid_mem *my_fitness;
    bool should_close = false;

    void init()
    {
        shm_id = shmget(ftok("./", 0), 0, 0666);
        if(shm_id < 0)                  printf("SHMGET::errno: %d", errno);

        data = (uint8_t *)shmat(shm_id, (void *)0, 0);
        if((void *)data == (void *)-1)  printf("SHMAT::errno: %d", errno);

        my_head    = ((GeneLibrary::shm_head_data *)data);
        my_data    = (uint8_t *)(data + my_head->p_data);
        my_fitness = (GeneLibrary::pid_mem *)(data + my_head->p_fitness);

        m_window = initscr();

        cbreak();
        nodelay(m_window, true);
        keypad(m_window, true);
    }

    void update()
    {
        erase();
        move(1, 1);
        
        printw("Gene total count: %ld", std::atomic_load(&my_head->gene_count_total));
        move(2, 1);
        printw("Gene fails count: %ld", get_failed_count());
        move(3, 1);
        printw("Fork fails: \t%ld", std::atomic_load(&my_head->fork_fail_count));
        move(4, 1);
        printw("Beaten: \t%ld", std::atomic_load(&my_head->sfree_fail_count));
        move(5, 1);
        printw("Mutate dead: \t%ld", std::atomic_load(&my_head->func_fail_count));
        move(6, 1);
        printw("Filled mem: \t%ld", std::atomic_load(&my_head->smalc_fail_count));
        move(7, 1);
        printw("End of life: \t%ld", std::atomic_load(&my_head->life_end_count));
        move(9, 1);
        printw("Active: \t%ld", std::atomic_load(&my_head->gene_count_total) - get_failed_count());
        move(10, 1);
        for(size_t i = 0; i < MAX_SIZE / 32; i++)
        {
            for(size_t j = 0; j < 32; j++)
            {
                GeneLibrary::pid_mem *a_pidm = my_fitness + i * 32 + j;
                if(a_pidm->ffitness > 1.00001)
                {
                    addch('@');
                }
                else if(a_pidm->ffitness >= 0.99999)
                {
                    addch('#');
                }
                else if(a_pidm->ffitness == 0.)
                {
                    addch('_');
                }
                else
                {
                    printw("%d", (int)(a_pidm->ffitness * 10));
                }
            }
            addstr("\n ");
        }
        refresh();
        usleep(16666);
    }

    void input_loop(void *)
    {
        ch = getch();
        move(16, 4);
        printw("aaa: %d", ch);
        switch (ch)
        {
        case -1:
            break;

        case 27:
            should_close = true;
            break;
        
        default:
            break;
        }
    }

    Viewer()
    {
        init();
    }
    ~Viewer()
    {
        endwin();
    }
private:
    char command[command_len];
    int ch;
    WINDOW *m_window;

    size_t get_failed_count()
    {
        size_t sum = 0;
        sum += my_head->fork_fail_count;
        sum += my_head->sfree_fail_count;
        sum += my_head->func_fail_count;
        sum += my_head->smalc_fail_count;
        sum += my_head->life_end_count;
        return sum;
    }

};

int main()
{
    Viewer a_viewer = Viewer();
    a_viewer.input_loop(nullptr);

    while(!a_viewer.should_close)
    {
        a_viewer.update();
        a_viewer.input_loop(nullptr);
    }

    a_viewer.my_head->is_terminate = true;

    return 0;
}