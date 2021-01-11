#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include "green.h"

#define FALSE 0
#define TRUE 1

#define STACK_SIZE 4096
#define LIMIT 10 // Specifying the maximum limit of the queue


static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL,NULL, FALSE};
static green_t *running = &main_green;

green_t* queueList[LIMIT];
green_list_t queue = {queueList,-1,-1,LIMIT};

// green_t* queue[LIMIT]; // Array implementation of queue
// int front, rear; // To append and getFirst the data elements in the queue respectively

static void init() __attribute__((constructor));

void init() {
  getcontext(&main_cntx);

//   front = rear = -1;

}


void append(green_t *elememt, green_list_t* list){
    // printf("afront %d \n", front);
    // printf("arear %d \n", rear);
    // int loop;
    // for(loop = 0; loop < 10; loop++){
    //     // printf("loop %d \n", loop);
    //     printf("%p \n", queue[loop]);
    // }
    if (list->rear == LIMIT - 1)
        list->rear = -1;
    
    if (list->front == - 1)
        list->front = 0;

    list->rear++;
    list->lst[list->rear] = elememt;
}


green_t *getFirst(green_list_t* list){
    // printf("gfront %d \n", front);
    // printf("grear %d \n", rear);
    // int loop;
    // for(loop = 0; loop < 10; loop++){
    //     // printf("loop %d \n", loop);
    //     printf("%p \n", queue[loop]);
    // }

    if (list->front == - 1){
        printf("front == -1 \n");
        printf("efront %d \n", list->front);
        printf("erear %d \n", list->rear);
        return NULL;
    }
    else{
        green_t* first = list->lst[list->front];
        list->lst[list->front] = NULL;
        if (list->front == LIMIT - 1)
            list->front = -1;
        list->front++;
        // printf("%p first\n", first);
        return first;
    }
}



void green_thread() {
    green_t *this = running;


    void *result = (*this->fun)(this->arg);

    // Place waiting (joining) thread in ready queue
   append(this->join, &queue);

    // save result of exection
    this->retval = result;

    // we're a zombie
    this->zombie = TRUE;

    // find the next thread to run
    green_t* next = getFirst(&queue);

    running = next;
    setcontext(next->context);
}

int green_yield() {
    // printf("front %d \n", front);
    // int loop;
    // for(loop = 0; loop < 10; loop++)
    //     printf("%p \n", queue[loop]);
    green_t *susp = running;
    // add susp to ready queue
    append(susp,&queue);

    // select the next thread for execution
    green_t* next = getFirst(&queue);

    running = next;
    swapcontext(susp->context, next->context);
    return 0;
}

int green_join(green_t *thread, void **res) {

    if (!thread->zombie){
        green_t *susp = running;
        // add as joining thread>
        thread->join = susp;

        // select the next thread for execution
        green_t* next = getFirst(&queue);
        running = next;

        swapcontext(susp->context, next->context);

    }




    // collect result


    res = (thread->retval);

    // free context


    free(thread->context);
    return 0;
}


int green_create(green_t *new, void *(*fun)(void*), void *arg) {

    ucontext_t *cntx = (ucontext_t *) malloc(sizeof(ucontext_t));
    getcontext(cntx);

    void *stack = malloc(STACK_SIZE);
    cntx->uc_stack.ss_sp = stack;
    cntx->uc_stack.ss_size = STACK_SIZE;
    makecontext(cntx, green_thread, 0);

    new->context = cntx;
    new->fun = fun;
    new->arg = arg;
    new->next = NULL;
    new->join = NULL;
    new->retval = NULL;
    new->zombie = FALSE;

    // add new to the ready queue
    append(new,&queue);
    return 0;
}



// void green_cond_init(green_cond_t* cond){
//     green_t* arrlist[LIMIT];
//     cond->suspList = green_t* arrlist[LIMIT];
// }