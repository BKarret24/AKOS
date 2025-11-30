#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>

#define MAXB 32
#define MAXQ 64

typedef struct {
    sem_t qmut_a;
    sem_t qmut_b;
    sem_t n_a;
    sem_t n_b;
    sem_t bsem[MAXB];
    int qa[MAXQ];
    int qb[MAXQ];
    int qa_h;
    int qa_t;
    int qb_h;
    int qb_t;
} shm_t;

shm_t *shm_ptr;
pid_t main_pid;
pid_t sell_a_pid = -1;
pid_t sell_b_pid = -1;
pid_t b_pid[MAXB];
int b_cnt = 0;

void sig_h(int s) {
    if (getpid() == main_pid) {
        int i;
        if (sell_a_pid > 0) kill(sell_a_pid, SIGTERM);
        if (sell_b_pid > 0) kill(sell_b_pid, SIGTERM);
        for (i = 0; i < b_cnt; i++) {
            if (b_pid[i] > 0) kill(b_pid[i], SIGTERM);
        }
    }
    _exit(0);
}

void q_push_a(int id) {
    int t = shm_ptr->qa_t;
    shm_ptr->qa[t] = id;
    shm_ptr->qa_t = (t + 1) % MAXQ;
}

void q_push_b(int id) {
    int t = shm_ptr->qb_t;
    shm_ptr->qb[t] = id;
    shm_ptr->qb_t = (t + 1) % MAXQ;
}

int q_pop_a() {
    int h = shm_ptr->qa_h;
    int id = shm_ptr->qa[h];
    shm_ptr->qa_h = (h + 1) % MAXQ;
    return id;
}

int q_pop_b() {
    int h = shm_ptr->qb_h;
    int id = shm_ptr->qb[h];
    shm_ptr->qb_h = (h + 1) % MAXQ;
    return id;
}

void run_seller(int dep) {
    setbuf(stdout, NULL);
    srand(time(NULL) ^ getpid());
    if (dep == 0) {
        printf("Продавец A запущен, pid=%d\n", getpid());
    } else {
        printf("Продавец B запущен, pid=%d\n", getpid());
    }
    while (1) {
        if (dep == 0) {
            if (sem_wait(&shm_ptr->n_a) == -1) _exit(0);
            if (sem_wait(&shm_ptr->qmut_a) == -1) _exit(0);
            int id = q_pop_a();
            sem_post(&shm_ptr->qmut_a);
            printf("Продавец A обслуживает покупателя %d\n", id);
            sleep(1 + rand() % 3);
            printf("Продавец A завершил обслуживание покупателя %d\n", id);
            sem_post(&shm_ptr->bsem[id]);
        } else {
            if (sem_wait(&shm_ptr->n_b) == -1) _exit(0);
            if (sem_wait(&shm_ptr->qmut_b) == -1) _exit(0);
            int id = q_pop_b();
            sem_post(&shm_ptr->qmut_b);
            printf("Продавец B обслуживает покупателя %d\n", id);
            sleep(1 + rand() % 3);
            printf("Продавец B завершил обслуживание покупателя %d\n", id);
            sem_post(&shm_ptr->bsem[id]);
        }
    }
}

void run_buyer(int id) {
    setbuf(stdout, NULL);
    srand(time(NULL) ^ getpid());
    int k = 1 + rand() % 5;
    int lst[5];
    int i;
    for (i = 0; i < k; i++) {
        lst[i] = rand() % 2;
    }
    printf("Покупатель %d начал покупки, товаров в списке: %d\n", id, k);
    for (i = 0; i < k; i++) {
        int d = lst[i];
        if (d == 0) {
            printf("Покупатель %d идет в отдел A за товаром %d\n", id, i + 1);
            sem_wait(&shm_ptr->qmut_a);
            q_push_a(id);
            sem_post(&shm_ptr->qmut_a);
            printf("Покупатель %d встал в очередь в отдел A\n", id);
            sem_post(&shm_ptr->n_a);
            sem_wait(&shm_ptr->bsem[id]);
            printf("Покупатель %d купил товар %d в отделе A\n", id, i + 1);
        } else {
            printf("Покупатель %d идет в отдел B за товаром %d\n", id, i + 1);
            sem_wait(&shm_ptr->qmut_b);
            q_push_b(id);
            sem_post(&shm_ptr->qmut_b);
            printf("Покупатель %d встал в очередь в отдел B\n", id);
            sem_post(&shm_ptr->n_b);
            sem_wait(&shm_ptr->bsem[id]);
            printf("Покупатель %d купил товар %d в отделе B\n", id, i + 1);
        }
        sleep(1);
    }
    printf("Покупатель %d завершил покупки и уходит из магазина\n", id);
    _exit(0);
}

