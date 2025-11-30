#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <signal.h>
#include <time.h>

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
sem_t *semA;
sem_t *semB;
sem_t *semQA;
sem_t *semQB;
sem_t *bsem[MAXB];

void snd(const char *s) {
    int f = open("/tmp/shop_log", O_WRONLY | O_NONBLOCK);
    if (f >= 0) {
        write(f, s, strlen(s));
        close(f);
    }
}
void hdl(int s) {
    printf("seller: завершение по сигналу\n");
    exit(0);
}
int main(int ac, char **av) {
    if (ac < 2) {
        printf("Использование: seller A|B\n");
        return 1;
    }
    char d = av[1][0];
    int dep;
    if (d == 'A' || d == 'a') dep = 0;
    else if (d == 'B' || d == 'b') dep = 1;
    else {
        printf("Неизвестный отдел: %c\n", d);
        return 1;
    }
    signal(SIGINT, hdl);
    signal(SIGTERM, hdl);
    setbuf(stdout, NULL);
    shm_fd = shm_open(SHM_N, O_RDWR, 0666);
    if (shm_fd < 0) {
        printf("seller: не удалось открыть разделяемую память\n");
        return 1;
    }
    shm_ptr = mmap(0, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        printf("seller: ошибка mmap\n");
        return 1;
    }
    semA = sem_open("/semA", 0);
    semB = sem_open("/semB", 0);
    semQA = sem_open("/semQA", 0);
    semQB = sem_open("/semQB", 0);
    if (!semA || !semB || !semQA || !semQB) {
        printf("seller: не удалось открыть семафоры отделов\n");
        return 1;
    }
    for (int i = 0; i < MAXB; i++) {
        char nm[32];
        sprintf(nm, "/semBUY_%d", i);
        bsem[i] = sem_open(nm, 0);
        if (!bsem[i]) {
            printf("seller: не удалось открыть семафор покупателя %d\n", i);
            return 1;
        }
    }
    srand(time(NULL) ^ getpid());
    if (dep == 0) {
        printf("Продавец A запущен, pid=%d\n", getpid());
        char t[128];
        sprintf(t, "OBS: Продавец A запущен pid=%d\n", getpid());
        snd(t);
    } else {
        printf("Продавец B запущен, pid=%d\n", getpid());
        char t[128];
        sprintf(t, "OBS: Продавец B запущен pid=%d\n", getpid());
        snd(t);
    }
    while (1) {
        int id;
        if (dep == 0) {
            sem_wait(semA);
            sem_wait(semQA);
            int h = shm_ptr->ha;
            id = shm_ptr->qa[h];
            shm_ptr->ha = (h + 1) % MAXQ;
            sem_post(semQA);
            printf("Продавец A обслуживает покупателя %d\n", id);
            char t[128];
            sprintf(t, "OBS: Продавец A обслуживает покупателя %d\n", id);
            snd(t);
            sleep(1 + rand() % 3);
            printf("Продавец A завершил обслуживание покупателя %d\n", id);
            sprintf(t, "OBS: Продавец A завершил обслуживание покупателя %d\n", id);
            snd(t);
        } else {
            sem_wait(semB);
            sem_wait(semQB);
            int h = shm_ptr->hb;
            id = shm_ptr->qb[h];
            shm_ptr->hb = (h + 1) % MAXQ;
            sem_post(semQB);
            printf("Продавец B обслуживает покупателя %d\n", id);
            char t[128];
            sprintf(t, "OBS: Продавец B обслуживает покупателя %d\n", id);
            snd(t);
            sleep(1 + rand() % 3);
            printf("Продавец B завершил обслуживание покупателя %d\n", id);
            sprintf(t, "OBS: Продавец B завершил обслуживание покупателя %d\n", id);
            snd(t);
        }
        if (id >= 0 && id < MAXB) {
            sem_post(bsem[id]);
        }
    }
    return 0;
}


