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
volatile sig_atomic_t server_running = 1;

void server_sigint_handler(int sig) {
    printf("\nСервер: Получен сигнал завершения\n");
    server_running = 0;
}

void cleanup() {
    if (shared_data != NULL) {
        shared_data->server_active = 0;
        munmap(shared_data, sizeof(shared_data_t));
    }
    if (shm_fd != -1) {
        close(shm_fd);
        shm_unlink(SHARED_OBJ_NAME);
        printf("Сервер: Разделяемая память удалена\n");
    }
    printf("Сервер: Завершение работы\n");
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = server_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Ошибка настройки обработчика сигнала");
        exit(1);
    }
}

int main() {
    printf("Сервер: Запуск чтения случайных чисел\n");
    
    setup_signal_handler();
    atexit(cleanup);
    
    shm_fd = shm_open(SHARED_OBJ_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Сервер: Ошибка открытия разделяемой памяти");
        exit(1);
    }
    
    shared_data = mmap(NULL, sizeof(shared_data_t), 
                      PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("Сервер: Ошибка отображения памяти");
        exit(1);
    }
    
    shared_data->server_active = 1;
    
    printf("Сервер: Ожидание чисел от клиента\n");
    printf("Сервер: Для завершения нажмите Ctrl+C\n");
    
    int last_number = -1;
    
    while (server_running) {
        if (shared_data->number != last_number) {
            last_number = shared_data->number;
            printf("Сервер: Получено число %d\n", last_number);
        }
        
        if (!shared_data->client_active) {
            printf("Сервер: Клиент завершил работу\n");
            break;
        }
        
        usleep(100000);
    }
    
    return 0;
}