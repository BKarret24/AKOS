#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>

#define NOBS 4

int fd_log;

void hdl(int s) {
    printf("disp10: завершение\n");
    close(fd_log);
    exit(0);
}

int main() {
    signal(SIGINT, hdl);
    signal(SIGTERM, hdl);
    setbuf(stdout, NULL);

    fd_log = open("/tmp/shop_log", O_RDONLY);
    if (fd_log < 0) {
        printf("disp10: ошибка открытия /tmp/shop_log\n");
        return 1;
    }

    printf("disp10: запущен\n");

    char buf[256];
    while (1) {
        int r = read(fd_log, buf, sizeof(buf) - 1);
        if (r > 0) {
            buf[r] = 0;
            for (int i = 0; i < NOBS; i++) {
                char fn[32];
                sprintf(fn, "/tmp/obs%d", i);
                int f = open(fn, O_WRONLY | O_NONBLOCK);
                if (f >= 0) {
                    write(f, buf, strlen(buf));
                    close(f);
                }
            }
        } else {
            usleep(10000);
        }
    }
}

