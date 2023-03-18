#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {
  int i;
  struct shm_page *pg = 0;
  struct proc *curproc = myproc();
  char *va = (char *)PGROUNDUP(curproc->sz);

  acquire(&(shm_table.lock));

  for (i = 0; i < 64; i++) {
    if (shm_table.shm_pages[i].id == id) {
      pg = shm_table.shm_pages + i;
    }
  }

  if (pg){ // Case 1: ID already exists
    if (!mappages(curproc->pgdir, va, PGSIZE, V2P(pg->frame),PTE_W|PTE_U)) {
      curproc->sz = (int)va + PGSIZE;
      pg->refcnt++;
      *pointer = va;
    } else {
      cprintf("Shared page with id %d could not be mapped.\n", id);
      release(&(shm_table.lock));
      return 1;
    }
  } else { // Case 2: ID doesn't exist
    for (i = 0; i <64; i++) {
      if (!shm_table.shm_pages[i].frame) {
        pg = shm_table.shm_pages + i;
        break;
      }
    }

    if (!pg) {
      cprintf("Shared memory table full.\n");
      release(&(shm_table.lock));
      return 1;
    }

    pg->frame = kalloc();
    memset(pg->frame, 0, PGSIZE);

    if (!mappages(curproc->pgdir, va, PGSIZE, V2P(pg->frame), PTE_W|PTE_U)) {
      curproc->sz = (int)va + PGSIZE;
      *pointer = va;
      pg->id = id;
      pg->refcnt++;
    } else {
      cprintf("Shared page with id %d could not be mapped.\n", id);
      release(&(shm_table.lock));
      return 1;
    }
  }

release(&(shm_table.lock));

return 0; //added to remove compiler warning -- you should decide what to return
}

int shm_close(int id) {
//you write this too!
int i;
struct shm_page *pg = 0;
acquire(&(shm_table.lock));

for (i = 0; i < 64; i++) {
    if (shm_table.shm_pages[i].id == id) {
      pg = shm_table.shm_pages + i;
    }
  }
  	
if(pg == 0 || pg->refcnt == 0) {
  cprintf("No shared memory to close.\n\n");
  release(&(shm_table.lock));
  return 1;
} else {
  pg->refcnt--;
}

if(pg->refcnt == 0) {
  pg->id = 0;
  pg->frame = 0; 
}

release(&(shm_table.lock));

return 0; //added to remove compiler warning -- you should decide what to return
}
