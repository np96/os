#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
int caught = 0;

int sigpid;
int signum;
bool temp;
void handler(int signo, siginfo_t *info, ucontext_t *uap) {
    if (temp == 1) return;
    temp = 1;
    caught = 1;
    signum = info->si_signo;
    sigpid = info->si_pid;
}

int main() {
    signal(SIGUSR1, handler);
    signal(SIGUSR2, handler);
    sleep(10);
    if (!caught) {
        printf("No signals were caught\n");
    } else {
        printf("SIGUSR%d from %d", (signum%2) + 1, sigpid);
        
    }
    return 0;

    
}