#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

volatile int rec_num = 0;
volatile int bit_cnt = 0;
int sender_pid;

void bit_handler(int nsig) {
    if (nsig == SIGUSR1) {
        rec_num |= (0 << bit_cnt);
    } else if (nsig == SIGUSR2) {
        rec_num |= (1 << bit_cnt);
    }
    bit_cnt++;
    kill(sender_pid, SIGUSR1);
}

void finish_handler(int nsig) {
    printf("Received number: %d\n", rec_num);
    exit(0);
}

int main() {
    printf("Recipient PID: %d\n", getpid());
    printf("Введите sender PID: ");
    scanf("%d", &sender_pid);
    
    signal(SIGUSR1, bit_handler);
    signal(SIGUSR2, bit_handler);
    signal(SIGINT, finish_handler);
    
    while (1) {
        pause();
    }
    
    return 0;
}
