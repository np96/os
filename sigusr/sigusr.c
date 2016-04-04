#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
bool caught;

void handler(int signo, siginfo_t *info, ucontext_t *uap) {
    if (signo == SIGUSR1) {
        printf("SIGUSR1 from %d\n", info->si_pid);
        caught = 1;
    }
    if (signo == SIGUSR2) {
        printf("SIGUSR2 from %d\n", info->si_pid);
        caught = 1;
    }
}

int main() {
    signal(SIGUSR1, handler);
    signal(SIGUSR2, handler);
    sleep(10);
    if (!caught) {
        printf("No signals were caught\n");
    }
    return 0;

    
}