// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void
kinit()
{
  for(int cpu = 0; cpu < NCPU; cpu++) {
    char lock_name[10];
    snprintf(lock_name, 10, "kmem_cpu%d", cpu);
    initlock(&kmem[cpu].lock, lock_name);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int curr_cpu = cpuid();
  pop_off();

  acquire(&kmem[curr_cpu].lock);
  r->next = kmem[curr_cpu].freelist;
  kmem[curr_cpu].freelist = r;
  release(&kmem[curr_cpu].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int curr_cpu = cpuid();
  pop_off();

  acquire(&kmem[curr_cpu].lock);
  r = kmem[curr_cpu].freelist;
  if(r) {
    kmem[curr_cpu].freelist = r->next;
    release(&kmem[curr_cpu].lock);
  } else {
    release(&kmem[curr_cpu].lock);
    for (int cpu = 0; cpu < NCPU; cpu++) {
      if (cpu != curr_cpu) {
        acquire(&kmem[MIN(cpu, curr_cpu)].lock);
        acquire(&kmem[MAX(cpu, curr_cpu)].lock);
        r = kmem[cpu].freelist;
        if(r) {
          kmem[cpu].freelist = r->next;
          release(&kmem[MAX(cpu, curr_cpu)].lock);
          release(&kmem[MIN(cpu, curr_cpu)].lock);
          break;
        }
        release(&kmem[MAX(cpu, curr_cpu)].lock);
        release(&kmem[MIN(cpu, curr_cpu)].lock);
      }
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