int main() {
    int n;
    int i;
    main_pid = getpid();
    setbuf(stdout, NULL);
    if (signal(SIGINT, sig_h) == SIG_ERR) {
        printf("Не удалось установить обработчик сигнала\n");
        return 1;
    }
    printf("Моделирование работы магазина (первая программа, 4-6 баллов)\n");
    printf("Введите число покупателей (не больше %d): ", MAXB);
    if (scanf("%d", &n) != 1) {
        printf("Ошибка ввода\n");
        return 1;
    }
    if (n <= 0 || n > MAXB) {
        printf("Некорректное число покупателей\n");
        return 1;
    }
    b_cnt = n;
    shm_ptr = mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shm_ptr == MAP_FAILED) {
        printf("Не удалось создать разделяемую память\n");
        return 1;
    }
    memset(shm_ptr, 0, sizeof(shm_t));
    if (sem_init(&shm_ptr->qmut_a, 1, 1) == -1 ||
        sem_init(&shm_ptr->qmut_b, 1, 1) == -1 ||
        sem_init(&shm_ptr->n_a, 1, 0) == -1 ||
        sem_init(&shm_ptr->n_b, 1, 0) == -1) {
        printf("Не удалось инициализировать семафоры\n");
        return 1;
    }
    for (i = 0; i < MAXB; i++) {
        if (sem_init(&shm_ptr->bsem[i], 1, 0) == -1) {
            printf("Не удалось инициализировать семафор покупателя\n");
            return 1;
        }
    }
    pid_t pid;
    pid = fork();
    if (pid == 0) {
        run_seller(0);
    } else if (pid > 0) {
        sell_a_pid = pid;
    } else {
        printf("Не удалось создать процесс продавца A\n");
        return 1;
    }
    pid = fork();
    if (pid == 0) {
        run_seller(1);
    } else if (pid > 0) {
        sell_b_pid = pid;
    } else {
        printf("Не удалось создать процесс продавца B\n");
        return 1;
    }
    for (i = 0; i < n; i++) {
        pid = fork();
        if (pid == 0) {
            run_buyer(i);
        } else if (pid > 0) {
            b_pid[i] = pid;
        } else {
            printf("Не удалось создать процесс покупателя\n");
            return 1;
        }
    }
    printf("Все процессы запущены. Магазин открыт.\n");
    for (i = 0; i < n; i++) {
        waitpid(b_pid[i], NULL, 0);
    }
    printf("Все покупатели завершили покупки. Магазин закрывается.\n");
    if (sell_a_pid > 0) kill(sell_a_pid, SIGTERM);
    if (sell_b_pid > 0) kill(sell_b_pid, SIGTERM);
    if (sell_a_pid > 0) waitpid(sell_a_pid, NULL, 0);
    if (sell_b_pid > 0) waitpid(sell_b_pid, NULL, 0);
    sem_destroy(&shm_ptr->qmut_a);
    sem_destroy(&shm_ptr->qmut_b);
    sem_destroy(&shm_ptr->n_a);
    sem_destroy(&shm_ptr->n_b);
    for (i = 0; i < MAXB; i++) {
        sem_destroy(&shm_ptr->bsem[i]);
    }
    munmap(shm_ptr, sizeof(shm_t));
    printf("Работа программы завершена\n");
    return 0;
}

