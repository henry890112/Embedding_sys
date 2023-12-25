/*
* sig_count.c
*/
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
/*
sig_atomic_t is an integer type which can be accessed as an atomic entity
在多線程的環境下，sig_atomic_t可以確保在一個動作中完成，不會被其他動作中斷, 避免race condition
*/
sig_atomic_t sigusr1_count = 0;
void handler (int signal_number)
{
    ++sigusr1_count;
/* add one, protected atomic action */
}
int main ()
{
    struct sigaction sa;
    struct timespec req;
    int retval;
    /* set the sleep time to 10 sec */
    memset(&req, 0, sizeof(struct timespec));
    req.tv_sec = 10;
    req.tv_nsec = 0;
    /* register handler to SIGUSR1 */
    memset(&sa, 0, sizeof (sa));
    sa.sa_handler = handler;
    sigaction (SIGUSR1, &sa, NULL);
    printf("Process (%d) is catching SIGUSR1 ...\n", getpid());
    /* sleep 10 sec */
    do
    {
        /*
        retval 變數被用來儲存 nanosleep 函數的返回值。如果休眠時間內未被中斷，它會設定為 0，表示成功休眠完整的時間。如果休眠被中斷（例如，接收到信號），則 retval 會設定為 -1。
        當 retval 變數為 0，表示休眠時間已經完整結束，do-while 迴圈會結束執行，程式將繼續執行下面的代碼。
        */
        kill(getpid(), SIGUSR1);  // 觸發 SIGUSR1 信號此example只會觸發一次
        retval = nanosleep(&req, &req);
    } 
    while(retval);
        printf ("SIGUSR1 was raised %d times\n", sigusr1_count);
    return 0;
}
