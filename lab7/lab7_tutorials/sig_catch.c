/*
* sig_catch.c
*/
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/*
struct sigaction {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int , siginfo_t , void *);
    sigset_t sa_mask;
    int sa_flags;
}
*/
void handler (int signo, siginfo_t *info, void *context)
{
/* show the process ID sent signal */
    printf ("Process (%d) sent SIGUSR1.n", info->si_pid);
}
int main (int argc, char *argv[])
{
    struct sigaction my_action;
    /* register handler to SIGUSR1 */
    memset(&my_action, 0, sizeof (struct sigaction));
    my_action.sa_flags = SA_SIGINFO;   // 得知什麼時候接收sigaction的資訊
    my_action.sa_sigaction = handler;
    sigaction(SIGUSR1, &my_action, NULL);
    printf("Process (%d) is catching SIGUSR1 ...\n", getpid());
    sleep(10);
    printf("Done.\n");
    return 0;
}