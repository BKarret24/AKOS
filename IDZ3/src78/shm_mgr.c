#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <signal.h>

#define SHM_N "/shm_mg"
#define MAXQ 64
#define MAXB 32

typedef struct {
    int qa[MAXQ];
    int qb[MAXQ];
    int ha;
    int ta;
    int hb;
    int tb;
} shm_t;

int shm_fd;
shm_t *shm_ptr;

void hdl(int s) {
    printf("shm_mgr: завершение запрещено\n");
}

int main() {
    signal(SIGINT, hdl);
    shm_fd = shm_open(SHM_N, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(shm_t));
    shm_ptr = mmap(0, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    memset(shm_ptr, 0, sizeof(shm_t));
    sem_open("/semA", O_CREAT, 0666, 0);
    sem_open("/semB", O_CREAT, 0666, 0);
    sem_open("/semQA", O_CREAT, 0666, 1);
    sem_open("/semQB", O_CREAT, 0666, 1);
    for (int i = 0; i < MAXB; i++) {
        char nm[32];
        sprintf(nm, "/semBUY_%d", i);
        sem_open(nm, O_CREAT, 0666, 0);
    }
    unlink("/tmp/shop_log");
    mkfifo("/tmp/shop_log", 0666);
    printf("shm_mgr: память и семафоры созданы\n");
    printf("shm_mgr: можно запускать продавцов и покупателей\n");

    while (1) pause();
}

