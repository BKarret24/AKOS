#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>

int fd;
char fn[32];

void hdl(int s) {
    printf("obs10: завершение\n");
    close(fd);
    exit(0);
}

int main(int ac, char **av) {
    if (ac < 2) {
        printf("Использование: obs10 <id>\n");
        return 1;
    }

    int id = atoi(av[1]);
    if (id < 0) {
        printf("Некорректный id\n");
        return 1;
    }

    signal(SIGINT, hdl);
    signal(SIGTERM, hdl);
    setbuf(stdout, NULL);

    sprintf(fn, "/tmp/obs%d", id);
    unlink(fn);
    mkfifo(fn, 0666);

    fd = open(fn, O_RDONLY);
    if (fd < 0) {
        printf("obs10: ошибка открытия канала\n");
        return 1;
    }

    printf("obs10 %d: запущен\n", id);

    char buf[256];
    while (1) {
        int r = read(fd, buf, sizeof(buf) - 1);
        if (r > 0) {
            buf[r] = 0;
            printf("%s", buf);
        } else {
            usleep(10000);
        }
    }
}

