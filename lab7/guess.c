#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

typedef struct {
    int guess;
    char result[8];
} data;

data *shared_memory;
int shmid, upper_bound, pid;
int lower_bound = 0;


void timer_handler(int signum) {

    if (upper_bound <= lower_bound) {
        printf("Error in guessing range.\n");
        exit(1);
    }

    shared_memory->guess = (lower_bound + upper_bound) / 2;
    // printf("current lower_bound: %d, upper_bound: %d, guess: %d\n", lower_bound, upper_bound, shared_memory->guess);
    printf("Guess: %d\n", shared_memory->guess);
    kill(pid, SIGUSR1);
    // sleep to wait for the result
    usleep(100);

    if (strcmp(shared_memory->result, "smaller") == 0) {
        lower_bound = shared_memory->guess + 1;
    } else if (strcmp(shared_memory->result, "bigger") == 0) {
        upper_bound = shared_memory->guess - 1;
    } else if (strcmp(shared_memory->result, "bingo") == 0) {
        printf("Guessed the number: %d\n", shared_memory->guess);
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <key> <upper_bound> <pid>\n", argv[0]);
        return 1;
    }

    key_t key = atoi(argv[1]);
    upper_bound = atoi(argv[2]);
    pid = atoi(argv[3]);

    shmid = shmget(key, sizeof(data), 0644|IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    shared_memory = (data *)shmat(shmid, NULL, 0);
    if (shared_memory == (void *)-1) {
        perror("shmat");
        return 1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = timer_handler;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer = {{1, 0}, {1, 0}};
    setitimer(ITIMER_REAL, &timer, NULL);

    while (1) {
        pause();
    }

    return 0;
}
