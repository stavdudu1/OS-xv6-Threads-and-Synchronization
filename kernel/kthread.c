#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

extern struct proc proc[NPROC];


struct spinlock ktid_lock;

extern void forkret(void);

void kthreadinit(struct proc *p)
{
  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    initlock(&kt->lock,"kt_lock");
    kt->state = UNUSED;
    kt->belong = p;

    // WARNING: Don't change this line!
    // get the pointer to the kernel stack of the kthread
    kt->kstack = KSTACK((int)((p - proc) * NKT + (kt - p->kthread)));
  }
}

struct kthread *mykthread()
{
  push_off();
  struct cpu *c = mycpu();
  struct kthread *kt = c->kt;
  pop_off();
  return kt;
}

struct trapframe *get_kthread_trapframe(struct proc *p, struct kthread *kt)
{
  struct trapframe* tf = p->base_trapframes + ((int)(kt - p->kthread));
  return tf;
}


int
allocktid(struct proc *p)
{
  int ktid;
  
  acquire(&p->ktid_lock);
  ktid = p->next_ktid;
  p->next_ktid = ktid +1;
  release(&p->ktid_lock);

  return ktid;
}

struct kthread* allockthread(struct proc *p) {
  struct kthread *kt;
  for (kt = p->kthread; kt < &p->kthread[NKT]; kt++){
    acquire(&kt->lock);
    if (kt->state == UNUSED){
      goto found;
    }
    else{
      release(&kt->lock);
    }
  }
  return 0;

found:
  kt->ktid = allocktid(p);
  kt->state = USED;
  if ((kt->trapframe = get_kthread_trapframe(p, kt)) == 0)
  {
    freekt(kt);
    release(&kt->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&kt->context, 0, sizeof(kt->context));
  kt->context.ra = (uint64)forkret;
  kt->context.sp = kt->kstack + PGSIZE;

  return kt;
}

void freekt(struct kthread* kt){
  kt->ktid = 0;
  kt->killed = 0;
  kt->xstate = 0;
  kt->chan = 0;
  kt->trapframe = 0;
  kt->state = UNUSED;
}

int kthread_create( void *(*start_func)(), uint64 stack, uint stack_size){
  struct proc *p = myproc();
  struct kthread *kt;
  if((kt = allockthread(p)) == 0){
    release(&kt->lock);  
    return -1;
  }
  else {
    *(kt->trapframe) = *(mykthread()->trapframe); //added
    kt->state = RUNNABLE;
    kt->trapframe->sp = (uint64)stack + stack_size;
    kt->trapframe->epc = (uint64)start_func;
  }
  release(&kt->lock);
  return kt->ktid;
}

int kthread_id()
{
  return mykthread()->ktid;
}

int kthread_kill(int ktid){
  
  struct proc *p = myproc();

  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++){
      acquire(&kt->lock);
      if(kt->ktid == ktid){
        kt->killed = 1;
        if(kt->state == SLEEPING){
          // Wake kthread from sleep().
          kt->state = RUNNABLE;
        }
        release(&kt->lock);
        return 0;
      }
      release(&kt->lock);
    }
    return -1;
}

void kthread_exit(int status)
{
  struct proc *p = myproc();
  struct kthread *kt = mykthread();
  int exist = 0;

  acquire(&p->lock);
  for (struct kthread *t=p->kthread; t < &p->kthread[NKT]; t++){
    acquire(&t->lock);
    if (t->state != ZOMBIE && t->state != UNUSED){
      exist++;
    }
    release(&t->lock);
  }
  release(&p->lock);
  
  if (exist == 1){ //need to terminate process
    exit(status);
  }
  else {
    acquire(&kt->lock);
    kt->xstate = status;
    kt->state = ZOMBIE;
    release(&kt->lock);
    wakeup(kt);
    acquire(&kt->lock);
    sched();
    panic("zombie exit");
  }  
}

int kthread_join(int ktid, uint64 status) 
{
  struct kthread *t;
  struct proc *p = myproc();
  int found;

  acquire (&p->lock); //added
  for(;;){
    found = 0;
    for(t = p->kthread; t < &p->kthread[NKT]; t++){
      acquire(&t->lock);
      if(t->ktid == ktid && t->state != UNUSED){
        found = 1;
        if(t->state == ZOMBIE){
          if(status != 0 && copyout(p->pagetable, (uint64)status, (char *)&t->xstate,
                                  sizeof(t->xstate)) < 0) {
            release(&t->lock);
            release(&p->lock);
            return -1;
          }
          freekt(t);
          release(&t->lock);
          release(&p->lock);
          return 0;
        }
        release(&t->lock);
        break;
      }
      else
        release(&t->lock);
    } 
    if(found == 0 || mykthread()->killed == 1){
      release(&p->lock);
      return -1;
    }
    // Wait for a child to exit.
    sleep(t, &p->lock);  //DOC: wait-sleep
  }
}

