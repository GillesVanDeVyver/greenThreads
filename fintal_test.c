#include <stdio.h>

#include "green.h"

#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

volatile int shared_counter = 0;
green_mutex_t mutex;
int zero, one, two;
void *mutex_test(void *arg) {
//   int id = *(int *) arg;
//   int incr = 100000;

//   // for(int i = 0; i < 100000; i++);
//   for (int i = 0; i < incr; i++) {
//     green_mutex_lock(&mutex);
//     printf("thread %d: %d\n", i, loop);
//     if (id == 0) {
//       zero++;
//     } else if (id == 1) {
//       one++;
//     } else {
//       two++;
//     }
//     shared_counter++;
//     green_mutex_unlock(&mutex);
//   }
// }

// void *test(void *arg) {
    int i = *(int*) arg;
    int loop = 10000;
    while (loop > 0) {
        for (int i = 0; i < 100000; i++);
        green_mutex_lock(&mutex);
        printf("thread %d: %d\n", i, loop);
        loop--;
        shared_counter++;
        green_mutex_unlock(&mutex);
    }
}

int main() {
green_t g0, g1, g2;
int a0 = 0;
int a1 = 1;
int a2 = 2;

green_mutex_init(&mutex);
green_create(&g0, mutex_test, &a0);
green_create(&g1, mutex_test, &a1);
// green_create(&g2, mutex_test, &a2);

green_join(&g0,NULL);
green_join(&g1,NULL);
// green_join(&g2,NULL);
printf("Zero: %d | One: %d | Two: %d\n", zero, one, two);
printf("Shared counter result: %d\n", shared_counter);
printf("Mutex test is done.\n");

return 0;
}
