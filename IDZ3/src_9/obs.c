#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

int fd;
char buf[256];

void hdl(int s) {
    printf("obs: завершение\n");
    close(fd);
    exit(0);
}

int main() {
    signal(SIGINT, hdl);
    signal(SIGTERM, hdl);
    setbuf(stdout, NULL);

    fd = open("/tmp/shop_log", O_RDONLY);
    if (fd < 0) {
        printf("obs: ошибка открытия канала\n");
        return 1;
    }

    printf("obs: наблюдатель запущен\n");

    while (1) {
        int r = read(fd, buf, sizeof(buf)-1);
        if (r > 0) {
            buf[r] = 0;
            printf("%s", buf);
        } else {
            usleep(10000);
        }
    }
}

