#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>
#include "green.h"
#include <stdlib.h> 


volatile int shared_counter = 0;

void *test(void *arg) {
    int i = *(int*) arg;
    int loop = 1000000;
    while (loop > 0) {
      // for (int i = 0; i < 100000000; i++);
      printf("thread %d: %d\n", i, loop);
      loop--;
      shared_counter++;
      // green_yield();
    }
}

int main() {

  green_t g0, g1;
  int a0 = 0;
  int a1 = 1;

  green_create(&g0, test, &a0);
  green_create(&g1, test, &a1);


  green_join(&g0, NULL);
  green_join(&g1, NULL);
    printf("Shared counter result: %d\n", shared_counter);

  printf("done\n");
  return 0;
}
