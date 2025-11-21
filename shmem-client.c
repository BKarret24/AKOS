#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#define SHARED_OBJ_NAME "random-numbers-shmem"
#define MAX_RANDOM 1000

typedef struct {
    int number;
    int client_active;
    int server_active;
} shared_data_t;

shared_data_t *shared_data = NULL;
int shm_fd = -1;
volatile sig_atomic_t client_running = 1;

void client_sigint_handler(int sig) {
    printf("\nКлиент: Получен сигнал завершения\n");
    client_running = 0;
}

void cleanup() {
    if (shared_data != NULL) {
        shared_data->client_active = 0;
        munmap(shared_data, sizeof(shared_data_t));
    }
    if (shm_fd != -1) {
        close(shm_fd);
    }
    printf("Клиент: Завершение работы\n");
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = client_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Ошибка настройки обработчика сигнала");
        exit(1);
    }
}

int main() {
    printf("Клиент: Запуск генератора случайных чисел\n");
    
    setup_signal_handler();
    atexit(cleanup);
    
    shm_fd = shm_open(SHARED_OBJ_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Клиент: Ошибка создания разделяемой памяти");
        exit(1);
    }
    
    if (ftruncate(shm_fd, sizeof(shared_data_t)) == -1) {
        perror("Клиент: Ошибка установки размера памяти");
        exit(1);
    }
    
    shared_data = mmap(NULL, sizeof(shared_data_t), 
                      PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("Клиент: Ошибка отображения памяти");
        exit(1);
    }
    
    shared_data->client_active = 1;
    shared_data->server_active = 0;
    shared_data->number = 0;
    
    printf("Клиент: Начало генерации чисел (0-%d)\n", MAX_RANDOM-1);
    printf("Клиент: Для завершения нажмите Ctrl+C\n");
    
    srand(time(NULL));
    
    while (client_running) {
        int random_num = rand() % MAX_RANDOM;
        shared_data->number = random_num;
        printf("Клиент: Сгенерировано число %d\n", random_num);
        sleep(1);
    }
    
    return 0;
}