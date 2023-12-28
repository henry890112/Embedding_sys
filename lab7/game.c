#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>


typedef struct {
    int guess;
    char result[8];
} data;

data *shared_memory;
int shmid, secret_number;

void cleanup_shared_memory(int signal) {
    if (shmid != -1) {
        // 解除映射 share memory detach
        shmdt(shmat(shmid, NULL, 0));
        // 删除共享内存
        shmctl(shmid, IPC_RMID, NULL);
        printf("cleanup shared memory\n");
    }
    exit(0);
}

void handle_signal(int signum) {
    if (shared_memory->guess == secret_number) {
        snprintf(shared_memory->result, 8, "bingo");
        printf("Guessed %d, bingo\n", shared_memory->guess);
    } else if (shared_memory->guess > secret_number) {
        snprintf(shared_memory->result, 8, "bigger");
        printf("Guessed %d, bigger\n", shared_memory->guess);
    } else {
        snprintf(shared_memory->result, 8, "smaller");
        printf("Guessed %d, smaller\n", shared_memory->guess);
    }
}

int main(int argc, char *argv[]) {

    signal(SIGINT, cleanup_shared_memory);

    if (argc != 3) {
        printf("Usage: %s <key> <secret_number>\n", argv[0]);
        return 1;
    }

    key_t key = atoi(argv[1]);
    secret_number = atoi(argv[2]);

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
    // print the process id
    printf("Process ID: %d\n", getpid());

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGUSR1, &sa, NULL);

    while (1) {
        pause(); // Wait for signals
    }

    // shmdt(shared_memory);
    // shmctl(shmid, IPC_RMID, NULL);

    // return 0;
}


