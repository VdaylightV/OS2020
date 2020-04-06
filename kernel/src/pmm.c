#include <common.h>

#define MAGIC 2

static void *kalloc(size_t size) {
	static uintptr_t brk = 0;
//	printf("[#BRK(I)]:%p\n",brk);
	brk = brk?
		ROUNDUP(brk, size) + size :
		(uintptr_t)_heap.start + size;
//	printf("[#BRK(II)]:%p\n",brk);
  return (void *)(brk - size);
}

static void kfree(void *ptr) {
}

static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)_heap.end - (uintptr_t)_heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, _heap.start, _heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
