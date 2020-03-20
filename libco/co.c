#include "co.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <setjmp.h>
#include <string.h>
#include <time.h>

#define STACK_SIZE (1<<10)

#define DEBUG

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {

  asm volatile (
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
      : : "b"((uintptr_t)sp),     "d"(entry), "a"(arg)
#else
    "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
      : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg)
#endif
  );

}

/*
#if __x86_64__

#else
*/

enum co_status {

    CO_NEW = 1,
	CO_RUNNING,
	CO_WAITING,
	CO_DEAD,

};

struct co {

	char *name;
	void (*func)(void *);
	void *arg;

	enum co_status status;
	struct co *    waiter;
	struct co *    next; // to connect members in list
	struct co *    brother; // to connect members in rand_pool
	jmp_buf        context;
	uint8_t        stack[STACK_SIZE];

};

struct co *current = NULL;

struct co *co_list = NULL;

struct co* rand_pool = NULL;

void list_append(struct co* head, struct co* new_co) {
	if(head == NULL) {
	    head = new_co;
#ifdef DEBUG
    printf("In list_append function: case head == NULL!\n");
#endif
	}
    else {
    	struct co* temp = head;
		while(temp->next != NULL) {
	    	temp = temp->next;
		}
		temp->next = new_co;
#ifdef DEBUG
    printf("In list_append function: case head != NULL!\n");
#endif
	}
}

void rand_pool_append(struct co* head, struct co* new_co) {
	if(head == NULL) {
	    head = new_co;
	}
    else {
    	struct co* temp = head;
		while(temp->brother != NULL) {
	    	temp = temp->brother;
		}
		temp->brother = new_co;
	}
}

void waiter_append(struct co* prev, struct co* current) {
	assert(prev->waiter == NULL);
	prev->waiter = current;
	assert(prev->waiter != NULL);
}

void rand_choose(struct co* head, struct co* candidate) {

	assert(head != NULL);

    int count = 0;

    struct co* temp = head;
    while(temp != NULL) {
        if(temp->status == CO_NEW || temp->status == CO_WAITING) {
	        rand_pool_append(rand_pool, temp);
		    count ++;
	    }
		temp = temp->next;
    }
	assert(rand_pool != NULL);

	int index = 0;
	srand((unsigned)time(0));
	if(count != 0) {
        index = rand() % count;
	}
	struct co* pool = rand_pool;
	for(int i=0; i < index; i ++) {
	    pool = pool->brother;
	}
	candidate = pool;

	assert(candidate != NULL);
#ifdef DEBUG
	printf("co %s was chosen to run!\n", candidate->name);
#endif
	rand_pool = NULL;
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {

	assert(name != NULL && func != NULL && arg != NULL);
	struct co *new_co = (struct co*)malloc(sizeof(struct co));
	new_co->name = (char*)malloc(32*sizeof(char));
	assert(new_co->name != NULL);
	strcpy(new_co->name, name);
	new_co->func = func;
	new_co->arg = arg;
	new_co->status = CO_NEW;
	new_co->next = NULL;
	new_co->brother = NULL;
	for(int i = 0; i < STACK_SIZE; i ++) {
	    new_co->stack[i] = 0;
	}
    
	list_append(co_list, new_co);
    assert(co_list != NULL);

#ifdef DEBUG
	printf("co %s is created!\n", new_co->name);
#endif 

    return new_co;
}

void co_wait(struct co *co) {

	if(current == NULL) {
		co->status = CO_RUNNING;
        current = co;	
		assert(current != NULL);

#ifdef DEBUG
	printf("main thread was waiting | co %s is running now!\n", current->name);
#endif 

	    current->func(current->arg);
	    current->status = CO_DEAD;
	    current = NULL;

#ifdef DEBUG
	printf("main thread was restored | co %s is finished Now!!\n", co->name);
#endif 

	free(co);
	}
	else {
	    current->status = CO_WAITING;
	    struct co *old_current = current;
	    co->status = CO_RUNNING;
		waiter_append(co, current);
	    current = co;

#ifdef DEBUG
	printf("co %s was replaced | co %s is runing Now!\n", old_current->name, current->name);
#endif 

	    current->func(current->arg);
	    current->status = CO_DEAD;
	    current = old_current;
		current->status = CO_RUNNING;

#ifdef DEBUG
	printf("co %s was restored | co %s is finished Now!!\n", current->name, co->name);
#endif 

	    free(co);
	}
}

void co_yield() {
    
	if(current == NULL) {

#ifdef DEBUG
		printf("Exit from main thread directly!\n");
#endif

	    exit(0);
	}
	else {
	    current->status = CO_WAITING;

#ifdef DEBUG
		printf("yield occured in co %s!\n", current->name);
#endif

        int val = setjmp(current->context);
        if (val == 0) {
            struct co* new_co = NULL;
			rand_choose(co_list, new_co);
			assert(new_co->status == CO_NEW || new_co->status == CO_WAITING);
			
			if (new_co->status == CO_NEW) {
			    stack_switch_call(&new_co->status, new_co->func, (uintptr_t)new_co->arg);
			}
			else {
			   longjmp(new_co->context, 2); 
			}
            
	    }
        else {
			current->status = CO_RUNNING;
            return;	
	    }	
	}
}
