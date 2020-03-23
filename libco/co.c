#include "co.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <setjmp.h>
#include <string.h>
#include <time.h>

#define STACK_SIZE (1<<16)

//#define DEBUG
#define TEST

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
#ifdef DEBUG
	printf("In function stack_switch_call!\n");
#endif
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

enum co_status {

    CO_NEW = 1,
	CO_RUNNING,
	CO_WAITING,
	CO_DEAD,

};

struct co {

	char *name;
	__attribute__ ((aligned (16))) void (*func)(void *);
	void *arg;

	enum co_status status;
	struct co *    waiter;
	struct co *    next; // to connect members in list
	struct co *    brother; // to connect members in rand_pool
	jmp_buf        context;
	uint8_t        stack[STACK_SIZE];

}__attribute__ ((aligned (16)));;

struct co *current = NULL;

struct co co_list;
struct co *co_list_head = &co_list;

struct co rand_pool;
struct co *rand_pool_head = &rand_pool;

void list_append(struct co* head, struct co* new_co) {
    struct co* temp = head;
	while(temp->next != NULL) {
	   	temp = temp->next;
	}
	temp->next = new_co;
#ifdef TEST
    printf("In list_append function! ");
#endif
}

void rand_pool_append(struct co* head, struct co* new_co) {
   	struct co* temp = head;
	while(temp->brother != NULL) {
    	temp = temp->brother;
	}
	temp->brother = new_co;
#ifdef TEST
    printf("In rand_pool_append function! co %s was appended\n", temp->brother->name);
#endif
}

void waiter_append(struct co* prev, struct co* current) {
	assert(prev->waiter == NULL);
	prev->waiter = current;
	assert(prev->waiter != NULL);
#ifdef TEST
    printf("In waiter_append function!\n");
#endif
}

void rand_choose(struct co* head, struct co* candidate, struct co* current) {

	assert(head != NULL);

    int count = 0;

    struct co* temp = head;
    while(temp != NULL) {
        if(temp->status == CO_NEW || temp->status == CO_WAITING) {
	        rand_pool_append(rand_pool_head, temp);
		    count ++;
	    }
		temp = temp->next;
    }
	assert(rand_pool_head != NULL);

#ifdef TEST
	printf("There %d co in rand pool!\n", count);
#endif

	int index = 0;
	srand((unsigned)time(0));
	if(count != 0) {
       	index = rand() % count + 1;
	}
	struct co* pool = rand_pool_head;
	for(int i=0; i < index; i ++) {
	   	pool = pool->brother;
	}
	candidate->brother = pool;

	if(!strcmp(candidate->brother->name, current->name)) {
	    if(count == 2) {
		    index = count + 1 - index;
	        pool = rand_pool_head;
	        for(int i=0; i < index; i ++) {
	   	        pool = pool->brother;
	        }
	        candidate->brother = pool;
		}

		else {
		    index = (index + 1) % count + 1;
	        pool = rand_pool_head;
	        for(int i=0; i < index; i ++) {
	   	        pool = pool->brother;
	        }
	        candidate->brother = pool;
		}
	}

	assert(candidate->brother != NULL);
#ifdef TEST
	printf("co %s was chosen to run!\n", candidate->brother->name);
#endif
    temp = rand_pool_head->brother;
	rand_pool_head->brother = NULL;
	while(temp->brother != NULL) {
	    struct co* old = temp;
		temp = temp->brother;
		old->brother = NULL;
	}
#ifdef TEST
	printf("rand pool was cleared!\n");
#endif
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
	/*
	for(int i = 0; i < STACK_SIZE; i ++) {
	    new_co->stack[i] = 0;
	}
	*/
    
	list_append(co_list_head, new_co);
    assert(co_list_head != NULL);

#ifdef TEST
	printf("co %s is created!\n", new_co->name);
#endif 

    return new_co;
}

void co_wait(struct co *co) {

	if(current == NULL) {
		co->status = CO_RUNNING;
        current = co;	
		assert(current != NULL);

#ifdef TEST
	printf("main thread was waiting | co %s is running now!\n", current->name);
#endif 

	    current->func(current->arg);
	    current->status = CO_DEAD;
	    current = NULL;

#ifdef TEST
	printf("main thread was restored | co %s is finished Now!!\n", co->name);
#endif 

	assert(co != NULL);
#ifdef TEST
	printf("co %s was to be free!\n", co->name);
#endif
#ifdef TEST
	printf("A pointer free happened in if clause whose condition is current == NULL | current co is main\n");
	printf("co->%p\n", co);
#endif
	free(co);
	}
	else {
	    current->status = CO_WAITING;
	    struct co *old_current = current;
	    co->status = CO_RUNNING;
		waiter_append(co, current);
	    current = co;

#ifdef TEST
	printf("co %s was replaced | co %s is runing Now!\n", old_current->name, current->name);
#endif 

	    current->func(current->arg);
	    current->status = CO_DEAD;
	    current = old_current;
		current->status = CO_RUNNING;

#ifdef TEST
	printf("co %s was restored | co %s is finished Now!!\n", current->name, co->name);
#endif 
	    assert(co != NULL);
#ifdef TEST
		printf("co %s was to be free!\n", co->name);
#endif
/*
#ifdef TEST
	printf("A pointer free happened in if clause whose condition is current != NULL, current co is %s\n", current->name);
#endif
*/
	    free(co);
	}
}

void co_yield() {
    
	if(current == NULL) {

#ifdef TEST
		printf("Exit from main thread directly!\n");
#endif

	    exit(0);
	}
	else {
	    current->status = CO_WAITING;

#ifdef TEST
		printf("yield occured in co %s!\n", current->name);
#endif

        int val = setjmp(current->context);
        if (val == 0) {
#ifdef TEST
		printf("The return value of setjmp is 0 | The current co is %s\n", current->name);
#endif
            struct co new_co;
			rand_choose(co_list_head, &new_co, current);
			assert(new_co.brother != NULL);
			assert(new_co.brother->status == CO_NEW || new_co.brother->status == CO_WAITING);
			if (new_co.brother->status == CO_NEW) {
#ifdef TEST
		        printf("Another co was chosen and it is a new co!\n");
#endif
				assert(new_co.brother->stack != NULL && new_co.brother->func != NULL && new_co.brother->arg != NULL);
/*
#ifdef DEBUG
		        printf("stack_space 1:%p\n", &new_co.brother->stack[0]);
		        printf("stack_space 2:%p\n", &new_co.brother->stack[STACK_SIZE]);
		        printf("func_entry:%p\n", new_co.brother->func);
		        printf("arg:%p\n", new_co.brother->arg);
#endif
*/
				/*
				int i = 0;
				for( i = 0; i < 20; i ++) {
				    if(((uintptr_t)(&new_co.brother->stack[STACK_SIZE-1-i])) % 16 == 0 ) {
						break;
					}
				}
				*/
#ifdef DEBUG
		        printf("Aligened stack:%p\n", &new_co.brother->stack[STACK_SIZE]);
#endif
				current = new_co.brother;
			    stack_switch_call(&new_co.brother->stack[STACK_SIZE - 11], new_co.brother->func, (uintptr_t)new_co.brother->arg);
			}

			else {
#ifdef TEST
		        printf("Another co was chosen and it is a waiting co!\n");
#endif
			   current = new_co.brother;
			   longjmp(new_co.brother->context, 2); 
			}
            
	    }
        else {
#ifdef TEST
		printf("The return value of setjmp is not  0 | The current co is %s\n", current->name);
#endif
			current->status = CO_RUNNING;
            return;	
	    }	
	}
}


