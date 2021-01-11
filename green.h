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
    struct green_t* lst;
    int front;
    int rear;
    int size;
} green_list_t;

int green_create(green_t *thread, void *(*fun)(void*), void *arg);
int green_yield();
int green_join(green_t *thread, void **res);
