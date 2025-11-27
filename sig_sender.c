#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

volatile int conf = 1;
volatile int curr_bit = 0;
int send_num;
int rec_pid;

void confirm_handler(int nsig) {
    confirmed = 1;
}

void send_bit(int bit) {
    if (bit == 0) {
        kill(rec_pid, SIGUSR1);
    } else {
        kill(rec_pid, SIGUSR2);
    }
}

int main() {
    printf("Sender PID: %d\n", getpid());
    printf("Введите recipient PID: ");
    scanf("%d", &rec_pid);
    
    printf("Введите число для передачи: ");
    scanf("%d", &send_num);
    
    signal(SIGUSR1, confirm_handler);
    
    unsigned int num = (unsigned int)send_num;
    
    for (int i = 0; i < 32; i++) {
        conf = 0;
        curr_bit = (num >> i) & 1;
        send_bit(curr_bit);
        
        while (!conf) {
            usleep(100);
        }
    }
    
    kill(rec_pid, SIGINT);
    return 0;
}
