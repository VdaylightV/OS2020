#include <common.h>
#include <klib.h>

/*
enum ops { OP_ALLOC = 1, OP_FREE };
struct op {
    enum ops type;
	union {
	    size_t alloc_size;
		void * free_addr;
	};
};

void stress_test() {
    while(1) {
	    struct op op = random_op();
      switch(op.type) {
        case OP_ALLOC:
          alloc_check(pmm->alloc(op.alloc_size, op.alloc_size));
          break;
        case OP_FREE:
          free(op.free_addr);
          break;
      }
	}
}
*/

void smoke_test() {
	/*
	while(1) {
	    uintptr_t ptr = (uintptr_t)pmm->alloc(4096*sizeof(char));
		pmm->free((void*)ptr);
	}
	*/
	
//	int i = 0;
   // for(; i < 20; i ++){
	while(1) {
//		if(i%5 == 0) {
	        void* ptr = pmm->alloc(sizeof(char));
//	        pmm->alloc(sizeof(char));
//		    printf("ptr->char: %p, I:%d from cpu #%d\n", ptr, i, _cpu());
		    pmm->free((void*)ptr);	
//		}
//		else if(i%5 == 1) {
//	        uintptr_t ptr = (uintptr_t)pmm->alloc(sizeof(uintptr_t));
	        ptr = pmm->alloc(sizeof(uintptr_t));
//		    printf("ptr->uintptr_t: %p, I:%d from cpu #%d\n", ptr, i, _cpu());
		    pmm->free((void*)ptr);	
//		}
//		else if(i%5 == 2){
//	        uintptr_t ptr = (uintptr_t)pmm->alloc(127*sizeof(char));
	        ptr = pmm->alloc(125*sizeof(char));
//		    printf("ptr->127: %p, I:%d from cpu #%d\n", ptr, i, _cpu());
		    pmm->free((void*)ptr);	
//		}
//		else if(i%5 == 3){
//	        uintptr_t ptr = (uintptr_t)pmm->alloc(63*sizeof(char));
	        ptr = pmm->alloc(63*sizeof(char));
//		    printf("ptr->63: %p, I:%d from cpu #%d\n", ptr, i, _cpu());
		    pmm->free((void*)ptr);	
//		}
//		else if(i%5 == 3){
//	        uintptr_t ptr = (uintptr_t)pmm->alloc(63*sizeof(char));
	        ptr = pmm->alloc(139*sizeof(char));
//		    printf("ptr->63: %p, I:%d from cpu #%d\n", ptr, i, _cpu());
		    pmm->free((void*)ptr);	
//		}
//		else {
//	        uintptr_t ptr = (uintptr_t)pmm->alloc(4096*sizeof(char));
	        ptr = pmm->alloc(4096*sizeof(char));
//		    printf("ptr->4096: %p, I:%d from cpu #%d\n", ptr, i, _cpu());
//		    pmm->free((void*)ptr);	
//		}
	}
    
	//printf("Done From cpu:%d\n", _cpu());
  while (1) ;
}

static void os_init() {
  pmm->init();
}

static void os_run() {
//  os->init();
  _mpe_init(smoke_test);
  
  /*
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    _putc(*s == '*' ? '0' + _cpu() : *s);
  }
  while (1) ;
  */
  
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};
