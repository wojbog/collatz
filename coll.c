#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>
#include <semaphore.h>

#define CHECK(result, textOnFail) \
    if (((long int)result) == -1) \
    {                             \
        perror(textOnFail);       \
        exit(1);                  \
    }

struct myData
{
    int number;
    int max_step;
    int level;
    bool full[50];
    int results[50][5];
};

sem_t *sem;

typedef int natural_number;
int collatz(natural_number x, natural_number maxi)
{
    int steps = 0;
    while (x != 1)
    {
        steps++;           // until you reach 1,
        if (x % 2)         // if the number is odd,
            x = 3 * x + 1; // multiply it by three and add one,
        else               // else, if the number is even,
            x /= 2;        // divide it by two
        if (steps > maxi)
        {
            return -1;
        }
    }

    return steps;
}

void init_max_steps(struct myData *data, int value)
{
    if (data->max_step == 0)
    {
        data->max_step = value;
        data->number = 11;
    }
}

int read_current_level_number(struct myData *data)
{
    return data->number;
}

int read_current_level(struct myData *data)
{
    return data->level;
}

int read_number(struct myData *data)
{
    int number = data->number;
    data->number++;

    return number;
}

void write_result(struct myData *data, int number_step, int number)
{
    if (data->full[number_step])
    {
        int maxi = 0;
        int maxi_index = 0;
        for (int i = 0; i < 5; i++)
        {

            if (data->results[number_step][i] > maxi)
            {
                maxi = data->results[number_step][i];
                maxi_index = i;
            }
        }
        if (maxi > number)
        {
            data->results[number_step][maxi_index] = number;
        }
        return;
    }
    else
    {
        for (int i = 0; i < 5; i++)
        {
            if (data->results[number_step][i] == 0)
            {
                data->results[number_step][i] = number;
                if (i == 4)
                {
                    data->full[number_step] = true;
                    data->level++;
                }
                return;
            }
        }
    }
}

void print_results(struct myData *data)
{
    for (int i = 0; i < data->max_step; ++i)
    {
        printf("number steps: %d, numbers: %d, %d, %d, %d, %d\n", i, data->results[i][0], data->results[i][1], data->results[i][2], data->results[i][3], data->results[i][4]);
    }
}

int main()
{
    int max_steps = 20;
    int fd = shm_open("/collatz_2", O_RDWR | O_CREAT, 0600);
    CHECK(fd, "shm_open failed");
    int r = ftruncate(fd, sizeof(struct myData));
    CHECK(r, "ftruncate failed");
    struct myData *data = mmap(NULL, sizeof(struct myData),
                               PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    CHECK(data, "mmap failed");
    close(fd);

    sem = sem_open("buffer.modify", O_RDWR | O_CREAT, 0666, 1);

    sem_wait(sem);
    init_max_steps(data, max_steps);
    sem_post(sem);

    sem_wait(sem);
    printf("%d\n", read_current_level_number(data));
    sem_post(sem);

    int current_number, step, level;
    while (1)
    {
        sem_wait(sem);
        current_number = read_number(data);
        sem_post(sem);

        step = collatz(current_number, max_steps);

        if (step != -1)
        {
            sem_wait(sem);
            write_result(data, step, current_number);
            sem_post(sem);
        }

        sem_wait(sem);
        level = read_current_level(data);
        sem_post(sem);
        if (level >= (max_steps - 10))
        {
            break;
        }

        // sem_wait(sem);
        // print_results(data);
        // sem_post(sem);

        // sem_wait(sem);
        // munmap(data, sizeof(struct myData));
        // sem_post(sem);

    }

    sem_wait(sem);
    print_results(data);
    sem_post(sem);
}