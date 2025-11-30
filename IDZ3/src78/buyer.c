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
sem_t *bsem;

void snd(const char *s) {
    int f = open("/tmp/shop_log", O_WRONLY | O_NONBLOCK);
    if (f >= 0) {
        write(f, s, strlen(s));
        close(f);
    }
}
void hdl(int s) {
    printf("buyer: завершение по сигналу\n");
    exit(0);
}
int main(int ac, char **av) {
    if (ac < 2) {
        printf("Использование: buyer <id>\n");
        return 1;
    }
    int id = atoi(av[1]);
    if (id < 0 || id >= MAXB) {
        printf("Некорректный id покупателя\n");
        return 1;
    }
    signal(SIGINT, hdl);
    signal(SIGTERM, hdl);
    setbuf(stdout, NULL);
    shm_fd = shm_open(SHM_N, O_RDWR, 0666);
    if (shm_fd < 0) {
        printf("buyer: не удалось открыть разделяемую память\n");
        return 1;
    }
    shm_ptr = mmap(0, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        printf("buyer: ошибка mmap\n");
        return 1;
    }
    semA = sem_open("/semA", 0);
    semB = sem_open("/semB", 0);
    semQA = sem_open("/semQA", 0);
    semQB = sem_open("/semQB", 0);
    if (!semA || !semB || !semQA || !semQB) {
        printf("buyer: не удалось открыть семафоры отделов\n");
        return 1;
    }
    char nm[32];
    sprintf(nm, "/semBUY_%d", id);
    bsem = sem_open(nm, 0);
    if (!bsem) {
        printf("buyer: не удалось открыть семафор покупателя\n");
        return 1;
    }
    srand(time(NULL) ^ getpid());
    int k = 1 + rand() % 5;
    int lst[5];
    for (int i = 0; i < k; i++) {
        lst[i] = rand() % 2;
    }
    printf("Покупатель %d начал покупки, товаров в списке: %d\n", id, k);
    {
        char t[128];
        sprintf(t, "OBS: Покупатель %d начал покупки (%d)\n", id, k);
        snd(t);
    }
    for (int i = 0; i < k; i++) {
        int d = lst[i];
        if (d == 0) {
            printf("Покупатель %d идет в отдел A за товаром %d\n", id, i + 1);
            {
                char t[128];
                sprintf(t, "OBS: Покупатель %d идет в отдел A за товаром %d\n", id, i + 1);
                snd(t);
            }
            sem_wait(semQA);
            int t = shm_ptr->ta;
            shm_ptr->qa[t] = id;
            shm_ptr->ta = (t + 1) % MAXQ;
            sem_post(semQA);
            printf("Покупатель %d встал в очередь в отдел A\n", id);
            {
                char t[128];
                sprintf(t, "OBS: Покупатель %d встал в очередь A\n", id);
                snd(t);
            }
            sem_post(semA);
        } else {
            printf("Покупатель %d идет в отдел B за товаром %d\n", id, i + 1);
            {
                char t[128];
                sprintf(t, "OBS: Покупатель %d идет в отдел B за товаром %d\n", id, i + 1);
                snd(t);
            }
            sem_wait(semQB);
            int t = shm_ptr->tb;
            shm_ptr->qb[t] = id;
            shm_ptr->tb = (t + 1) % MAXQ;
            sem_post(semQB);
            printf("Покупатель %d встал в очередь в отдел B\n", id);
            {
                char t[128];
                sprintf(t, "OBS: Покупатель %d встал в очередь B\n", id);
                snd(t);
            }
            sem_post(semB);
        }
        sem_wait(bsem);
        if (d == 0) {
            printf("Покупатель %d купил товар %d в отделе A\n", id, i + 1);
            {
                char t[128];
                sprintf(t, "OBS: Покупатель %d купил товар %d в отделе A\n", id, i + 1);
                snd(t);
            }
        } else {
            printf("Покупатель %d купил товар %d в отделе B\n", id, i + 1);
            {
                char t[128];
                sprintf(t, "OBS: Покупатель %d купил товар %d в отделе B\n", id, i + 1);
                snd(t);
            }
        }
        sleep(1);
    }
    printf("Покупатель %d завершил покупки и уходит из магазина\n", id);
    {
        char t[128];
        sprintf(t, "OBS: Покупатель %d завершил покупки\n", id);
        snd(t);
    }
    return 0;
}
