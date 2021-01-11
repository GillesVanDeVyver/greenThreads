#include <ucontext.h>

typedef struct green_t {
    struct ucontext_t *context;
    void *(*fun)(void*);
    void *arg;
    struct green_t *next;
    struct green_t *join;
    void *retval;
    int zombie;
} green_t;

typedef struct green_cond_t {
    struct green_list_t* suspList;
} green_cond_t;

typedef struct green_list_t {
    struct green_t** lst;
    int front;
    int rear;
    int size;
} green_list_t;

typedef struct green_mutex_t {
    struct green_list_t* suspList;
    volatile int taken;
} green_mutex_t;

int green_create(green_t *thread, void *(*fun)(void*), void *arg);
int green_yield();
int green_join(green_t *thread, void **res);

void green_cond_init(green_cond_t* cond);
void green_cond_wait(green_cond_t* cond);
void green_cond_signal(green_cond_t* cond);

int green_mutex_init (green_mutex_t *mutex);
int green_mutex_lock (green_mutex_t *mutex);
int green_mutex_unlock (green_mutex_t *mutex);
