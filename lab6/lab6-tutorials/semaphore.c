/* doodle.c
 *
 * This program shows how P () and V () can be implemented,
 * then uses a semaphore that everyone has access to.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <unistd.h>

#define DOODLE_SEM_KEY 1122334455

/* P () - returns 0 if OK; -1 if there was a problem */
int P(int s)
{
    struct sembuf sop; /* the operation parameters */
    sop.sem_num = 0;
    /* access the 1st (and only) sem in the array */
    sop.sem_op = -1;
    /* wait..*/
    sop.sem_flg = 0;
    /* no special options needed */
    if (semop(s, &sop, 1) < 0) {
        fprintf(stderr, "P(): semop failed: %s\n", strerror(errno));
        return -1;
    } else {
        return 0;
    }
}

/* V() - returns 0 if OK; -1 if there was a problem */
int V(int s)
{
    struct sembuf sop; /* the operation parameters */
    sop.sem_num = 0;
    /* the 1st (and only) sem in the array */
    sop.sem_op = 1;
    /* signal */
    sop.sem_flg = 0;
    /* no special options needed */
    if (semop(s, &sop, 1) < 0) {
        fprintf(stderr, "V(): semop failed: %s\n", strerror(errno));
        return -1;
    } else {
        return 0;
    }
}

int main(int argc, char **argv)
{
    int s, secs;
    long int key;

    if (argc != 2) {
        fprintf(stderr, "%s: specify a key (long)\n", argv[0]);
        exit(1);
    }

    /* get values from the command line */
    if (sscanf(argv[1], "%ld", &key) != 1) {
        /* convert arg to a long integer */
        fprintf(stderr, "%s: argument #1 must be a long integer\n", argv[0]);
        exit(1);
    }

    // s = semget(key, 1, IPC_CREAT | 0666);
    s = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666);

    if (s < 0) {
        if (errno == EEXIST) {
            // 信號量已經存在，只取得其 ID
            s = semget(key, 1, 0666);
            printf("Semaphore already exists. Semaphore ID: %d\n", s);
        }
        else
        {
            fprintf(stderr, "%s: cannot create or find semaphore %ld: %s\n", argv[0], key, strerror(errno));
            exit(1);
        }
    } else {
        // 初次創建信號量，將其初值設置為 1
        printf("Semaphore first created. Semaphore ID: %d\n", s);
        if (semctl(s, 0, SETVAL, 1) < 0)
        {
            fprintf(stderr, "Error setting initial value for the semaphore: %s\n", strerror(errno));
            exit(1);
        }
    }

    printf("Semaphore created successfully. Semaphore ID: %d\n", s);


    while (1) {
        printf("#secs to doodle in the critical section? (0 to exit): ");
        // scanf("%d", &secs);

        if (scanf("%d", &secs) != 1) {
            fprintf(stderr, "Error reading input.\n");
            exit(1);
        }

        if (secs == 0)
        {
            // 刪除信號量
            if (semctl(s, 0, IPC_RMID, 0) < 0) {
                fprintf(stderr, "Error removing the semaphore: %s\n", strerror(errno));
                exit(1);
            }
            printf("Semaphore removed successfully.\n");
            break;
        }

        printf("Preparing to enter the critical section..\n");
        P(s);

        printf("Now in the critical section! Sleep %d secs..\n", secs);

        while (secs) {
            printf("%d...doodle...\n", secs--);
            sleep(1);
        }

        printf("Leaving the critical section..\n");
        V(s);
    }

    return 0;
}
