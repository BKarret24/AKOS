#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdatomic.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

typedef enum { A = 0, B = 1 } dep_t;

atomic_int stop = 0;

void h(int s) { atomic_store(&stop, 1); }

pthread_mutex_t lm = PTHREAD_MUTEX_INITIALIZER;
FILE *lf = NULL;

void pr(const char *fmt, ...) {
    va_list ap;
    pthread_mutex_lock(&lm);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    fflush(stdout);
    va_end(ap);
    if (lf) {
        va_start(ap, fmt);
        vfprintf(lf, fmt, ap);
        fflush(lf);
        va_end(ap);
    }
    pthread_mutex_unlock(&lm);
}

typedef struct cus {
    int id;
    int k;
    dep_t *r;
    int p;
    pthread_mutex_t m;
    pthread_cond_t cv;
    int done;
    unsigned int rnd;
    int ok;
} cus_t;

typedef struct nd {
    cus_t *c;
    struct nd *n;
} nd_t;

typedef struct dep {
    char name;
    pthread_mutex_t m;
    pthread_cond_t cv;
    nd_t *h;
    nd_t *t;
    int cnt;
} dep2_t;

dep2_t d1, d2;

void push(dep2_t *d, cus_t *c) {
    nd_t *x = (nd_t *)malloc(sizeof(nd_t));
    x->c = c;
    x->n = NULL;
    pthread_mutex_lock(&d->m);
    if (!d->t) d->h = d->t = x;
    else { d->t->n = x; d->t = x; }
    pthread_cond_signal(&d->cv);
    pthread_mutex_unlock(&d->m);
}

cus_t *pop(dep2_t *d) {
    nd_t *x = d->h;
    if (!x) return NULL;
    d->h = x->n;
    if (!d->h) d->t = NULL;
    cus_t *c = x->c;
    free(x);
    return c;
}

void *sell(void *a) {
    dep2_t *d = (dep2_t *)a;
    pr("Продавец %c начал\n", d->name);
    while (1) {
        pthread_mutex_lock(&d->m);
        while (!d->h && !atomic_load(&stop)) pthread_cond_wait(&d->cv, &d->m);
        if (!d->h && atomic_load(&stop)) { pthread_mutex_unlock(&d->m); break; }
        cus_t *c = pop(d);
        pthread_mutex_unlock(&d->m);

        if (!c) continue;

        pr("Продавец %c обслуживает %d\n", d->name, c->id);
        usleep(120000 + (useconds_t)(rand_r(&c->rnd) % 220000u));
        d->cnt++;

        pthread_mutex_lock(&c->m);
        c->done = 1;
        pthread_cond_signal(&c->cv);
        pthread_mutex_unlock(&c->m);
    }
    pr("Продавец %c ушел\n", d->name);
    return NULL;
}

void *buy(void *a) {
    cus_t *c = (cus_t *)a;
    pr("Покупатель %d пришел\n", c->id);
    while (c->p < c->k && !atomic_load(&stop)) {
        dep2_t *d = (c->r[c->p] == A) ? &d1 : &d2;

        pthread_mutex_lock(&c->m);
        c->done = 0;
        pthread_mutex_unlock(&c->m);

        pr("Покупатель %d очередь %c\n", c->id, d->name);
        push(d, c);
        if (atomic_load(&stop)) break;

        pthread_mutex_lock(&c->m);
        while (!c->done && !atomic_load(&stop)) pthread_cond_wait(&c->cv, &c->m);
        pthread_mutex_unlock(&c->m);

        if (atomic_load(&stop)) break;

        c->p++;
        usleep(50000 + (useconds_t)(rand_r(&c->rnd) % 110000u));
    }
    if (c->p == c->k && !atomic_load(&stop)) {
        c->ok = 1;
        pr("Покупатель %d купил\n", c->id);
    } else {
        pr("Покупатель %d ушел\n", c->id);
    }
    return NULL;
}

int main(int argc, char **argv) {
    signal(SIGINT, h);

    int n = 10, day = 10, seed = (int)time(NULL);
    const char *out = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--buyers") && i + 1 < argc) n = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--day") && i + 1 < argc) day = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--seed") && i + 1 < argc) seed = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) out = argv[++i];
    }

    if (out) lf = fopen(out, "w");

    srand((unsigned)seed);

    d1.name = 'A'; d2.name = 'B';
    pthread_mutex_init(&d1.m, NULL);
    pthread_mutex_init(&d2.m, NULL);
    pthread_cond_init(&d1.cv, NULL);
    pthread_cond_init(&d2.cv, NULL);
    d1.h = d1.t = NULL; d2.h = d2.t = NULL;
    d1.cnt = d2.cnt = 0;

    pr("Старт: покупателей=%d день=%d seed=%d\n", n, day, seed);

    pthread_t s1, s2;
    pthread_create(&s1, NULL, sell, &d1);
    pthread_create(&s2, NULL, sell, &d2);

    cus_t *cs = (cus_t *)calloc((size_t)n, sizeof(cus_t));
    pthread_t *ts = (pthread_t *)calloc((size_t)n, sizeof(pthread_t));

    for (int i = 0; i < n; i++) {
        cs[i].id = i + 1;
        cs[i].k = 1 + rand() % 5;
        cs[i].r = (dep_t *)malloc((size_t)cs[i].k * sizeof(dep_t));
        for (int j = 0; j < cs[i].k; j++) cs[i].r[j] = (dep_t)(rand() % 2);
        cs[i].p = 0;
        cs[i].ok = 0;
        cs[i].rnd = (unsigned int)(seed + i * 101);

        pthread_mutex_init(&cs[i].m, NULL);
        pthread_cond_init(&cs[i].cv, NULL);
        cs[i].done = 0;

        pthread_create(&ts[i], NULL, buy, &cs[i]);
        usleep(30000);
    }

    for (int t = 0; t < day && !atomic_load(&stop); t++) sleep(1);
    atomic_store(&stop, 1);
    pr("Закрытие\n");

    pthread_mutex_lock(&d1.m); pthread_cond_broadcast(&d1.cv); pthread_mutex_unlock(&d1.m);
    pthread_mutex_lock(&d2.m); pthread_cond_broadcast(&d2.cv); pthread_mutex_unlock(&d2.m);

    for (int i = 0; i < n; i++) {
        pthread_mutex_lock(&cs[i].m);
        pthread_cond_broadcast(&cs[i].cv);
        pthread_mutex_unlock(&cs[i].m);
    }

    for (int i = 0; i < n; i++) pthread_join(ts[i], NULL);
    pthread_join(s1, NULL);
    pthread_join(s2, NULL);

    int ok = 0;
    for (int i = 0; i < n; i++) ok += cs[i].ok;

    pr("Итого A: %d\n", d1.cnt);
    pr("Итого B: %d\n", d2.cnt);
    pr("Завершили: %d из %d\n", ok, n);

    for (int i = 0; i < n; i++) {
        pthread_mutex_destroy(&cs[i].m);
        pthread_cond_destroy(&cs[i].cv);
        free(cs[i].r);
    }
    free(cs);
    free(ts);

    pthread_cond_destroy(&d1.cv);
    pthread_cond_destroy(&d2.cv);
    pthread_mutex_destroy(&d1.m);
    pthread_mutex_destroy(&d2.m);

    if (lf) fclose(lf);
    return 0;
}
