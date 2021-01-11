#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include "green.h"
#include <unistd.h>

#define FALSE 0
#define TRUE 1

#define STACK_SIZE 4096
#define LIMIT 100 // Specifying the maximum limit of the queue
#define PERIOD 100


static sigset_t block;

void timer_handler(int);
 
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

    sigemptyset(&block);
    sigaddset(&block, SIGVTALRM);

    struct sigaction act = {0};
    struct timeval interval;
    struct itimerval period;

    act.sa_handler = timer_handler;
    assert(sigaction(SIGVTALRM, &act, NULL) == 0);

    interval.tv_sec = 0;
    interval.tv_usec = PERIOD;
    period.it_interval = interval;
    period.it_value = interval;
    setitimer(ITIMER_VIRTUAL, &period, NULL);

}



void append(green_t *elememt, green_list_t* list){
    // sigprocmask(SIG_BLOCK, &block, NULL);
    if (list->rear == LIMIT - 1)
        list->rear = -1;
    
    if (list->front == - 1)
        list->front = 0;

    list->rear++;
    list->lst[list->rear] = elememt;
    // sigprocmask(SIG_UNBLOCK, &block, NULL);
}


green_t *getFirst(green_list_t* list){
    // sigprocmask(SIG_BLOCK, &block, NULL);
    // printf("gfront %d \n", list->front);
    // printf("grear %d \n", list->rear);
    // int loop;
    // for(loop = 0; loop < 15; loop++){
    //     // printf("loop %d \n", loop);
    //     printf("%p \n", list->lst[loop]);
    // }

    if (list->front == - 1){
        // printf("front == -1 \n");
        // printf("efront %d \n", list->front);
        // printf("erear %d \n", list->rear);
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
    // sigprocmask(SIG_UNBLOCK, &block, NULL);
}



void green_thread() {
    // sigprocmask(SIG_BLOCK, &block, NULL);

    sigprocmask(SIG_BLOCK, &block, NULL);
    green_t *this = running;
    sigprocmask(SIG_UNBLOCK, &block, NULL);

    void *result = (*this->fun)(this->arg);

    sigprocmask(SIG_BLOCK, &block, NULL);


    // Place waiting (joining) thread in ready queue
    append(this->join, &queue);
    // sigprocmask(SIG_BLOCK, &block, NULL);
    // save result of exection
    this->retval = result;

    // we're a zombie
    this->zombie = TRUE;

    // find the next thread to run
    green_t* next = getFirst(&queue);
    // sigprocmask(SIG_BLOCK, &block, NULL);
    running = next;
    // sigprocmask(SIG_UNBLOCK, &block, NULL);
    setcontext(next->context);
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    
}

int green_yield() {
    // sigprocmask(SIG_BLOCK, &block, NULL);
    green_t *susp = running;
    // add susp to ready queue
    append(susp,&queue);
    // sigprocmask(SIG_BLOCK, &block, NULL);
    // select the next thread for execution
    green_t* next = getFirst(&queue);
    // sigprocmask(SIG_BLOCK, &block, NULL);
    running = next;
    // sigprocmask(SIG_UNBLOCK, &block, NULL);
    swapcontext(susp->context, next->context);
    return 0;
}

int green_join(green_t *thread, void **res) {
    sigprocmask(SIG_UNBLOCK, &block, NULL);

    if (!thread->zombie){
        sigprocmask(SIG_UNBLOCK, &block, NULL);
        green_t *susp = running;
        // add as joining thread>
        thread->join = susp;

        // select the next thread for execution
        green_t* next = getFirst(&queue);
        // sigprocmask(SIG_BLOCK, &block, NULL);
        running = next;
        swapcontext(susp->context, next->context);
        sigprocmask(SIG_UNBLOCK, &block, NULL);

    }

    // sigprocmask(SIG_BLOCK, &block, NULL);


    // collect result


    res = (thread->retval);

    // free context

    
    free(thread->context);
    // sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}


int green_create(green_t *new, void *(*fun)(void*), void *arg) {
    // sigprocmask(SIG_BLOCK, &block, NULL);

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
    // sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}


void green_cond_init(green_cond_t* cond){
    

    green_t *arrlist = (green_t *)malloc(LIMIT*sizeof(green_t *));


    struct green_list_t* newSuspList = (green_list_t *)malloc(sizeof(green_list_t));
    
    
    cond->suspList = newSuspList;

    newSuspList->lst = (green_t **)malloc(LIMIT*sizeof(green_t *));
    newSuspList->front = -1;
    newSuspList->rear = -1;
    newSuspList->size = LIMIT;
}

void green_cond_wait(green_cond_t* cond){
    // sigprocmask(SIG_BLOCK, &block, NULL);
    struct green_list_t* currSuspList = cond->suspList;
    
    green_t *susp = running;
    // add susp to suspList queue

    append(susp,currSuspList);
    // sigprocmask(SIG_BLOCK, &block, NULL);
    // select the next thread for execution
    green_t* next = getFirst(&queue);
    // sigprocmask(SIG_BLOCK, &block, NULL);

    running = next;
    // sigprocmask(SIG_UNBLOCK, &block, NULL);
    swapcontext(susp->context, next->context);

}

void green_cond_signal(green_cond_t* cond){
    // sigprocmask(SIG_BLOCK, &block, NULL);
    if (cond->suspList!=NULL){
        green_t* woken = getFirst(cond->suspList);
        // sigprocmask(SIG_BLOCK, &block, NULL);
        if (woken!=NULL){
            append(woken,&queue);
            // sigprocmask(SIG_BLOCK, &block, NULL);
        }
    }
    // sigprocmask(SIG_BLOCK, &block, NULL);
}

void timer_handler (int sig) {
    // printf("timer handler \n"); => crashes
    write(1, "timer handler\n",14);
    sigprocmask(SIG_BLOCK, &block, NULL);
    green_t *susp = running;

    //Add the running to the ready queue
    append(susp,&queue);
    
    //Find the next thread for execution
    green_t* next = getFirst(&queue);
    running = next;
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    swapcontext(susp->context, next->context);
}

int green_mutex_init(green_mutex_t *mutex) {
    mutex->taken = FALSE;

    green_t *arrlist = (green_t *)malloc(LIMIT*sizeof(green_t *));


    struct green_list_t* newSuspList = (green_list_t *)malloc(sizeof(green_list_t));


    mutex->suspList = newSuspList;

    newSuspList->lst = (green_t **)malloc(LIMIT*sizeof(green_t *));
    newSuspList->front = -1;
    newSuspList->rear = -1;
    newSuspList->size = LIMIT;
}


int green_mutex_lock (green_mutex_t *mutex) {
  //Block timer interrupt
  sigprocmask(SIG_BLOCK, &block, NULL);

  green_t *susp = running;

  if (mutex->taken) {
    // suspend the running thread
    green_t *susp = running;
    struct green_list_t* currSuspList = mutex->suspList;

    // find the next thread

    green_t* next = getFirst(&queue);
    running = next;
    swapcontext(susp->context, next->context);
  }
  else{
    // take the lock
    mutex->taken = TRUE;;
  }
  // unblock
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

int green_mutex_unlock (green_mutex_t *mutex) {
    //Block timer interrupt
    sigprocmask(SIG_BLOCK, &block, NULL);
    
    
        // move suspended threads to ready queue
        struct green_list_t* currSuspList = mutex->suspList;
        green_t* next = getFirst(currSuspList);
    if (next != NULL) {
        append(next, &queue);
    } else{
        // release lock
        mutex->taken = FALSE;
    }

    //unblock
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}