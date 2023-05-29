#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define INF 10000000

struct myData {
  int number;
  int max_step;
  int level;
  int full[INF];
  int results[INF][5];
};

sem_t *sem;
volatile __sig_atomic_t stopFlag = 0;
void ctrlC(int num) { stopFlag = 1; }

typedef int natural_number;
int collatz(natural_number x, natural_number maxi) {
  int steps = 0;
  while (x != 1) {
    steps++;         // until you reach 1,
    if (x % 2)       // if the number is odd,
      x = 3 * x + 1; // multiply it by three and add one,
    else             // else, if the number is even,
      x /= 2;        // divide it by two
    if (steps >= maxi) {
      return -1;
    }
  }

  return steps;
}

int init_max_steps(struct myData *data, int value) {
  if (data->max_step == 0) {
    data->max_step = value;
    data->number = 11;
  } else if (data->max_step != value) {
    return 0;
  }
  return 1;
}

int read_current_level_number(struct myData *data) { return data->number; }

int read_current_level(struct myData *data) { return data->level; }

int read_number(struct myData *data) {
  int number = data->number;
  data->number++;

  return number;
}

void write_result(struct myData *data, int number_step, int number) {
  if (data->full[number_step]) {
    int maxi = 0;
    int maxi_index = 0;
    for (int i = 0; i < 5; i++) {

      if (data->results[number_step][i] > maxi) {
        maxi = data->results[number_step][i];
        maxi_index = i;
      }
    }
    if (maxi > number) {
      data->results[number_step][maxi_index] = number;
    }
    return;
  } else {
    for (int i = 0; i < 5; i++) {
      if (data->results[number_step][i] == 0) {
        data->results[number_step][i] = number;
        if (i == 4) {
          data->full[number_step] = 1;
          if (number_step > 10)
            data->level++;
        }
        return;
      }
    }
  }
}

void print_results(struct myData *data) {
  for (int i = 11; i < data->max_step; i++) {
    printf("number steps: %d, numbers: %d, %d, %d, %d, %d\n", i,
           data->results[i][0], data->results[i][1], data->results[i][2],
           data->results[i][3], data->results[i][4]);
  }
}

int main(int argc, char *argv[]) {

  if (argc != 2) {
    perror("no max steps");
    exit(1);
  }

  int max_steps = atoi(argv[1]);

  //   int max_steps = 200000;
  int fd = shm_open("/collatz", O_RDWR | O_CREAT, 0600);
  if (fd == -1) {
    perror("shm_open failed");
    exit(1);
  }

  int r = ftruncate(fd, sizeof(struct myData));
  if (r == -1) {
    perror("ftruncate failed");
    exit(1);
  }
  struct myData *data = mmap(NULL, sizeof(struct myData),
                             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if ((long int)data == -1) {
    perror("mmap failed");
    exit(1);
  }

  close(fd);
  sem = sem_open("buffer.modify", O_RDWR | O_CREAT, 0666, 1);

  signal(SIGINT, ctrlC);

  sem_wait(sem);
  int check = init_max_steps(data, max_steps);
  sem_post(sem);

  if (!check) {
    perror("max step has been already defined");
    exit(1);
  }

  int current_number = 0, step, level;
  while (!stopFlag) {
    sem_wait(sem);
    current_number = read_number(data);
    sem_post(sem);

    step = collatz(current_number, max_steps);

    if (step != -1) {
      sem_wait(sem);
      write_result(data, step, current_number);
      sem_post(sem);
    }

    sem_wait(sem);
    level = read_current_level(data);
    sem_post(sem);
    if (level >= (max_steps - 11)) {
      break;
    }

    if (stopFlag) {
      sem_wait(sem);
      munmap(&data, sizeof(struct myData));
      sem_post(sem);
      return 0;
    }
  }

  sem_wait(sem);
  munmap(&data, sizeof(struct myData));
  sem_post(sem);

  sem_wait(sem);
  print_results(data);
  sem_post(sem);

  return 0;
}