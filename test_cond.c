/*
    Old test from before the conditon wait was changed.
*/
#include <stdio.h>

#include "green.h"

static green_cond_t cond;
static int flag = 0;

void *test(void *arg) {
    int id = *(int *) arg;
    int loop = 4;
    while (loop > 0) {
        if (flag == id) {
            printf("Thread %d: %d\n", id, loop);
            loop--;
            flag = (id + 1) % 3;
            green_cond_signal(&cond);
        } else {
            green_cond_wait(&cond, &cond_mutex);
        }
    }
}

/*
    Test the conditon part of the seminar
*/
int main () {
  green_t g0, g1, g2;
  int a0 = 0;
  int a1 = 1;
  int a2 = 2;

  green_cond_init(&cond);
  green_create(&g0, test_cond, &a0);
  green_create(&g1, test_cond, &a1);
  green_create(&g2, test_cond, &a2);
  green_join(&g0);
  green_join(&g1);
  green_join(&g2);
  printf("Test of condition is done.\n");

  return 0;
}
